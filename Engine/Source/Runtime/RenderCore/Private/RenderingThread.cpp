// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RenderingThread.cpp: Rendering thread implementation.
=============================================================================*/

#include "RenderCore.h"
#include "RenderingThread.h"
#include "RHI.h"
#include "TickableObjectRenderThread.h"
#include "ExceptionHandling.h"
#include "TaskGraphInterfaces.h"

//
// Globals
//



RENDERCORE_API bool GIsThreadedRendering = false;
RENDERCORE_API bool GUseThreadedRendering = false;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	RENDERCORE_API bool GMainThreadBlockedOnRenderThread = false;
#endif // #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

static FRunnable* GRenderingThreadRunnable = NULL;

/** If the rendering thread has been terminated by an unhandled exception, this contains the error message. */
FString GRenderingThreadError;

/**
 * Polled by the game thread to detect crashes in the rendering thread.
 * If the rendering thread crashes, it sets this variable to false.
 */
volatile bool GIsRenderingThreadHealthy = true;

RENDERCORE_API bool GGameThreadWantsToSuspendRendering = false;

/** Whether the rendering thread is suspended (not even processing the tickables) */
volatile int32 GIsRenderingThreadSuspended = 0;

/** Thread used for rendering */
RENDERCORE_API FRunnableThread* GRenderingThread = NULL;

// Unlike IsInRenderingThread, this will always return false if we are running single threaded. It only returns true if this is actually a separate rendering thread. Mostly useful for checks
RENDERCORE_API bool IsInActualRenderingThread()
{
	return GRenderingThread && FPlatformTLS::GetCurrentThreadId() == GRenderingThread->GetThreadID();
}

RENDERCORE_API bool IsInRenderingThread()
{
	return !GRenderingThread || GIsRenderingThreadSuspended || (FPlatformTLS::GetCurrentThreadId() == GRenderingThread->GetThreadID());
}

/**
 * Maximum rate the rendering thread will tick tickables when idle (in Hz)
 */
float GRenderingThreadMaxIdleTickFrequency = 40.f;

/** Function to stall the rendering thread **/
static void SuspendRendering()
{
	FPlatformAtomics::InterlockedIncrement(&GIsRenderingThreadSuspended);
	FPlatformMisc::MemoryBarrier();
}

/** Function to wait and resume rendering thread **/
static void WaitAndResumeRendering()
{
	while ( GIsRenderingThreadSuspended )
	{
		// Just sleep a little bit.
		FPlatformProcess::Sleep( 0.001f ); //@todo this should be a more principled wait
	}
    
    // set the thread back to real time mode
    FPlatformProcess::SetRealTimeMode();
}

/**
 *	Constructor that flushes and suspends the renderthread
 *	@param bRecreateThread	- Whether the rendering thread should be completely destroyed and recreated, or just suspended.
 */
FSuspendRenderingThread::FSuspendRenderingThread( bool bInRecreateThread )
{
	bRecreateThread = bInRecreateThread;
	bUseRenderingThread = GUseThreadedRendering;
	bWasRenderingThreadRunning = GIsThreadedRendering;
	if ( bRecreateThread )
	{
		GUseThreadedRendering = false;
		StopRenderingThread();
		FPlatformAtomics::InterlockedIncrement( &GIsRenderingThreadSuspended );
	}
	else
	{
		if ( GIsRenderingThreadSuspended == 0 )
		{
			// First tell the render thread to finish up all pending commands and then suspend its activities.
			
			// this ensures that async stuff will be completed too
			FlushRenderingCommands();
			
			if (GIsThreadedRendering)
			{
				FGraphEventRef CompleteHandle = FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
					FSimpleDelegateGraphTask::FDelegate::CreateStatic(&SuspendRendering),
					TEXT("SuspendRendering"),
					NULL,
					ENamedThreads::RenderThread);

				// Busy wait while Kismet debugging, to avoid opportunistic execution of game thread tasks
				// If the game thread is already executing tasks, then we have no choice but to spin
				if (GIntraFrameDebuggingGameThread || FTaskGraphInterface::Get().IsThreadProcessingTasks(ENamedThreads::GameThread) ) 
				{
					while (!GIsRenderingThreadSuspended)
					{
						FPlatformProcess::Sleep(0.0f);
					}
				}
				else
				{
					FTaskGraphInterface::Get().WaitUntilTaskCompletes(CompleteHandle, ENamedThreads::GameThread);
				}
				check(GIsRenderingThreadSuspended);
			
				// Now tell the render thread to busy wait until it's resumed
				FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
					FSimpleDelegateGraphTask::FDelegate::CreateStatic(&WaitAndResumeRendering),
					TEXT("WaitAndResumeRendering"),
					NULL,
					ENamedThreads::RenderThread);
			}
			else
			{
				SuspendRendering();
			}
		}
		else
		{
			// The render-thread is already suspended. Just bump the ref-count.
			FPlatformAtomics::InterlockedIncrement( &GIsRenderingThreadSuspended );
		}
	}
}

