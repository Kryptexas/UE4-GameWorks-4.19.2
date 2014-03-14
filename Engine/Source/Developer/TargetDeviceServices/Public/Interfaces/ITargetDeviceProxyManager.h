// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ITargetDeviceProxyManager.h: Declares the ITargetDeviceProxyManager interface.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of ITargetDeviceProxyManager.
 */
typedef TSharedPtr<class ITargetDeviceProxyManager> ITargetDeviceProxyManagerPtr;

/**
 * Type definition for shared references to instances of ITargetDeviceProxyManager.
 */
typedef TSharedRef<class ITargetDeviceProxyManager> ITargetDeviceProxyManagerRef;


/**
 * Interface for device proxy managers.
 */
class ITargetDeviceProxyManager
{
public:
	
	/**
	 * Finds or adds a proxy for the specified device identifier.
	 *
	 * @param DeviceId - The identifier of the device to create the proxy for.
	 *
	 * @return The proxy.
	 */
	virtual ITargetDeviceProxyRef FindOrAddProxy( const FString& DeviceId ) = 0;

	/**
	 * Finds the proxy for the specified device identifier.
	 *
	 * @param DeviceId - The identifier of the device to create the proxy for.
	 *
	 * @return The proxy, or nullptr if it couldn't be found.
	 */
	virtual ITargetDeviceProxyPtr FindProxy( const FString& DeviceId ) = 0;

	/**
	 * Gets a list of devices found by the device discovery.
	 *
	 * @param PlatformName - The the name of the target platform to get proxies for (or empty string for all proxies).
	 * @param IncludeUnshared - Whether to include devices that are not being shared with the local user.
	 * @param OutProxies - Will hold the list of devices found by the locator
	 */
	virtual void GetProxies( const FString& PlatformName, bool IncludeUnshared, TArray<ITargetDeviceProxyPtr>& OutProxies ) = 0;

public:

	/**
	 * Gets an event delegate that is executed when a target device proxy was added.
	 *
	 * @return The event delegate.
	 */
	DECLARE_EVENT_OneParam(ITargetDeviceServiceManager, FOnTargetDeviceProxyAdded, const ITargetDeviceProxyRef& /*AddedProxy*/);
	virtual FOnTargetDeviceProxyAdded& OnProxyAdded( ) = 0;

	/**
	 * Gets an event delegate that is executed when a target device proxy was removed.
	 *
	 * @return The event delegate.
	 */
	DECLARE_EVENT_OneParam(ITargetDeviceServiceManager, FOnTargetDeviceProxyRemoved, const ITargetDeviceProxyRef& /*RemovedProxy*/);
	virtual FOnTargetDeviceProxyRemoved& OnProxyRemoved( ) = 0;

protected:

	ITargetDeviceProxyManager( ) { }
};
