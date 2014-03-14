// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	FTargetDeviceProxy.h: Declares the FTargetDeviceProxy class.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of FTargetDeviceProxy.
 */
typedef TSharedPtr<class FTargetDeviceProxy> FTargetDeviceProxyPtr;

/**
 * Type definition for shared references to instances of FTargetDeviceProxy.
 */
typedef TSharedRef<class FTargetDeviceProxy> FTargetDeviceProxyRef;


/**
 * Implementation of the device proxy.
 */
class FTargetDeviceProxy
	: public ITargetDeviceProxy
{
public:

	/**
	 * Default constructor
	 */
	FTargetDeviceProxy( ) { }

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InId - The identifier of the target device to create this proxy for.
	 */
	FTargetDeviceProxy( const FString& InId );

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InId - The identifier of the target device to create this proxy for.
	 * @param Message - The message to initialize from.
	 * @param Context - The message context.
	 */
	FTargetDeviceProxy( const FString& InId, const FTargetDeviceServicePong& Message, const IMessageContextRef& Context );

public:

	/**
	 * Gets the time at which the proxy was last updated.
	 *
	 * @return Date and time of the last update.
	 */
	const FDateTime& GetLastUpdateTime( ) const
	{
		return LastUpdateTime;
	}

	/*
	 * Updates the proxy's information from the given device service response.
	 *
	 * @param Message - The message containing the response.
	 * @param Context - The message context.
	 */
	void UpdateFromMessage( const FTargetDeviceServicePong& Message, const IMessageContextRef& Context );

public:

	// Begin ITargetDeviceProxy interface

	virtual const bool CanMultiLaunch( ) const OVERRIDE
	{
		return SupportsMultiLaunch;
	}

	virtual bool CanPowerOff( ) const OVERRIDE
	{
		return SupportsPowerOff;
	}

	virtual bool CanPowerOn( ) const OVERRIDE
	{
		return SupportsPowerOn;
	}

	virtual bool CanReboot( ) const OVERRIDE
	{
		return SupportsReboot;
	}

	virtual bool DeployApp( const TMap<FString, FString>& Files, const FGuid& TransactionId ) OVERRIDE;

	virtual const FString& GetDeviceId( ) OVERRIDE
	{
		return Id;
	}

	virtual const FString& GetHostName( ) const OVERRIDE
	{
		return HostName;
	}

	virtual const FString& GetHostUser( ) const OVERRIDE
	{
		return HostUser;
	}

	virtual const FString& GetMake( ) const OVERRIDE
	{
		return Make;
	}

	virtual const FString& GetModel( ) const OVERRIDE
	{
		return Model;
	}

	virtual const FString& GetName( ) const OVERRIDE
	{
		return Name;
	}

	virtual const FString& GetDeviceUser( ) const OVERRIDE
	{
		return DeviceUser;
	}

	virtual const FString& GetDeviceUserPassword( ) const OVERRIDE
	{
		return DeviceUserPassword;
	}

	virtual const FString& GetPlatformName( ) const OVERRIDE
	{
		return Platform;
	}

	virtual const FString& GetType( ) const OVERRIDE
	{
		return Type;
	}

	virtual bool IsConnected( ) const OVERRIDE
	{
		return Connected;
	}

	virtual bool IsShared( ) const OVERRIDE
	{
		return Shared;
	}

	virtual bool LaunchApp( const FString& AppId, EBuildConfigurations::Type BuildConfiguration, const FString& Params ) OVERRIDE;

	virtual FOnTargetDeviceProxyDeployCommitted& OnDeployCommitted( ) OVERRIDE
	{
		return DeployCommittedDelegate;
	}

	virtual FOnTargetDeviceProxyDeployFailed& OnDeployFailed( ) OVERRIDE
	{
		return DeployFailedDelegate;
	}

	virtual FOnTargetDeviceProxyLaunchFailed& OnLaunchFailed( ) OVERRIDE
	{
		return LaunchFailedDelegate;
	}

	virtual void PowerOff( bool Force ) OVERRIDE;

	virtual void PowerOn( ) OVERRIDE;

	virtual void Reboot( ) OVERRIDE;

	virtual void Run( const FString& ExecutablePath, const FString& Params ) OVERRIDE;

	// End ITargetDeviceProxy interface

protected:

	/**
	 * Initializes the message endpoint.
	 */
	void InitializeMessaging( );

private:

	// Handles FTargetDeviceServiceDeployFinishedMessage messages.
	void HandleDeployFinishedMessage( const FTargetDeviceServiceDeployFinished& Message, const IMessageContextRef& Context );

	// Handles FTargetDeviceServiceLaunchFinishedMessage messages.
	void HandleLaunchFinishedMessage( const FTargetDeviceServiceLaunchFinished& Message, const IMessageContextRef& Context );

private:

	// Holds a flag indicating whether the device is connected.
	bool Connected;

	// Holds the name of the computer that hosts the device.
	FString HostName;

	// Holds the name of the user that owns the device.
	FString HostUser;

	// Holds the remote device's identifier.
	FString Id;

	// Holds the time at which the last ping reply was received.
	FDateTime LastUpdateTime;

	// Holds the device make.
	FString Make;

	// Holds the remote device's message bus address.
	FMessageAddress MessageAddress;

	// Holds the local message bus endpoint.
	FMessageEndpointPtr MessageEndpoint;

	// Holds the remote device's model.
	FString Model;

	// Holds the name of the device.
	FString Name;

	// Holds the device's platform name.
	FString Platform;

	// Holds device user
	FString DeviceUser;

	// Holds device user password
	FString DeviceUserPassword;

	// Holds a flag indicating whether the device is being shared with other users.
	bool Shared;

	// Holds a flag indicating whether the device supports multi-launch.
	bool SupportsMultiLaunch;

	// Holds a flag indicating whether the device can power off.
	bool SupportsPowerOff;

	// Holds a flag indicating whether the device can be power on.
	bool SupportsPowerOn;

	// Holds a flag indicating whether the device can reboot.
	bool SupportsReboot;

	// Holds the device type.
	FString Type;

private:

	// Holds a delegate to be invoked when a build has been deployed to the target device.
	FOnTargetDeviceProxyDeployCommitted DeployCommittedDelegate;

	// Holds a delegate to be invoked when a build has failed to deploy to the target device.
	FOnTargetDeviceProxyDeployFailed DeployFailedDelegate;

	// Holds a delegate to be invoked when a build has failed to launch on the target device.
	FOnTargetDeviceProxyLaunchFailed LaunchFailedDelegate;

	// Holds a delegate to be invoked when a build has succeeded to launch on the target device.
	FOnTargetDeviceProxyLaunchSucceeded LaunchSucceededDelegate;
};
