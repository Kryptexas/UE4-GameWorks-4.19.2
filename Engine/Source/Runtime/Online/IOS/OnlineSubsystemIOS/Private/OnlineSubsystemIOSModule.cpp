// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemIOSPrivatePCH.h"
#include "HttpModule.h"

IMPLEMENT_MODULE( FOnlineSubsystemIOSModule, OnlineSubsystemIOS );

void FOnlineSubsystemIOSModule::StartupModule()
{
	bool bSuccess = false;
	UE_LOG(LogOnline, Display, TEXT("FOnlineSubsystemIOSModule::StartupModule()"));

	FHttpModule::Get();

	// Create and register our singleton factory with the main online subsystem for easy access
	FOnlineSubsystemIOS* OnlineSubsystem = FOnlineSubsystemIOS::Create();
	if (OnlineSubsystem->IsEnabled())
	{
		if (OnlineSubsystem->Init())
		{
			UE_LOG(LogOnline, Warning, TEXT("FOnlineSubsystemIOSModule has initialized!"));
			FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
			OSS.RegisterPlatformService(IOS_SUBSYSTEM, OnlineSubsystem);
			bSuccess = true;
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("GameCenFOnlineSubsystemIOSModuleter failed to initialize!"));
		}
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("FOnlineSubsystemIOSModule was disabled"));
	}

	if (!bSuccess)
	{
		FOnlineSubsystemIOS::Destroy();
	}
}


void FOnlineSubsystemIOSModule::ShutdownModule()
{
	UE_LOG(LogOnline, Display, TEXT("FOnlineSubsystemIOSModule::ShutdownModule()"));

}