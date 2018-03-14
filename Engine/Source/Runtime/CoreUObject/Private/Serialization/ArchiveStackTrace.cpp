// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Serialization/ArchiveStackTrace.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectThreadContext.h"
#include "UObject/UnrealType.h"
#include "HAL/PlatformStackWalk.h"
#include "Serialization/AsyncLoading.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/OutputDeviceHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogArchiveDiff, Log, All);

class FIgnoreDiffManager
{
	int32 IgnoreCount;

public:
	FIgnoreDiffManager()
		: IgnoreCount(0)
	{}
	void Push()
	{
		IgnoreCount++;
	}
	void Pop()
	{
		IgnoreCount--;
		check(IgnoreCount >= 0);
	}
	bool ShouldIgnoreDiff() const
	{
		return !!IgnoreCount;
	}
};

static FIgnoreDiffManager GIgnoreDiffManager;

FArchiveStackTraceIgnoreScope::FArchiveStackTraceIgnoreScope(bool bInIgnore /* = true */)
	: bIgnore(bInIgnore)
{
	if (bIgnore)
	{
		GIgnoreDiffManager.Push();
	}
}
FArchiveStackTraceIgnoreScope::~FArchiveStackTraceIgnoreScope()
{
	if (bIgnore)
	{
		GIgnoreDiffManager.Pop();
	}
}

FString FArchiveStackTrace::FCallstackData::ToString(const TCHAR* CallstackCutoffText, int32 Indent) const
{
	const TCHAR* CALLSTACK_LINE_TERMINATOR = TEXT("\n"); // because LINE_TERMINATOR doesn't work well wit EC
	const TCHAR* INDENT = FCString::Spc(Indent);

	FString HumanReadableString;
	FString StackTraceText = Callstack;
	if (CallstackCutoffText != nullptr)
	{
		// If the cutoff string is provided, remove all functions starting with the one specifiec in the cutoff string
		int32 CutoffIndex = StackTraceText.Find(CallstackCutoffText, ESearchCase::CaseSensitive);
		if (CutoffIndex > 0)
		{
			CutoffIndex = StackTraceText.Find(TEXT("\n"), ESearchCase::CaseSensitive, ESearchDir::FromEnd, CutoffIndex - 1);
			if (CutoffIndex > 0)
			{
				StackTraceText = StackTraceText.Left(CutoffIndex + 1);
			}
		}
	}

	TArray<FString> StackLines;
	StackTraceText.ParseIntoArrayLines(StackLines);
	for (FString& StackLine : StackLines)
	{
		if (StackLine.StartsWith(TEXT("0x")))
		{
			int32 CutoffIndex = StackLine.Find(TEXT(" "), ESearchCase::CaseSensitive);
			if (CutoffIndex >= -1 && CutoffIndex < StackLine.Len() - 2)
			{
				StackLine = StackLine.Mid(CutoffIndex + 1);
			}
		}
		HumanReadableString += INDENT;
		HumanReadableString += StackLine;
		HumanReadableString += CALLSTACK_LINE_TERMINATOR;
	}

	if (!SerializedObject.IsEmpty())
	{
		HumanReadableString += CALLSTACK_LINE_TERMINATOR;
		HumanReadableString += INDENT;
		HumanReadableString += TEXT("Serialized Object: ");
		HumanReadableString += SerializedObject;
		HumanReadableString += CALLSTACK_LINE_TERMINATOR;
	}
	if (!SerializedProperty.IsEmpty())
	{
		if (SerializedObject.IsEmpty())
		{
			HumanReadableString += CALLSTACK_LINE_TERMINATOR;
		}
		HumanReadableString += INDENT;
		HumanReadableString += TEXT("Serialized Property: ");
		HumanReadableString += SerializedProperty;
		HumanReadableString += CALLSTACK_LINE_TERMINATOR;
	}
	return HumanReadableString;
}

FArchiveStackTrace::FArchiveStackTrace(const TCHAR* InFilename, bool bInCollectCallstacks, const FArchiveDiffMap* InDiffMap)
	: FLargeMemoryWriter(0, false, InFilename)
	, DiffMap(InDiffMap)
	, bCollectCallstacks(bInCollectCallstacks)
	, bCallstacksDirty(true)
	, StackTraceSize(65535)
	, LastSerializeCallstack(nullptr)
	, ThreadContext(FUObjectThreadContext::Get())
{
	ArIsSaving = true;

	StackTrace = (ANSICHAR*)FMemory::Malloc(StackTraceSize);
	StackTrace[0] = 0;
}

