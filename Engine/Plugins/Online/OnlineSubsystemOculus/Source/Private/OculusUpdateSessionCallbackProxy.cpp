// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "OculusUpdateSessionCallbackProxy.h"
#include "OnlineSubsystemOculusPrivate.h"
#include "CoreOnline.h"
#include "Online.h"
#include "OnlineSessionInterfaceOculus.h"

UOculusUpdateSessionCallbackProxy::UOculusUpdateSessionCallbackProxy(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
	, UpdateCompleteDelegate(FOnUpdateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnUpdateCompleted))
	, bShouldEnqueueInMatchmakingPool(false)
{
}

UOculusUpdateSessionCallbackProxy* UOculusUpdateSessionCallbackProxy::SetSessionEnqueue(bool bShouldEnqueueInMatchmakingPool)
{
	UOculusUpdateSessionCallbackProxy* Proxy = NewObject<UOculusUpdateSessionCallbackProxy>();
	Proxy->SetFlags(RF_StrongRefOnFrame);
	Proxy->bShouldEnqueueInMatchmakingPool = bShouldEnqueueInMatchmakingPool;
	return Proxy;
}

void UOculusUpdateSessionCallbackProxy::Activate()
{
	auto OculusSessionInterface = Online::GetSessionInterface(OCULUS_SUBSYSTEM);

	if (OculusSessionInterface.IsValid())
	{
		UpdateCompleteDelegateHandle = OculusSessionInterface->AddOnUpdateSessionCompleteDelegate_Handle(UpdateCompleteDelegate);

		FOnlineSessionSettings Settings;
		Settings.bShouldAdvertise = bShouldEnqueueInMatchmakingPool;
		OculusSessionInterface->UpdateSession(GameSessionName, Settings);
	}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("Oculus platform service not available. Skipping UpdateSession."));
		OnFailure.Broadcast();
	}
}

void UOculusUpdateSessionCallbackProxy::OnUpdateCompleted(FName SessionName, bool bWasSuccessful)
{
	auto OculusSessionInterface = Online::GetSessionInterface(OCULUS_SUBSYSTEM);

	if (OculusSessionInterface.IsValid())
	{
		OculusSessionInterface->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateCompleteDelegateHandle);
	}

	if (bWasSuccessful)
	{
		OnSuccess.Broadcast();
	}
	else
	{
		OnFailure.Broadcast();
	}

}
