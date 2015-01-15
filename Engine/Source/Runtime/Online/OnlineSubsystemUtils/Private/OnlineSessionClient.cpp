// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	OnlineSessionClient.cpp: Online session related implementations 
	(creating/joining/leaving/destroying sessions)
=============================================================================*/

#include "OnlineSubsystemUtilsPrivatePCH.h"

#include "Engine/GameInstance.h"

UOnlineSessionClient::UOnlineSessionClient(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bHandlingDisconnect = false;
	bIsFromInvite = false;
}

UWorld* UOnlineSessionClient::GetWorld() const
{
	ULocalPlayer* LP = Cast<ULocalPlayer>(GetOuter());
	if (LP != nullptr && LP->PlayerController != nullptr)
	{
		return LP->PlayerController->GetWorld();
	}

	return nullptr;
}

APlayerController* UOnlineSessionClient::GetPlayerController()
{
	ULocalPlayer* LP = Cast<ULocalPlayer>(GetOuter());
	if (LP != nullptr)
	{
		return LP->PlayerController;
	}

	return nullptr;
}

int32 UOnlineSessionClient::GetControllerId()
{
	ULocalPlayer* LP = Cast<ULocalPlayer>(GetOuter());
	if (LP != nullptr)
	{
		return LP->GetControllerId();
	}

	return INVALID_CONTROLLERID;
}

TSharedPtr<FUniqueNetId> UOnlineSessionClient::GetUniqueId()
{
	ULocalPlayer* LP = Cast<ULocalPlayer>(GetOuter());
	if (LP != nullptr)
	{
		return LP->GetCachedUniqueNetId();
	}

	return nullptr;
}

void UOnlineSessionClient::RegisterOnlineDelegates(UWorld* InWorld)
{
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(InWorld);
	if (OnlineSub)
	{
		SessionInt = OnlineSub->GetSessionInterface();
		if (SessionInt.IsValid())
		{
			int32 ControllerId = GetControllerId();
			if (ControllerId != INVALID_CONTROLLERID)
			{
				// Always on the lookout for invite acceptance (via actual invite or join from external ui)
				OnSessionInviteAcceptedDelegate       = FOnSessionInviteAcceptedDelegate::CreateUObject(this, &UOnlineSessionClient::OnSessionInviteAccepted);
				OnSessionInviteAcceptedDelegateHandle = SessionInt->AddOnSessionInviteAcceptedDelegate_Handle(ControllerId, OnSessionInviteAcceptedDelegate);
			}
		}

		OnJoinSessionCompleteDelegate           = FOnJoinSessionCompleteDelegate   ::CreateUObject(this, &UOnlineSessionClient::OnJoinSessionComplete);
		OnEndForJoinSessionCompleteDelegate     = FOnEndSessionCompleteDelegate    ::CreateUObject(this, &UOnlineSessionClient::OnEndForJoinSessionComplete);
		OnDestroyForJoinSessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &UOnlineSessionClient::OnDestroyForJoinSessionComplete);
	}

	// Register disconnect delegate even if we don't have an online system
	OnDestroyForMainMenuCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &UOnlineSessionClient::OnDestroyForMainMenuComplete);
}

void UOnlineSessionClient::ClearOnlineDelegates(UWorld* InWorld)
{
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(InWorld);
	if (OnlineSub)
	{
		if (SessionInt.IsValid())
		{
			int32 ControllerId = GetControllerId();
			if (ControllerId != INVALID_CONTROLLERID)
			{
				SessionInt->ClearOnSessionInviteAcceptedDelegate_Handle(ControllerId, OnSessionInviteAcceptedDelegateHandle);
			}
		}
	}

	SessionInt = NULL;
}

void UOnlineSessionClient::OnSessionInviteAccepted(int32 LocalUserNum, bool bWasSuccessful, const FOnlineSessionSearchResult& SearchResult)
{
	UE_LOG(LogOnline, Verbose, TEXT("OnSessionInviteAccepted LocalUserNum: %d bSuccess: %d"), LocalUserNum, bWasSuccessful);
	// Don't clear invite accept delegate

	if (bWasSuccessful)
	{
		if (SearchResult.IsValid())
		{
			bIsFromInvite = true;
			check(GetControllerId() == LocalUserNum);
			JoinSession(LocalUserNum, GameSessionName, SearchResult);
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("Invite accept returned no search result."));
		}
	}
}

void UOnlineSessionClient::OnEndForJoinSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogOnline, Verbose, TEXT("OnEndForJoinSessionComplete %s bSuccess: %d"), *SessionName.ToString(), bWasSuccessful);
	SessionInt->ClearOnEndSessionCompleteDelegate_Handle(OnEndForJoinSessionCompleteDelegateHandle);
	DestroyExistingSession_Impl(OnDestroyForJoinSessionCompleteDelegateHandle, SessionName, OnDestroyForJoinSessionCompleteDelegate);
}

void UOnlineSessionClient::EndExistingSession(FName SessionName, FOnEndSessionCompleteDelegate& Delegate)
{
	EndExistingSession_Impl(SessionName, Delegate);
}

/**
 * Implementation of EndExistingSession
 *
 * @param SessionName name of session to end
 * @param Delegate delegate to call at session end
 * @return Handle to the added delegate.
 */