/** Destructor that starts the renderthread again */
FSuspendRenderingThread::~FSuspendRenderingThread()
{
	if ( bRecreateThread )
	{
		GUseThreadedRendering = bUseRenderingThread;
		FPlatformAtomics::InterlockedDecrement( &GIsRenderingThreadSuspended );
		if ( bUseRenderingThread && bWasRenderingThreadRunning )
		{
			StartRenderingThread();
            
            // Now tell the render thread to set it self to real time mode
            FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
                FSimpleDelegateGraphTask::FDelegate::CreateStatic(&FPlatformProcess::SetRealTimeMode),
                TEXT("SetRealTimeMode"),
                NULL,
                ENamedThreads::RenderThread);
        }
	}
	else
	{
		// Resume the render thread again. 
		FPlatformAtomics::InterlockedDecrement( &GIsRenderingThreadSuspended );
	}
}


/**
 * Tick all rendering thread tickable objects
 */

/** Static array of tickable objects that are ticked from rendering thread*/
FTickableObjectRenderThread::FRenderingThreadTickableObjectsArray FTickableObjectRenderThread::RenderingThreadTickableObjects;

void TickRenderingTickables()
{
	static double LastTickTime = FPlatformTime::Seconds();

	// calc how long has passed since last tick
	double CurTime = FPlatformTime::Seconds();
	float DeltaSeconds = CurTime - LastTickTime;
	
	if (DeltaSeconds < (1.f/GRenderingThreadMaxIdleTickFrequency))
	{
		return;
	}

	uint32 ObjectsThatResumedRendering = 0;

	// tick any rendering thread tickables
	for (int32 ObjectIndex = 0; ObjectIndex < FTickableObjectRenderThread::RenderingThreadTickableObjects.Num(); ObjectIndex++)
	{
		FTickableObjectRenderThread* TickableObject = FTickableObjectRenderThread::RenderingThreadTickableObjects[ObjectIndex];
		// make sure it wants to be ticked and the rendering thread isn't suspended
		if (TickableObject->IsTickable())
		{
			if (GGameThreadWantsToSuspendRendering && TickableObject->NeedsRenderingResumedForRenderingThreadTick())
			{
				ObjectsThatResumedRendering++;
			}
			STAT(FScopeCycleCounter(TickableObject->GetStatId());)
			TickableObject->Tick(DeltaSeconds);
		}
	}
	// update the last time we ticked
	LastTickTime = CurTime;

	// if no ticked objects resumed rendering, make sure we're suspended if game thread wants us to be
	if (ObjectsThatResumedRendering == 0 && GGameThreadWantsToSuspendRendering)
	{
		// Nothing to do?
	}
}

/** Accumulates how many cycles the renderthread has been idle. It's defined in RenderingThread.cpp. */
uint32 GRenderThreadIdle[ERenderThreadIdleTypes::Num] = {0};
/** Accumulates how times renderthread was idle. It's defined in RenderingThread.cpp. */
uint32 GRenderThreadNumIdle[ERenderThreadIdleTypes::Num] = {0};
/** How many cycles the renderthread used (excluding idle time). It's set once per frame in FViewport::Draw. */
uint32 GRenderThreadTime = 0;

/** The rendering thread main loop */
void RenderingThreadMain( FEvent* TaskGraphBoundSyncEvent )
{
	ENamedThreads::RenderThread = ENamedThreads::Type(ENamedThreads::ActualRenderingThread);
	ENamedThreads::RenderThread_Local = ENamedThreads::Type(ENamedThreads::ActualRenderingThread_Local);
	FTaskGraphInterface::Get().AttachToThread(ENamedThreads::RenderThread);
	FPlatformMisc::MemoryBarrier();

	// Inform main thread that the render thread has been attached to the taskgraph and is ready to receive tasks
	if( TaskGraphBoundSyncEvent != NULL )
	{
		TaskGraphBoundSyncEvent->Trigger();
	}

	check(GIsThreadedRendering);
	FTaskGraphInterface::Get().ProcessThreadUntilRequestReturn(ENamedThreads::RenderThread);
	FPlatformMisc::MemoryBarrier();
	check(!GIsThreadedRendering);
	ENamedThreads::RenderThread = ENamedThreads::GameThread;
	ENamedThreads::RenderThread_Local = ENamedThreads::GameThread_Local;
	FPlatformMisc::MemoryBarrier();

#if STATS
	FThreadStats::ExplicitFlush();
#endif
}