FArchiveStackTrace::~FArchiveStackTrace()
{
	FMemory::Free(StackTrace);

	for (TPair<uint32, FCallstackData>& UniqueCallstackPair : UniqueCallstacks)
	{
		FMemory::Free(UniqueCallstackPair.Value.Callstack);
	}
}

ANSICHAR* FArchiveStackTrace::AddUniqueCallstack(const ANSICHAR* InCallstack, UObject* InSerializedObject, UProperty* InSerializedProperty, uint32& OutCallstackCRC)
{
	ANSICHAR* Callstack = nullptr;
	if (bCollectCallstacks)
	{
		OutCallstackCRC = FCrc::StrCrc32(StackTrace);

		if (FCallstackData* ExistingCallstack = UniqueCallstacks.Find(OutCallstackCRC))
		{
			Callstack = ExistingCallstack->Callstack;
		}
		else
		{
			int32 CallstackSize = FCStringAnsi::Strlen(InCallstack) + 1;
			Callstack = (ANSICHAR*)FMemory::Malloc(CallstackSize);
			FCStringAnsi::Strcpy(Callstack, CallstackSize, InCallstack);
			UniqueCallstacks.Add(OutCallstackCRC, FCallstackData(Callstack,
				InSerializedObject ? *InSerializedObject->GetFullName() : nullptr,
				InSerializedProperty ? *InSerializedProperty->GetFullName() : nullptr));
		}
	}
	else
	{
		OutCallstackCRC = 0;
	}
	return Callstack;
}

void FArchiveStackTrace::Serialize(void* InData, int64 Num)
{
	if (Num)
	{
#if UE_BUILD_DEBUG
		const int32 StackIgnoreCount = 5;
#else
		const int32 StackIgnoreCount = 4;
#endif

		int64 CurrentOffset = Tell();

		// Walk the stack and dump it to the allocated memory.
		bool bShouldCollectCallstack = bCollectCallstacks && (DiffMap == nullptr || IsInDiffMap(CurrentOffset)) && !GIgnoreDiffManager.ShouldIgnoreDiff();
		if (bShouldCollectCallstack)
		{
			StackTrace[0] = '\0';
			FPlatformStackWalk::StackWalkAndDump(StackTrace, StackTraceSize, StackIgnoreCount);
			// Make sure we compare the new stack trace with the last one in the next if statement
			bCallstacksDirty = true;
		}
		
		if (LastSerializeCallstack == nullptr || (bCallstacksDirty && FCStringAnsi::Strcmp(LastSerializeCallstack, StackTrace) != 0))
		{			
			uint32 CallstackCRC = 0;
			if (CallstackAtOffsetMap.Num() == 0 || CurrentOffset > CallstackAtOffsetMap.Last().Offset)
			{
				// New data serialized at the end of archive buffer
				LastSerializeCallstack = AddUniqueCallstack(StackTrace, ThreadContext.SerializedObject, GetSerializedProperty(), CallstackCRC);
				CallstackAtOffsetMap.Add(FCallstactAtOffset(CurrentOffset, CallstackCRC, GIgnoreDiffManager.ShouldIgnoreDiff()));
			}
			else
			{
				// This happens usually after Seek() so we need to find the exiting offset or insert a new one
				int32 CallstackToUpdateIndex = GetCallstackAtOffset(CurrentOffset, 0);
				check(CallstackToUpdateIndex != -1);
				FCallstactAtOffset& CallstackToUpdate = CallstackAtOffsetMap[CallstackToUpdateIndex];
				LastSerializeCallstack = AddUniqueCallstack(StackTrace, ThreadContext.SerializedObject, GetSerializedProperty(), CallstackCRC);
				if (CallstackToUpdate.Offset == CurrentOffset)
				{					
					CallstackToUpdate.Callstack = CallstackCRC;
				}
				else
				{
					// Insert a new callstack
					check(CallstackToUpdate.Offset < CurrentOffset);
					CallstackAtOffsetMap.Insert(FCallstactAtOffset(CurrentOffset, CallstackCRC, GIgnoreDiffManager.ShouldIgnoreDiff()), CallstackToUpdateIndex + 1);
				}			
			}
			check(CallstackCRC != 0 || !bShouldCollectCallstack);
		}
		else if (LastSerializeCallstack)
		{
			// Skip callstack comparison on next serialize call unless we grab a stack trace
			bCallstacksDirty = false;
		}
	}
	FLargeMemoryWriter::Serialize(InData, Num);
}

