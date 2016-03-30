// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "CorePrivatePCH.h"
#include "ThreadHeartBeat.h"

FThreadHeartBeat::FThreadHeartBeat()
: Thread(nullptr)
{
	// We don't care about programs for now so no point in spawning the extra thread
#if !IS_PROGRAM
	Thread = FRunnableThread::Create(this, TEXT("FHeartBeatThread"), 0, TPri_BelowNormal);
#endif
}

FThreadHeartBeat::~FThreadHeartBeat()
{
	delete Thread;
	Thread = nullptr;
}

FThreadHeartBeat& FThreadHeartBeat::Get()
{
	static FThreadHeartBeat Singleton;
	return Singleton;
}

	//~ Begin FRunnable Interface.
bool FThreadHeartBeat::Init()
{
	return true;
}

uint32 FThreadHeartBeat::Run()
{
	while (StopTaskCounter.GetValue() == 0)
	{
		uint32 ThreadThatHung = CheckHeartBeat();
		if (ThreadThatHung != FThreadHeartBeat::InvalidThreadId)
		{
			const SIZE_T StackTraceSize = 65535;
			ANSICHAR* StackTrace = (ANSICHAR*)GMalloc->Malloc(StackTraceSize);
			StackTrace[0] = 0;
			// Walk the stack and dump it to the allocated memory. This process usually allocates a lot of memory.
			FPlatformStackWalk::ThreadStackWalkAndDump(StackTrace, StackTraceSize, 0, ThreadThatHung);
			FString StackTraceText(StackTrace);
			TArray<FString> StackLines;
			StackTraceText.ParseIntoArrayLines(StackLines);

			FString ThreadName(ThreadThatHung == GGameThreadId ? TEXT("GameThread") : FThreadManager::Get().GetThreadName(ThreadThatHung));
			if (ThreadName.IsEmpty())
			{
				ThreadName = FString::Printf(TEXT("unknown thread (%u)"), ThreadThatHung);
			}
			UE_LOG(LogCore, Error, TEXT("Infinite stall detected on %s:"), *ThreadName);
			for (FString& StackLine : StackLines)
			{
				UE_LOG(LogCore, Error, TEXT("%s"), *StackLine);
			}
			
			FString StackTrimmed;
			for (int32 LineIndex = 0; LineIndex < StackLines.Num() && StackTrimmed.Len() < 512; ++LineIndex)
			{
				StackTrimmed += StackLines[LineIndex];
				StackTrimmed += TEXT("\n");
			}
			UE_LOG(LogCore, Fatal, TEXT("Infinite stall detected on %s:\n%s"), *ThreadName, *StackTrimmed);
		}
		FPlatformProcess::SleepNoStats(0.5f);
	}

	return 0;
}

void FThreadHeartBeat::Stop()
{
	StopTaskCounter.Increment();
}

void FThreadHeartBeat::HeartBeat()
{
	uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
	FScopeLock HeartBeatLock(&HeartBeatCritical);
	double& HeartBeatTime = ThreadHeartBeat.FindOrAdd(ThreadId);
	HeartBeatTime = FPlatformTime::Seconds();
}

uint32 FThreadHeartBeat::CheckHeartBeat()
{
	// Editor and debug builds run too slow to measure them correctly
#if !WITH_EDITORONLY_DATA && !IS_PROGRAM && !UE_BUILD_DEBUG
	if (!GIsRequestingExit && !FPlatformMisc::IsDebuggerPresent())
	{
		const double HangDuration = 25.0;
		const double CurrentTime = FPlatformTime::Seconds();
		FScopeLock HeartBeatLock(&HeartBeatCritical);
		for (TPair<uint32, double>& LastHeartBeat : ThreadHeartBeat)
		{
			if ((CurrentTime - LastHeartBeat.Value) > HangDuration)
			{
				LastHeartBeat.Value = CurrentTime;
				return LastHeartBeat.Key;
			}
		}
	}
#endif
	return InvalidThreadId;
}

void FThreadHeartBeat::KillHeartBeat()
{
	uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
	FScopeLock HeartBeatLock(&HeartBeatCritical);
	ThreadHeartBeat.Remove(ThreadId);
}