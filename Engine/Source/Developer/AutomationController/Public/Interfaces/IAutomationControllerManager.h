// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IAutomationController.h: Declares the IAutomationController interface.
=============================================================================*/

#pragma once

/**
 * Type definition for shared pointers to instances of IAutomationControllerManager.
 */
typedef TSharedPtr<class IAutomationControllerManager> IAutomationControllerManagerPtr;

/**
 * Type definition for shared references to instances of IAutomationControllerManager.
 */
typedef TSharedRef<class IAutomationControllerManager> IAutomationControllerManagerRef;

// Enum to indicate the state of the workers in the automation controller
namespace EAutomationControllerModuleState 
{
	enum Type
	{
		// Has workers available
		Ready,
		// Is running tests
		Running,
		// No workers available
		Disabled
	};
};

/**
* Enum used to set which reports to export
*/
namespace EFileExportType
{
	enum Type
	{
		FET_Status		= 0x00000001, // Export test status
		FET_Warnings	= 0x00000002, // Export warnings
		FET_Errors		= 0x00000004, // Export Errors
		FET_Logs		= 0x00000008, // Export logs
		FET_All			= 0x00000010  // Export everything
	};

	// Helper functions to set and check export flags
	
	/**
	* Check if a flag is set
	*
	* @param InMask The mask to check
	* @param InFlag The flag to check
	* @return True if the flag is set
	*/
	FORCEINLINE static bool IsSet( const uint32& InMask, const EFileExportType::Type InFlag )
	{
		if ( InMask & ( 1<<(uint32)InFlag ) )
		{
			return true;
		}
		return false;
	}

	/**
	* Remove a flag
	*
	* @param InMask The mask to unset the flag on
	* @param InFlag The flag to remove
	*/
	FORCEINLINE static void RemoveFlag( uint32& InMask, const EFileExportType::Type InFlag )
	{
		InMask &= ~( 1<<(uint32)InFlag );
	}

	/**
	* Set a flag
	*
	* @param InMask The mask to set the flag on
	* @param InFlag The flag to set
	*/
	FORCEINLINE static void SetFlag( uint32& InMask, const EFileExportType::Type InFlag )
	{
		InMask |= ( 1<<(uint32)InFlag );
	}

};


/** Console command delegate type (takes no arguments.)  This is a void callback function. */
DECLARE_DELEGATE(FOnAutomationControllerTestsRefreshed);

/** Delegate for setting the automation controller status in the UI */
DECLARE_DELEGATE_OneParam(FOnAutomationControllerManagerTestsAvailable, EAutomationControllerModuleState::Type);

/** Delegate to call when the automation controller is shutdown. */
DECLARE_DELEGATE(FOnAutomationControllerManagerShutdown);


/**
 * Interface for AutomationController .
 */
class IAutomationControllerManager
{
public:

	/** 
	 * Broadcast a request for workers to ping if they are available 
	 * 
	 * @param InSessionID the session ID to run the test on
	 */
	virtual void RequestAvailableWorkers( const FGuid& InSessionId ) = 0;

	/** Send request for tests that are available to run */
	virtual void RequestTests() = 0;

	/** 
	 * Resets all enabled tests to be able to run during Tick by local/remote machines
	 *
	 * @param bIsLocalSeesion - indicate if this is a local session. We do not time out local session automation tests
	 */
	virtual void RunTests( const bool bIsLocalSession = false ) = 0;

	/**
	 * Stops all running tests.
	 */
	virtual void StopTests() = 0;

	/** Initializes the automation controller */
	virtual void Init() = 0;

	/**
	 * Request an asset to be loaded in the editor
	 *
	 * @param InAssetName - the asset name
	 */
	virtual void RequestLoadAsset( const FString& InAssetName ) = 0;

	/** Tick function that will execute enabled tests for different device clusters */
	virtual void Tick() = 0;

	/**
	 * Filters the visible list of tests.
	 */
	virtual void SetFilter( TSharedPtr< AutomationFilterCollection > InFilter ) = 0;

	/**
	 * Gives the array of test results to the UI.
	 */
	virtual TArray <TSharedPtr <IAutomationReport> >& GetReports() = 0;

