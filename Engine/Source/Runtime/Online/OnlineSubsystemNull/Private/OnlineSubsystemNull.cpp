// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemNullPrivatePCH.h"
#include "OnlineSubsystemNull.h"
#include "OnlineAsyncTaskManagerNull.h"
#include "ModuleManager.h"
#include "Engine.h"

#include "OnlineSessionInterfaceNull.h"
#include "OnlineLeaderboardInterfaceNull.h"
#include "OnlineIdentityNull.h"
#include "OnlineAchievementsInterfaceNull.h"

FOnlineSubsystemNull* FOnlineSubsystemNull::NullSingleton = NULL;

// FOnlineSubsystemNullModule

IMPLEMENT_MODULE(FOnlineSubsystemNullModule, OnlineSubsystemNull);

/**
 * Called right after the module DLL has been loaded and the module object has been created
 * Registers the actual implementation of the Null online subsystem with the engine
 */
void FOnlineSubsystemNullModule::StartupModule()
{
	bool bSuccess = false;
	// Create and register our singleton factory with the main online subsystem for easy access
	FOnlineSubsystemNull* OnlineSubsystem = FOnlineSubsystemNull::Create();
	if (OnlineSubsystem->IsEnabled())
	{
		if (OnlineSubsystem->Init())
		{
			FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
			OSS.RegisterPlatformService(NULL_SUBSYSTEM, OnlineSubsystem);
			bSuccess = true;
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("Null API failed to initialize!"));
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("Null API disabled!"));
	}

	if (!bSuccess)
	{
		FOnlineSubsystemNull::Destroy();
	}
}

/**
 * Called before the module is unloaded, right before the module object is destroyed.
 * Overloaded to shut down all loaded online subsystems
 */
void FOnlineSubsystemNullModule::ShutdownModule()
{
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.UnregisterPlatformService(NULL_SUBSYSTEM);
	FOnlineSubsystemNull::Destroy();
}

IOnlineSessionPtr FOnlineSubsystemNull::GetSessionInterface() const
{
	return SessionInterface;
}

IOnlineFriendsPtr FOnlineSubsystemNull::GetFriendsInterface() const
{
	//return FriendInterface;
	return NULL;
}

IOnlineSharedCloudPtr FOnlineSubsystemNull::GetSharedCloudInterface() const
{
	return NULL;
}

IOnlineUserCloudPtr FOnlineSubsystemNull::GetUserCloudInterface() const
{
	//return UserCloudInterface;
	return NULL;
}

IOnlineEntitlementsPtr FOnlineSubsystemNull::GetEntitlementsInterface() const
{
	return NULL;
};

IOnlineLeaderboardsPtr FOnlineSubsystemNull::GetLeaderboardsInterface() const
{
	return LeaderboardsInterface;
}

IOnlineVoicePtr FOnlineSubsystemNull::GetVoiceInterface() const
{
	//return VoiceInterface;
	return NULL;
}

IOnlineExternalUIPtr FOnlineSubsystemNull::GetExternalUIInterface() const
{
	//return ExternalUIInterface;
	return NULL;
}

IOnlineTimePtr FOnlineSubsystemNull::GetTimeInterface() const
{
	return NULL;
}

IOnlineIdentityPtr FOnlineSubsystemNull::GetIdentityInterface() const
{
	return IdentityInterface;
}

IOnlineTitleFilePtr FOnlineSubsystemNull::GetTitleFileInterface() const
{
	return NULL;
}

IOnlineStorePtr FOnlineSubsystemNull::GetStoreInterface() const
{
	return NULL;
}

IOnlineEventsPtr FOnlineSubsystemNull::GetEventsInterface() const
{
	return NULL;
}

IOnlineAchievementsPtr FOnlineSubsystemNull::GetAchievementsInterface() const
{
	return AchievementsInterface;
}

IOnlineSharingPtr FOnlineSubsystemNull::GetSharingInterface() const
{
	return NULL;
}

IOnlineUserPtr FOnlineSubsystemNull::GetUserInterface() const
{
	return NULL;
}

IOnlineMessagePtr FOnlineSubsystemNull::GetMessageInterface() const
{
	return NULL;
}

IOnlinePresencePtr FOnlineSubsystemNull::GetPresenceInterface() const
{
	return NULL;
}

bool FOnlineSubsystemNull::Tick(float DeltaTime)
{
	if (OnlineAsyncTaskThreadRunnable)
	{
		OnlineAsyncTaskThreadRunnable->GameTick();
	}

 	if (SessionInterface.IsValid())
 	{
 		SessionInterface->Tick(DeltaTime);
 	}

	return true;
}

bool FOnlineSubsystemNull::Init()
{
	const bool bNullInit = true;
	
	if (bNullInit)
	{
		// Create the online async task thread
		OnlineAsyncTaskThreadRunnable = new FOnlineAsyncTaskManagerNull(this);
		check(OnlineAsyncTaskThreadRunnable);
		OnlineAsyncTaskThread = FRunnableThread::Create(OnlineAsyncTaskThreadRunnable, TEXT("OnlineAsyncTaskThreadNull"), 0, 0, 128 * 1024, TPri_Normal);
		check(OnlineAsyncTaskThread);
		UE_LOG_ONLINE(Verbose, TEXT("Created thread (ID:%d)."), OnlineAsyncTaskThread->GetThreadID());

 		SessionInterface = MakeShareable(new FOnlineSessionNull(this));
		LeaderboardsInterface = MakeShareable(new FOnlineLeaderboardsNull(this));
		IdentityInterface = MakeShareable(new FOnlineIdentityNull(this));
		AchievementsInterface = MakeShareable(new FOnlineAchievementsNull(this));
	}
	else
	{
		Shutdown();
	}

	return bNullInit;
}

bool FOnlineSubsystemNull::Shutdown()
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSubsystemNull::Shutdown()"));

	if (OnlineAsyncTaskThread)
	{
		// Destroy the online async task thread
		delete OnlineAsyncTaskThread;
		OnlineAsyncTaskThread = NULL;
	}

	if (OnlineAsyncTaskThreadRunnable)
	{
		delete OnlineAsyncTaskThreadRunnable;
		OnlineAsyncTaskThreadRunnable = NULL;
	}
	
 	#define DESTRUCT_INTERFACE(Interface) \
 	if (Interface.IsValid()) \
 	{ \
 		ensure(Interface.IsUnique()); \
 		Interface = NULL; \
 	}
 
 	// Destruct the interfaces
	DESTRUCT_INTERFACE(AchievementsInterface);
	DESTRUCT_INTERFACE(IdentityInterface);
	DESTRUCT_INTERFACE(LeaderboardsInterface);
 	DESTRUCT_INTERFACE(SessionInterface);

	#undef DESTRUCT_INTERFACE
	
	return true;
}

FString FOnlineSubsystemNull::GetAppId() const
{
	return TEXT("");
}

bool FOnlineSubsystemNull::Exec(const TCHAR* Cmd, FOutputDevice& Ar)
{
	return false;
}

FOnlineSubsystemNull* FOnlineSubsystemNull::Create()
{
	if (NullSingleton == NULL)
	{
		NullSingleton = new FOnlineSubsystemNull();
	}

	return NullSingleton;
}

void FOnlineSubsystemNull::Destroy()
{
	if (NullSingleton != NULL)
	{
		NullSingleton->Shutdown();
		delete NullSingleton;
		NullSingleton = NULL;
	}
}

bool FOnlineSubsystemNull::IsEnabled()
{
	return true;
}
