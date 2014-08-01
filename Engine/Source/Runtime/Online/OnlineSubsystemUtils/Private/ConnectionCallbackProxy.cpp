// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemUtilsPrivatePCH.h"

//////////////////////////////////////////////////////////////////////////
// UConnectionCallbackProxy

UConnectionCallbackProxy::UConnectionCallbackProxy(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

	OnLoginCompleteDelegate.BindUObject(this, &ThisClass::OnLoginCompleted);

}

UConnectionCallbackProxy* UConnectionCallbackProxy::ConnectToService(class APlayerController* PlayerController)
{
	UConnectionCallbackProxy* Proxy = NewObject<UConnectionCallbackProxy>();
	Proxy->PlayerControllerWeakPtr = PlayerController;
	return Proxy;
}

void UConnectionCallbackProxy::Activate()
{
	FOnlineSubsystemBPCallHelper Helper(TEXT("ConnectToService"));
	Helper.QueryIDFromPlayerController(PlayerControllerWeakPtr.Get());

	if (Helper.IsValid())
	{
		IOnlineIdentityPtr OnlineIdentity = Helper.OnlineSub->GetIdentityInterface();
		if (OnlineIdentity.IsValid())
		{
			ULocalPlayer * localPlayer = CastChecked<ULocalPlayer>(PlayerControllerWeakPtr.Get()->Player);

			if (!OnlineIdentity->OnLoginCompleteDelegates[localPlayer->ControllerId].IsBoundToObject(this))
			{
				OnlineIdentity->AddOnLoginCompleteDelegate(localPlayer->ControllerId, OnLoginCompleteDelegate);
				OnlineIdentity->Login(localPlayer->ControllerId, FOnlineAccountCredentials()); /// Probably need to supply real creds here somehow... doesn't apply for all imple however.
			}
			else
			{
				// already trying to login!
			}
			
			// OnQueryCompleted will get called, nothing more to do now
			return;
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("Connection control not supported by online subsystem"), ELogVerbosity::Warning);
		}
	}

	// Fail immediately
	OnFailure.Broadcast(0);
}

void UConnectionCallbackProxy::OnLoginCompleted(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	FOnlineSubsystemBPCallHelper Helper(TEXT("ConnectToService"));
	Helper.QueryIDFromPlayerController(PlayerControllerWeakPtr.Get());

	if (Helper.IsValid())
	{
		IOnlineIdentityPtr OnlineIdentity = Helper.OnlineSub->GetIdentityInterface();
		if (OnlineIdentity.IsValid())
		{
			OnlineIdentity->ClearOnLoginCompleteDelegate(LocalUserNum, OnLoginCompleteDelegate);
		}
	}

	if (bWasSuccessful)
	{
		OnSuccess.Broadcast(0);
	}
	else
	{
		OnFailure.Broadcast(0);
	}
}
