// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//Google Play Services

#include "OnlineSubsystemGooglePlayPrivatePCH.h"
#include "AndroidApplication.h"

FOnlineSubsystemGooglePlay::FOnlineSubsystemGooglePlay()
	: OnlineAsyncTaskThread(NULL)
	, IdentityInterface(NULL)
	, AchievementsInterface(NULL)
{

}

IOnlineIdentityPtr FOnlineSubsystemGooglePlay::GetIdentityInterface() const
{
	return IdentityInterface;
}


IOnlineStorePtr FOnlineSubsystemGooglePlay::GetStoreInterface() const
{
	return nullptr;
}


IOnlineSessionPtr FOnlineSubsystemGooglePlay::GetSessionInterface() const
{
	return nullptr;
}


IOnlineFriendsPtr FOnlineSubsystemGooglePlay::GetFriendsInterface() const
{
	return nullptr;
}


IOnlineSharedCloudPtr FOnlineSubsystemGooglePlay::GetSharedCloudInterface() const
{
	return nullptr;
}


IOnlineUserCloudPtr FOnlineSubsystemGooglePlay::GetUserCloudInterface() const
{
	return nullptr;
}

IOnlineLeaderboardsPtr FOnlineSubsystemGooglePlay::GetLeaderboardsInterface() const
{
	return nullptr;
}


IOnlineVoicePtr FOnlineSubsystemGooglePlay::GetVoiceInterface() const
{
	return nullptr;
}


IOnlineExternalUIPtr FOnlineSubsystemGooglePlay::GetExternalUIInterface() const
{
	return nullptr;
}


IOnlineTimePtr FOnlineSubsystemGooglePlay::GetTimeInterface() const
{
	return nullptr;
}



IOnlineTitleFilePtr FOnlineSubsystemGooglePlay::GetTitleFileInterface() const
{
	return nullptr;
}


IOnlineEntitlementsPtr FOnlineSubsystemGooglePlay::GetEntitlementsInterface() const
{
	return nullptr;
}

IOnlineAchievementsPtr FOnlineSubsystemGooglePlay::GetAchievementsInterface() const
{
	return AchievementsInterface;
}

bool FOnlineSubsystemGooglePlay::Init() 
{
	FPlatformMisc::LowLevelOutputDebugStringf(TEXT("FOnlineSubsystemAndroid::Init"));
	
	IdentityInterface = MakeShareable(new FOnlineIdentityGooglePlay(this));
	AchievementsInterface = MakeShareable(new FOnlineAchievementsGooglePlay(this));

	extern void AndroidThunkCpp_GooglePlayConnect();
	AndroidThunkCpp_GooglePlayConnect();

	return true;
}

bool FOnlineSubsystemGooglePlay::Tick(float DeltaTime)
{
	return true;
}


bool FOnlineSubsystemGooglePlay::Shutdown() 
{
	UE_LOG(LogOnline, Log, TEXT("FOnlineSubsystemAndroid::Shutdown()"));

#define DESTRUCT_INTERFACE(Interface) \
	if (Interface.IsValid()) \
	{ \
	ensure(Interface.IsUnique()); \
	Interface = NULL; \
	}

	// Destruct the interfaces
	DESTRUCT_INTERFACE(AchievementsInterface);
	DESTRUCT_INTERFACE(IdentityInterface);
#undef DESTRUCT_INTERFACE

	return true;
}


FString FOnlineSubsystemGooglePlay::GetAppId() const 
{
	//get app id from settings. 
	return TEXT( "AndroidAppIDPlaceHolder" );
}


bool FOnlineSubsystemGooglePlay::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) 
{
	return false;
}

bool FOnlineSubsystemGooglePlay::IsEnabled(void)
{
	bool bEnabled = true;
	GConfig->GetBool(TEXT("/Script/AndroidRuntimeSettings.AndroidRuntimeSettings"), TEXT("bEnableGooglePlaySupport"), bEnabled, GEngineIni);
	return bEnabled;
}
