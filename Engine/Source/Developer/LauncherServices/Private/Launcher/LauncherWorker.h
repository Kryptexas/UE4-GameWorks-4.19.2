// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LauncherWorker.h: Declares the FLauncherWorker class.
=============================================================================*/

#pragma once

/**
 * Implements the launcher's worker thread.
 */
class FLauncherWorker
	: public FRunnable
	, public ILauncherWorker
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InProfile - The profile to process.
	 * @param InDeviceProxyManager - The target device proxy manager to use.
	 * @param LaunchType - The launch type, i.e. UE4 or Rocket.
	 */
	FLauncherWorker( const ITargetDeviceProxyManagerRef& InDeviceProxyManager, const ILauncherProfileRef& InProfile );

	~FLauncherWorker()
	{
		TaskChain.Reset();
	}

public:

	virtual bool Init( ) OVERRIDE;

	virtual uint32 Run( ) OVERRIDE;

	virtual void Stop( ) OVERRIDE;

	virtual void Exit( ) OVERRIDE { }


public:

	virtual void Cancel( ) OVERRIDE;

	virtual ELauncherWorkerStatus::Type GetStatus( ) const  OVERRIDE
	{
		return Status;
	}

	virtual int32 GetTasks( TArray<ILauncherTaskPtr>& OutTasks ) const OVERRIDE;

	virtual FOutputMessageReceivedDelegate& OnOutputReceived()
	{
		return OutputMessageReceived;
	}

	virtual FOnStageStartedDelegate& OnStageStarted()
	{
		return StageStarted;
	}

	virtual FOnStageCompletedDelegate& OnStageCompleted()
	{
		return StageCompleted;
	}

	virtual FOnLaunchCompletedDelegate& OnCompleted()
	{
		return LaunchCompleted;
	}

	virtual FOnLaunchCanceledDelegate& OnCanceled()
	{
		return LaunchCanceled;
	}

protected:

	/**
	 * Creates the tasks for the specified profile.
	 *
	 * @param InProfile - The profile to create tasks for.
	 */
	void CreateAndExecuteTasks( const ILauncherProfileRef& InProfile );

	void OnTaskStarted(const FString& TaskName);
	void OnTaskCompleted(const FString& TaskName);

private:

	// Holds a critical section to lock access to selected properties.
	FCriticalSection CriticalSection;

	// Holds a pointer  to the device proxy manager.
	ITargetDeviceProxyManagerPtr DeviceProxyManager;

	// Holds a pointer to the launcher profile.
	ILauncherProfilePtr Profile;

	// Holds the worker's current status.
	ELauncherWorkerStatus::Type Status;

	// Holds the first task in the task chain.
	TSharedPtr<FLauncherTask> TaskChain;

	// holds the read and write pipes
	void* ReadPipe;
	void* WritePipe;

	// message delegate
	FOutputMessageReceivedDelegate OutputMessageReceived;
	FOnStageStartedDelegate StageStarted;
	FOnStageCompletedDelegate StageCompleted;
	FOnLaunchCompletedDelegate LaunchCompleted;
	FOnLaunchCanceledDelegate LaunchCanceled;
};
