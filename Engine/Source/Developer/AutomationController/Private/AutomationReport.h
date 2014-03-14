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
	virtual void Empty() OVERRIDE;

	/** Get the asset associated with tis report */
	virtual FString GetAssetName() const OVERRIDE;

	/**
	 * Returns the complete command line for an automation test including any relevant parameters
	 */
	virtual FString GetCommand() const OVERRIDE;

	/**
	 * Returns the name of this level in the test hierarchy for the purposes of grouping.
	 * (Editor.Maps.LoadAll.parameters) would have 3 internal tree nodes and a variable number of leaf nodes depending on how many maps existed
	 * @param bAddDecoration - True is the name should have the number of child tests appended
	 * @return the name of this level in the hierarchy for use in UI
	 */
	virtual const FString& GetDisplayName() const OVERRIDE;

	/**
	 * Returns the name of this level in the test hierarchy for the purposes of UI.
	 * (Editor.Maps.LoadAll.parameters) would have 3 internal tree nodes and a variable number of leaf nodes depending on how many maps existed
	 * @return the name of this level in the hierarchy for use in UI
	 */
	virtual FString GetDisplayNameWithDecoration() const OVERRIDE;

	/** Recursively gets the number of child nodes */
	virtual int32 GetTotalNumChildren() const OVERRIDE;
	/** Recursively gets the number of enabled tests */
	virtual int32 GetEnabledTestsNum() const OVERRIDE;

	/** Return if this test should be executed */
	virtual bool IsEnabled() const OVERRIDE;
	/** Sets whether this test should be executed or not */
	virtual void SetEnabled(bool bShouldBeEnabled) OVERRIDE;
	
	/** Sets whether this test is supported on a particular platform */
	virtual void SetSupport(const int32 ClusterIndex) OVERRIDE;
	/** Returns if a particular platform is supported */
	virtual bool IsSupported(const int32 ClusterIndex) const OVERRIDE;

	/** Sets the test type for parent reports */
	virtual void SetTestType( const uint8 TestType ) OVERRIDE;
	/** Returns the test type */
	virtual uint8 GetTestType( ) const OVERRIDE;

	/** Returns true if this is a parent test report */
	virtual const bool IsParent( ) OVERRIDE;
	/** Returns true if this is a smoke test report */
	virtual const bool IsSmokeTest( ) OVERRIDE;
	/**
	 * Filters the visible tests based on search text, execution status, regression test, etc
	 * @param InFilterDesc - The description of how to decide if this report should be visible
	 * @param ParentPassedFilter - If the parent passed the filter
	 * @return whether this report or any of its children passed the filter
	 */
	virtual bool SetFilter( TSharedPtr< AutomationFilterCollection > InFilter, const bool ParentPassedFilter = false ) OVERRIDE;

	/** Returns the array of child reports that should be visible to the UI based on filtering */
	virtual TArray<TSharedPtr<IAutomationReport> >& GetFilteredChildren() OVERRIDE;

	/** Returns the array of child reports */
	virtual TArray<TSharedPtr<IAutomationReport> >& GetChildReports() OVERRIDE;

	/** Recursively resets the report to "needs to be run", clears cached warnings and errors */
	virtual void ResetForExecution() OVERRIDE;
	/**
	 * Sets the results of the test for use by the UI
	 * @param ClusterIndex - Index of the platform reporting the results of this test.  See AutomationDeviceClusterManager
	 * @param InResults - The new set of results
	 */
	virtual void SetResults( const int32 ClusterIndex, const FAutomationTestResults& InResults ) OVERRIDE;

	/**
	 * Returns completion statistics for this branch of the testing hierarchy
	 * @param ClusterIndex - Index of the platform reporting the results of this test.  See AutomationDeviceClusterManager
	 * @param OutCompletionState - Collection structure for execution statistics
	 */
	virtual void GetCompletionStatus(const int32 ClusterIndex, FAutomationCompleteState& OutCompletionState) OVERRIDE;

	/**
	 * Returns the state of the test (not run, in process, success, failure)
	 * @param ClusterIndex - Index of the platform reporting the results of this test.  See AutomationDeviceClusterManager
	 * @return the current state of the test
	 */
	virtual EAutomationState::Type GetState(const int32 ClusterIndex) const OVERRIDE;

	/**
	 * Gets a copy of errors and warnings that were found
	 *
	 * @param ClusterIndex - Index of the platform we are requesting test results for.
	 *
	 * @return The collection of results for the given cluster index
	 */
	virtual const FAutomationTestResults& GetResults( const int32 ClusterIndex ) OVERRIDE;

	/**
	 * Gets the name of the instance that ran the test
	 * @param ClusterIndex - Index of the platform reporting the results of this test.  See AutomationDeviceClusterManager
	 * @return the name of the device
	 */
	virtual FString GetGameInstanceName( const int32 ClusterIndex ) OVERRIDE;

	/**
	 * Add a child test to the hierarchy, creating internal tree nodes as needed
	 * If NewTestName is Editor.Maps.Loadall.TestName, this will create nodes for Editor, Maps, Loadall, and then a leaf node for the testname with the associated commandline
	 * @param TestInfo - Structure containing all the test info
	 * @param ClusterIndex - Index of the platform reporting the results of this test.  See AutomationDeviceClusterManager
	 * @return - the automation report
	 */
	virtual TSharedPtr<IAutomationReport> EnsureReportExists(FAutomationTestInfo& TestInfo, const int32 ClusterIndex) OVERRIDE;

	/**
	 * Returns the next test in the hierarchy to run
	 * @param bOutAllTestsComplete - Whether or not all enabled tests have completed execution for this platform (cluster index)
	 * @param ClusterIndex - Index of the platform reporting the results of this test.  See AutomationDeviceClusterManager
	 * @param NumDevicesInCluster - The number of devices which are in this cluster
	 *
	 * @return The next report
	 */
	virtual TSharedPtr<IAutomationReport> GetNextReportToExecute(bool& bOutAllTestsComplete, const int32 ClusterIndex, const int32 NumDevicesInCluster) OVERRIDE;

	/** 
	 * Have any results got errors
	 * @return true if they have errors
	 */
	virtual const bool HasErrors() OVERRIDE;

	/** 
	 * Have any results got warnings
	 * @return true if they have warnings
	 */
	virtual const bool HasWarnings() OVERRIDE;

	/**
	 * Gets the min and max time this test took to execute
	 * @param OutMinTime - Minimum execution time for all device types
	 * @param OutMaxTime - Maximum execution time for all device types
	 * @return whether any test has completed successfully
	 */
	virtual const bool GetDurationRange(float& OutMinTime, float& OutMaxTime) OVERRIDE;

	/**
	 * Get the number of devices which have been given this test to run
	 *
	 * @return - The number of devices who have been given this test to run
	 */
	virtual const int32 GetNumDevicesRunningTest() const OVERRIDE;

	/**
	 * Get the number of participants this test requires.
	 *
	 * @Return - The number of devices needed for this test to execute
	 */
	virtual const int32 GetNumParticipantsRequired() const OVERRIDE;
	
	/**
	 * Set the number of participants this test requires if less than what is already set.
	 *
	 * @param NewCount - The number of further devices 
	 */
	virtual void SetNumParticipantsRequired( const int32 NewCount ) OVERRIDE;

	/**
	 * Increment the number of network responses
	 * @return - whether we have received responses from all participants
	 */
	virtual bool IncrementNetworkCommandResponses() OVERRIDE;

	/**
	 * Resets the number of network responses back to zero
	 */
	virtual void ResetNetworkCommandResponses() OVERRIDE;

	/**
	 * Should we expand this node in the UI - A child has passed the filter
	 * @return true if we should expand the node
	 */
	virtual const bool ExpandInUI() const OVERRIDE;

	/** Stop the test which is creating this report */
	virtual void StopRunningTest() OVERRIDE;

	// End IAutomationReport Interface
	
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
	TArray< FAutomationTestResults > Results;

	/** Structure holding the test info */
	FAutomationTestInfo TestInfo;
};