/**
 * Advances stats for the rendering thread.
 */
void RenderingThreadTick(int64 StatsFrame, int32 MasterDisableChangeTagStartFrame)
{
#if STATS
	int64 Frame = StatsFrame;
	if (!FThreadStats::IsCollectingData() ||  MasterDisableChangeTagStartFrame != FThreadStats::MasterDisableChangeTag())
	{
		Frame = -StatsFrame; // mark this as a bad frame
	}
	static FStatNameAndInfo Adv(NAME_AdvanceFrame, "", TEXT(""), EStatDataType::ST_int64, true, false);
	FThreadStats::AddMessage(Adv.GetEncodedName(), EStatOperation::AdvanceFrameEventRenderThread, Frame);
	if( IsInActualRenderingThread() )
	{
		FThreadStats::ExplicitFlush();
	}
#endif
}

/** The rendering thread runnable object. */
class FRenderingThread : public FRunnable
{
public:
	/** 
	 * Sync event to make sure that render thread is bound to the task graph before main thread queues work against it.
	 */
	FEvent* TaskGraphBoundSyncEvent;

	FRenderingThread() : FRunnable()
	{
		TaskGraphBoundSyncEvent	= FPlatformProcess::CreateSynchEvent(true);
		RHIFlushResources();
	}

	// FRunnable interface.
	virtual bool Init(void) 
	{ 
		// Acquire rendering context ownership on the current thread
		RHIAcquireThreadOwnership();

		return true; 
	}

	virtual void Exit(void) 
	{
		// Release rendering context ownership on the current thread
		RHIReleaseThreadOwnership();
	}

	virtual void Stop(void)
	{
	}

	virtual uint32 Run(void)
	{
		GRenderThreadId = FPlatformTLS::GetCurrentThreadId();

#if PLATFORM_WINDOWS
		if ( !FPlatformMisc::IsDebuggerPresent() || GAlwaysReportCrash )
		{
#if !PLATFORM_SEH_EXCEPTIONS_DISABLED
			__try
#endif
			{
				RenderingThreadMain( TaskGraphBoundSyncEvent );
			}
#if !PLATFORM_SEH_EXCEPTIONS_DISABLED
			__except( ReportCrash( GetExceptionInformation() ) )
			{
#if WITH_EDITORONLY_DATA
				GRenderingThreadError = GErrorHist;
#endif

				// Use a memory barrier to ensure that the game thread sees the write to GRenderingThreadError before
				// the write to GIsRenderingThreadHealthy.
				FPlatformMisc::MemoryBarrier();

				GIsRenderingThreadHealthy = false;
			}
#endif
		}
		else
#endif // PLATFORM_WINDOWS
		{
			RenderingThreadMain( TaskGraphBoundSyncEvent );
		}
		GRenderThreadId = 0;
#if STATS
		FThreadStats::ExplicitFlush();
		FThreadStats::Shutdown();
#endif

		return 0;
	}
};

/**
 * If the rendering thread is in its idle loop (which ticks rendering tickables
 */
volatile bool GRunRenderingThreadHeartbeat = false;

/** The rendering thread heartbeat runnable object. */
class FRenderingThreadTickHeartbeat : public FRunnable
{
public:

	// FRunnable interface.
	virtual bool Init(void) 
	{ 
		return true; 
	}

	virtual void Exit(void) 
	{
	}

	virtual void Stop(void)
	{
	}

	virtual uint32 Run(void)
	{
		while(GRunRenderingThreadHeartbeat)
		{
			FPlatformProcess::Sleep(1.f/(4.0f * GRenderingThreadMaxIdleTickFrequency));
			if (!GIsRenderingThreadSuspended)
			{
				ENQUEUE_UNIQUE_RENDER_COMMAND(
					HeartbeatTickTickables,
				{
					// make sure that rendering thread tickables get a chance to tick, even if the render thread is starving
					if (!GIsRenderingThreadSuspended)
					{
						TickRenderingTickables();
					}
				});
			}
		}
		return 0;
	}
};

