// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemUtilsPrivatePCH.h"

//////////////////////////////////////////////////////////////////////////
// UAchievementQueryCallbackProxy

UAchievementQueryCallbackProxy::UAchievementQueryCallbackProxy(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

UAchievementQueryCallbackProxy* UAchievementQueryCallbackProxy::CacheAchievements(class APlayerController* PlayerController)
{
	UAchievementQueryCallbackProxy* Proxy = NewObject<UAchievementQueryCallbackProxy>();
	Proxy->PlayerControllerWeakPtr = PlayerController;
	Proxy->bFetchDescriptions = false;
	return Proxy;
}

UAchievementQueryCallbackProxy* UAchievementQueryCallbackProxy::CacheAchievementDescriptions(class APlayerController* PlayerController)
{
	UAchievementQueryCallbackProxy* Proxy = NewObject<UAchievementQueryCallbackProxy>();
	Proxy->PlayerControllerWeakPtr = PlayerController;
	Proxy->bFetchDescriptions = true;
	return Proxy;
}

void UAchievementQueryCallbackProxy::Activate()
{
	FOnlineSubsystemBPCallHelper Helper(TEXT("CacheAchievements or CacheAchievementDescriptions"));
	Helper.QueryIDFromPlayerController(PlayerControllerWeakPtr.Get());

	if (Helper.IsValid())
	{
		IOnlineAchievementsPtr Achievements = Helper.OnlineSub->GetAchievementsInterface();
		if (Achievements.IsValid())
		{
			FOnQueryAchievementsCompleteDelegate QueryFinishedDelegate = FOnQueryAchievementsCompleteDelegate::CreateUObject(this, &ThisClass::OnQueryCompleted);
			
			if (bFetchDescriptions)
			{
				Achievements->QueryAchievementDescriptions(*Helper.UserID, QueryFinishedDelegate);
			}
			else
			{
				Achievements->QueryAchievements(*Helper.UserID, QueryFinishedDelegate);
			}

			// OnQueryCompleted will get called, nothing more to do now
			return;
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("Achievements not supported by Online Subsystem"), ELogVerbosity::Warning);
		}
	}

	// Fail immediately
	OnFailure.Broadcast();
}

void UAchievementQueryCallbackProxy::OnQueryCompleted(const FUniqueNetId& UserID, bool bSuccess)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast();
	}
	else
	{
		OnFailure.Broadcast();
	}
}
