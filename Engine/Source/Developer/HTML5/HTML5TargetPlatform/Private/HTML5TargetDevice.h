// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	HTML5TargetDevice.h: Declares the HTML5TargetDevice class.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of FHTML5TargetDevice.
 */
typedef TSharedPtr<class FHTML5TargetDevice, ESPMode::ThreadSafe> FHTML5TargetDevicePtr;

/**
 * Type definition for shared references to instances of FHTML5TargetDevice.
 */
typedef TSharedRef<class FHTML5TargetDevice, ESPMode::ThreadSafe> FHTML5TargetDeviceRef;


/**
 * Implements a HTML5 target device.
 */
class FHTML5TargetDevice
	: public ITargetDevice
{
public:

	/**
	 * Creates and initializes a new HTML5 target device.
	 *
	 * @param InTarget - The TMAPI target interface.
	 * @param InTargetManager - The TMAPI target manager.
	 * @param InTargetPlatform - The target platform.
	 * @param InName - The device name.
	 */
	FHTML5TargetDevice( const ITargetPlatform& InTargetPlatform, const FString& InName )
		: TargetPlatform(InTargetPlatform)
	{ }

	/**
	 * Destructor.
	 */
	~FHTML5TargetDevice( )
	{
		Disconnect();
	}


public:

	// Begin ITargetDevice interface

	virtual bool Connect( ) OVERRIDE;

	virtual bool Deploy( const FString& SourceFolder, FString& OutAppId ) OVERRIDE;

	virtual void Disconnect( ) OVERRIDE;

	virtual ETargetDeviceTypes::Type GetDeviceType( ) const OVERRIDE
	{
		return ETargetDeviceTypes::Browser;
	}

	virtual FTargetDeviceId GetId( ) const OVERRIDE;

	virtual FString GetName( ) const OVERRIDE;

	virtual FString GetOperatingSystemName( ) OVERRIDE;

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

	virtual bool Launch( const FString& AppId, EBuildConfigurations::Type BuildConfiguration, EBuildTargets::Type BuildTarget, const FString& Params, uint32* OutProcessId ) OVERRIDE;

	virtual bool PowerOff( bool Force ) OVERRIDE;

	virtual bool PowerOn( ) OVERRIDE;

	virtual bool Reboot( bool bReconnect = false ) OVERRIDE;

	virtual bool Run( const FString& ExecutablePath, const FString& Params, uint32* OutProcessId ) OVERRIDE;

	virtual bool SupportsFeature( ETargetDeviceFeatures::Type Feature ) const OVERRIDE;

	virtual bool SupportsSdkVersion( const FString& VersionString ) const OVERRIDE;

	virtual bool TerminateProcess( const int32 ProcessId ) OVERRIDE;

	virtual void SetUserCredentials( const FString & UserName, const FString & UserPassword ) OVERRIDE;

	virtual bool GetUserCredentials( FString & OutUserName, FString & OutUserPassword ) OVERRIDE;

	// End ITargetDevice interface

private:

	// Holds a reference to the device's target platform.
	const ITargetPlatform& TargetPlatform;
};
