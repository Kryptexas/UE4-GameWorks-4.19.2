// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemUtilsPrivatePCH.h"

//////////////////////////////////////////////////////////////////////////
// UConnectionCallbackProxy

UConnectionCallbackProxy::UConnectionCallbackProxy(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
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
		IOnlineConnectionPtr Connection = Helper.OnlineSub->GetConnectionInterface();
		if (Connection.IsValid())
		{
			FOnConnectionCompleteDelegate ConnectionFinishedDelegate = FOnConnectionCompleteDelegate::CreateUObject(this, &ThisClass::OnConnectCompleted);

			Connection->Connect(ConnectionFinishedDelegate); // this will get called when everything is resolved

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

void UConnectionCallbackProxy::OnConnectCompleted(int errorCode, bool bSuccess)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(0);
	}
	else
	{
		OnFailure.Broadcast(0);
	}
}
