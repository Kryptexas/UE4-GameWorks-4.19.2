// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TargetDeviceProxyManager.h: Declares the FTargetDeviceProxyManager class.
=============================================================================*/

#pragma once


/**
 * Implements a class which locates devices based on criteria for use in the Launcher.
 */
class FTargetDeviceProxyManager
	: public ITargetDeviceProxyManager
{
public:

	/**
	 * Default constructor.
	 */
	FTargetDeviceProxyManager( );

	/**
	 * Destructor.
	 */
	virtual ~FTargetDeviceProxyManager( );

public:

	// Begin ITargetDeviceProxyLocator interface

	virtual ITargetDeviceProxyRef FindOrAddProxy( const FString& DeviceId ) OVERRIDE;

	virtual ITargetDeviceProxyPtr FindProxy( const FString& DeviceId ) OVERRIDE;

	virtual void GetProxies( const FString& PlatformName, bool IncludeUnshared, TArray<ITargetDeviceProxyPtr>& OutProxies ) OVERRIDE;

	DECLARE_DERIVED_EVENT(FTargetDeviceProxyManager, ITargetDeviceProxyManager::FOnTargetDeviceProxyAdded, FOnTargetDeviceProxyAdded);
	virtual FOnTargetDeviceProxyAdded& OnProxyAdded( ) OVERRIDE
	{
		return ProxyAddedDelegate;
	}

	DECLARE_DERIVED_EVENT(FTargetDeviceProxyManager, ITargetDeviceProxyManager::FOnTargetDeviceProxyRemoved, FOnTargetDeviceProxyRemoved);
	virtual FOnTargetDeviceProxyRemoved& OnProxyRemoved( ) OVERRIDE
	{
		return ProxyRemovedDelegate;
	}

	// End ITargetDeviceProxyLocator interface

protected:

	/**
	 * Removes all target device proxies that timed out.
	 */
	void RemoveDeadProxies( );

	/**
	 * Pings all target devices on the network.
	 */
	void SendPing( );

private:
	
	// Handles FTargetDeviceServicePong messages.
	void HandlePongMessage( const FTargetDeviceServicePong& Message, const IMessageContextRef& Context );

	// Handles ticks from the ticker.
	bool HandleTicker( float DeltaTime );

private:

	// Holds the message endpoint.
	FMessageEndpointPtr MessageEndpoint;

	// Holds the collection of proxies.
	TMap<FString, FTargetDeviceProxyPtr> Proxies;

private:

	// Holds a delegate that is invoked when a target device proxy has been added.
	FOnTargetDeviceProxyAdded ProxyAddedDelegate;

	// Holds a delegate that is invoked when a target device proxy has been removed.
	FOnTargetDeviceProxyRemoved ProxyRemovedDelegate;

	// Holds a delegate to be invoked when the widget ticks.
	FTickerDelegate TickDelegate;
};