int32 FArchiveStackTrace::GetCallstackAtOffset(int64 InOffset, int32 MinOffsetIndex)
{
	if (InOffset < 0 || InOffset > TotalSize() || MinOffsetIndex < 0 || MinOffsetIndex >= CallstackAtOffsetMap.Num())
	{
		return -1;
	}

	// Find the index of the offset the InOffset maps to
	int32 OffsetForCallstackIndex = -1;
	int32 MaxOffsetIndex = CallstackAtOffsetMap.Num() - 1;

	// Binary search
	for (; MinOffsetIndex <= MaxOffsetIndex; )
	{
		int32 SearchIndex = (MinOffsetIndex + MaxOffsetIndex) / 2;
		if (CallstackAtOffsetMap[SearchIndex].Offset < InOffset)
		{
			MinOffsetIndex = SearchIndex + 1;
		}
		else if (CallstackAtOffsetMap[SearchIndex].Offset > InOffset)
		{
			MaxOffsetIndex = SearchIndex - 1;
		}
		else
		{
			OffsetForCallstackIndex = SearchIndex;
			break;
		}
	}
	
	if (OffsetForCallstackIndex == -1)
	{
		// We didn't find the exact offset value so let's try to find the first one that is lower than the requested one
		MinOffsetIndex = FMath::Min(MinOffsetIndex, CallstackAtOffsetMap.Num() - 1);
		for (int32 FirstLowerOffsetIndex = MinOffsetIndex; FirstLowerOffsetIndex >= 0; --FirstLowerOffsetIndex)
		{
			if (CallstackAtOffsetMap[FirstLowerOffsetIndex].Offset < InOffset)
			{
				OffsetForCallstackIndex = FirstLowerOffsetIndex;
				break;
			}
		}
		check(OffsetForCallstackIndex != -1);
		check(CallstackAtOffsetMap[OffsetForCallstackIndex].Offset < InOffset);
		check(OffsetForCallstackIndex == (CallstackAtOffsetMap.Num() - 1) || CallstackAtOffsetMap[OffsetForCallstackIndex + 1].Offset > InOffset);
	}

	return OffsetForCallstackIndex;
}

bool FArchiveStackTrace::LoadPackageIntoMemory(const TCHAR* InFilename, FPackageData& OutPackageData)
{
	FArchive* UAssetFileArchive = IFileManager::Get().CreateFileReader(InFilename);
	if (!UAssetFileArchive || UAssetFileArchive->TotalSize() == 0)
	{
		// The package doesn't exist on disk
		OutPackageData.Data = nullptr;
		OutPackageData.Size = 0;
		OutPackageData.HeaderSize = 0;
		OutPackageData.StartOffset = 0;
		return false;
	}
	else
	{
		// Handle EDL packages (uexp files)
		FArchive* ExpFileArchive = nullptr;
		OutPackageData.Size = UAssetFileArchive->TotalSize();
		if (IsEventDrivenLoaderEnabledInCookedBuilds())
		{
			FString UExpFilename = FPaths::ChangeExtension(InFilename, TEXT("uexp"));
			ExpFileArchive = IFileManager::Get().CreateFileReader(*UExpFilename);
			if (ExpFileArchive)
			{				
				// The header size is the current package size
				OutPackageData.HeaderSize = OutPackageData.Size;
				// Grow the buffer size to append the uexp file contents
				OutPackageData.Size += ExpFileArchive->TotalSize();
			}
		}
		OutPackageData.Data = (uint8*)FMemory::Malloc(OutPackageData.Size);
		UAssetFileArchive->Serialize(OutPackageData.Data, UAssetFileArchive->TotalSize());

		if (ExpFileArchive)
		{
			// If uexp file is present, append its contents at the end of the buffer
			ExpFileArchive->Serialize(OutPackageData.Data + OutPackageData.HeaderSize, ExpFileArchive->TotalSize());
			delete ExpFileArchive;
			ExpFileArchive = nullptr;
		}
	}
	delete UAssetFileArchive;

	return true;
}

