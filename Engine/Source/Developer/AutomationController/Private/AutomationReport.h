// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AutomationTestResults.h: Declares the AutomationTestResults interface.
=============================================================================*/

#pragma once


/**
 * Interface for automation test results modules.
 */
class FAutomationReport : public IAutomationReport
{
public:

	FAutomationReport(FAutomationTestInfo& TestInfo, bool bIsParent = false);

	// Begin IAutomationReport Interface

	/** Remove all child tests */
	virtual void Empty() override;

	/** Get the asset associated with tis report */
	virtual FString GetAssetName() const override;

	/**
	 * Returns the complete command line for an automation test including any relevant parameters
	 */
	virtual FString GetCommand() const override;

	/**
	 * Returns the name of this level in the test hierarchy for the purposes of grouping.
	 * (Editor.Maps.LoadAll.parameters) would have 3 internal tree nodes and a variable number of leaf nodes depending on how many maps existed
	 * @param bAddDecoration - True is the name should have the number of child tests appended
	 * @return the name of this level in the hierarchy for use in UI
	 */
	virtual const FString& GetDisplayName() const override;

	/**
	 * Returns the name of this level in the test hierarchy for the purposes of UI.
	 * (Editor.Maps.LoadAll.parameters) would have 3 internal tree nodes and a variable number of leaf nodes depending on how many maps existed
	 * @return the name of this level in the hierarchy for use in UI
	 */
	virtual FString GetDisplayNameWithDecoration() const override;

	/** Recursively gets the number of child nodes */
	virtual int32 GetTotalNumChildren() const override;
	/** Recursively gets the number of enabled tests */
	virtual int32 GetEnabledTestsNum() const override;
	/** Recursively gets the names of enabled tests */
	virtual void GetEnabledTestNames(TArray<FString>& OutEnabledTestNames, FString CurrentPath) const override;
	/** Recursively sets the enabled flag based on if the current test exists in the enabled tests array */
	virtual void SetEnabledTests(const TArray<FString>& EnabledTests, FString CurrentPath) override;

	/** Return if this test should be executed */
	virtual bool IsEnabled() const override;
	/** Sets whether this test should be executed or not */
	virtual void SetEnabled(bool bShouldBeEnabled) override;
	
	/** Sets whether this test is supported on a particular platform */
	virtual void SetSupport(const int32 ClusterIndex) override;
	/** Returns if a particular platform is supported */
	virtual bool IsSupported(const int32 ClusterIndex) const override;

	/** Sets the test type for parent reports */
	virtual void SetTestType( const uint8 TestType ) override;
	/** Returns the test type */
	virtual uint8 GetTestType( ) const override;

	/** Returns true if this is a parent test report */
	virtual const bool IsParent( ) override;
	/** Returns true if this is a smoke test report */
	virtual const bool IsSmokeTest( ) override;
	/**
	 * Filters the visible tests based on search text, execution status, regression test, etc
	 * @param InFilterDesc - The description of how to decide if this report should be visible
	 * @param ParentPassedFilter - If the parent passed the filter
	 * @return whether this report or any of its children passed the filter
	 */
	virtual bool SetFilter( TSharedPtr< AutomationFilterCollection > InFilter, const bool ParentPassedFilter = false ) override;

	/** Returns the array of child reports that should be visible to the UI based on filtering */
	virtual TArray<TSharedPtr<IAutomationReport> >& GetFilteredChildren() override;

	/** Returns the array of child reports */
	virtual TArray<TSharedPtr<IAutomationReport> >& GetChildReports() override;

	/** Updates the report when the number of clusters changes */
	virtual void ClustersUpdated(const int32 NumClusters) override;

	/** 
	 * Recursively resets the report to "needs to be run", clears cached warnings and errors
	 * @param NumTestPasses - The number of test passes so we know how many results to create
	 */
	virtual void ResetForExecution(const int32 NumTestPasses) override;

	/**
	 * Sets the results of the test for use by the UI
	 * @param ClusterIndex - Index of the platform reporting the results of this test.  See AutomationDeviceClusterManager
	 * @param PassIndex - Which test pass these results are for
	 * @param InResults - The new set of results
	 */
	virtual void SetResults( const int32 ClusterIndex, const int32 PassIndex, const FAutomationTestResults& InResults ) override;

	/**
	 * Returns completion statistics for this branch of the testing hierarchy
	 * @param ClusterIndex - Index of the platform reporting the results of this test.  See AutomationDeviceClusterManager
	 * @param PassIndex - Which test pass to get the status of
	 * @param OutCompletionState - Collection structure for execution statistics
	 */
	virtual void GetCompletionStatus(const int32 ClusterIndex, const int32 PassIndex, FAutomationCompleteState& OutCompletionState) override;

	/**
	 * Returns the state of the test (not run, in process, success, failure)
	 * @param ClusterIndex - Index of the platform reporting the results of this test.  See AutomationDeviceClusterManager
	 * @param PassIndex - Which test pass to get the state of
	 * @return the current state of the test
	 */
	virtual EAutomationState::Type GetState(const int32 ClusterIndex, const int32 PassIndex) const override;

	/**
	 * Gets a copy of errors and warnings that were found
	 *
	 * @param ClusterIndex - Index of the platform we are requesting test results for.
	 * @param PassIndex - Index of the test pass to get the results for.
	 *
	 * @return The collection of results for the given cluster index
	 */
	virtual const FAutomationTestResults& GetResults( const int32 ClusterIndex, const int32 PassIndex ) override;

	/**
	 * Gets the number of available test results for a given cluster
	 * @param ClusterIndex - Index of the platform .
	 * @return The number of results available for this cluster
	 */
	virtual const int32 GetNumResults( const int32 ClusterIndex ) override;

