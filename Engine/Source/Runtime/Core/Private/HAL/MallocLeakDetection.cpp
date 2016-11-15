// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MallocLeakDetection.cpp: Helper class to track memory allocations
=============================================================================*/

#include "CorePrivatePCH.h"
#include "MemoryMisc.h"
#include "MallocLeakDetection.h"

#if MALLOC_LEAKDETECTION

FMallocLeakDetection::FMallocLeakDetection()
	: bCaptureAllocs(false)
	, MinAllocationSize(0)
	, bDumpOustandingAllocs(false)
{
	Contexts.Empty(16);
}

FMallocLeakDetection& FMallocLeakDetection::Get()
{
	static FMallocLeakDetection Singleton;
	return Singleton;
}

FMallocLeakDetection::~FMallocLeakDetection()
{	
}

void FMallocLeakDetection::PushContext(const FString& Context)
{
	Contexts.Push(Context);
}

void FMallocLeakDetection::PopContext()
{
	Contexts.Pop(false);
}

bool FMallocLeakDetection::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	if (FParse::Command(&Cmd, TEXT("mallocleak")))
	{
		if (FParse::Command(&Cmd, TEXT("start")))
		{
			uint32 FilterSize = 64;
			FParse::Value(Cmd, TEXT("filtersize="), FilterSize);

			UE_LOG(LogConsoleResponse, Display, TEXT("Starting tracking of allocations >= %d KB"), FilterSize);
			SetAllocationCollection(true, FilterSize * 1024);
		}
		else if (FParse::Command(&Cmd, TEXT("stop")))
		{
			uint32 FilterSize = 64;
			FParse::Value(Cmd, TEXT("filtersize="), FilterSize);

			FilterSize *= 1024;

			UE_LOG(LogConsoleResponse, Display, TEXT("Stopping allocation tracking and clearing data."));
			SetAllocationCollection(false);

			UE_LOG(LogConsoleResponse, Display, TEXT("Dumping unique calltacks with %i bytes or more oustanding."), FilterSize);
			DumpOpenCallstacks(FilterSize);

			ClearData();
		}
		else if (FParse::Command(&Cmd, TEXT("Clear")))
		{
			UE_LOG(LogConsoleResponse, Display, TEXT("Clearing tracking data."));
			ClearData();
		}
		else if (FParse::Command(&Cmd, TEXT("Dump")))
		{
			uint32 FilterSize = 64;
			FParse::Value(Cmd, TEXT("filtersize="), FilterSize);
			FilterSize *= 1024;

			FString FileName;
			FParse::Value(Cmd, TEXT("name="), FileName);

			UE_LOG(LogConsoleResponse, Display, TEXT("Dumping unique calltacks with %i KB or more oustanding."), FilterSize/1024);
			DumpOpenCallstacks(FilterSize, *FileName);
		}
		else
		{
			UE_LOG(LogConsoleResponse, Display, TEXT("'MallocLeak Start [filtersize=Size]'  Begins accumulating unique callstacks of allocations > Size KBytes\n")
				TEXT("'MallocLeak Stop filtersize=[Size]'  Dumps outstanding unique callstacks and stops accumulation (with an optional filter size in KBytes, if not specified 128 KB is assumed).  Also clears data.\n")
				TEXT("'MallocLeak Clear'  Clears all accumulated data.  Does not change start/stop state.\n")
				TEXT("'MallocLeak Dump filtersize=[Size] [save]' Dumps oustanding unique callstacks with optional size filter in KBytes (if size is not specified it is assumed to be 128 KB).\n"));
		}

		return true;
	}
	return false;
}

void FMallocLeakDetection::AddCallstack(FCallstackTrack& Callstack)
{
	FScopeLock Lock(&AllocatedPointersCritical);
	uint32 CallstackHash = Callstack.GetHash();
	FCallstackTrack& UniqueCallstack = UniqueCallstacks.FindOrAdd(CallstackHash);
	//if we had a hash collision bail and lose the data rather than corrupting existing data.
	if (UniqueCallstack.Count > 0 && UniqueCallstack != Callstack)
	{
		ensureMsgf(false, TEXT("Callstack hash collision.  Throwing away new stack."));
		return;
	}

	if (UniqueCallstack.Count == 0)
	{
		UniqueCallstack = Callstack;
	}
	else
	{
		UniqueCallstack.Size += Callstack.Size;
		UniqueCallstack.LastFrame = Callstack.LastFrame;
	}
	UniqueCallstack.Count++;	
}

