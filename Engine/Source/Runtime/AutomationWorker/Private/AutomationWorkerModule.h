// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AutomationWorkerModule.h: Declares the FAutomationWorkerModule class.
=============================================================================*/

#pragma once


/**
 * Implements the Automation Worker module.
 */
class FAutomationWorkerModule
	: public IAutomationWorkerModule
{
public:

	// Begin IModuleInterface interface

	virtual void StartupModule() OVERRIDE;

	virtual bool SupportsDynamicReloading() OVERRIDE;

	// End IModuleInterface interface

public:

	// Begin IAutomationWorkerModule interface

	virtual void Tick() OVERRIDE;

	virtual void RunTest( const FString& InTestToRun, const int32 InRoleIndex, FStopTestEvent const& InStopTestEvent ) OVERRIDE;

	// End IAutomationWorkerModule interface

protected:

	/**
	 * Execute all latent commands and when complete send results message back to the automation controller
	 */
	bool ExecuteLatentCommands();

	/**
	 * Execute all network commands and when complete send results message back to the automation controller
	 */
	bool ExecuteNetworkCommands();

	/**
	 * Initializes the automation worker.
	 */
	void Initialize();

	/**
	 * Network phase is complete (if there were any network commands).  Send ping back to the controller
	 */
	void ReportNetworkCommandComplete();

	/**
	 * Test is complete. Send results back to controller
	 */
	void ReportTestComplete();

	/** 
	 * Send a list of all the tests supported by the worker
	 *
	 * @param ControllerAddress - The message address of the controller that requested the tests.
	 */
	void SendTests( const FMessageAddress& ControllerAddress );

private:

	// Handles FAutomationWorkerFindWorkers messages.
	void HandleFindWorkersMessage( const FAutomationWorkerFindWorkers& Message, const IMessageContextRef& Context );

	// Handles message endpoint shutdowns.
	void HandleMessageEndpointShutdown();

	// Handles FAutomationWorkerNextNetworkCommandReply messages.
	void HandleNextNetworkCommandReplyMessage( const FAutomationWorkerNextNetworkCommandReply& Message, const IMessageContextRef& Context );

	// Handles FAutomationWorkerPing messages.
	void HandlePingMessage( const FAutomationWorkerPing& Message, const IMessageContextRef& Context );

	// Handles FAutomationWorkerResetTests messages.
	void HandleResetTests( const FAutomationWorkerResetTests& Message, const IMessageContextRef& Context );

	// Handles FAutomationWorkerRequestTests messages.
	void HandleRequestTestsMessage( const FAutomationWorkerRequestTests& Message, const IMessageContextRef& Context );

	// Handles FAutomationWorkerRunTests messages.
	void HandleRunTestsMessage( const FAutomationWorkerRunTests& Message, const IMessageContextRef& Context );

#if WITH_ENGINE
	/** Invoked when we have screen shot to send. */
	void HandleScreenShotCaptured(int32 Width, int32 Height, const TArray<FColor>& Bitmap, const FString& ScreenShotName);
#endif

private:

	// The collection of test data we are to send to a controller
	TArray< FAutomationTestInfo > TestInfo;

private:

	// Holds the messaging endpoint.
	FMessageEndpointPtr MessageEndpoint;

	//ID of the controller sending the test request
	FGuid TestRequesterGUID;

	//identifier for the controller to know if the results should be discarded or not
	uint32 ExecutionCount;

	//execute one of the tests by request of the controller
	FString TestName;

	/** Whether the controller has requested that the network command should execute */
	bool bExecuteNextNetworkCommand;

	/** Whether we are processing sub-commands of a network command */
	bool bExecutingNetworkCommandResults;

	/** Delegate to fire when the test is complete */
	FStopTestEvent StopTestEvent;
};
