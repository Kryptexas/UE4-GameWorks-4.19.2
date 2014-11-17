// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Implements a helper class that manages all profiles in the Launcher
 */
class FLauncherProfileManager
	: public ILauncherProfileManager
{
public:

	/** Default constructor. */
	FLauncherProfileManager( );


public:

	// Begin ILauncherProfileManager interface

	virtual void AddDeviceGroup( const ILauncherDeviceGroupRef& DeviceGroup ) override;

	virtual ILauncherDeviceGroupRef AddNewDeviceGroup( ) override;

	virtual ILauncherProfileRef AddNewProfile( ) override;

	virtual void AddProfile( const ILauncherProfileRef& Profile ) override;

	virtual ILauncherProfilePtr FindProfile( const FString& ProfileName ) override;

	virtual const TArray<ILauncherDeviceGroupPtr>& GetAllDeviceGroups( ) const override;

	virtual const TArray<ILauncherProfilePtr>& GetAllProfiles( ) const override;

	virtual ILauncherDeviceGroupPtr GetDeviceGroup( const FGuid& GroupId ) const override;

	virtual ILauncherProfilePtr GetProfile( const FGuid& ProfileId ) const override;

	virtual ILauncherProfilePtr LoadProfile( FArchive& Archive ) override;

	virtual void LoadSettings( ) override;

	virtual FOnLauncherProfileManagerDeviceGroupAdded& OnDeviceGroupAdded( ) override
	{
		return DeviceGroupAddedDelegate;
	}

	virtual FOnLauncherProfileManagerDeviceGroupRemoved& OnDeviceGroupRemoved( ) override
	{
		return DeviceGroupRemovedDelegate;
	}

	virtual FOnLauncherProfileManagerProfileAdded& OnProfileAdded( ) override
	{
		return ProfileAddedDelegate;
	}

	virtual FOnLauncherProfileManagerProfileRemoved& OnProfileRemoved( ) override
	{
		return ProfileRemovedDelegate;
	}

	virtual void RemoveDeviceGroup( const ILauncherDeviceGroupRef& DeviceGroup ) override;

	virtual void RemoveProfile( const ILauncherProfileRef& Profile ) override;

	virtual void SaveProfile( const ILauncherProfileRef& Profile, FArchive& Archive ) override;

	virtual void SaveSettings( ) override;

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
