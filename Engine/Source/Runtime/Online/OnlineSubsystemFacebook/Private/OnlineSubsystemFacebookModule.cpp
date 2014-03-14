// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemFacebookPrivatePCH.h"

// FOnlineSubsystemFacebookModule

IMPLEMENT_MODULE(FOnlineSubsystemFacebookModule, OnlineSubsystemFacebook);

void FOnlineSubsystemFacebookModule::StartupModule()
{
	UE_LOG(LogOnline, Log, TEXT("Facebook Startup!"));

	bool bSuccess = false;
	// Create and register our singleton factory with the main online subsystem for easy access
	FOnlineSubsystemFacebook* OnlineSubsystem = FOnlineSubsystemFacebook::Create();
	if (OnlineSubsystem->IsEnabled())
	{
		if (OnlineSubsystem->Init())
		{
			FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
			OSS.RegisterPlatformService(FACEBOOK_SUBSYSTEM, OnlineSubsystem);
			bSuccess = true;
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("Facebook API failed to initialize!"));
		}
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("Facebook API disabled!"));
	}

	if (!bSuccess)
	{
		FOnlineSubsystemFacebook::Destroy();
	}
}

void FOnlineSubsystemFacebookModule::ShutdownModule()
{
	UE_LOG(LogOnline, Log, TEXT("Facebook Shutdown!"));

	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.UnregisterPlatformService(FACEBOOK_SUBSYSTEM);
	FOnlineSubsystemFacebook::Destroy();
}