FDelegateHandle UOnlineSessionClient::EndExistingSession_Impl(FName SessionName, FOnEndSessionCompleteDelegate& Delegate)
{
	FDelegateHandle Result;

	if (SessionInt.IsValid())
	{
		Result = SessionInt->AddOnEndSessionCompleteDelegate_Handle(Delegate);
		SessionInt->EndSession(SessionName);
	}
	else
	{
		Delegate.ExecuteIfBound(SessionName, true);
	}

	return Result;
}

void UOnlineSessionClient::OnDestroyForJoinSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogOnline, Verbose, TEXT("OnDestroyForJoinSessionComplete %s bSuccess: %d"), *SessionName.ToString(), bWasSuccessful);
	if (SessionInt.IsValid())
	{
		SessionInt->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroyForJoinSessionCompleteDelegateHandle);
	}

	if (bWasSuccessful)
	{
		int32 ControllerId = GetControllerId();
		if (ControllerId != INVALID_CONTROLLERID)
		{
			JoinSession(ControllerId, SessionName, CachedSessionResult);
		}
	}

	bHandlingDisconnect = false;
}

void UOnlineSessionClient::OnDestroyForMainMenuComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogOnline, Verbose, TEXT("OnDestroyForMainMenuComplete %s bSuccess: %d"), *SessionName.ToString(), bWasSuccessful);
	if (SessionInt.IsValid())
	{
		SessionInt->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroyForMainMenuCompleteDelegateHandle);
	}	

	APlayerController* PC = GetPlayerController();
	if (PC)
	{
		// Call disconnect to force us back to the menu level
		GEngine->HandleDisconnect(PC->GetWorld(), PC->GetWorld()->GetNetDriver());
	}

	bHandlingDisconnect = false;
}

void UOnlineSessionClient::DestroyExistingSession(FName SessionName, FOnDestroySessionCompleteDelegate& Delegate)
{
	FDelegateHandle UnusedHandle;
	DestroyExistingSession_Impl(UnusedHandle, SessionName, Delegate);
}

/**
 * Implementation of DestroyExistingSession
 *
 * @param OutResult Handle to the added delegate.
 * @param SessionName name of session to destroy
 * @param Delegate delegate to call at session destruction
 */
void UOnlineSessionClient::DestroyExistingSession_Impl(FDelegateHandle& OutResult, FName SessionName, FOnDestroySessionCompleteDelegate& Delegate)
{
	if (SessionInt.IsValid())
	{
		OutResult = SessionInt->AddOnDestroySessionCompleteDelegate_Handle(Delegate);
		SessionInt->DestroySession(SessionName);
	}
	else
	{
		OutResult = FDelegateHandle();
		Delegate.ExecuteIfBound(SessionName, true);
	}
}

void UOnlineSessionClient::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG(LogOnline, Verbose, TEXT("OnJoinSessionComplete %s bSuccess: %d"), *SessionName.ToString(), static_cast<int32>(Result));
	SessionInt->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);

	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		FString URL;
		if (SessionInt->GetResolvedConnectString(SessionName, URL))
		{
			APlayerController* PC = GetPlayerController();
			if (PC)
			{
				if (bIsFromInvite)
				{
					URL += TEXT("?bIsFromInvite");
					bIsFromInvite = false;
				}
				PC->ClientTravel(URL, TRAVEL_Absolute);
			}
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("Failed to join session %s"), *SessionName.ToString());
		}
	}
}

void UOnlineSessionClient::JoinSession(int32 LocalUserNum, FName SessionName, const FOnlineSessionSearchResult& SearchResult)
{
	// Clean up existing sessions if applicable
	EOnlineSessionState::Type SessionState = SessionInt->GetSessionState(SessionName);
	if (SessionState != EOnlineSessionState::NoSession)
	{
		CachedSessionResult = SearchResult;
		OnEndForJoinSessionCompleteDelegateHandle = EndExistingSession_Impl(SessionName, OnEndForJoinSessionCompleteDelegate);
	}
	else
	{
		UGameInstance * const GameInstance = GetPlayerController()->GetGameInstance();
		GameInstance->JoinSession(static_cast<ULocalPlayer*>(GetPlayerController()->Player), SearchResult);
		/*OnJoinSessionCompleteDelegateHandle = SessionInt->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);
		SessionInt->JoinSession(LocalUserNum, SessionName, SearchResult);*/
	}
}

void UOnlineSessionClient::HandleDisconnect(UWorld *World, UNetDriver *NetDriver)
{
	bool bWasHandled = HandleDisconnectInternal(World, NetDriver);
	
	if (!bWasHandled)
	{
		// This may have been a pending net game that failed, let the engine handle it (dont tear our stuff down)
		// (Would it be better to return true/false based on if we handled the disconnect or not? Let calling code call GEngine stuff
		GEngine->HandleDisconnect(World, NetDriver);
	}
}

bool UOnlineSessionClient::HandleDisconnectInternal(UWorld* World, UNetDriver* NetDriver)
{
	APlayerController* PC = GetPlayerController();
	if (PC)
	{
		// This was a disconnect for our active world, we will handle it
		if (PC->GetWorld() == World)
		{
			// Prevent multiple calls to this async flow
			if (!bHandlingDisconnect)
			{
				bHandlingDisconnect = true;
				DestroyExistingSession_Impl(OnDestroyForMainMenuCompleteDelegateHandle, GameSessionName, OnDestroyForMainMenuCompleteDelegate);
			}

			return true;
		}
	}

	return false;
}