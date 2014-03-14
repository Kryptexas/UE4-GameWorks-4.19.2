// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemAmazonPCH.h"
#include "OnlineSubsystemAmazon.h"
#include "OnlineSubsystemAmazonModule.h"
#include "OnlineIdentityAmazon.h"

// FOnlineSubsystemAmazonModule
IMPLEMENT_MODULE(FOnlineSubsystemAmazonModule, OnlineSubsystemAmazon);

void FOnlineSubsystemAmazonModule::StartupModule()
{
	UE_LOG(LogOnline, Log, TEXT("Amazon Startup!"));

	bool bSuccess = false;
	// Create and register our singleton factory with the main online subsystem for easy access
	FOnlineSubsystemAmazon* OnlineSubsystem = FOnlineSubsystemAmazon::Create();
	if (OnlineSubsystem->IsEnabled())
	{
		if (OnlineSubsystem->Init())
		{
			FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
			OSS.RegisterPlatformService(AMAZON_SUBSYSTEM, OnlineSubsystem);
			bSuccess = true;
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("Amazon API failed to initialize!"));
		}
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("Amazon API disabled!"));
	}

	if (!bSuccess)
	{
		FOnlineSubsystemAmazon::Destroy();
	}
}

void FOnlineSubsystemAmazonModule::ShutdownModule()
{
	UE_LOG(LogOnline, Log, TEXT("Amazon Shutdown!"));

	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.UnregisterPlatformService(AMAZON_SUBSYSTEM);
	FOnlineSubsystemAmazon::Destroy();
}

IOnlineIdentityPtr FOnlineSubsystemAmazon::GetIdentityInterface() const
{
	return IdentityInterface;
}


bool FOnlineSubsystemAmazon::Tick(float DeltaTime)
{
	if (IdentityInterface.IsValid())
	{
		TickToggle ^= 1;
		IdentityInterface->Tick(DeltaTime, TickToggle);
	}
	return true;
}

bool FOnlineSubsystemAmazon::Init()
{
	IdentityInterface = MakeShareable(new FOnlineIdentityAmazon());
	return true;
}

bool FOnlineSubsystemAmazon::Shutdown()
{
	UE_LOG(LogOnline, Display, TEXT("FOnlineSubsystemAmazon::Shutdown()"));
	IdentityInterface = NULL;
	return true;
}

FString FOnlineSubsystemAmazon::GetAppId() const
{
	return TEXT("Amazon");
}

bool FOnlineSubsystemAmazon::Exec(const TCHAR* Cmd, FOutputDevice& Ar)
{
	return false;
}

FOnlineSubsystemAmazon* FOnlineSubsystemAmazon::AmazonSingleton = NULL;

FOnlineSubsystemAmazon* FOnlineSubsystemAmazon::Create()
{
	if (AmazonSingleton == NULL)
	{
		AmazonSingleton = new FOnlineSubsystemAmazon();
	}
	return AmazonSingleton;
}

void FOnlineSubsystemAmazon::Destroy()
{
	delete AmazonSingleton;
	AmazonSingleton = NULL;
}

bool FOnlineSubsystemAmazon::IsEnabled(void)
{
	// Check the ini for disabling Amazon
	bool bEnableAmazon = true;
	if (GConfig->GetBool(TEXT("OnlineSubsystemAmazon"), TEXT("bEnabled"), bEnableAmazon, GEngineIni) && 
		bEnableAmazon)
	{
		// Check the commandline for disabling Amazon
		bEnableAmazon = !FParse::Param(FCommandLine::Get(),TEXT("NOAMAZON"));
#if UE_EDITOR
		if (bEnableAmazon)
		{
			bEnableAmazon = IsRunningDedicatedServer() || IsRunningGame();
		}
#endif
	}
	return bEnableAmazon;
}

FOnlineSubsystemAmazon::FOnlineSubsystemAmazon() :
	IdentityInterface(NULL),
	TickToggle(0)
{

}

FOnlineSubsystemAmazon::~FOnlineSubsystemAmazon()
{

}