	/**
	 * Finds the current pass by looking at the current state
	 * @param ClusterIndex - Index of the platform.
	 * @return The current pass index
	 */
	virtual const int32 GetCurrentPassIndex( const int32 ClusterIndex ) override;

	/**
	 * Gets the name of the instance that ran the test
	 * @param ClusterIndex - Index of the platform reporting the results of this test.  See AutomationDeviceClusterManager
	 * @return the name of the device
	 */
	virtual FString GetGameInstanceName( const int32 ClusterIndex ) override;

	/**
	 * Add a child test to the hierarchy, creating internal tree nodes as needed
	 * If NewTestName is Editor.Maps.Loadall.TestName, this will create nodes for Editor, Maps, Loadall, and then a leaf node for the testname with the associated commandline
	 * @param TestInfo - Structure containing all the test info
	 * @param ClusterIndex - Index of the platform reporting the results of this test.  See AutomationDeviceClusterManager
	 * @param NumPasses - The number of passes we are going to perform.  Used to make sure we have enough results.
	 * @return - the automation report
	 */
	virtual TSharedPtr<IAutomationReport> EnsureReportExists(FAutomationTestInfo& TestInfo, const int32 ClusterIndex, const int32 NumPasses) override;

	/**
	 * Returns the next test in the hierarchy to run
	 * @param bOutAllTestsComplete - Whether or not all enabled tests have completed execution for this platform (cluster index)
	 * @param ClusterIndex - Index of the platform reporting the results of this test.  See AutomationDeviceClusterManager
	 * @param PassIndex - Which test pass we are currently on
	 * @param NumDevicesInCluster - The number of devices which are in this cluster
	 *
	 * @return The next report
	 */
	virtual TSharedPtr<IAutomationReport> GetNextReportToExecute(bool& bOutAllTestsComplete, const int32 ClusterIndex, const int32 PassIndex, const int32 NumDevicesInCluster) override;

	/** 
	 * Have any results got errors
	 * @return true if they have errors
	 */
	virtual const bool HasErrors() override;

	/** 
	 * Have any results got warnings
	 * @return true if they have warnings
	 */
	virtual const bool HasWarnings() override;

	/**
	 * Gets the min and max time this test took to execute
	 * @param OutMinTime - Minimum execution time for all device types
	 * @param OutMaxTime - Maximum execution time for all device types
	 * @return whether any test has completed successfully
	 */
	virtual const bool GetDurationRange(float& OutMinTime, float& OutMaxTime) override;

	/**
	 * Get the number of devices which have been given this test to run
	 *
	 * @return - The number of devices who have been given this test to run
	 */
	virtual const int32 GetNumDevicesRunningTest() const override;

	/**
	 * Get the number of participants this test requires.
	 *
	 * @Return - The number of devices needed for this test to execute
	 */
	virtual const int32 GetNumParticipantsRequired() const override;
	
	/**
	 * Set the number of participants this test requires if less than what is already set.
	 *
	 * @param NewCount - The number of further devices 
	 */
	virtual void SetNumParticipantsRequired( const int32 NewCount ) override;

	/**
	 * Increment the number of network responses
	 * @return - whether we have received responses from all participants
	 */
	virtual bool IncrementNetworkCommandResponses() override;

	/**
	 * Resets the number of network responses back to zero
	 */
	virtual void ResetNetworkCommandResponses() override;

	/**
	 * Should we expand this node in the UI - A child has passed the filter
	 * @return true if we should expand the node
	 */
	virtual const bool ExpandInUI() const override;

	/** Stop the test which is creating this report */
	virtual void StopRunningTest() override;

	/**
	* Notification on whether we should, or should not, track this reports history
	*/
	virtual void TrackHistory(const bool bShouldTrack, const int32 NumReportsToTrack) override;

	/**
	* Get the history items of this particular test
	*
	* @return A reference to the items of this tests previous runs.
	*/
	virtual const TArray<TSharedPtr<FAutomationHistoryItem>>& GetHistory() const override;

	// End IAutomationReport Interface
	
private:

	/** 
	 * Export the current report as part of it's tracked history 
	 */
	void AddToHistory();

	/** 
	 * Load this reports tracked history 
	 */
	void LoadHistory();

	/**
	 * Update what is tracked for this reports history.
	 */
	void MaintainHistory();

private:

	/** True if this test should be executed */
	bool bEnabled;

	/** True if this test is a parent */
	bool bIsParent;

	/** True if this report should be expanded in the UI */
	bool bNodeExpandInUI;

	/** True if this report has passed the filter and should be highlighted in the UI */
	bool bSelfPassesFilter;

	/** List of bits that represents which device types requested this test */
	uint32 SupportFlags;

	/** Number of responses from network commands */
	uint32 NumberNetworkResponsesReceived;

	/** Number of required devices for this test */
	uint32 RequiredDeviceCount;
	
	/** All child tests */
	TArray<TSharedPtr<IAutomationReport> >ChildReports;

	/** Filtered child tests */
	TArray<TSharedPtr<IAutomationReport> >FilteredChildReports;

	/** Results from execution of the test (per cluster) */
	TArray< TArray<FAutomationTestResults> > Results;

	/** Structure holding the test info */
	FAutomationTestInfo TestInfo;

	/** Flag to determine whether this report should track it's history */
	bool bTrackingHistory;

	/** Flag to determine how many history items to keep */
	int32 NumRecordsToKeep;

	/** The collection of history items which holds the results of previous runs. */
	TArray<TSharedPtr<FAutomationHistoryItem> > HistoryItems;
};
