// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "OnlineAsyncTaskGooglePlayShowLoginUI.h"
#include "OnlineSubsystemGooglePlay.h"
#include "OnlineIdentityInterfaceGooglePlay.h"

THIRD_PARTY_INCLUDES_START
#include "gpg/player_manager.h"
THIRD_PARTY_INCLUDES_END

FOnlineAsyncTaskGooglePlayShowLoginUI::FOnlineAsyncTaskGooglePlayShowLoginUI(
	FOnlineSubsystemGooglePlay* InSubsystem,
	int InPlayerId,
	const FOnLoginUIClosedDelegate& InDelegate)
	: FOnlineAsyncTaskGooglePlayAuthAction(InSubsystem)
	, PlayerId(InPlayerId)
	, Delegate(InDelegate)
{
}

void FOnlineAsyncTaskGooglePlayShowLoginUI::Start_OnTaskThread()
{	
	if(Subsystem->GetGameServices() == nullptr)
	{
		UE_LOG(LogOnline, Log, TEXT("FOnlineAsyncTaskGooglePlayShowLoginUI: GameServicesPtr is null."));
		bWasSuccessful = false;
		bIsComplete = true;
		return;
	}

	if(Subsystem->GetGameServices()->IsAuthorized())
	{
		UE_LOG(LogOnline, Log, TEXT("FOnlineAsyncTaskGooglePlayShowLoginUI: User authorized."));
		// User is already authorized, nothing to do.
		bWasSuccessful = true;
		Subsystem->GetGameServices()->Players().FetchSelf([this](gpg::PlayerManager::FetchSelfResponse const &response) { OnFetchSelfResponse(response); });
	}
	else
	{
		UE_LOG(LogOnline, Log, TEXT("FOnlineAsyncTaskGooglePlayShowLoginUI: User NOT authorized, start UI."));
		// The user isn't authorized, show the sign-in UI.
		Subsystem->GetGameServices()->StartAuthorizationUI();
	}
}

void FOnlineAsyncTaskGooglePlayShowLoginUI::Finalize()
{
	UE_LOG(LogOnline, Log, TEXT("FOnlineAsyncTaskGooglePlayShowLoginUI: Finalize."));
	// Async task manager owns the task and is responsible for cleaning it up.
	Subsystem->CurrentShowLoginUITask = nullptr;
}

void FOnlineAsyncTaskGooglePlayShowLoginUI::TriggerDelegates()
{
	UE_LOG(LogOnline, Log, TEXT("FOnlineAsyncTaskGooglePlayShowLoginUI: TriggerDelegates Success: %d."), bWasSuccessful);
	TSharedPtr<const FUniqueNetIdString> UserId = Subsystem->GetIdentityGooglePlay()->GetCurrentUserId();

	if (bWasSuccessful && !UserId.IsValid())
	{
		UserId = MakeShareable(new FUniqueNetIdString());
		Subsystem->GetIdentityGooglePlay()->SetCurrentUserId(UserId);
	}
	else if (!bWasSuccessful)
	{
		Subsystem->GetIdentityGooglePlay()->SetCurrentUserId(nullptr);
	}

	Delegate.ExecuteIfBound(UserId, PlayerId);
}

void FOnlineAsyncTaskGooglePlayShowLoginUI::ProcessGoogleClientConnectResult(bool bInSuccessful, FString AccessToken)
{
	UE_LOG(LogOnline, Display, TEXT("FOnlineAsyncTaskGooglePlayShowLoginUI::ProcessGoogleClientConnectResult %d %s"), bInSuccessful, *AccessToken);

	if (bInSuccessful)
	{
		Subsystem->GetIdentityGooglePlay()->SetAuthTokenFromGoogleConnectResponse(AccessToken);
	}
	else
	{
		Subsystem->GetIdentityGooglePlay()->SetAuthTokenFromGoogleConnectResponse(TEXT("NONE"));
	}

	bIsComplete = true;
}

void FOnlineAsyncTaskGooglePlayShowLoginUI::OnAuthActionFinished(gpg::AuthOperation InOp, gpg::AuthStatus InStatus)
{
	UE_LOG(LogOnline, Log, TEXT("FOnlineAsyncTaskGooglePlayShowLoginUI::OnAuthActionFinished %d %d"), (int32)InOp, (int32)InStatus);

	gpg::GameServices* GameServices = Subsystem->GetGameServices();
	const bool bIsAuthorized = GameServices ? GameServices->IsAuthorized() : false;
	UE_LOG(LogOnline, Warning, TEXT("FOnlineAsyncTaskGooglePlayShowLoginUI::Authorized %d"), bIsAuthorized);

	if (InOp == gpg::AuthOperation::SIGN_IN)
	{
		bWasSuccessful = (InStatus == gpg::AuthStatus::VALID);
		if (bWasSuccessful)
		{
			UE_LOG(LogOnline, Log, TEXT("FOnlineAsyncTaskGooglePlayShowLoginUI Fetching Self"));
			Subsystem->GetGameServices()->Players().FetchSelf([this](gpg::PlayerManager::FetchSelfResponse const &response) { OnFetchSelfResponse(response); });
		}
		else
		{
			UE_LOG(LogOnline, Log, TEXT("FOnlineAsyncTaskGooglePlayShowLoginUI Failure"));
			bIsComplete = true;
		}
	}
}

void FOnlineAsyncTaskGooglePlayShowLoginUI::OnFetchSelfResponse(const gpg::PlayerManager::FetchSelfResponse& SelfResponse)
{
	UE_LOG(LogOnline, Log, TEXT("FOnlineAsyncTaskGooglePlayShowLoginUI::OnFetchSelfResponse"));
	if(gpg::IsSuccess(SelfResponse.status))
	{
		UE_LOG(LogOnline, Log, TEXT("FOnlineAsyncTaskGooglePlayShowLoginUI FetchSelf success"));
		Subsystem->GetIdentityGooglePlay()->SetPlayerDataFromFetchSelfResponse(SelfResponse.data);

		extern void AndroidThunkCpp_GoogleClientConnect();
		AndroidThunkCpp_GoogleClientConnect();
		// bIsComplete set by response from googleClientConnect in ProcessGoogleClientConnectResult
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("FetchSelf Response Status Not Successful"));
		bIsComplete = true;
	}
}
