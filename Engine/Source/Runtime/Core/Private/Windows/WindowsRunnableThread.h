// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * This is the base interface for all runnable thread classes. It specifies the
 * methods used in managing its life cycle.
 */
class FRunnableThreadWin : public FRunnableThread
{
	/**
	 * The thread handle for the thread
	 */
	HANDLE Thread;

	/**
	 * The runnable object to execute on this thread
	 */
	FRunnable* Runnable;

	/** 
	 * Sync event to make sure that Init() has been completed before allowing the main thread to continue
	 */
	FEvent* ThreadInitSyncEvent;

	/** 
	 * Sync event to make sure that CreateInteral() has been completed before allowing the thread to be auto-deleted
	 */
	FEvent* ThreadCreatedSyncEvent;

	/** 
	 * Flag used when the thread is waiting for the caller to finish setting it up before it can delete itself.
	 */
	FThreadSafeCounter WantsToDeleteSelf;

	/**
	 * Whether we should delete ourselves on thread exit
	 */
	bool bShouldDeleteSelf;

	/**
	 * Whether we should delete the runnable on thread exit
	 */
	bool bShouldDeleteRunnable;

	/**
	 * The priority to run the thread at
	 */
	EThreadPriority ThreadPriority;

	/**
	 * The Affinity to run the thread with
	 */
	uint64 ThreadAffintyMask;

	/**
	* ID set during thread creation
	*/
	uint32 ThreadID;

	/**
	 * Name of the thread.
	 */
	FString ThreadName;

	/**
	 * Helper function to set thread names, visible by the debugger.
	 * @param ThreadID		Thread ID for the thread who's name is going to be set
	 * @param ThreadName	Name to set
	 */
	static void SetThreadName( uint32 ThreadID, LPCSTR ThreadName )
	{
		/**
		 * Code setting the thread name for use in the debugger.
		 *
		 * http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
		 */
		const uint32 MS_VC_EXCEPTION=0x406D1388;

		struct THREADNAME_INFO
		{
			uint32 dwType;		// Must be 0x1000.
			LPCSTR szName;		// Pointer to name (in user addr space).
			uint32 dwThreadID;	// Thread ID (-1=caller thread).
			uint32 dwFlags;		// Reserved for future use, must be zero.
		};

		// on the xbox setting thread names messes up the XDK COM API that UnrealConsole uses so check to see if they have been
		// explicitly enabled
		Sleep(10);
		THREADNAME_INFO ThreadNameInfo;
		ThreadNameInfo.dwType		= 0x1000;
		ThreadNameInfo.szName		= ThreadName;
		ThreadNameInfo.dwThreadID	= ThreadID;
		ThreadNameInfo.dwFlags		= 0;

#if !PLATFORM_SEH_EXCEPTIONS_DISABLED
		__try
#endif
		{
			RaiseException( MS_VC_EXCEPTION, 0, sizeof(ThreadNameInfo)/sizeof(ULONG_PTR), (ULONG_PTR*)&ThreadNameInfo );
		}
#if !PLATFORM_SEH_EXCEPTIONS_DISABLED
		__except( EXCEPTION_EXECUTE_HANDLER )
		{
		}
#endif
	}

	/**
	 * The thread entry point. Simply forwards the call on to the right
	 * thread main function
	 */
	static ::DWORD STDCALL _ThreadProc(LPVOID pThis)
	{
		check(pThis);
		return ((FRunnableThreadWin*)pThis)->GuardedRun();
	}

	/** Guarding works only if debugger is not attached or GAlwaysReportCrash is true. */
	uint32 GuardedRun();

	/**
	 * The real thread entry point. It calls the Init/Run/Exit methods on
	 * the runnable object
	 */
	uint32 Run();

public:
	FRunnableThreadWin()
		: Thread(NULL)
		, Runnable(NULL)
		, ThreadInitSyncEvent(NULL)
		, ThreadCreatedSyncEvent(NULL)
		, WantsToDeleteSelf(0)
		, bShouldDeleteSelf(false)
		, bShouldDeleteRunnable(false)
		, ThreadPriority(TPri_Normal)
		, ThreadID(NULL)
	{

	}

	~FRunnableThreadWin()
	{
		// Clean up our thread if it is still active
		if (Thread != NULL)
		{
			Kill(true);
		}
		FRunnableThread::GetThreadRegistry().Remove(ThreadID);
		delete ThreadCreatedSyncEvent;
	}
	
	virtual void SetThreadPriority(EThreadPriority NewPriority) OVERRIDE
	{
		// Don't bother calling the OS if there is no need
		if (NewPriority != ThreadPriority)
		{
			ThreadPriority = NewPriority;
			// Change the priority on the thread
			::SetThreadPriority(Thread,
				ThreadPriority == TPri_AboveNormal ? THREAD_PRIORITY_ABOVE_NORMAL :
				ThreadPriority == TPri_BelowNormal ? THREAD_PRIORITY_BELOW_NORMAL :
				THREAD_PRIORITY_NORMAL);
		}
	}

