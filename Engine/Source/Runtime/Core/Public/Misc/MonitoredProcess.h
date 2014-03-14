// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	MonitoredProcess.h: Declares the FMonitoredProcess class
==============================================================================================*/

#pragma once

/**
 * Declares a delegate that is executed when a monitored process completed.
 *
 * The first parameter is the process return code.
 */
DECLARE_DELEGATE_OneParam(FOnMonitoredProcessCompleted, int32)

/**
 * Declares a delegate that is executed when a monitored process produces output.
 *
 * The first parameter is the produced output.
 */
DECLARE_DELEGATE_OneParam(FOnMonitoredProcessOutput, FString)


/**
 * Implements an external process that can be monitored.
 */
class CORE_API FMonitoredProcess
	: public FRunnable
{
public:

	/**
	 * Creates a new monitored process.
	 *
	 * @param InURL - The URL of the executable to launch.
	 * @param InParams - The command line parameters.
	 * @param InHidden - Whether the window of the process should be hidden.
	 */
	FMonitoredProcess( const FString& InURL, const FString& InParams, bool InHidden );

	/**
	 * Destructor.
	 */
	~FMonitoredProcess( );


public:

	/**
	 * Cancels the process.
	 *
	 * @param InKillTree - Whether to kill the entire process tree when canceling this process.
	 */
	void Cancel( bool InKillTree = false )
	{
		Canceling = true;
		KillTree = InKillTree;
	}

	/**
	 * Gets the duration of time that the task has been running.
	 *
	 * @return Time duration.
	 */
	FTimespan GetDuration( ) const;

	/**
	 * Checks whether the process is still running.
	 *
	 * @return true if the process is running, false otherwise.
	 */
	bool IsRunning( ) const
	{
		return (Thread != NULL);
	}

	/**
	 * Launches the process.
	 */
	bool Launch( );


public:

	/**
	 * Returns a delegate that is executed when the process has been canceled.
	 *
	 * @return The delegate.
	 */
	FSimpleDelegate& OnCanceled( )
	{
		return CanceledDelegate;
	}

	/**
	 * Returns a delegate that is executed when a monitored process completed.
	 *
	 * @return The delegate.
	 */
	FOnMonitoredProcessCompleted& OnCompleted( )
	{
		return CompletedDelegate;
	}

	/**
	 * Returns a delegate that is executed when a monitored process produces output.
	 *
	 * @return The delegate.
	 */
	FOnMonitoredProcessOutput& OnOutput( )
	{
		return OutputDelegate;
	}


public:

	// Begin FRunnable interface

	virtual bool Init( ) OVERRIDE
	{
		return true;
	}

	virtual uint32 Run( ) OVERRIDE;

	virtual void Stop( ) OVERRIDE
	{
		Cancel();
	}

	virtual void Exit( ) OVERRIDE { }

	// End FRunnable interface


protected:

	/**
	 * Processes the given output string.
	 *
	 * @param Output - The output string to process.
	 */
	void ProcessOutput( const FString& Output );


private:

	// Whether the process is being canceled.
	bool Canceling;

	// Holds the time at which the process ended.
	FDateTime EndTime;

	// Whether the window of the process should be hidden.
	bool Hidden;

	// Whether to kill the entire process tree when cancelling this process.
	bool KillTree;

	// Holds the command line parameters.
	FString Params;

	// Holds the handle to the process.
	FProcHandle ProcessHandle;

	// Holds the read pipe.
	void* ReadPipe;

	// Holds the time at which the process started.
	FDateTime StartTime;

	// Holds the monitoring thread object.
	FRunnableThread* Thread;

	// Holds the URL of the executable to launch.
	FString URL;

	// Holds the write pipe.
	void* WritePipe;


private:

	// Holds a delegate that is executed when the process has been canceled.
	FSimpleDelegate CanceledDelegate;

	// Holds a delegate that is executed when a monitored process completed.
	FOnMonitoredProcessCompleted CompletedDelegate;

	// Holds a delegate that is executed when a monitored process produces output.
	FOnMonitoredProcessOutput OutputDelegate;
};
