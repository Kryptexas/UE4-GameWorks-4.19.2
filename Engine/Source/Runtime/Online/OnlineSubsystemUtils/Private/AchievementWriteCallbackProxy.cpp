// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemUtilsPrivatePCH.h"

//////////////////////////////////////////////////////////////////////////
// ULeaderboardQueryCallbackProxy

UAchievementWriteCallbackProxy::UAchievementWriteCallbackProxy(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

UAchievementWriteCallbackProxy* UAchievementWriteCallbackProxy::WriteAchievementProgress(class APlayerController* PlayerController, FName AchievementName, float Progress)
{
	UAchievementWriteCallbackProxy* Proxy = NewObject<UAchievementWriteCallbackProxy>();

	Proxy->WriteObject = MakeShareable(new FOnlineAchievementsWrite);
	Proxy->WriteObject->SetFloatStat(AchievementName, Progress);
	Proxy->PlayerControllerWeakPtr = PlayerController;

	return Proxy;
}

void UAchievementWriteCallbackProxy::Activate()
{
	FOnlineSubsystemBPCallHelper Helper(TEXT("WriteAchievementObject"));
	Helper.QueryIDFromPlayerController(PlayerControllerWeakPtr.Get());

	if (Helper.IsValid())
	{
		IOnlineAchievementsPtr Achievements = Helper.OnlineSub->GetAchievementsInterface();
		if (Achievements.IsValid())
		{
			FOnlineAchievementsWriteRef WriteObjectRef = WriteObject.ToSharedRef();
			FOnAchievementsWrittenDelegate WriteFinishedDelegate = FOnAchievementsWrittenDelegate::CreateUObject(this, &ThisClass::OnAchievementWritten);
			Achievements->WriteAchievements(*Helper.UserID, WriteObjectRef, WriteFinishedDelegate);

			// OnAchievementWritten will get called, nothing more to do
			return;
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("WriteAchievementObject - Achievements not supported by Online Subsystem"), ELogVerbosity::Warning);
		}
	}

	// Fail immediately
	OnFailure.Broadcast();
	WriteObject.Reset();
}

void UAchievementWriteCallbackProxy::OnAchievementWritten(const FUniqueNetId& UserID, bool bSuccess)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast();
	}
	else
	{
		OnFailure.Broadcast();
	}

	WriteObject.Reset();
}

void UAchievementWriteCallbackProxy::BeginDestroy()
{
	WriteObject.Reset();

	Super::BeginDestroy();
}