void FMallocLeakDetection::RemoveCallstack(FCallstackTrack& Callstack)
{
	FScopeLock Lock(&AllocatedPointersCritical);
	uint32 CallstackHash = FCrc::MemCrc32(Callstack.CallStack, sizeof(Callstack.CallStack), 0);
	FCallstackTrack* UniqueCallstack = UniqueCallstacks.Find(CallstackHash);
	if (UniqueCallstack)
	{
		UniqueCallstack->Count--;
		UniqueCallstack->Size -= Callstack.Size;
		if (UniqueCallstack->Count == 0)
		{
			UniqueCallstack = nullptr;
			UniqueCallstacks.Remove(CallstackHash);
		}
	}
}

void FMallocLeakDetection::SetAllocationCollection(bool bEnabled, int32 Size)
{
	FScopeLock Lock(&AllocatedPointersCritical);
	bCaptureAllocs = bEnabled;

	if (bEnabled)
	{
		MinAllocationSize = Size;
	}
}

void FMallocLeakDetection::DumpOpenCallstacks(uint32 FilterSize, const TCHAR* FileName /*= nullptr*/)
{
	const int32 MaxCallstackLineChars = 2048;
	char CallstackString[MaxCallstackLineChars];
	TCHAR CallstackStringWide[MaxCallstackLineChars];
	FMemory::Memzero(CallstackString);
	TArray<uint32> SortedKeys;
	SIZE_T TotalSize = 0;
	SIZE_T ReportedSize = 0;

	// Make sure we preallocate memory so GetKeys() doesn't allocate!
	SortedKeys.Empty(UniqueCallstacks.Num() + 32);

	{
		FScopeLock Lock(&AllocatedPointersCritical);

		for (const auto& Pair : UniqueCallstacks)
		{
			if (Pair.Value.Size >= FilterSize)
			{
				SortedKeys.Add(Pair.Key);

				ReportedSize += Pair.Value.Size;
			}

			TotalSize += Pair.Value.Size;
		}

		SortedKeys.Sort([this](uint32 lhs, uint32 rhs) {

			FCallstackTrack& Left = UniqueCallstacks[lhs];
			FCallstackTrack& Right = UniqueCallstacks[rhs];

			return Right.Size < Left.Size;
		});
	}	


	FOutputDevice* ReportAr = nullptr;
	FArchive* FileAr = nullptr;
	FOutputDeviceArchiveWrapper* FileArWrapper = nullptr;

	if (FileName)
	{
		const FString PathName = *(FPaths::ProfilingDir() + TEXT("MallocLeak/"));
		IFileManager::Get().MakeDirectory(*PathName);
		
		FString FilePath = PathName + CreateProfileFilename(FileName, TEXT(".report"), true);

		FileAr = IFileManager::Get().CreateDebugFileWriter(*FilePath);
		FileArWrapper = new FOutputDeviceArchiveWrapper(FileAr);
		ReportAr = FileArWrapper;

		//UE_LOG(LogConsoleResponse, Log, TEXT("MemReportDeferred: saving to %s"), *FilePath);
	}

#define LOG_OUTPUT(Format, ...) \
	do {\
		if (ReportAr){\
			ReportAr->Logf(Format, ##__VA_ARGS__);\
		}\
		else {\
			FPlatformMisc::LowLevelOutputDebugStringf(Format, ##__VA_ARGS__);\
			FPlatformMisc::LowLevelOutputDebugStringf(TEXT("\n"));\
		}\
	} while(0);\

	uint32 EffectiveFilter = FilterSize ? FilterSize : MinAllocationSize;

	const float InvToMb = 1.0 / (1024 * 1024);
	FPlatformMemoryStats MemoryStats = FPlatformMemory::GetStats();

	LOG_OUTPUT(TEXT("Current Memory: %.02fMB (Peak: %.02fMB)."),
		MemoryStats.UsedPhysical * InvToMb,
		MemoryStats.PeakUsedPhysical * InvToMb);

	LOG_OUTPUT(TEXT("Leak detection allocation filter: %dKB"), MinAllocationSize / 1024);
	LOG_OUTPUT(TEXT("Leak detection report filter: %dKB"), EffectiveFilter / 1024);

	LOG_OUTPUT(TEXT("Found %i open callstacks that hold %uMB of memory on frame: %i"), UniqueCallstacks.Num(), TotalSize / (1024 * 1024), (int32)GFrameCounter);
	LOG_OUTPUT(TEXT("Dumping %d callstacks that hold more than %uKBs and account for %uMB"), SortedKeys.Num(), EffectiveFilter / 1024, ReportedSize / (1024 * 1024));
		
	for (const auto& Key : SortedKeys)
	{
		const FCallstackTrack& Callstack = UniqueCallstacks[Key];
		if (Callstack.Size >= FilterSize)
		{
			bool KnownDeleter = KnownDeleters.Contains(Callstack.GetHash());

			LOG_OUTPUT(TEXT("\nAllocSize: %i KB, Num: %i, FirstFrame %i, LastFrame %i, KnownDeleter: %d")
				, Callstack.Size / 1024
				, Callstack.Count
				, Callstack.FirstFame
				, Callstack.LastFrame
				, KnownDeleter);

			for (int32 i = 0; i < FCallstackTrack::Depth; ++i)
			{
				FPlatformStackWalk::ProgramCounterToHumanReadableString(i, Callstack.CallStack[i], CallstackString, MaxCallstackLineChars);

				if (Callstack.CallStack[i] && FCStringAnsi::Strlen(CallstackString) > 0)
				{
					//convert ansi -> tchar without mallocs in case we are in OOM handler.
					for (int32 CurrChar = 0; CurrChar < MaxCallstackLineChars; ++CurrChar)
					{
						CallstackStringWide[CurrChar] = CallstackString[CurrChar];
					}

					LOG_OUTPUT(TEXT("%s"), CallstackStringWide);
				}
				FMemory::Memzero(CallstackString);
			}			

			TArray<FString> SortedContexts;

			for (const auto& Pair : OpenPointers)
			{
				if (Pair.Value.GetHash() == Key)
				{
					if (PointerContexts.Contains(Pair.Key))
					{
						SortedContexts.Add(*PointerContexts.Find(Pair.Key));
					}
				}
			}

			if (SortedContexts.Num())
			{
				LOG_OUTPUT(TEXT("%d contexts:"), SortedContexts.Num(), CallstackStringWide);

				SortedContexts.Sort();

				for (const auto& Str : SortedContexts)
				{
					LOG_OUTPUT(TEXT("\t%s"), *Str);
				}
			}

			LOG_OUTPUT(TEXT("\n"), CallstackStringWide);
		}
	}

#undef LOG_OUTPUT

	if (FileArWrapper != nullptr)
	{
		FileArWrapper->TearDown();
		delete FileArWrapper;
		delete FileAr;
	}
}

void FMallocLeakDetection::ClearData()
{
	FScopeLock Lock(&AllocatedPointersCritical);
	OpenPointers.Empty();
	UniqueCallstacks.Empty();
	PointerContexts.Empty();
}

void FMallocLeakDetection::Malloc(void* Ptr, SIZE_T Size)
{
	if (Ptr)
	{
		if (bCaptureAllocs && (MinAllocationSize == 0 || Size >= MinAllocationSize))
		{
			FScopeLock Lock(&AllocatedPointersCritical);
			if (!bRecursive)
			{
				bRecursive = true;
				FCallstackTrack Callstack;
				FPlatformStackWalk::CaptureStackBackTrace(Callstack.CallStack, FCallstackTrack::Depth);
				Callstack.FirstFame = GFrameCounter;
				Callstack.LastFrame = GFrameCounter;
				Callstack.Size = Size;
				AddCallstack(Callstack);
				OpenPointers.Add(Ptr, Callstack);

				if (Contexts.Num())
				{
					PointerContexts.Add(Ptr, Contexts.Top());
				}

				bRecursive = false;
			}
		}		
	}
}

void FMallocLeakDetection::Realloc(void* OldPtr, void* NewPtr, SIZE_T Size)
{	
	if (bCaptureAllocs)
	{
		FScopeLock Lock(&AllocatedPointersCritical);		
		Free(OldPtr);
		Malloc(NewPtr, Size);
	}
}

void FMallocLeakDetection::Free(void* Ptr)
{
	if (Ptr)
	{
		if (bCaptureAllocs)
		{
			FScopeLock Lock(&AllocatedPointersCritical);			
			if (!bRecursive)
			{
				bRecursive = true;
				FCallstackTrack* Callstack = OpenPointers.Find(Ptr);
				if (Callstack)
				{
					RemoveCallstack(*Callstack);
					KnownDeleters.Add(Callstack->GetHash());
				}
				OpenPointers.Remove(Ptr);
				PointerContexts.Remove(Ptr);
				bRecursive = false;
			}
		}
	}
}

FMallocLeakDetectionProxy::FMallocLeakDetectionProxy(FMalloc* InMalloc)
	: UsedMalloc(InMalloc)
	, Verify(FMallocLeakDetection::Get())
{
	
}

#endif // MALLOC_LEAKDETECTION