	virtual void SetThreadAffinityMask(uint64 AffinityMask) OVERRIDE
	{
		// Change the processor affinity on the thread
		::SetThreadAffinityMask(Thread, (DWORD_PTR)AffinityMask);
	}

	virtual void Suspend(bool bShouldPause = 1) OVERRIDE
	{
		check(Thread);
		if (bShouldPause == true)
		{
			SuspendThread(Thread);
		}
		else
		{
			ResumeThread(Thread);
		}
	}

	virtual bool Kill(bool bShouldWait = false) OVERRIDE
	{
		check(Thread && "Did you forget to call Create()?");
		bool bDidExitOK = true;
		// Let the runnable have a chance to stop without brute force killing
		if (Runnable)
		{
			Runnable->Stop();
		}
		// If waiting was specified, wait the amount of time. If that fails,
		// brute force kill that thread. Very bad as that might leak.
		if (bShouldWait == true)
		{
			// Wait indefinitely for the thread to finish.  IMPORTANT:  It's not safe to just go and
			// kill the thread with TerminateThread() as it could have a mutex lock that's shared
			// with a thread that's continuing to run, which would cause that other thread to
			// dead-lock.  (This can manifest itself in code as simple as the synchronization
			// object that is used by our logging output classes.  Trust us, we've seen it!)
			WaitForSingleObject(Thread,INFINITE);
		}
		// Now clean up the thread handle so we don't leak
		CloseHandle(Thread);
		Thread = NULL;
		// delete the runnable if requested and we didn't shut down gracefully already.
		if (Runnable && bShouldDeleteRunnable == true)
		{
			delete Runnable;
			Runnable = NULL;
		}
		// Delete ourselves if requested and we didn't shut down gracefully already.
		// This check prevents a double-delete of self when we shut down gracefully.
		if (!bDidExitOK && bShouldDeleteSelf == true)
		{
			delete this;
		}
		return bDidExitOK;
	}

	virtual void WaitForCompletion() OVERRIDE
	{
		// Block until this thread exits
		WaitForSingleObject(Thread,INFINITE);
	}

	virtual uint32 GetThreadID() OVERRIDE
	{
		return ThreadID;
	}

	virtual FString GetThreadName() OVERRIDE
	{
		return ThreadName;
	}

protected:
	virtual bool CreateInternal(FRunnable* InRunnable, const TCHAR* InThreadName,
		bool bAutoDeleteSelf = false, bool bAutoDeleteRunnable = false, uint32 InStackSize = 0,
		EThreadPriority InThreadPri = TPri_Normal, uint64 InThreadAffinityMask = 0) OVERRIDE
	{
		check(InRunnable);
		Runnable = InRunnable;
		bShouldDeleteSelf = bAutoDeleteSelf;
		bShouldDeleteRunnable = bAutoDeleteRunnable;
		ThreadAffintyMask = InThreadAffinityMask;

		// Create a sync event to guarantee the Init() function is called first
		ThreadInitSyncEvent	= FPlatformProcess::CreateSynchEvent(true);
		// Create a sync event to guarantee the thread will not delete itself until it has been fully set up.
		ThreadCreatedSyncEvent = FPlatformProcess::CreateSynchEvent(true);

		// Create the new thread
		Thread = CreateThread(NULL,InStackSize,_ThreadProc,this,STACK_SIZE_PARAM_IS_A_RESERVATION,(::DWORD *)&ThreadID);
		// If it fails, clear all the vars
		if (Thread == NULL)
		{
			if (bAutoDeleteRunnable == true)
			{
				delete InRunnable;
			}
			Runnable = NULL;
		}
		else
		{
			// Let the thread start up, then set the name for debug purposes.
			ThreadInitSyncEvent->Wait(INFINITE);
			ThreadName = InThreadName ? InThreadName : TEXT("Unnamed UE4");
			SetThreadName( ThreadID, TCHAR_TO_ANSI( *ThreadName ) );
			FRunnableThread::GetThreadRegistry().Add(ThreadID, this);
			SetThreadPriority(InThreadPri);
		}

		// Cleanup the sync event
		delete ThreadInitSyncEvent;
		ThreadInitSyncEvent = NULL;
		return Thread != NULL;
	}

	virtual bool NotifyCreated() OVERRIDE
	{
		const bool bHasFinished = !!WantsToDeleteSelf.GetValue();
		// It's ok to delete this thread if it wants to delete self.
		ThreadCreatedSyncEvent->Trigger();
		return bHasFinished;
	}
};