	/**
	 * Get num devices types.
	 */
	virtual int32 GetNumDeviceClusters() const = 0;

	/**
	 * Get num devices in specified cluster.
	 */
	virtual int32 GetNumDevicesInCluster(const int32 ClusterIndex) const = 0;

	/**
	 * Get name of a particular device cluster.
	 */
	virtual FString GetDeviceTypeName(const int32 ClusterIndex) const = 0;

	/**
	 * Get a game instance name
	 *
	 * @param ClusterIndex The cluster Index
 	 * @param DeviceIndex The Device Index
	 */
	virtual FString GetGameInstanceName(const int32 ClusterIndex, const int32 DeviceIndex) const = 0;

	/**
	 * Sets whether all visible tests are enabled or not.
	 */
	virtual void SetVisibleTestsEnabled(const bool bEnabled) = 0;

	/**
	 * Returns number of tests that will be run.
	 */
	virtual int32 GetEnabledTestsNum() const = 0;

	/**
	 * Gets the controller's current test state.
	 *
	 * @return The test state.
	 */
	virtual EAutomationControllerModuleState::Type GetTestState( ) const = 0;

	/**
	 * Sets whether the automation tests should include developer content directories.
	 */
	virtual void SetDeveloperDirectoryIncluded(const bool bInDeveloperDirectoryIncluded) = 0;

	/**
	 * Returns whether the automation tests should include developer content directories.
	 */
	virtual bool IsDeveloperDirectoryIncluded(void) const = 0;

	/**
	 * Sets whether the automation tests should include visual commandlet.
	 */
	virtual void SetVisualCommandletFilter(const bool bInVisualCommandletFilterOn) = 0;

	/**
	 * Returns whether the automation tests should include visual commandlet.
	 *
	 * @return true if the visual commandlet should be included, false otherwise.
	 */
	virtual bool IsVisualCommandletFilterOn(void) const = 0;

	/**
	 * Check if the automation tests have completed
	 *
	 * @return true if the tests are available
	 */
	virtual const bool CheckTestResultsAvailable() const = 0;

	/**
	 * Check if the automation tests results have errors
	 *
	 * @return true if the tests have errors
	 */
	virtual const bool ReportsHaveErrors() const = 0;

	/**
	 * Check if the automation tests results have warnings
	 *
	 * @return true if the tests have warnings
	 */
	virtual const bool ReportsHaveWarnings() const = 0;

	/**
	 * Check if the automation tests results have logs
	 *
	 * @return true if the tests have logs
	 */
	virtual const bool ReportsHaveLogs() const = 0;

	/**
	 * Remove results from the automation controller module
	 */
	virtual void ClearAutomationReports() = 0;

	/**
	 * Generate an automation report
	 *
	 * @param FileExporTypeMask - The types of report to export. Warning errors etc
	 */
	virtual const bool ExportReport( uint32 FileExportTypeMask ) = 0;

	/** 
	 * Check that the test we are looking to run is runnable
	 *
	 * @param InReport - The test we are checking is runnable
	 *
	 * @return true if the test can be run, FALSE if not
	 */
	virtual bool IsTestRunnable( IAutomationReportPtr InReport ) const = 0;

	/**
	 * Removes all callbacks.
	 */
	virtual void RemoveCallbacks() = 0;

	/**
	 * Shuts down the messaging system.
	 */
	virtual void Shutdown() = 0;

	/**
	 * Starts up the messaging system.
	 */
	virtual void Startup() = 0;


public:

	/**
	 * Gets a delegate that is invoked when the controller manager shuts down.
	 *
	 * @return The delegate.
	 */
	virtual FOnAutomationControllerManagerShutdown& OnShutdown( ) = 0;

	/**
	 * Gets a delegate that is invoked when the controller has tests available.
	 *
	 * @return The delegate.
	 */
	virtual FOnAutomationControllerManagerTestsAvailable& OnTestsAvailable( ) = 0;

	/**
	 * Gets a delegate that is invoked when the controller's test status changes.
	 *
	 * @return The delegate.
	 */
	virtual FOnAutomationControllerTestsRefreshed& OnTestsRefreshed( ) = 0;
};
