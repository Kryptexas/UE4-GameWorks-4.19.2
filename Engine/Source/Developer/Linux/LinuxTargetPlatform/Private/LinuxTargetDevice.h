// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LocalLinuxTargetDevice.h: Declares the LocalLinuxTargetDevice class.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of FLinuxTargetDevice.
 */
typedef TSharedPtr<class FLinuxTargetDevice, ESPMode::ThreadSafe> FLinuxTargetDevicePtr;

/**
 * Type definition for shared references to instances of FLinuxTargetDevice.
 */
typedef TSharedRef<class FLinuxTargetDevice, ESPMode::ThreadSafe> FLinuxTargetDeviceRef;


/**
 * Implements a Linux target device.
 */
class FLinuxTargetDevice
	: public ITargetDevice
{
public:

	/**
	 * Creates and initializes a new device for the specified target platform.
	 *
	 * @param InTargetPlatform - The target platform.
	 */
	FLinuxTargetDevice( const ITargetPlatform& InTargetPlatform, const FTargetDeviceId & InDeviceId )
		: TargetPlatform(InTargetPlatform)
		, TargetDeviceId(InDeviceId)
	{ }


public:

	virtual bool Connect( ) OVERRIDE
	{
		return true;
	}

	virtual bool Deploy( const FString& SourceFolder, FString& OutAppId ) OVERRIDE;

	virtual void Disconnect( )
	{ }

	virtual ETargetDeviceTypes::Type GetDeviceType( ) const OVERRIDE
	{
		return ETargetDeviceTypes::Desktop;
	}

	virtual FTargetDeviceId GetId( ) const OVERRIDE
	{
		return TargetDeviceId;
	}

	virtual FString GetName( ) const OVERRIDE
	{
		return FPlatformProcess::ComputerName();
	}

	virtual FString GetOperatingSystemName( ) OVERRIDE
	{
		return TEXT("Linux-GNU");
	}

	virtual int32 GetProcessSnapshot( TArray<FTargetDeviceProcessInfo>& OutProcessInfos ) OVERRIDE;

	virtual const class ITargetPlatform& GetTargetPlatform( ) const OVERRIDE
	{
		return TargetPlatform;
	}

	virtual bool IsConnected( )
	{
		return true;
	}

	virtual bool IsDefault( ) const OVERRIDE
	{
		return true;
	}

	virtual bool PowerOff( bool Force ) OVERRIDE
	{
		return false;
	}

	virtual bool PowerOn( ) OVERRIDE
	{
		return false;
	}

	virtual bool Launch( const FString& AppId, EBuildConfigurations::Type BuildConfiguration, EBuildTargets::Type BuildTarget, const FString& Params, uint32* OutProcessId ) OVERRIDE;
	
	virtual bool Reboot( bool bReconnect = false ) OVERRIDE;

	virtual bool Run( const FString& ExecutablePath, const FString& Params, uint32* OutProcessId ) OVERRIDE;

	virtual bool SupportsFeature( ETargetDeviceFeatures::Type Feature ) const OVERRIDE;

	virtual bool SupportsSdkVersion( const FString& VersionString ) const OVERRIDE;

	virtual void SetUserCredentials( const FString & UserName, const FString & UserPassword ) OVERRIDE;

	virtual bool GetUserCredentials( FString & OutUserName, FString & OutUserPassword ) OVERRIDE;

	virtual bool TerminateProcess( const int32 ProcessId ) OVERRIDE;


private:

	// Holds a reference to the device's target platform.
	const ITargetPlatform& TargetPlatform;

	/** Device id */
	FTargetDeviceId TargetDeviceId;

	/** User name on the remote machine */
	FString UserName;

	/** User password on the remote machine */
	FString UserPassword;
};