void FArchiveStackTrace::CompareWithInternal(const FPackageData& SourcePackage, const FPackageData& DestPackage, const TCHAR* AssetFilename, const TCHAR* CallstackCutoffText, const int32 MaxDiffsToLog, int32& InOutDiffsLogged, FArchiveDiffStats& OutStats)
{
#if NO_LOGGING
	const int32 Indent = 0;
#else
	const int32 Indent = FOutputDeviceHelper::FormatLogLine(ELogVerbosity::Warning, LogArchiveDiff.GetCategoryName(), TEXT(""), GPrintLogTimes).Len();
#endif

	const TCHAR* CALLSTACK_LINE_TERMINATOR = TEXT("\n"); // because LINE_TERMINATOR doesn't work well wit EC
	const int64 SourceSize = SourcePackage.Size - SourcePackage.StartOffset;
	const int64 DestSize = DestPackage.Size - DestPackage.StartOffset;
	const int64 SizeToCompare = FMath::Min(SourceSize, DestSize);
	
	if (SourceSize != DestSize)
	{
		UE_LOG(LogArchiveDiff, Warning, TEXT("%s: Size mismatch: on disk: %lld vs memory: %lld"), AssetFilename, SourceSize, DestSize);
		OutStats.DiffSize += DestPackage.Size - SourcePackage.Size;
	}

	FString LastDifferenceCallstackDataText;
	int32 LastDifferenceCallstackOffsetIndex = -1;
	int32 NumDiffsLocal = 0;
	int32 NumDiffsLoggedLocal = 0;

	for (int64 LocalOffset = 0; LocalOffset < SizeToCompare; ++LocalOffset)
	{
		const int64 SourceAbsoluteOffset = LocalOffset + SourcePackage.StartOffset;
		const int64 DestAbsoluteOffset = LocalOffset + DestPackage.StartOffset;

		if (SourcePackage.Data[SourceAbsoluteOffset] != DestPackage.Data[DestAbsoluteOffset])
		{
			if (DiffMap == nullptr || IsInDiffMap(DestAbsoluteOffset))
			{
				int32 DifferenceCallstackoffsetIndex = GetCallstackAtOffset(DestAbsoluteOffset, FMath::Max(LastDifferenceCallstackOffsetIndex, 0));
				if (DifferenceCallstackoffsetIndex >= 0 && DifferenceCallstackoffsetIndex != LastDifferenceCallstackOffsetIndex)
				{
					const FCallstactAtOffset& CallstackAtOffset = CallstackAtOffsetMap[DifferenceCallstackoffsetIndex];
					const FCallstackData& DifferenceCallstackData = UniqueCallstacks[CallstackAtOffset.Callstack];
					FString DifferenceCallstackDataText = DifferenceCallstackData.ToString(CallstackCutoffText, Indent);
					if (LastDifferenceCallstackDataText.Compare(DifferenceCallstackDataText, ESearchCase::CaseSensitive) != 0)
					{
						if (CallstackAtOffset.bIgnore == false && (MaxDiffsToLog < 0 || InOutDiffsLogged < MaxDiffsToLog))
						{
							UE_LOG(LogArchiveDiff, Warning, TEXT("%s: Difference at offset %lld%s, callstack:%s%s%s"),
								AssetFilename,
								CallstackAtOffset.Offset - DestPackage.StartOffset,
								DestAbsoluteOffset > CallstackAtOffset.Offset ? *FString::Printf(TEXT("(+%lld)"), DestAbsoluteOffset - CallstackAtOffset.Offset) : TEXT(""),
								CALLSTACK_LINE_TERMINATOR, CALLSTACK_LINE_TERMINATOR, *DifferenceCallstackDataText);
							InOutDiffsLogged++;
							NumDiffsLoggedLocal++;
						}
						LastDifferenceCallstackDataText = MoveTemp(DifferenceCallstackDataText);
						OutStats.NumDiffs++;
						NumDiffsLocal++;
					}
				}
				else if (DifferenceCallstackoffsetIndex < 0)
				{
					UE_LOG(LogArchiveDiff, Warning, TEXT("%s: Difference at offset %lld, unknown callstack"), AssetFilename, LocalOffset);
				}
				LastDifferenceCallstackOffsetIndex = DifferenceCallstackoffsetIndex;
			}
			else
			{
				// Each byte will count as a difference but without callstack data there's no way around it
				OutStats.NumDiffs++;
				NumDiffsLocal++;
			}
			OutStats.DiffSize++;
		}
	}

	if (MaxDiffsToLog >= 0 && NumDiffsLocal > NumDiffsLoggedLocal)
	{
		UE_LOG(LogArchiveDiff, Warning, TEXT("%s: %lld difference(s) not logged."), AssetFilename, NumDiffsLocal - NumDiffsLoggedLocal);
	}
}

