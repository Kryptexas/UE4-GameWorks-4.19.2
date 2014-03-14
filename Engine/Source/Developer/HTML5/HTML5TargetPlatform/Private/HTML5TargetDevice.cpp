// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "HTML5TargetPlatformPrivatePCH.h"

/* ITargetDevice interface
 *****************************************************************************/

bool FHTML5TargetDevice::Connect( )
{
	return false;
}


bool FHTML5TargetDevice::Deploy( const FString& SourceFolder, FString& OutAppId )
{
	return false;
}


void FHTML5TargetDevice::Disconnect( )
{
}


FTargetDeviceId FHTML5TargetDevice::GetId( ) const
{
	return FTargetDeviceId(TargetPlatform.PlatformName(), GetName());
}


FString FHTML5TargetDevice::GetName( ) const
{
	return FPlatformProcess::ComputerName();
}


FString FHTML5TargetDevice::GetOperatingSystemName( )
{
	return TEXT("HTML5 Browser");
}


int32 FHTML5TargetDevice::GetProcessSnapshot( TArray<FTargetDeviceProcessInfo>& OutProcessInfos ) 
{
	return OutProcessInfos.Num();
}


bool FHTML5TargetDevice::Launch( const FString& AppId, EBuildConfigurations::Type BuildConfiguration, EBuildTargets::Type BuildTarget, const FString& Params, uint32* OutProcessId )
{
	return Run(AppId, Params, OutProcessId);
}


bool FHTML5TargetDevice::PowerOff( bool Force )
{
	return false;
}


bool FHTML5TargetDevice::PowerOn( )
{
	return false;
}



bool FHTML5TargetDevice::Reboot( bool bReconnect )
{
	return false;
}


bool FHTML5TargetDevice::Run( const FString& ExecutablePath, const FString& Params, uint32* OutProcessId )
{
	return false;
}


bool FHTML5TargetDevice::SupportsFeature( ETargetDeviceFeatures::Type Feature ) const
{
	switch (Feature)
	{
	case ETargetDeviceFeatures::MultiLaunch:
		return true;

		// @todo gmp: implement turning off remote PCs
	case ETargetDeviceFeatures::PowerOff:
		return false;

		// @todo gmp: implement turning on remote PCs (wake on LAN)
	case ETargetDeviceFeatures::PowerOn:
		return false;

	case ETargetDeviceFeatures::Reboot:
		return false;
	}

	return false;
}


bool FHTML5TargetDevice::SupportsSdkVersion( const FString& VersionString ) const
{
	// @todo filter SDK versions

	return true;
}

void FHTML5TargetDevice::SetUserCredentials( const FString & UserName, const FString & UserPassword )
{
}

bool FHTML5TargetDevice::GetUserCredentials( FString & OutUserName, FString & OutUserPassword )
{
	return false;
}

bool FHTML5TargetDevice::TerminateProcess( const int32 ProcessId )
{
	return false;
}

