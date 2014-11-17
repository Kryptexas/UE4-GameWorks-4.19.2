// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemUtilsPrivatePCH.h"

//////////////////////////////////////////////////////////////////////////
// UInAppPurchaseCallbackProxy

UInAppPurchaseCallbackProxy::UInAppPurchaseCallbackProxy(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PurchaseRequest = nullptr;
	WorldPtr = nullptr;
}


void UInAppPurchaseCallbackProxy::Trigger(APlayerController* PlayerController, const FString& ProductIdentifier)
{
	bFailedToEvenSubmit = true;

	WorldPtr = (PlayerController != nullptr) ? PlayerController->GetWorld() : nullptr;
	if (APlayerState* PlayerState = (PlayerController != NULL) ? PlayerController->PlayerState : nullptr)
	{
		if (IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get())
		{
			IOnlineStorePtr StoreInterface = OnlineSub->GetStoreInterface();
			if (StoreInterface.IsValid())
			{
				bFailedToEvenSubmit = false;

				// Register the completion callback
				InAppPurchaseCompleteDelegate = FOnInAppPurchaseCompleteDelegate::CreateUObject(this, &UInAppPurchaseCallbackProxy::OnInAppPurchaseComplete);
				StoreInterface->AddOnInAppPurchaseCompleteDelegate(InAppPurchaseCompleteDelegate);

				// Set-up, and trigger the transaction through the store interface
				PurchaseRequest = MakeShareable(new FOnlineInAppPurchaseTransaction());
				FOnlineInAppPurchaseTransactionRef PurchaseRequestRef = PurchaseRequest.ToSharedRef();
				StoreInterface->BeginPurchase(ProductIdentifier, PurchaseRequestRef);
			}
			else
			{
				FFrame::KismetExecutionMessage(TEXT("UInAppPurchaseCallbackProxy::Trigger - In-App Purchases are not supported by Online Subsystem"), ELogVerbosity::Warning);
			}
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("UInAppPurchaseCallbackProxy::Trigger - Invalid or uninitialized OnlineSubsystem"), ELogVerbosity::Warning);
		}
	}
	else
	{
		FFrame::KismetExecutionMessage(TEXT("UInAppPurchaseCallbackProxy::Trigger - Invalid player state"), ELogVerbosity::Warning);
	}

	if (bFailedToEvenSubmit && (PlayerController != NULL))
	{
		OnInAppPurchaseComplete(EInAppPurchaseState::Failed);
	}
}


void UInAppPurchaseCallbackProxy::OnInAppPurchaseComplete(EInAppPurchaseState::Type CompletionState)
{
	RemoveDelegate();

	SavedPurchaseState = CompletionState;

	if (SavedPurchaseState == EInAppPurchaseState::Success && PurchaseRequest.IsValid())
	{
		SavedProductInformation = PurchaseRequest->ProvidedProductInformation;
	}

	if (UWorld* World = WorldPtr.Get())
	{
		World->GetTimerManager().SetTimer(this, &UInAppPurchaseCallbackProxy::OnInAppPurchaseComplete_Delayed, 0.001f, false);
	}

	PurchaseRequest = NULL;
}


void UInAppPurchaseCallbackProxy::OnInAppPurchaseComplete_Delayed()
{
	if (SavedPurchaseState == EInAppPurchaseState::Success)
	{
		OnSuccess.Broadcast(SavedPurchaseState, SavedProductInformation);
	}
	else
	{
		OnFailure.Broadcast(SavedPurchaseState, SavedProductInformation);
	}
}


void UInAppPurchaseCallbackProxy::RemoveDelegate()
{
	if (!bFailedToEvenSubmit)
	{
		if (IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get())
		{
			IOnlineStorePtr InAppPurchases = OnlineSub->GetStoreInterface();
			if (InAppPurchases.IsValid())
			{
				InAppPurchases->ClearOnInAppPurchaseCompleteDelegate(InAppPurchaseCompleteDelegate);
			}
		}
	}
}


void UInAppPurchaseCallbackProxy::BeginDestroy()
{
	PurchaseRequest = nullptr;
	RemoveDelegate();

	Super::BeginDestroy();
}


UInAppPurchaseCallbackProxy* UInAppPurchaseCallbackProxy::CreateProxyObjectForInAppPurchase(class APlayerController* PlayerController, const FString& ProductIdentifier)
{
	UInAppPurchaseCallbackProxy* Proxy = NewObject<UInAppPurchaseCallbackProxy>();
	Proxy->Trigger(PlayerController, ProductIdentifier);
	return Proxy;
}