void FArchiveStackTrace::CompareWith(const TCHAR* InFilename, const int64 TotalHeaderSize, const TCHAR* CallstackCutoffText, const int32 MaxDiffsToLog, FArchiveDiffStats& OutStats)
{
	FPackageData SourcePackage;

	OutStats.NewFileTotalSize = TotalSize();

	if (LoadPackageIntoMemory(InFilename, SourcePackage))
	{	
		FPackageData DestPackage;
		DestPackage.Data = GetData();
		DestPackage.Size = TotalSize();
		DestPackage.HeaderSize = TotalHeaderSize;
		DestPackage.StartOffset = 0;

		UE_LOG(LogArchiveDiff, Display, TEXT("Comparing: %s"), *GetArchiveName());

		int32 NumLoggedDiffs = 0;

		{
			FPackageData SourcePackageHeader = SourcePackage;
			SourcePackageHeader.Size = SourcePackageHeader.HeaderSize;
			SourcePackageHeader.HeaderSize = 0;
			SourcePackageHeader.StartOffset = 0;

			FPackageData DestPackageHeader = DestPackage;
			DestPackageHeader.Size = TotalHeaderSize;
			DestPackageHeader.HeaderSize = 0;
			DestPackageHeader.StartOffset = 0;

			CompareWithInternal(SourcePackageHeader, DestPackageHeader, InFilename, CallstackCutoffText, MaxDiffsToLog, NumLoggedDiffs, OutStats);
		}

		{
			FPackageData SourcePackageExports = SourcePackage;
			SourcePackageExports.HeaderSize = 0;
			SourcePackageExports.StartOffset = SourcePackage.HeaderSize;

			FPackageData DestPackageExports = DestPackage;
			DestPackageExports.HeaderSize = 0;
			DestPackageExports.StartOffset = TotalHeaderSize;

			FString AssetName;
			if (DestPackage.HeaderSize > 0)
			{
				AssetName = FPaths::ChangeExtension(InFilename, TEXT("uexp"));
			}
			else
			{
				AssetName = InFilename;
			}

			CompareWithInternal(SourcePackageExports, DestPackageExports, *AssetName, CallstackCutoffText, MaxDiffsToLog, NumLoggedDiffs, OutStats);
		}
		
		FMemory::Free(SourcePackage.Data);
	}
	else
	{		
		UE_LOG(LogArchiveDiff, Warning, TEXT("New package: %s"), *GetArchiveName());
		OutStats.DiffSize = OutStats.NewFileTotalSize;
	}
}

