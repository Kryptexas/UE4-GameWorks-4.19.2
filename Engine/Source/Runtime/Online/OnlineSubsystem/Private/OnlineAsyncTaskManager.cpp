// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemPrivatePCH.h"
#include "OnlineAsyncTaskManager.h"


uint32 FOnlineAsyncItem::OnlineThreadId = 0;
uint32 FOnlineAsyncItem::GameThreadId = 0;

/** Set to zero so that Run() can prevent a second invocation */
int32 FOnlineAsyncTaskManager::InvocationCount = 0;

/** The default value for the polling interval when not set by config */
#define POLLING_INTERVAL_MS 50

FOnlineAsyncTaskManager::FOnlineAsyncTaskManager() :
	WorkEvent(NULL),
	PollingInterval(POLLING_INTERVAL_MS),
	bRequestingExit(0)
{
}

/**
 * Init the online async task manager. Creates the event used to wake up the
 * thread when there is work to be done
 *
 * @return True if initialization was successful, false otherwise
 */
bool FOnlineAsyncTaskManager::Init(void)
{
	WorkEvent = FPlatformProcess::CreateSynchEvent();
	int32 PollingConfig = POLLING_INTERVAL_MS;
	// Read the polling interval to use from the INI file
	if (GConfig->GetInt(TEXT("OnlineSubsystem"),TEXT("PollingIntervalInMs"),PollingConfig,GEngineIni))
	{
		PollingInterval = (uint32)PollingConfig;
	}

	FPlatformAtomics::InterlockedExchange((volatile int32*)&FOnlineAsyncItem::GameThreadId, GGameThreadId);
	return WorkEvent != NULL;
}

/**
 * This is where all per object thread work is done. This is only called
 * if the initialization was successful.
 *
 * @return The exit code of the runnable object
 */
uint32 FOnlineAsyncTaskManager::Run(void)
{
	check(InvocationCount == 0);
	InvocationCount++;
	// This should not be set yet
	check(FOnlineAsyncItem::OnlineThreadId == 0);
	FPlatformAtomics::InterlockedExchange((volatile int32*)&FOnlineAsyncItem::OnlineThreadId, FPlatformTLS::GetCurrentThreadId());

	do 
	{
		int32 InitialQueueSize = 0;
		int32 CurrentQueueSize = 0;
		FOnlineAsyncTask* Task = NULL;

		// Wait for a trigger event to start work
		WorkEvent->Wait(PollingInterval);
		if (!bRequestingExit)
		{
			SCOPE_CYCLE_COUNTER(STAT_Online_Async);
			{
				FScopeLock LockInQueue(&InQueueLock);
				InitialQueueSize = InQueue.Num();
				SET_DWORD_STAT(STAT_Online_AsyncTasks, InitialQueueSize);
			}
			
			double TimeElapsed = 0;

			// Chance for services to do work
			OnlineTick();

			TArray<FOnlineAsyncTask*> CopyParallelTasks;

			// Grab a copy of the parallel list
			{
				FScopeLock LockParallelTasks(&ParallelTasksLock);

				CopyParallelTasks = ParallelTasks;
			}

			// Tick all the parallel tasks
			for ( auto it = CopyParallelTasks.CreateIterator(); it; ++it )
			{
				Task = *it;

				Task->Tick();

				if (Task->IsDone())
				{
					UE_LOG(LogOnline, Log, TEXT("Async task '%s' completed in %f seconds with %d (Parallel)"),
						*Task->ToString(),
						Task->GetElapsedTime(),
						Task->WasSuccessful());

					// Task is done, remove from the incoming queue and add to the outgoing queue
					RemoveFromParallelTasks( Task );

					AddToOutQueue( Task );
				}
			}

			// Now process the serial "In" queue
			do
			{	
				Task = NULL;

				// Grab the current task from the queue
				{
					FScopeLock LockInQueue(&InQueueLock);
					CurrentQueueSize = InQueue.Num();
					if (CurrentQueueSize > 0)
					{
						Task = InQueue[0];
					}
				}

				if (Task)
				{
					Task->Tick();
	
					if (Task->IsDone())
					{
						UE_LOG(LogOnline, Log, TEXT("Async task '%s' completed in %f seconds with %d"),
							*Task->ToString(),
							Task->GetElapsedTime(),
							Task->WasSuccessful());

						// Task is done, remove from the incoming queue and add to the outgoing queue
						PopFromInQueue();
						AddToOutQueue(Task);
					}
					else
					{
						Task = NULL;
					}
				}
			}
			while (Task != NULL);
		}
	} 
	while (!bRequestingExit);

	return 0;
}

/**
* This is called if a thread is requested to terminate early
*/
void FOnlineAsyncTaskManager::Stop(void)
{
	FPlatformAtomics::InterlockedExchange(&bRequestingExit, 1);
	WorkEvent->Trigger();
}

