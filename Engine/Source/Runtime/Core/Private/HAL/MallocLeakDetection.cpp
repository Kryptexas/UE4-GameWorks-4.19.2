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
	, bDumpOustandingAllocs(false)
{
}

FMallocLeakDetection& FMallocLeakDetection::Get()
{
	static FMallocLeakDetection Singleton;
	return Singleton;
}

FMallocLeakDetection::~FMallocLeakDetection()
{	
}

bool FMallocLeakDetection::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	if (FParse::Command(&Cmd, TEXT("mallocleak")))
	{
		if (FParse::Command(&Cmd, TEXT("start")))
		{
			UE_LOG(LogConsoleResponse, Display, TEXT("Starting allocation tracking."));
			SetAllocationCollection(true);
		}
		else if (FParse::Command(&Cmd, TEXT("stop")))
		{
			uint32 FilterSize = 128 * 1024;
			FParse::Value(Cmd, TEXT("filtersize="), FilterSize);

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
			uint32 FilterSize = 128 * 1024;
			FParse::Value(Cmd, TEXT("filtersize="), FilterSize);

			UE_LOG(LogConsoleResponse, Display, TEXT("Dumping unique calltacks with %i bytes or more oustanding."), FilterSize);
			DumpOpenCallstacks(FilterSize);
		}
		else
		{
			UE_LOG(LogConsoleResponse, Display, TEXT("'MallocLeak Start'  Begins accumulating unique callstacks\n")
				TEXT("'MallocLeak Stop filtersize=[Size]'  Dumps outstanding unique callstacks and stops accumulation (with an optional filter size in bytes, if not specified 128 KB is assumed).  Also clears data.\n")
				TEXT("'MallocLeak Clear'  Clears all accumulated data.  Does not change start/stop state.\n")
				TEXT("'MallocLeak Dump filtersize=[Size]' Dumps oustanding unique callstacks with optional size filter in bytes (if size is not specified it is assumed to be 128 KB).\n"));
		}

		return true;
	}
	return false;
}

void FMallocLeakDetection::AddCallstack(FCallstackTrack& Callstack)
{
	FScopeLock Lock(&AllocatedPointersCritical);
	uint32 CallstackHash = FCrc::MemCrc32(Callstack.CallStack, sizeof(Callstack.CallStack), 0);	
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

void FMallocLeakDetection::SetAllocationCollection(bool bEnabled)
{
	FScopeLock Lock(&AllocatedPointersCritical);
	bCaptureAllocs = bEnabled;
}

void FMallocLeakDetection::DumpOpenCallstacks(uint32 FilterSize)
{
	//could be called when OOM so avoid UE_LOG functions.
	FScopeLock Lock(&AllocatedPointersCritical);
	FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Dumping out of %i possible open callstacks filtered with more than %u bytes on frame: %i\n"), UniqueCallstacks.Num(), FilterSize, (int32)GFrameCounter);
	const int32 MaxCallstackLineChars = 2048;
	char CallstackString[MaxCallstackLineChars];
	TCHAR CallstackStringWide[MaxCallstackLineChars];
	FMemory::Memzero(CallstackString);
	for (const auto& Pair : UniqueCallstacks)
	{
		const FCallstackTrack& Callstack = Pair.Value;
		if (Callstack.Size >= FilterSize)
		{
			FPlatformMisc::LowLevelOutputDebugStringf(TEXT("AllocSize: %i, Num: %i, FirstFrameEverAllocated: %i\n"), Callstack.Size, Callstack.Count, Callstack.FrameNumber);
			for (int32 i = 0; i < FCallstackTrack::Depth; ++i)
			{
				FPlatformStackWalk::ProgramCounterToHumanReadableString(i, Callstack.CallStack[i], CallstackString, MaxCallstackLineChars);
				//convert ansi -> tchar without mallocs in case we are in OOM handler.
				for (int32 CurrChar = 0; CurrChar < MaxCallstackLineChars; ++CurrChar)
				{
					CallstackStringWide[CurrChar] = CallstackString[CurrChar];
				}
				FPlatformMisc::LowLevelOutputDebugStringf(TEXT("%s\n"), CallstackStringWide);
				FMemory::Memzero(CallstackString);
			}
		}
	}
}

void FMallocLeakDetection::ClearData()
{
	FScopeLock Lock(&AllocatedPointersCritical);
	OpenPointers.Empty();
	UniqueCallstacks.Empty();
}

void FMallocLeakDetection::Malloc(void* Ptr, SIZE_T Size)
{
	if (Ptr)
	{
		if (bCaptureAllocs)
		{
			FScopeLock Lock(&AllocatedPointersCritical);
			if (!bRecursive)
			{
				bRecursive = true;
				FCallstackTrack Callstack;
				FPlatformStackWalk::CaptureStackBackTrace(Callstack.CallStack, FCallstackTrack::Depth);
				Callstack.FrameNumber = GFrameCounter;
				Callstack.Size = Size;
				AddCallstack(Callstack);
				OpenPointers.Add(Ptr, Callstack);
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
				}
				OpenPointers.Remove(Ptr);
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