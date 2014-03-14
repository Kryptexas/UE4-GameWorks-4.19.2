// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LauncherProfileManager.h: Declares the FLauncherProfileManager class.
=============================================================================*/

#pragma once


/**
 * Implements a helper class that manages all profiles in the Launcher
 */
class FLauncherProfileManager
	: public ILauncherProfileManager
{
public:

	/**
	 * Default constructor.
	 */
	FLauncherProfileManager( );


public:

	// Begin ILauncherProfileManager interface

	virtual void AddDeviceGroup( const ILauncherDeviceGroupRef& DeviceGroup ) OVERRIDE;

	virtual ILauncherDeviceGroupRef AddNewDeviceGroup( ) OVERRIDE;

	virtual ILauncherProfileRef AddNewProfile( ) OVERRIDE;

	virtual void AddProfile( const ILauncherProfileRef& Profile ) OVERRIDE;

	virtual ILauncherProfilePtr FindProfile( const FString& ProfileName ) OVERRIDE;

	virtual const TArray<ILauncherDeviceGroupPtr>& GetAllDeviceGroups( ) const OVERRIDE;

	virtual const TArray<ILauncherProfilePtr>& GetAllProfiles( ) const OVERRIDE;

	virtual ILauncherDeviceGroupPtr GetDeviceGroup( const FGuid& GroupId ) const OVERRIDE;

	virtual ILauncherProfilePtr GetProfile( const FGuid& ProfileId ) const OVERRIDE;

	virtual ILauncherProfilePtr LoadProfile( FArchive& Archive ) OVERRIDE;

	virtual void LoadSettings( ) OVERRIDE;

	virtual FOnLauncherProfileManagerDeviceGroupAdded& OnDeviceGroupAdded( ) OVERRIDE
	{
		return DeviceGroupAddedDelegate;
	}

	virtual FOnLauncherProfileManagerDeviceGroupRemoved& OnDeviceGroupRemoved( ) OVERRIDE
	{
		return DeviceGroupRemovedDelegate;
	}

	virtual FOnLauncherProfileManagerProfileAdded& OnProfileAdded( ) OVERRIDE
	{
		return ProfileAddedDelegate;
	}

	virtual FOnLauncherProfileManagerProfileRemoved& OnProfileRemoved( ) OVERRIDE
	{
		return ProfileRemovedDelegate;
	}

	virtual void RemoveDeviceGroup( const ILauncherDeviceGroupRef& DeviceGroup ) OVERRIDE;

	virtual void RemoveProfile( const ILauncherProfileRef& Profile ) OVERRIDE;

	virtual void SaveProfile( const ILauncherProfileRef& Profile, FArchive& Archive ) OVERRIDE;

	virtual void SaveSettings( ) OVERRIDE;

	// End ILauncherProfileManager interface


protected:

	/*
	 * Loads all the device groups from a config file
	 */
	void LoadDeviceGroups( );

	/*
	 * Load all profiles from disk.
	 */
	void LoadProfiles( );

	/**
	 * Create a new device group from the specified string value.
	 *
	 * @param GroupString - The string to parse.
	 *
	 * @return The new device group object.
	 */
	ILauncherDeviceGroupPtr ParseDeviceGroup( const FString& GroupString );

	/*
	 * Saves all the device groups to a config file
	 */
	void SaveDeviceGroups( );

	/*
	 * Saves all profiles to disk.
	 */
	void SaveProfiles( );


protected:

	/**
	 * Gets the folder in which profile files are stored.
	 *
	 * @return The folder path.
	 */
	static FString GetProfileFolder( )
	{
		return FPaths::EngineSavedDir() / TEXT("Launcher");
	}


private:

	// Holds the collection of device groups.
	TArray<ILauncherDeviceGroupPtr> DeviceGroups;

	// Holds the collection of launcher profiles.
	TArray<ILauncherProfilePtr> Profiles;


private:

	// Holds a delegate to be invoked when a device group was added.
	FOnLauncherProfileManagerDeviceGroupAdded DeviceGroupAddedDelegate;

	// Holds a delegate to be invoked when a device group was removed.
	FOnLauncherProfileManagerDeviceGroupRemoved DeviceGroupRemovedDelegate;

	// Holds a delegate to be invoked when a launcher profile was added.
	FOnLauncherProfileManagerProfileAdded ProfileAddedDelegate;

	// Holds a delegate to be invoked when a launcher profile was removed.
	FOnLauncherProfileManagerProfileRemoved ProfileRemovedDelegate;
};
