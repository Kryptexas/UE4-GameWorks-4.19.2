// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LocalMacTargetDevice.h: Declares the TLocalMacTargetDevice class.
=============================================================================*/

#pragma once


/**
 * Template for local Mac target devices.
 */
class FLocalMacTargetDevice
	: public ITargetDevice
{
public:

	/**
	 * Creates and initializes a new device for the specified target platform.
	 *
	 * @param InTargetPlatform - The target platform.
	 */
	FLocalMacTargetDevice( const ITargetPlatform& InTargetPlatform )
		: TargetPlatform(InTargetPlatform)
	{ }


public:

	virtual bool Connect( ) OVERRIDE
	{
		return true;
	}

	virtual bool Deploy( const FString& SourceFolder, FString& OutAppId ) OVERRIDE
	{
		OutAppId = TEXT("");

		FString PlatformName = TEXT("Mac");
		FString DeploymentDir = FPaths::EngineIntermediateDir() / TEXT("Devices") / PlatformName;

		// delete previous build
		IFileManager::Get().DeleteDirectory(*DeploymentDir, false, true);

		// copy files into device directory
		TArray<FString> FileNames;

		IFileManager::Get().FindFilesRecursive(FileNames, *SourceFolder, TEXT("*.*"), true, false);

		for (int32 FileIndex = 0; FileIndex < FileNames.Num(); ++FileIndex)
		{
			const FString& SourceFilePath = FileNames[FileIndex];
			FString DestFilePath = DeploymentDir + SourceFilePath.RightChop(SourceFolder.Len());

			IFileManager::Get().Copy(*DestFilePath, *SourceFilePath);
		}

		return true;
	}

	virtual void Disconnect( )
	{ }

	virtual ETargetDeviceTypes::Type GetDeviceType( ) const OVERRIDE
	{
		return ETargetDeviceTypes::Desktop;
	}

	virtual FTargetDeviceId GetId( ) const OVERRIDE
	{
		return FTargetDeviceId(TargetPlatform.PlatformName(), GetName());
	}

	virtual FString GetName( ) const OVERRIDE
	{
		return FPlatformProcess::ComputerName();
	}

	virtual FString GetOperatingSystemName( ) OVERRIDE
	{
		return TEXT("OS X");
	}

	virtual int32 GetProcessSnapshot( TArray<FTargetDeviceProcessInfo>& OutProcessInfos ) OVERRIDE
	{
		return 0;
	}

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

	virtual bool Launch( const FString& AppId, EBuildConfigurations::Type BuildConfiguration, EBuildTargets::Type Target, const FString& Params, uint32* OutProcessId ) OVERRIDE
	{
		// build executable path
		FString PlatformName = TEXT("Mac");
		FString ExecutableName = TEXT("UE4");
		if (BuildConfiguration != EBuildConfigurations::Development)
		{
			ExecutableName += FString::Printf(TEXT("-%s-%s"), *PlatformName, EBuildConfigurations::ToString(BuildConfiguration));
		}

		FString ExecutablePath = FPaths::EngineIntermediateDir() / TEXT("Devices") / PlatformName / TEXT("Engine") / TEXT("Binaries") / PlatformName / (ExecutableName + TEXT(".app/Contents/MacOS/") + ExecutableName);

		// launch the game
		FProcHandle ProcessHandle = FPlatformProcess::CreateProc(*ExecutablePath, *Params, true, false, false, OutProcessId, 0, NULL, NULL);
		return ProcessHandle.Close();
	}

	virtual bool PowerOff( bool Force ) OVERRIDE
	{
		return false;
	}

	virtual bool PowerOn( ) OVERRIDE
	{
		return false;
	}

	virtual bool Reboot( bool bReconnect = false ) OVERRIDE
	{
#if PLATFORM_MAC
		NSAppleScript* Script = [[NSAppleScript alloc] initWithSource:@"tell application \"System Events\" to restart"];
		NSDictionary* ErrorDict = [NSDictionary dictionary];
		[Script executeAndReturnError: &ErrorDict];
#endif
		return true;
	}

	virtual bool Run( const FString& ExecutablePath, const FString& Params, uint32* OutProcessId ) OVERRIDE
	{
		FProcHandle ProcessHandle = FPlatformProcess::CreateProc(*ExecutablePath, *Params, true, false, false, OutProcessId, 0, NULL, NULL);
		return ProcessHandle.Close();
	}

	virtual bool SupportsFeature( ETargetDeviceFeatures::Type Feature ) const OVERRIDE
	{
		switch (Feature)
		{
		case ETargetDeviceFeatures::MultiLaunch:
			return true;

		case ETargetDeviceFeatures::Reboot:
			return true;
		}

		return false;
	}

	virtual bool SupportsSdkVersion( const FString& VersionString ) const OVERRIDE
	{
		// @todo filter SDK versions
		return true;
	}

	virtual void SetUserCredentials( const FString & UserName, const FString & UserPassword ) OVERRIDE
	{
	}

	virtual bool GetUserCredentials( FString & OutUserName, FString & OutUserPassword ) OVERRIDE
	{
		return false;
	}

	virtual bool TerminateProcess( const int32 ProcessId ) OVERRIDE
	{
		return false;
	}


private:

	// Holds a reference to the device's target platform.
	const ITargetPlatform& TargetPlatform;
};