FRunnableThread* GRenderingThreadHeartbeat = NULL;
FRunnable* GRenderingThreadRunnableHeartbeat = NULL;

// not done in the CVar system as we don't access to render thread specifics there
struct FConsoleRenderThreadPropagation : public IConsoleThreadPropagation
{
	virtual void OnCVarChange(int32& Dest, int32 NewValue)
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			OnCVarChange1,
			int32&, Dest, Dest,
			int32, NewValue, NewValue,
		{
			Dest = NewValue;
		});
	}
	
	virtual void OnCVarChange(float& Dest, float NewValue)
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			OnCVarChange2,
			float&, Dest, Dest,
			float, NewValue, NewValue,
		{
			Dest = NewValue;
		});
	}
	
	virtual void OnCVarChange(FString& Dest, const FString& NewValue)
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			OnCVarChange3,
			FString&, Dest, Dest,
			const FString&, NewValue, NewValue,
		{
			Dest = NewValue;
		});
	}

	static FConsoleRenderThreadPropagation& GetSingleton()
	{
		static FConsoleRenderThreadPropagation This;

		return This;
	}

};

void StartRenderingThread()
{
	static uint32 ThreadCount = 0;
	check(!GIsThreadedRendering && GUseThreadedRendering);

	// Turn on the threaded rendering flag.
	GIsThreadedRendering = true;

	// Create the rendering thread.
	GRenderingThreadRunnable = new FRenderingThread();

	EThreadPriority RenderingThreadPrio = TPri_Normal;

	GRenderingThread = FRunnableThread::Create(GRenderingThreadRunnable, *FString::Printf(TEXT("RenderingThread %d"), ThreadCount), 0, 0, 0, RenderingThreadPrio);

	// Wait for render thread to have taskgraph bound before we dispatch any tasks for it.
	((FRenderingThread*)GRenderingThreadRunnable)->TaskGraphBoundSyncEvent->Wait();

	// register
	IConsoleManager::Get().RegisterThreadPropagation(GRenderingThread->GetThreadID(), &FConsoleRenderThreadPropagation::GetSingleton());

	// ensure the thread has actually started and is idling
	FRenderCommandFence Fence;
	Fence.BeginFence();
	Fence.Wait();

	GRunRenderingThreadHeartbeat = true;
	// Create the rendering thread heartbeat
	GRenderingThreadRunnableHeartbeat = new FRenderingThreadTickHeartbeat();

	GRenderingThreadHeartbeat = FRunnableThread::Create(GRenderingThreadRunnableHeartbeat, *FString::Printf(TEXT("RTHeartBeat %d"), ThreadCount), 0, 0, 16 * 1024, TPri_AboveNormal);

	ThreadCount++;
}

void StopRenderingThread()
{
	// This function is not thread-safe. Ensure it is only called by the main game thread.
	check( IsInGameThread() );
	
	// unregister
	IConsoleManager::Get().RegisterThreadPropagation();

	// stop the render thread heartbeat first
	if (GRunRenderingThreadHeartbeat)
	{
		GRunRenderingThreadHeartbeat = false;
		// Wait for the rendering thread heartbeat to return.
		GRenderingThreadHeartbeat->WaitForCompletion();
		GRenderingThreadHeartbeat = NULL;
		delete GRenderingThreadRunnableHeartbeat;
		GRenderingThreadRunnableHeartbeat = NULL;
	}

	if( GIsThreadedRendering )
	{
		// Get the list of objects which need to be cleaned up when the rendering thread is done with them.
		FPendingCleanupObjects* PendingCleanupObjects = GetPendingCleanupObjects();

		// Make sure we're not in the middle of streaming textures.
		(*GFlushStreamingFunc)();

		// Wait for the rendering thread to finish executing all enqueued commands.
		FlushRenderingCommands();

		// The rendering thread may have already been stopped during the call to GFlushStreamingFunc or FlushRenderingCommands.
		if ( GIsThreadedRendering )
		{
			check( GRenderingThread );
			check(!GIsRenderingThreadSuspended);

			// Turn off the threaded rendering flag.
			GIsThreadedRendering = false;

			FGraphEventRef QuitTask = TGraphTask<FReturnGraphTask>::CreateTask(NULL, ENamedThreads::GameThread).ConstructAndDispatchWhenReady(ENamedThreads::RenderThread);

			// Busy wait while Kismet debugging, to avoid opportunistic execution of game thread tasks
			// If the game thread is already executing tasks, then we have no choice but to spin
			if (GIntraFrameDebuggingGameThread || FTaskGraphInterface::Get().IsThreadProcessingTasks(ENamedThreads::GameThread) ) 
			{
				while ((QuitTask.GetReference() != NULL) && QuitTask->IsComplete())
				{
					FPlatformProcess::Sleep(0.0f);
				}
			}
			else
			{
				FTaskGraphInterface::Get().WaitUntilTaskCompletes(QuitTask, ENamedThreads::GameThread_Local);
			}

			// Wait for the rendering thread to return.
			GRenderingThread->WaitForCompletion();

			// Destroy the rendering thread objects.
			delete GRenderingThread;
			GRenderingThread = NULL;
			
			delete GRenderingThreadRunnable;
			GRenderingThreadRunnable = NULL;
		}

		// Delete the pending cleanup objects which were in use by the rendering thread.
		delete PendingCleanupObjects;
	}
}

