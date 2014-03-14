// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AutomationControllerModule.h: Declares the FAutomationControllerModule class.
=============================================================================*/

#pragma once


/**
 * Implements the AutomationController module.
 */
class FAutomationControllerManager
	: public IAutomationControllerManager
{
public:

	// Begin IAutomationController Interface

	virtual void RequestAvailableWorkers( const FGuid& InSessionId ) OVERRIDE;

	virtual void RequestTests() OVERRIDE;

	virtual void RunTests( const bool bIsLocalSession) OVERRIDE;

	virtual void StopTests() OVERRIDE;

	virtual void Init() OVERRIDE;

	virtual void RequestLoadAsset( const FString& InAssetName ) OVERRIDE;

	virtual void Tick() OVERRIDE;

	virtual void SetFilter( TSharedPtr< AutomationFilterCollection > InFilter ) OVERRIDE
	{
		ReportManager.SetFilter( InFilter );
	}

	virtual TArray <TSharedPtr <IAutomationReport> >& GetReports() OVERRIDE
	{
		return ReportManager.GetFilteredReports();
	}

	virtual int32 GetNumDeviceClusters() const OVERRIDE
	{
		return DeviceClusterManager.GetNumClusters();
	}

	virtual int32 GetNumDevicesInCluster(const int32 ClusterIndex) const OVERRIDE
	{
		return DeviceClusterManager.GetNumDevicesInCluster(ClusterIndex);
	}

	virtual FString GetDeviceTypeName(const int32 ClusterIndex) const OVERRIDE
	{
		return DeviceClusterManager.GetClusterDeviceType(ClusterIndex);
	}

	virtual FString GetGameInstanceName(const int32 ClusterIndex, const int32 DeviceIndex) const OVERRIDE
	{
		return DeviceClusterManager.GetClusterDeviceName(ClusterIndex, DeviceIndex);
	}

	virtual void SetVisibleTestsEnabled(const bool bEnabled) OVERRIDE
	{
		ReportManager.SetVisibleTestsEnabled (bEnabled);
	}

	virtual int32 GetEnabledTestsNum() const OVERRIDE
	{
		return ReportManager.GetEnabledTestsNum();
	}

	virtual EAutomationControllerModuleState::Type GetTestState( ) const OVERRIDE
	{
		return AutomationTestState;
	}

	virtual void SetDeveloperDirectoryIncluded(const bool bInDeveloperDirectoryIncluded) OVERRIDE
	{
		bDeveloperDirectoryIncluded = bInDeveloperDirectoryIncluded;
	}

	virtual bool IsDeveloperDirectoryIncluded(void) const OVERRIDE
	{
		return bDeveloperDirectoryIncluded;
	}

	virtual void SetVisualCommandletFilter(const bool bInVisualCommandletFilterOn) OVERRIDE
	{
		bVisualCommandletFilterOn = bInVisualCommandletFilterOn;
	}

	virtual bool IsVisualCommandletFilterOn(void) const OVERRIDE
	{
		return bVisualCommandletFilterOn;
	}

	virtual const bool CheckTestResultsAvailable() const OVERRIDE
	{
		return 	bTestResultsAvailable;
	}

	virtual const bool ReportsHaveErrors() const OVERRIDE
	{
		return bHasErrors;
	}

	virtual const bool ReportsHaveWarnings() const OVERRIDE
	{
		return bHasWarning;
	}

	virtual const bool ReportsHaveLogs() const OVERRIDE
	{
		return bHasLogs;
	}

	virtual void ClearAutomationReports() OVERRIDE
	{
		ReportManager.Empty();
	}

	virtual const bool ExportReport(uint32 FileExportTypeMask) OVERRIDE;

	virtual bool IsTestRunnable( IAutomationReportPtr InReport ) const OVERRIDE;

	virtual void RemoveCallbacks() OVERRIDE;

	virtual void Shutdown() OVERRIDE;

	virtual void Startup() OVERRIDE;

	virtual FOnAutomationControllerManagerShutdown& OnShutdown( ) OVERRIDE
	{
		return ShutdownDelegate;
	}

	virtual FOnAutomationControllerManagerTestsAvailable& OnTestsAvailable( ) OVERRIDE
	{
		return TestsAvailableDelegate;
	}

	virtual FOnAutomationControllerTestsRefreshed& OnTestsRefreshed( ) OVERRIDE
	{
		return TestsRefreshedDelegate;
	}

	// End IAutomationController Interface


protected:

	/**
	 * Adds a ping result from a running test.
	 *
	 * @param ResponderAddress - The address of the message endpoint that responded to a ping.
	 */
	void AddPingResult( const FMessageAddress& ResponderAddress );

	/**
	 * Checks the child result.
	 *
	 * @param InReport - The child result to check.
	 */
	void CheckChildResult( TSharedPtr< IAutomationReport > InReport );

	/**
	 * Execute the next task thats available
	 *
	 * @param ClusterIndex			The Cluster index of the device type we intend to use
	 * @param bAllTestsCompleted	Whether all tests have been completed
	 */
	void ExecuteNextTask( int32 ClusterIndex, OUT bool& bAllTestsCompleted );

	/**
	 * Distributes any tests that are pending and deal with tests finishing.
	 */
	void ProcessAvailableTasks();

	/**
	 * Processes the results after tests are complete.
	 */
	void ProcessResults();

	/**
	 * Removes the test info.
	 *
	 * @param TestToRemove - The test to remove.
	 */
	void RemoveTestRunning( const FMessageAddress& TestToRemove );

	/**
	 * Resets the data holders which have been used to generate the tests available from a worker.
	 */
	void ResetIntermediateTestData()
	{
		TestInfo.Empty();
	}

	/**
	 * Changes the controller state.
	 */
	void SetControllerStatus( EAutomationControllerModuleState::Type AutomationTestState );

	/**
	 * stores the tests that are valid for a particular device classification.
	 */
	void SetTestNames(const FMessageAddress& AutomationWorkerAddress);

	/**
	 * Updates the tests to ensure they are all still running.
	 */
	void UpdateTests();


private:

	// Handles FAutomationWorkerFindWorkersResponse messages.
	void HandleFindWorkersResponseMessage( const FAutomationWorkerFindWorkersResponse& Message, const IMessageContextRef& Context );

	// Handles FAutomationWorkerPong messages.
	void HandlePongMessage( const FAutomationWorkerPong& Message, const IMessageContextRef& Context );

	// Handles FAutomationWorkerScreenImage messages..
	void HandleReceivedScreenShot( const FAutomationWorkerScreenImage& Message, const IMessageContextRef& Context );

	// Handles FAutomationWorkerRequestNextNetworkCommand messages.
	void HandleRequestNextNetworkCommandMessage( const FAutomationWorkerRequestNextNetworkCommand& Message, const IMessageContextRef& Context );

	// Handles FAutomationWorkerRequestTestsReply messages.
	void HandleRequestTestsReplyMessage( const FAutomationWorkerRequestTestsReply& Message, const IMessageContextRef& Context );

	// Handles FAutomationWorkerRunTestsReply messages.
	void HandleRunTestsReplyMessage( const FAutomationWorkerRunTestsReply& Message, const IMessageContextRef& Context );

	// Handles FAutomationWorkerWorkerOffline messages.
	void HandleWorkerOfflineMessage( const FAutomationWorkerWorkerOffline& Message, const IMessageContextRef& Context );


private:

	/** Session this controller is currently communicating with */
	FGuid ActiveSessionId;

	/** The automation test state */
	EAutomationControllerModuleState::Type AutomationTestState;

	/** Whether to include developer content in the automation tests */
	bool bDeveloperDirectoryIncluded;

	/** Some tests have errors */
	bool bHasErrors;

	/** Some tests have warnings */
	bool bHasWarning;

	/** Some tests have logs */
	bool bHasLogs;

	/** Is this a local session */
	bool bIsLocalSession;

	/** Are tests results available */
	bool bTestResultsAvailable;

	/** Whether to use visual commandlet **/
	bool bVisualCommandletFilterOn;

	/** Timer to keep track of the last time tests were updated */
	double CheckTestTimer;

	/** Whether tick is still executing tests for different clusters */
	uint32 ClusterDistributionMask;

	/** Available worker GUIDs */
	FAutomationDeviceClusterManager DeviceClusterManager;

	/** The iteration number of executing the tests.  Ensures restarting the tests won't allow stale results to try and commit */
	uint32 ExecutionCount;

	/** Last time the update tests function was ticked */
	double LastTimeUpdateTicked;

	/** Holds the messaging endpoint. */
	FMessageEndpointPtr MessageEndpoint;

	/** Counter for number of workers we have received responses from for Refreshing the Test List */
	uint32 RefreshTestResponses;

	/**available stats/status for all tests*/
	FAutomationReportManager ReportManager;

	// The number we are expecting to receive from a worker
	int32 NumOfTestsToReceive;

	// The collection of test data we are to send to a controller
	TArray< FAutomationTestInfo > TestInfo;

	/** A data holder to keep track of how long tests have been running */
	struct FTestRunningInfo
	{
		FTestRunningInfo( FMessageAddress InMessageAddress ):
			OwnerMessageAddress( InMessageAddress),
			LastPingTime( 0.f )
		{
		}
		/** The test runners message address */
		FMessageAddress OwnerMessageAddress;
		/** The time since we had a ping from the instance*/
		float LastPingTime;
	};

	/** A array of running tests */
	TArray< FTestRunningInfo > TestRunningArray;


private:

	// Holds a delegate that is invoked when the controller shuts down.
	FOnAutomationControllerManagerShutdown ShutdownDelegate;

	// Holds a delegate that is invoked when the controller has tests available.
	FOnAutomationControllerManagerTestsAvailable TestsAvailableDelegate;

	// Holds a delegate that is invoked when the controller's tests are being refreshed.
	FOnAutomationControllerTestsRefreshed TestsRefreshedDelegate;
};
