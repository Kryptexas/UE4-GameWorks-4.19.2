// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LauncherAutomatedServiceProvider.h: Declares the FLauncherAutomatedServiceProvider class.
=============================================================================*/

#pragma once


/**
 * Implements a helper class that manages running the launcher as an automated process
 */
class FLauncherAutomatedServiceProvider
	: public ILauncherAutomatedServiceProvider
{
public:

	/**
	 * Default constructor.
	 */
	FLauncherAutomatedServiceProvider( );


public:

	// Begin ILauncherAutomatedServiceProvider interface

	virtual int32 GetExitCode( ) OVERRIDE
	{
		return bHasErrors ? 1 : 0;
	}

	virtual bool IsRunning( ) OVERRIDE
	{
		return bIsReadyToShutdown == false && bHasErrors == false;
	}

	virtual void Setup( const TCHAR* Params ) OVERRIDE;

	virtual void Shutdown( ) OVERRIDE;

	virtual void Tick( float DeltaTime ) OVERRIDE;

	// End ILauncherAutomatedServiceProvider interface


protected:

	/*
	 * Sets up the profile manager responsible for deploying the session
	 *
	 * @param Params - The settings required to determine the profile/device group to use.
	 */
	void SetupProfileAndGroupSettings( const TCHAR* Params );

	/*
	 * Start the automation tests
	 */
	void StartAutomationTests( );


private:

	/*
	 * Handles devices being added to the proxy manager
	 *
	 * @param AddedProxy - The newly added device proxy to the device proxy manager
	 */
	void HandleDeviceProxyManagerProxyAdded( const ITargetDeviceProxyRef& AddedProxy );


private:

	/** The device group which holds device ids for the session we are about to launch */
	ILauncherDeviceGroupPtr AutomatedDeviceGroup;

	/** The profile which holds the session data to be launched */
	ILauncherProfilePtr AutomatedProfile;

	/** Holds the automation test manager that runs the automation tests */
	TSharedPtr< FAutomatedTestManager > AutomatedTestManager;

	/** Whether we encountered any service issues */
	bool bHasErrors;

	/** Flag used to determine whether we have spawned all instances across the session, and we should now cease trying to deploy */
	bool bHasLaunchedAllInstances;

	/** When the service has finished and all spawned instances have closed, the service should close automatically, This is used in the IsReunning() check */
	bool bIsReadyToShutdown;

	/** Have we requested tests yet */
	bool bRunTestsRequested;

	/** A flag we can use to determine whether the profile we are using can be destroyed at startup to prevent it from being saved */
	bool bShouldDeleteProfileWhenComplete;

	/** The id's of instances we have successfully deployed to */
	TArray< ITargetDeviceProxyPtr > DeployedInstances;

	/** Our interface for dealing with spawned devices in a session */
	ITargetDeviceProxyManagerPtr DeviceProxyManager;

	/** The last time we attempted to deploy to a device */
	double LastDevicePingTime;

	/** A handle to the launchers profile manager */
	ILauncherProfileManagerPtr ProfileManager;

	/** Holds the session ID */
	FGuid SessionID;
	
	/** Time since the last time we attempted to shutdown an instance */
	double TimeSinceLastShutdownRequest;
	
	/** Time since the session was launched */
	float TimeSinceSessionLaunched;
 };
