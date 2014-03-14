// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "CorePrivate.h"
#include "WindowsRunnableThread.h"
#include "ExceptionHandling.h"

DEFINE_LOG_CATEGORY_STATIC(LogThreadingWindows, Log, All);

uint32 FRunnableThreadWin::GuardedRun()
{
	uint32 ExitCode = 1;

#if PLATFORM_XBOXONE
	if( ThreadAffintyMask )
	{
		SetThreadAffinityMask(ThreadAffintyMask);
	}
	UE_LOG(LogThreadingWindows, Log, TEXT("Runnable thread %s is on Process %d."), *ThreadName  , static_cast<uint32>(::GetCurrentProcessorNumber()) );
#endif

#if !PLATFORM_SEH_EXCEPTIONS_DISABLED
	if( !FPlatformMisc::IsDebuggerPresent() || GAlwaysReportCrash )
	{
		__try
		{
			ExitCode = Run();
		}
		__except( ReportCrash( GetExceptionInformation() ) )
		{
			// Make sure the information which thread crashed makes it into the log.
			UE_LOG(LogThreadingWindows, Error, TEXT("Runnable thread %s crashed."), *ThreadName);
			GWarn->Flush();

			// Append the thread name at the end of the error report.
			FCString::Strncat(GErrorHist, TEXT("\r\nCrash in runnable thread "), ARRAY_COUNT(GErrorHist) - 1);
			FCString::Strncat(GErrorHist, *ThreadName, ARRAY_COUNT(GErrorHist) - 1);

			ExitCode = 1;
			// Generate status report.				
			GError->HandleError();
			// Throw an error so that main thread shuts down too (otherwise task graph stalls forever).
			UE_LOG(LogThreadingWindows, Fatal, TEXT("Runnable thread %s crashed."), *ThreadName);
		}
	}
	else
#endif
	{
		ExitCode = Run();
	}

	return ExitCode;
}

uint32 FRunnableThreadWin::Run()
{
	// Assume we'll fail init
	uint32 ExitCode = 1;
	check(Runnable);

#if PLATFORM_XBOXONE
	if( ThreadAffintyMask )
	{
		SetThreadAffinityMask(ThreadAffintyMask);
	}
	UE_LOG(LogThreadingWindows, Log, TEXT("Runnable thread %s is on Process %d."), *ThreadName  , static_cast<uint32>(::GetCurrentProcessorNumber()) );
#endif

	// Initialize the runnable object
	if (Runnable->Init() == true)
	{
		// Initialization has completed, release the sync event
		ThreadInitSyncEvent->Trigger();
		// Now run the task that needs to be done
		ExitCode = Runnable->Run();
		// Allow any allocated resources to be cleaned up
		Runnable->Exit();
	}
	else
	{
		// Initialization has failed, release the sync event
		ThreadInitSyncEvent->Trigger();
	}

	// Should we delete the runnable?
	if (bShouldDeleteRunnable == true)
	{
		delete Runnable;
		Runnable = NULL;
	}
	// Clean ourselves up without waiting
	if (bShouldDeleteSelf == true)
	{
		// Make sure the caller knows we want to delete this thread if we're still int CreateInternal.
		WantsToDeleteSelf.Increment();
		// Wait until the caller has finished setting up this thread in case Runnable execution was very short.
		ThreadCreatedSyncEvent->Wait();
		// Now clean up the thread handle so we don't leak
		CloseHandle(Thread);
		Thread = NULL;
		delete this;
	}
	return ExitCode;
}