void CheckRenderingThreadHealth()
{
	if(!GIsRenderingThreadHealthy)
	{
#if WITH_EDITORONLY_DATA
		GErrorHist[0] = 0;
#endif
		GIsCriticalError = false;
		UE_LOG(LogRendererCore, Fatal,TEXT("Rendering thread exception:\r\n%s"),*GRenderingThreadError);
	}
	

	if (IsInGameThread())
	{
		GLog->FlushThreadedLogs();
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		TGuardValue<bool> GuardMainThreadBlockedOnRenderThread(GMainThreadBlockedOnRenderThread,true);
#endif
		SCOPE_CYCLE_COUNTER(STAT_PumpMessages);
		FPlatformMisc::PumpMessages(false);
	}
}

bool IsRenderingThreadHealthy()
{
	return GIsRenderingThreadHealthy;
}


void FRenderCommandFence::BeginFence()
{
	if (!GIsThreadedRendering)
	{
		return;
	}
	else
	{
		if (IsFenceComplete())
		{
			CompletionEvent = TGraphTask<FNullGraphTask>::CreateTask(NULL, ENamedThreads::GameThread).ConstructAndDispatchWhenReady(TEXT("FenceRenderCommand"), ENamedThreads::RenderThread);
		}
		else
		{
			// we already had a fence, so we will chain this one to the old one as a prerequisite
			FGraphEventArray Prerequistes;
			Prerequistes.Add(CompletionEvent);
			CompletionEvent = TGraphTask<FNullGraphTask>::CreateTask(&Prerequistes, ENamedThreads::GameThread).ConstructAndDispatchWhenReady(TEXT("FenceRenderCommand"), ENamedThreads::RenderThread);
		}
	}
}

bool FRenderCommandFence::IsFenceComplete() const
{
	if (!GIsThreadedRendering)
	{
		return true;
	}
	check(IsInGameThread());
	CheckRenderingThreadHealth();
	if (!CompletionEvent.GetReference() || CompletionEvent->IsComplete())
	{
		CompletionEvent = NULL; // this frees the handle for other uses, the NULL state is considered completed
		return true;
	}
	return false;
}

/** How many cycles the gamethread used (excluding idle time). It's set once per frame in FViewport::Draw. */
uint32 GGameThreadTime = 0;
/** How many cycles it took to swap buffers to present the frame. */
uint32 GSwapBufferTime = 0;

static int32 GTimeToBlockOnRenderFence = 1;
static FAutoConsoleVariableRef CVarTimeToBlockOnRenderFence(
	TEXT("g.TimeToBlockOnRenderFence"),
	GTimeToBlockOnRenderFence,
	TEXT("Number of milliseconds the game thread should block when waiting on a render thread fence.")
	);

/**
 * Block the game thread waiting for a task to finish on the rendering thread.
 */