/**
* Called in the context of the aggregating thread to perform any cleanup.
*/
void FOnlineAsyncTaskManager::Exit(void)
{
	FOnlineAsyncItem::GameThreadId = 0;
	FOnlineAsyncItem::OnlineThreadId = 0;
	InvocationCount = 0;
}

/**
* Add online async tasks that need processing onto the incoming queue
* @param NewTask - some request of the online services
*/
void FOnlineAsyncTaskManager::AddToInQueue(FOnlineAsyncTask* NewTask)
{
	// assert if not game thread	
	check(FPlatformTLS::GetCurrentThreadId() == FOnlineAsyncItem::GameThreadId);
	FScopeLock Lock(&InQueueLock);
	InQueue.Add(NewTask);
	WorkEvent->Trigger();
}

/**
*	Remove the current async task from the queue
*/
void FOnlineAsyncTaskManager::PopFromInQueue()
{
	// assert if not game thread
	check(FPlatformTLS::GetCurrentThreadId() == FOnlineAsyncItem::OnlineThreadId);

	FScopeLock Lock(&InQueueLock);
	InQueue.RemoveAt(0);
}

/**
* Add completed online async tasks that need processing onto the queue
* @param CompletedItem - some finished request of the online services
*/
void FOnlineAsyncTaskManager::AddToOutQueue(FOnlineAsyncItem* CompletedItem)
{
	FScopeLock Lock(&OutQueueLock);
	OutQueue.Add(CompletedItem);
}

void FOnlineAsyncTaskManager::AddToParallelTasks(FOnlineAsyncTask* NewTask)
{
	FScopeLock LockParallelTasks(&ParallelTasksLock);

	ParallelTasks.Add( NewTask );
}

void FOnlineAsyncTaskManager::RemoveFromParallelTasks(FOnlineAsyncTask* OldTask)
{
	FScopeLock LockParallelTasks(&ParallelTasksLock);

	ParallelTasks.Remove( OldTask );
}

/**
*	** CALL ONLY FROM GAME THREAD **
* Give the completed async tasks a chance to marshal their data back onto the game thread
* Calling delegates where appropriate
*/
void FOnlineAsyncTaskManager::GameTick()
{
	// assert if not game thread
	check(FPlatformTLS::GetCurrentThreadId() == FOnlineAsyncItem::GameThreadId);

	FOnlineAsyncItem* Item = NULL;
	int32 CurrentQueueSize = 0;

	do 
	{
		Item = NULL;
		// Grab a completed task from the queue
		{
			FScopeLock LockOutQueue(&OutQueueLock);
			CurrentQueueSize = OutQueue.Num();
			if (CurrentQueueSize > 0)
			{
				Item = OutQueue[0];
				OutQueue.RemoveAt(0);
			}
		}

		if (Item)
		{
			// Finish work and trigger delegates
			Item->Finalize();
			Item->TriggerDelegates();
			delete Item;
		}
	}
	while (Item != NULL);
}


// Non blocking single threaded tick for Online Async Tasks. 
void FOnlineAsyncTaskManager::Tick()
{

		// parallel Q.
		FOnlineAsyncTask* Task = NULL;

		// Tick Online services ( possibly callbacks ). 
		OnlineTick();

		// Tick all the parallel tasks - Tick unrelated tasks together. 

		// Create a copy of existing tasks. 
		TArray<FOnlineAsyncTask*> CopyParallelTasks	= ParallelTasks;

		// Iterate. 
		for ( auto it = CopyParallelTasks.CreateIterator(); it; ++it )
		{
			Task = *it;
			Task->Tick();

			if (Task->IsDone())
			{
				UE_LOG(LogOnline, Log, TEXT("Async parallel Task '%s' completed in %f seconds with %d (Parallel)"),
					*Task->ToString(),
					Task->GetElapsedTime(),
					Task->WasSuccessful());

				// Task is done, fixup the original parallel task queue. 
				RemoveFromParallelTasks( Task );
				AddToOutQueue( Task );
			}
		}

       // Serial Q.
	   if ( InQueue.Num() == 0 )
			return; 

		// Pick up the first element in the queue ( "current task" ). 
		Task = InQueue[0]; 

        // Needed ?
		if ( Task == NULL )
			return; 

		Task->Tick();
	
		if (Task->IsDone())
		{
			UE_LOG(LogOnline, Log, TEXT("Async serial task '%s' completed in %f seconds with %d"),
				*Task->ToString(),
				Task->GetElapsedTime(),
				Task->WasSuccessful());

			// Task is done, remove from the incoming queue and add to the outgoing queue
			PopFromInQueue();
			AddToOutQueue(Task);
		}
} 
