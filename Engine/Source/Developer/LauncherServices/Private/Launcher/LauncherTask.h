// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LauncherTask.h: Declares the FLauncherTask class.
=============================================================================*/

#pragma once


/**
 * Abstract base class for launcher tasks.
 */
class FLauncherTask
	: public FRunnable
	, public ILauncherTask
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InName - The name of the task.
	 * @param InDependency - An optional task that this task depends on.
	 */
	FLauncherTask( const FString& InName, const FString& InDesc, void* InReadPipe, void* InWritePipe)
		: Name(InName)
		, Desc(InDesc)
		, ReadPipe(InReadPipe)
		, WritePipe(InWritePipe)
		, Status(ELauncherTaskStatus::Pending)
	{ }


public:

	/**
	 * Adds a task that will execute after this task completed.
	 *
	 * Continuations must be added before this task starts.
	 *
	 * @param Task - The task to add.
	 */
	void AddContinuation( const TSharedPtr<FLauncherTask>& Task )
	{
		if (Status == ELauncherTaskStatus::Pending)
		{
			Continuations.AddUnique(Task);
		}	
	}

	/**
	 * Executes the tasks.
	 *
	 * @param ChainState - State data of the task chain.
	 */
	void Execute( const FLauncherTaskChainState& ChainState )
	{
		check(Status == ELauncherTaskStatus::Pending);

		LocalChainState = ChainState;

		Thread = FRunnableThread::Create(this, TEXT("FLauncherTask"), false, false, 0, TPri_Normal);

		if (Thread == NULL)
		{

		}
	}

	/**
	 * Gets the list of tasks to be executed after this task.
	 *
	 * @return Continuation task list.
	 */
	virtual const TArray<TSharedPtr<FLauncherTask> >& GetContinuations( ) const
	{
		return Continuations;
	}

	/**
	 * Checks whether the task chain has finished execution.
	 *
	 * A task chain is finished when this task and all its continuations are finished.
	 *
	 * @return true if the chain is finished, false otherwise.
	 */
	bool IsChainFinished( ) const
	{
		if (IsFinished())
		{
			for (int32 ContinuationIndex = 0; ContinuationIndex < Continuations.Num(); ++ContinuationIndex)
			{
				if (!Continuations[ContinuationIndex]->IsChainFinished())
				{
					return false;
				}
			}

			return true;
		}

		return false;
	}

	bool Succeeded() const
	{
		if (IsChainFinished())
		{
			for (int32 ContinuationIndex = 0; ContinuationIndex < Continuations.Num(); ++ContinuationIndex)
			{
				if (!Continuations[ContinuationIndex]->Succeeded())
				{
					return false;
				}
			}
		}

		return Status == ELauncherTaskStatus::Completed;
	}

public:

	// Begin FRunnable interface

	virtual bool Init( ) OVERRIDE
	{
		return true;
	}

	virtual uint32 Run( ) OVERRIDE
	{
		Status = ELauncherTaskStatus::Busy;

		StartTime = FDateTime::UtcNow();

		FPlatformProcess::Sleep(0.5f);

		TaskStarted.Broadcast(Name);
		if (PerformTask(LocalChainState))
		{
			Status = ELauncherTaskStatus::Completed;

			TaskCompleted.Broadcast(Name);
			ExecuteContinuations();
		}
		else
		{
			if (Status == ELauncherTaskStatus::Canceling)
			{
				Status = ELauncherTaskStatus::Canceled;
			}
			else
			{
				Status = ELauncherTaskStatus::Failed;
			}

			TaskCompleted.Broadcast(Name);

			CancelContinuations();
		}

		EndTime = FDateTime::UtcNow();
		
		return 0;
	}

	virtual void Stop( ) OVERRIDE
	{
		Cancel();
	}

	virtual void Exit( ) OVERRIDE { }

	// End FRunnable interface


public:

	// Begin ILauncherTask interface

	virtual void Cancel( ) OVERRIDE
	{
		if (Status == ELauncherTaskStatus::Busy)
		{
			Status = ELauncherTaskStatus::Canceling;
		}
		else if (Status == ELauncherTaskStatus::Pending)
		{
			Status = ELauncherTaskStatus::Canceled;
		}

		CancelContinuations();
	}

	virtual FTimespan GetDuration( ) const OVERRIDE
	{
		if (Status == ELauncherTaskStatus::Pending)
		{
			return FTimespan::Zero();
		}

		if (Status == ELauncherTaskStatus::Busy)
		{
			return (FDateTime::UtcNow() - StartTime);
		}

		return (EndTime - StartTime);
	}

	virtual const FString& GetName( ) const OVERRIDE
	{
		return Name;
	}

	virtual const FString& GetDesc( ) const OVERRIDE
	{
		return Desc;
	}

	virtual ELauncherTaskStatus::Type GetStatus( ) const OVERRIDE
	{
		return Status;
	}

	virtual bool IsFinished( ) const OVERRIDE
	{
		return ((Status == ELauncherTaskStatus::Canceled) ||
				(Status == ELauncherTaskStatus::Completed) ||
				(Status == ELauncherTaskStatus::Failed));
	}

	virtual FOnTaskStartedDelegate& OnStarted() OVERRIDE
	{
		return TaskStarted;
	}

	virtual FOnTaskCompletedDelegate& OnCompleted() OVERRIDE
	{
		return TaskCompleted;
	}

	// End ILauncherTask interface


protected:

	/**
	 * Performs the actual task.
	 *
	 * @param ChainState - Holds the state of the task chain.
	 *
	 * @return true if the task completed successfully, false otherwise.
	 */
	virtual bool PerformTask( FLauncherTaskChainState& ChainState ) = 0;


protected:

	/**
	 * Cancels all continuation tasks.
	 */
	void CancelContinuations( ) const
	{
		for (int32 ContinuationIndex = 0; ContinuationIndex < Continuations.Num(); ++ContinuationIndex)
		{
			Continuations[ContinuationIndex]->Cancel();
		}
	}

	/**
	 * Executes all continuation tasks.
	 */
	void ExecuteContinuations( ) const
	{
		for (int32 ContinuationIndex = 0; ContinuationIndex < Continuations.Num(); ++ContinuationIndex)
		{
			Continuations[ContinuationIndex]->Execute(LocalChainState);
		}
	}


private:

	// Holds the tasks that execute after this task completed.
	TArray<TSharedPtr<FLauncherTask> > Continuations;

	// Holds the time at which the task ended.
	FDateTime EndTime;

	// Holds the local state of this task chain.
	FLauncherTaskChainState LocalChainState;

	// Holds the task's name.
	FString Name;

	// Holds the task's description
	FString Desc;

	// Holds the time at which the task started.
	FDateTime StartTime;

	// Holds the status of this task
	ELauncherTaskStatus::Type Status;

	// Holds the thread that's running this task.
	FRunnableThread* Thread;

	// message delegates
	FOnStageStartedDelegate TaskStarted;
	FOnStageCompletedDelegate TaskCompleted;

protected:

	// read and write pipe
	void* ReadPipe;
	void* WritePipe;
};
