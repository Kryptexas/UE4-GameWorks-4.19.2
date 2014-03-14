// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ITargetDeviceServiceManager.h: Declares the ITargetDeviceServiceManager interface.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of ITargetDeviceServiceManager.
 */
typedef TSharedPtr<class ITargetDeviceServiceManager> ITargetDeviceServiceManagerPtr;

/**
 * Type definition for shared references to instances of ITargetDeviceServiceManager.
 */
typedef TSharedRef<class ITargetDeviceServiceManager> ITargetDeviceServiceManagerRef;


/**
 * Interface for target device service managers.
 */
class ITargetDeviceServiceManager
{
public:

	/**
	 * Adds a service to the list of services that are started automatically.
	 *
	 * A preliminary device name may be assigned to services that expose devices which
	 * could not be discovered automatically or are currently unavailable. This name will
	 * be replaced with the actual device name once the physical device becomes available.
	 *
	 * @param DeviceId - The device identifier for the service to add.
	 * @param PreliminaryDeviceName - The preliminary device name.
	 *
	 * @return true if the service added, false otherwise.
	 *
	 * @see GetServices
	 * @see RemoveStartupService
	 */
	virtual bool AddStartupService( const FTargetDeviceId& DeviceId, const FString& PreliminaryDeviceName ) = 0;

	/**
	 * Gets the collection of target device services managed by this instance.
	 *
	 * @param OutServices - Will contain the collection of services.
	 *
	 * @return The number of services returned.
	 */
	virtual int32 GetServices( TArray<ITargetDeviceServicePtr>& OutServices ) = 0;

	/**
	 * Removes a service from the list of services that are started automatically.
	 *
	 * @param DeviceId - The device identifier for the service to remove.
	 *
	 * @see AddStartupService
	 * @see GetServices
	 */
	virtual void RemoveStartupService( const FTargetDeviceId& DeviceId ) = 0;

public:

	/**
	 * Gets an event delegate that is executed when a target device service was added.
	 *
	 * @return The event delegate.
	 */
	DECLARE_EVENT_OneParam(ITargetDeviceServiceManager, FOnTargetDeviceServiceAdded, const ITargetDeviceServiceRef& /*AddedService*/);
	virtual FOnTargetDeviceServiceAdded& OnServiceAdded( ) = 0;

	/**
	 * Gets an event delegate that is executed when a target device service was removed.
	 *
	 * @return The event delegate.
	 */
	DECLARE_EVENT_OneParam(ITargetDeviceServiceManager, FOnTargetDeviceServiceRemoved, const ITargetDeviceServiceRef& /*RemovedService*/);
	virtual FOnTargetDeviceServiceRemoved& OnServiceRemoved( ) = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~ITargetDeviceServiceManager( ) { }
};