static void GameThreadWaitForTask(const FGraphEventRef& Task, bool bEmptyGameThreadTasks = false)
{
	check(IsInGameThread());
	check(IsValidRef(Task));

	if (!Task->IsComplete())
	{
		SCOPE_CYCLE_COUNTER(STAT_GameIdleTime);
		{
			static int32 NumRecursiveCalls = 0;
			static TArray<FEvent*> EventPool;
			
			// Check for recursion. It's not completely safe but because we pump messages while 
			// blocked it is expected.
			NumRecursiveCalls++;
			if (NumRecursiveCalls > 1)
			{
				UE_LOG(LogRendererCore,Warning,TEXT("FlushRenderingCommands called recursively! %d calls on the stack."), NumRecursiveCalls);
			}
			if (NumRecursiveCalls > 1 || FTaskGraphInterface::Get().IsThreadProcessingTasks(ENamedThreads::GameThread))
			{
				bEmptyGameThreadTasks = false; // we don't do this on recursive calls or if we are at a blueprint breakpoint
			}

			// Grab an event from the pool and fire off a task to trigger it.
			FEvent* Event = NULL;
			if (EventPool.Num() > 0)
			{
				Event = EventPool.Pop();
			}
			else
			{
				Event = FPlatformProcess::CreateSynchEvent();
			}
			FTaskGraphInterface::Get().TriggerEventWhenTaskCompletes(Event, Task, ENamedThreads::GameThread);

			// Check rendering thread health needs to be called from time to
			// time in order to pump messages, otherwise the RHI may block
			// on vsync causing a deadlock. Also we should make sure the
			// rendering thread hasn't crashed :)
			bool bDone;
			uint32 WaitTime = FMath::Clamp<uint32>(GTimeToBlockOnRenderFence, 0, 33);
			do
			{
				CheckRenderingThreadHealth();
				if (bEmptyGameThreadTasks)
				{
					// process gamehtread tasks if there are any
					FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
				}
				bDone = Event->Wait(WaitTime);
			}
			while (!bDone);

			// Return the event to the pool and decrement the recursion counter.
			EventPool.Push(Event);
			NumRecursiveCalls--;
		}
	}
}

/**
 * Waits for pending fence commands to retire.
 */
void FRenderCommandFence::Wait(bool bProcessGameThreadTasks) const
{
	if (!IsFenceComplete())
	{
#if 0
		// on most platforms this is a better solution because it doesn't spin
		// windows needs to pump messages
		if (bProcessGameThreadTasks)
		{
			FTaskGraphInterface::Get().WaitUntilTaskCompletes(CompletionEvent, ENamedThreads::GameThread);
		}
#endif
		GameThreadWaitForTask(CompletionEvent, bProcessGameThreadTasks);
	}
}

/**
 * List of tasks that must be completed before we start a render frame
 * Note, normally, you don't need the render command themselves in this list workers that queue render commands are usually sufficient
 */
static FCompletionList FrameRenderPrerequisites;

/**
 * Adds a task that must be completed either before the next scene draw or a flush rendering commands
 * Note, normally, you don't need the render command themselves in this list workers that queue render commands are usually sufficient
 * @param TaskToAdd, task to add as a pending render thread task
 */
void AddFrameRenderPrerequisite(const FGraphEventRef& TaskToAdd)
{
	FrameRenderPrerequisites.Add(TaskToAdd);
}

/**
 * Gather the frame render prerequisites and make sure all render commands are at least queued
 */
void AdvanceFrameRenderPrerequisite()
{
	checkSlow(IsInGameThread()); 
	FGraphEventRef PendingComplete = FrameRenderPrerequisites.CreatePrerequisiteCompletionHandle();
	if (PendingComplete.GetReference())
	{
		GameThreadWaitForTask(PendingComplete);
	}
}


/**
 * Waits for the rendering thread to finish executing all pending rendering commands.  Should only be used from the game thread.
 */
void FlushRenderingCommands()
{
	AdvanceFrameRenderPrerequisite();

	// Find the objects which may be cleaned up once the rendering thread command queue has been flushed.
	FPendingCleanupObjects* PendingCleanupObjects = GetPendingCleanupObjects();

	// Issue a fence command to the rendering thread and wait for it to complete.
	FRenderCommandFence Fence;
	Fence.BeginFence();
	Fence.Wait();

	// Delete the objects which were enqueued for deferred cleanup before the command queue flush.
	delete PendingCleanupObjects;
}


/** The set of deferred cleanup objects which are pending cleanup. */
static TLockFreePointerList<FDeferredCleanupInterface>	PendingCleanupObjectsList;

FPendingCleanupObjects::FPendingCleanupObjects()
{
	check(IsInGameThread());
	PendingCleanupObjectsList.PopAll(CleanupArray);
}

FPendingCleanupObjects::~FPendingCleanupObjects()
{
	for(int32 ObjectIndex = 0;ObjectIndex < CleanupArray.Num();ObjectIndex++)
	{
		CleanupArray[ObjectIndex]->FinishCleanup();
	}
}

void BeginCleanup(FDeferredCleanupInterface* CleanupObject)
{
	PendingCleanupObjectsList.Push(CleanupObject);
}

FPendingCleanupObjects* GetPendingCleanupObjects()
{
	return new FPendingCleanupObjects;
}
