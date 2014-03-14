// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TargetDeviceServiceManager.h: Declares the FTargetDeviceServiceManager class.
=============================================================================*/

#pragma once


/**
 * Implements a target device service manager.
 *
 * Services that were running at the 
 */
class FTargetDeviceServiceManager
	: public ITargetDeviceServiceManager
{
public:

	/**
	 * Default constructor.
	 */
	FTargetDeviceServiceManager( );

	/**
	 * Destructor.
	 */
	~FTargetDeviceServiceManager( );

public:

	// Begin ITargetDeviceServiceManager interface

	virtual bool AddStartupService( const FTargetDeviceId& DeviceId, const FString& PreliminaryDeviceName ) OVERRIDE;

	virtual int32 GetServices( TArray<ITargetDeviceServicePtr>& OutServices ) OVERRIDE;

	DECLARE_DERIVED_EVENT(FTargetDeviceServiceManager, ITargetDeviceServiceManager::FOnTargetDeviceServiceAdded, FOnTargetDeviceServiceAdded);
	virtual FOnTargetDeviceServiceAdded& OnServiceAdded( ) OVERRIDE
	{
		return ServiceAddedDelegate;
	}

	DECLARE_DERIVED_EVENT(FTargetDeviceServiceManager, ITargetDeviceServiceManager::FOnTargetDeviceServiceRemoved, FOnTargetDeviceServiceRemoved);
	virtual FOnTargetDeviceServiceRemoved& OnServiceRemoved( ) OVERRIDE
	{
		return ServiceRemovedDelegate;
	}

	virtual void RemoveStartupService( const FTargetDeviceId& DeviceId ) OVERRIDE;

	// End ITargetDeviceServiceManager interface

protected:

	/**
	 * Adds a device service for the given target device.
	 *
	 * @param DeviceId - The identifier of the device to add a service for.
	 * @param PreliminaryDeviceName - The preliminary name to assign to this device.
	 *
	 * @return true if the service was created, false otherwise.
	 */
	bool AddService( const FTargetDeviceId& DeviceId, const FString& PreliminaryDeviceName );

	/**
	 * Initializes target platforms callbacks.
	 *
	 * @see ShutdownTargetPlatforms
	 */
	void InitializeTargetPlatforms( );

	/**
	 * Loads all settings from disk.
	 *
	 * @see SaveSettings
	 */
	void LoadSettings( );

	/**
	 * Removes the device service for the given target device.
	 *
	 * @param DeviceId - The identifier of the device to remove the service for.
	 */
	void RemoveService( const FTargetDeviceId& DeviceId );

	/**
	 * Saves all settings to disk.
	 *
	 * @see LoadSettings
	 */
	void SaveSettings( );

	/**
	 * Shuts down target platforms callbacks.
	 *
	 * @see InitializeTargetPlatforms
	 */
	void ShutdownTargetPlatforms( );

private:

	// Callback for handling message bus shutdowns.
	void HandleMessageBusShutdown( );

	// Callback for discovered target devices.
	void HandleTargetPlatformDeviceDiscovered( const ITargetDeviceRef& DiscoveredDevice );

	// Callback for lost target devices.
	void HandleTargetPlatformDeviceLost( const ITargetDeviceRef& LostDevice );

private:

	// Holds a critical section object.
	FCriticalSection CriticalSection;

	// Holds the list of locally managed device services.
	TMap<FTargetDeviceId, ITargetDeviceServicePtr> DeviceServices;

	// Holds a weak pointer to the message bus.
	IMessageBusWeakPtr MessageBusPtr;

	// Holds the collection of identifiers for devices that start automatically (shared/unshared).
	TMap<FTargetDeviceId, bool> StartupServices;

private:

	// Holds a delegate that is invoked when a target device service was added.
	FOnTargetDeviceServiceAdded ServiceAddedDelegate;

	// Holds a delegate that is invoked when a target device service was removed.
	FOnTargetDeviceServiceRemoved ServiceRemovedDelegate;
};