bool FArchiveStackTrace::GenerateDiffMapInternal(const FPackageData& SourcePackage, const FPackageData& DestPackage, int32 MaxDiffsToFind, FArchiveDiffMap& OutDiffMap)
{
	bool bIdentical = true;
	int32 LastDifferenceCallstackOffsetIndex = -1;
	FCallstackData* DifferenceCallstackData = nullptr;

	const int64 SourceSize = SourcePackage.Size - SourcePackage.StartOffset;
	const int64 DestSize = DestPackage.Size - DestPackage.StartOffset;
	const int64 SizeToCompare = FMath::Min(SourceSize, DestSize);
	
	for (int64 LocalOffset = 0; LocalOffset < SizeToCompare; ++LocalOffset)
	{
		const int64 SourceAbsoluteOffset = LocalOffset + SourcePackage.StartOffset;
		const int64 DestAbsoluteOffset = LocalOffset + DestPackage.StartOffset;
		if (SourcePackage.Data[SourceAbsoluteOffset] != DestPackage.Data[DestAbsoluteOffset])
		{
			bIdentical = false;
			if (OutDiffMap.Num() < MaxDiffsToFind)
			{
				int64 DifferenceCallstackoffsetIndex = GetCallstackAtOffset(DestAbsoluteOffset, FMath::Max(LastDifferenceCallstackOffsetIndex, 0));
				if (DifferenceCallstackoffsetIndex >= 0 && DifferenceCallstackoffsetIndex != LastDifferenceCallstackOffsetIndex)
				{
					const FCallstactAtOffset& CallstackAtOffset = CallstackAtOffsetMap[DifferenceCallstackoffsetIndex];
					if (!CallstackAtOffset.bIgnore)
					{
						FArchiveDiffInfo OffsetAndSize;
						OffsetAndSize.Offset = CallstackAtOffset.Offset;
						OffsetAndSize.Size = GetSerializedDataSizeForOffsetIndex(DifferenceCallstackoffsetIndex);
						OutDiffMap.Add(OffsetAndSize);
					}
				}
				LastDifferenceCallstackOffsetIndex = DifferenceCallstackoffsetIndex;
			}
		}
	}

	if (SourceSize < DestSize)
	{
		bIdentical = false;

		// Add all the remaining callstacks to the diff map
		for (int32 OffsetIndex = LastDifferenceCallstackOffsetIndex + 1; OffsetIndex < CallstackAtOffsetMap.Num() && OutDiffMap.Num() < MaxDiffsToFind; ++OffsetIndex)
		{
			const FCallstactAtOffset& CallstackAtOffset = CallstackAtOffsetMap[OffsetIndex];
			// Compare against the size without start offset as all callstack offsets are absolute (from the merged header + exports file)
			if (CallstackAtOffset.Offset < DestPackage.Size)
			{
				if (!CallstackAtOffset.bIgnore)
				{
					FArchiveDiffInfo OffsetAndSize;
					OffsetAndSize.Offset = CallstackAtOffset.Offset;
					OffsetAndSize.Size = GetSerializedDataSizeForOffsetIndex(OffsetIndex);
					OutDiffMap.Add(OffsetAndSize);
				}
			}
			else
			{
				break;
			}
		}
	}
	else if (SourceSize > DestSize)
	{
		bIdentical = false;
	}
	return bIdentical;
}

bool FArchiveStackTrace::GenerateDiffMap(const TCHAR* InFilename, int64 TotalHeaderSize, int32 MaxDiffsToFind, FArchiveDiffMap& OutDiffMap)
{
	check(MaxDiffsToFind > 0);

	FPackageData SourcePackage;
	bool bIdentical = LoadPackageIntoMemory(InFilename, SourcePackage);
	if (bIdentical)
	{
		bool bHeaderIdentical = true;
		bool bExportsIdentical = true;

		FPackageData DestPackage;
		DestPackage.Data = GetData();
		DestPackage.Size = TotalSize();
		DestPackage.HeaderSize = TotalHeaderSize;
		DestPackage.StartOffset = 0;

		{
			FPackageData SourcePackageHeader = SourcePackage;
			SourcePackageHeader.Size = SourcePackageHeader.HeaderSize;
			SourcePackageHeader.HeaderSize = 0;
			SourcePackageHeader.StartOffset = 0;

			FPackageData DestPackageHeader = DestPackage;
			DestPackageHeader.Size = TotalHeaderSize;
			DestPackageHeader.HeaderSize = 0;
			DestPackageHeader.StartOffset = 0;

			bHeaderIdentical = GenerateDiffMapInternal(SourcePackageHeader, DestPackageHeader, MaxDiffsToFind, OutDiffMap);
		}

		{
			FPackageData SourcePackageExports = SourcePackage;
			SourcePackageExports.HeaderSize = 0;
			SourcePackageExports.StartOffset = SourcePackage.HeaderSize;

			FPackageData DestPackageExports = DestPackage;
			DestPackageExports.HeaderSize = 0;
			DestPackageExports.StartOffset = TotalHeaderSize;

			bExportsIdentical = GenerateDiffMapInternal(SourcePackageExports, DestPackageExports, MaxDiffsToFind, OutDiffMap);
		}

		bIdentical = bHeaderIdentical && bExportsIdentical;

		FMemory::Free(SourcePackage.Data);
	}

	return bIdentical;
}


bool FArchiveStackTrace::IsIdentical(const TCHAR* InFilename, int64 BufferSize, const uint8* BufferData)
{
	FPackageData SourcePackage;
	bool bIdentical = LoadPackageIntoMemory(InFilename, SourcePackage);

	if (bIdentical)
	{
		if (BufferSize == SourcePackage.Size)
		{
			bIdentical = (FMemory::Memcmp(SourcePackage.Data, BufferData, BufferSize) == 0);
		}
		else
		{
			bIdentical = false;
		}
		FMemory::Free(SourcePackage.Data);
	}
	
	return bIdentical;
}

