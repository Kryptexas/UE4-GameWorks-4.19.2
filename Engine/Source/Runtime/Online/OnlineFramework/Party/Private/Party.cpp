// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PartyPrivatePCH.h"
#include "Party.h"
#include "PartyGameState.h"
#include "Engine/GameInstance.h"

#include "Online.h"
#include "OnlineSubsystemUtils.h"

#define LOCTEXT_NAMESPACE "Parties"

UParty::UParty(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	bLeavingPersistentParty(false)
{
	PartyGameStateClass = UPartyGameState::StaticClass();
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
	}
}

void UParty::Init()
{
	FCoreUObjectDelegates::PostLoadMap.AddUObject(this, &UParty::OnPostLoadMap);
}

void UParty::InitPIE()
{
	OnPostLoadMap();
}

void UParty::OnPostLoadMap()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		RegisterIdentityDelegates();
		RegisterPartyDelegates();
	}
}

void UParty::OnShutdown()
{
	for (const auto& PartyKeyValue : JoinedParties)
	{
		UPartyGameState* Party = PartyKeyValue.Value;
		if (Party)
		{
			Party->OnShutdown();
		}
	}

	JoinedParties.Empty();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		FCoreUObjectDelegates::PostLoadMap.RemoveAll(this);

		UnregisterIdentityDelegates();
		UnregisterPartyDelegates();
	}
}

void UParty::OnLogoutComplete(int32 LocalUserNum, bool bWasSuccessful)
{
	if (JoinedParties.Num())
	{
		UE_LOG(LogParty, Log, TEXT("Party cleanup on logout"));
		for (const auto& PartyKeyValue : JoinedParties)
		{
			const FString& PartyId = PartyKeyValue.Key;
			UPartyGameState* Party = PartyKeyValue.Value;
			if (Party)
			{
				UE_LOG(LogParty, Log, TEXT("[%s] Removed"), *PartyId);
				Party->HandleRemovedFromParty(EMemberExitedReason::Left);
			}
		}

		JoinedParties.Empty();
	}
}

void UParty::OnLoginStatusChanged(int32 LocalUserNum, ELoginStatus::Type OldStatus, ELoginStatus::Type NewStatus, const FUniqueNetId& NewId)
{
	if (NewStatus == ELoginStatus::NotLoggedIn)
	{
		if (JoinedParties.Num())
		{
			UE_LOG(LogParty, Log, TEXT("Party cleanup on logout"));
			for (const auto& PartyKeyValue : JoinedParties)
			{
				const FString& PartyId = PartyKeyValue.Key;
				UPartyGameState* Party = PartyKeyValue.Value;
				if (Party)
				{
					UE_LOG(LogParty, Log, TEXT("[%s] Removed"), *PartyId);
					Party->HandleRemovedFromParty(EMemberExitedReason::Left);
				}
			}

			JoinedParties.Empty();
		}
	}
}

void UParty::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(InThis, Collector);

	UParty* This = CastChecked<UParty>(InThis);
	check(This);

	TArray<UPartyGameState*> Parties;
	This->JoinedParties.GenerateValueArray(Parties);
	Collector.AddReferencedObjects(Parties);
}

void UParty::RegisterIdentityDelegates()
{
	UWorld* World = GetWorld();
	check(World);

	IOnlineIdentityPtr IdentityInt = Online::GetIdentityInterface(World);
	if (IdentityInt.IsValid())
	{
		// Unbind and then rebind
		UnregisterIdentityDelegates();

		FOnLogoutCompleteDelegate OnLogoutCompleteDelegate;
		OnLogoutCompleteDelegate.BindUObject(this, &ThisClass::OnLogoutComplete);

		FOnLoginStatusChangedDelegate OnLoginStatusChangedDelegate;
		OnLoginStatusChangedDelegate.BindUObject(this, &ThisClass::OnLoginStatusChanged);

		for (int32 LocalPlayerId = 0; LocalPlayerId < MAX_LOCAL_PLAYERS; LocalPlayerId++)
		{
			LogoutCompleteDelegateHandle[LocalPlayerId] = IdentityInt->AddOnLogoutCompleteDelegate_Handle(LocalPlayerId, OnLogoutCompleteDelegate);
			LogoutStatusChangedDelegateHandle[LocalPlayerId] = IdentityInt->AddOnLoginStatusChangedDelegate_Handle(LocalPlayerId, OnLoginStatusChangedDelegate);
		}
	}
}

void UParty::UnregisterIdentityDelegates()
{
	UWorld* World = GetWorld();
	IOnlineIdentityPtr IdentityInt = Online::GetIdentityInterface(World);
	if (IdentityInt.IsValid())
	{
		for (int32 LocalPlayerId = 0; LocalPlayerId < MAX_LOCAL_PLAYERS; LocalPlayerId++)
		{
			if (LogoutCompleteDelegateHandle[LocalPlayerId].IsValid())
			{
				IdentityInt->ClearOnLogoutCompleteDelegate_Handle(LocalPlayerId, LogoutCompleteDelegateHandle[LocalPlayerId]);
			}

			if (LogoutStatusChangedDelegateHandle[LocalPlayerId].IsValid())
			{
				IdentityInt->ClearOnLoginStatusChangedDelegate_Handle(LocalPlayerId, LogoutStatusChangedDelegateHandle[LocalPlayerId]);
			}
		}
	}
}

void UParty::RegisterPartyDelegates()
{
	UnregisterPartyDelegates();

	UWorld* World = GetWorld();
	check(World);

	IOnlinePartyPtr PartyInt = Online::GetPartyInterface(World);
	if (PartyInt.IsValid())
	{
		PartyConfigChangedDelegateHandle = PartyInt->AddOnPartyConfigChangedDelegate_Handle(FOnPartyConfigChangedDelegate::CreateUObject(this, &ThisClass::PartyConfigChangedInternal));
		PartyMemberJoinedDelegateHandle = PartyInt->AddOnPartyMemberJoinedDelegate_Handle(FOnPartyMemberJoinedDelegate::CreateUObject(this, &ThisClass::PartyMemberJoinedInternal));
		PartyDataReceivedDelegateHandle = PartyInt->AddOnPartyDataReceivedDelegate_Handle(FOnPartyDataReceivedDelegate::CreateUObject(this, &ThisClass::PartyDataReceivedInternal));
		PartyMemberDataReceivedDelegateHandle = PartyInt->AddOnPartyMemberDataReceivedDelegate_Handle(FOnPartyMemberDataReceivedDelegate::CreateUObject(this, &ThisClass::PartyMemberDataReceivedInternal));
		PartyJoinRequestReceivedDelegateHandle = PartyInt->AddOnPartyJoinRequestReceivedDelegate_Handle(FOnPartyJoinRequestReceivedDelegate::CreateUObject(this, &ThisClass::PartyJoinRequestReceivedInternal));
		PartyMemberChangedDelegateHandle = PartyInt->AddOnPartyMemberChangedDelegate_Handle(FOnPartyMemberChangedDelegate::CreateUObject(this, &ThisClass::PartyMemberChangedInternal));
		PartyMemberExitedDelegateHandle = PartyInt->AddOnPartyMemberExitedDelegate_Handle(FOnPartyMemberExitedDelegate::CreateUObject(this, &ThisClass::PartyMemberExitedInternal));
		PartyPromotionLockoutChangedDelegateHandle = PartyInt->AddOnPartyPromotionLockoutChangedDelegate_Handle(FOnPartyPromotionLockoutChangedDelegate::CreateUObject(this, &ThisClass::PartyPromotionLockoutStateChangedInternal));
	}
}

void UParty::UnregisterPartyDelegates()
{
	UWorld* World = GetWorld();
	IOnlinePartyPtr PartyInt = Online::GetPartyInterface(World);
	if (PartyInt.IsValid())
	{
		PartyInt->ClearOnPartyConfigChangedDelegate_Handle(PartyConfigChangedDelegateHandle);
		PartyInt->ClearOnPartyMemberJoinedDelegate_Handle(PartyMemberJoinedDelegateHandle);
		PartyInt->ClearOnPartyDataReceivedDelegate_Handle(PartyDataReceivedDelegateHandle);
		PartyInt->ClearOnPartyMemberDataReceivedDelegate_Handle(PartyMemberDataReceivedDelegateHandle);
		PartyInt->ClearOnPartyJoinRequestReceivedDelegate_Handle(PartyJoinRequestReceivedDelegateHandle);
		PartyInt->ClearOnPartyMemberChangedDelegate_Handle(PartyMemberChangedDelegateHandle);
		PartyInt->ClearOnPartyMemberExitedDelegate_Handle(PartyMemberExitedDelegateHandle);
		PartyInt->ClearOnPartyPromotionLockoutChangedDelegate_Handle(PartyPromotionLockoutChangedDelegateHandle);
	}
}

void UParty::NotifyPreClientTravel()
{
	for (auto& JoinedParty : JoinedParties)
	{
		UPartyGameState* PartyState = JoinedParty.Value;
		PartyState->PreClientTravel();
	}
}

UPartyGameState* UParty::GetParty(const FOnlinePartyId& InPartyId) const
{
	if (InPartyId.IsValid())
	{
		UPartyGameState* const * PartyPtr = JoinedParties.Find(InPartyId.ToString());
		return PartyPtr ? *PartyPtr : nullptr;
	}
	
	return nullptr;
}

UPartyGameState* UParty::GetPersistentParty() const
{
	if (PersistentPartyId.IsValid())
	{
		UPartyGameState* const * PartyPtr = JoinedParties.Find(PersistentPartyId->ToString());
		return PartyPtr ? *PartyPtr : nullptr;
	}

	return nullptr;
}

void UParty::PartyConfigChangedInternal(const FUniqueNetId& InLocalUserId, const FOnlinePartyId& InPartyId, const TSharedRef<FPartyConfiguration>& InPartyConfig)
{
	UE_LOG(LogParty, Log, TEXT("[%s] Party config changed"), *InPartyId.ToString());

	UPartyGameState* PartyState = GetParty(InPartyId);
	if (PartyState)
	{
		PartyState->HandlePartyConfigChanged(InPartyConfig);
	}
	else
	{
		UE_LOG(LogParty, Warning, TEXT("[%s]: Missing party state during config change"), *InPartyId.ToString());
	}
}

void UParty::PartyMemberJoinedInternal(const FUniqueNetId& InLocalUserId, const FOnlinePartyId& InPartyId, const FUniqueNetId& InMemberId)
{
	UE_LOG(LogParty, Log, TEXT("[%s] Player %s joined"), *InPartyId.ToString(), *InMemberId.ToString());

	UPartyGameState* PartyState = GetParty(InPartyId);
	if (PartyState)
	{
		PartyState->HandlePartyMemberJoined(InMemberId);
	}
	else
	{
		UE_LOG(LogParty, Warning, TEXT("[%s]: Missing party state player: %s"), *InPartyId.ToString(), *InMemberId.ToString());
	}
}

void UParty::PartyDataReceivedInternal(const FUniqueNetId& InLocalUserId, const FOnlinePartyId& InPartyId, const TSharedRef<FOnlinePartyData>& InPartyData)
{
	UE_LOG(LogParty, Log, TEXT("[%s] party data received"), *InPartyId.ToString());

	UPartyGameState* PartyState = GetParty(InPartyId);
	if (PartyState)
	{
		PartyState->HandlePartyDataReceived(InPartyData);
	}
	else
	{
		UE_LOG(LogParty, Warning, TEXT("[%s]: Missing party state to apply data."), *InPartyId.ToString());
	}
}

void UParty::PartyMemberDataReceivedInternal(const FUniqueNetId& InLocalUserId, const FOnlinePartyId& InPartyId, const FUniqueNetId& InMemberId, const TSharedRef<FOnlinePartyData>& InPartyMemberData)
{
	UE_LOG(LogParty, Log, TEXT("[%s] Player %s data received"), *InPartyId.ToString(), *InMemberId.ToString());

	UPartyGameState* PartyState = GetParty(InPartyId);
	if (PartyState)
	{
		PartyState->HandlePartyMemberDataReceived(InMemberId, InPartyMemberData);
	}
	else
	{
		UE_LOG(LogParty, Warning, TEXT("[%s]: Missing party state to apply data for player %s"), *InPartyId.ToString(), *InMemberId.ToString());
	}
}

void UParty::PartyJoinRequestReceivedInternal(const FUniqueNetId& InLocalUserId, const FOnlinePartyId& InPartyId, const FUniqueNetId& SenderId)
{
	UPartyGameState* PartyState = GetParty(InPartyId);
	if (PartyState)
	{
		PartyState->HandlePartyJoinRequestReceived(InLocalUserId, SenderId);
	}
	else
	{
		UE_LOG(LogParty, Warning, TEXT("[%s]: Missing party state to process join request."), *InPartyId.ToString());
	}
}

void UParty::PartyMemberChangedInternal(const FUniqueNetId& InLocalUserId, const FOnlinePartyId& InPartyId, const FUniqueNetId& InMemberId, const EMemberChangedReason InReason)
{
	if (InLocalUserId == InMemberId)
	{
		UE_LOG(LogParty, Log, TEXT("[%s] [%s] local member %s"), *InPartyId.ToString(), *InMemberId.ToString(), ToString(InReason));
	}
	else
	{
		UE_LOG(LogParty, Log, TEXT("[%s] [%s] remote member %s"), *InPartyId.ToString(), *InMemberId.ToString(), ToString(InReason));
	}

	UPartyGameState* PartyState = GetParty(InPartyId);
	if (!PartyState)
	{
		UE_LOG(LogParty, Warning, TEXT("[%s]: Missing party state during member change."), *InPartyId.ToString());
	}

	if (InReason == EMemberChangedReason::Disconnected)
	{
		// basically cleanup because they get kicked for now as long as party config says
		// PartyConfig.bShouldRemoveOnDisconnection = true
		// handled by PartyMemberLeft 
	}
	else if (InReason == EMemberChangedReason::Rejoined)
	{
		// can't happen as long as 
		// PartyConfig.bShouldRemoveOnDisconnection = true
	}
	else if (InReason == EMemberChangedReason::Promoted)
	{
		if (PartyState)
		{
			PartyState->HandlePartyMemberPromoted(InMemberId);
		}

		if (PersistentPartyId.IsValid() &&
			(InPartyId == *PersistentPartyId))
		{
			FUniqueNetIdRepl NewPartyLeader;
			NewPartyLeader.SetUniqueNetId(InMemberId.AsShared());
			UpdatePersistentPartyLeader(NewPartyLeader);
		}
	}
}

void UParty::PartyMemberExitedInternal(const FUniqueNetId& InLocalUserId, const FOnlinePartyId& InPartyId, const FUniqueNetId& InMemberId, const EMemberExitedReason InReason)
{
	if (InLocalUserId == InMemberId)
	{
		UE_LOG(LogParty, Log, TEXT("[%s] [%s] local member removed. Reason: %s"), *InPartyId.ToString(), *InMemberId.ToString(), ToString(InReason));
	}
	else
	{
		UE_LOG(LogParty, Log, TEXT("[%s] [%s] remote member exited. Reason: %s"), *InPartyId.ToString(), *InMemberId.ToString(), ToString(InReason));
	}

	if (InLocalUserId == InMemberId)
	{
		if (InReason == EMemberExitedReason::Left)
		{
			// EMemberExitedReason::Left -> local player chose to leave, handled by leave completion delegate
		}
		else
		{
			UPartyGameState* PartyState = GetParty(InPartyId);
			if (!PartyState)
			{
				UE_LOG(LogParty, Warning, TEXT("[%s]: Missing party state during local player exit."), *InPartyId.ToString());
			}

			// EMemberExitedReason::Removed -> removed from other reasons beside kick (can't happen?)
			// EMemberExitedReason::Disbanded -> leader kicked everyone
			// EMemberExitedReason::Kicked -> party/leader kicked you
			// EMemberExitedReason::Unknown -> bad but try to cleanup anyway
			if (PartyState)
			{
				PartyState->HandleRemovedFromParty(InReason);
			}

			JoinedParties.Remove(InPartyId.ToString());
		}

		// If the removal was the persistent party, make sure we are in a good state
		if (PersistentPartyId.IsValid() &&
			(InPartyId == *PersistentPartyId))
		{
			RestorePersistentPartyState();
		}
	}
	else
	{
		UPartyGameState* PartyState = GetParty(InPartyId);
		if (!PartyState)
		{
			UE_LOG(LogParty, Warning, TEXT("[%s]: Missing party state during remote player exit."), *InPartyId.ToString());
		}

		if (PartyState)
		{
			// EMemberExitedReason::Left -> player chose to leave
			// EMemberExitedReason::Removed -> player was removed forcibly (timeout from disconnect, etc)
			// EMemberExitedReason::Disbanded -> leader kicked everyone (only leader should get this)
			// EMemberExitedReason::Kicked -> party/leader kicked this player
			PartyState->HandlePartyMemberLeft(InMemberId, InReason);
		}
	}
}

void UParty::PartyPromotionLockoutStateChangedInternal(const FUniqueNetId& LocalUserId, const FOnlinePartyId& InPartyId, const bool bLockoutState)
{
	UE_LOG(LogParty, Log, TEXT("[%s] party lockout state changed to %s"), *InPartyId.ToString(), bLockoutState ? TEXT("true") : TEXT("false"));

	UPartyGameState* PartyState = GetParty(InPartyId);
	if (PartyState)
	{
		PartyState->HandleLockoutPromotionStateChange(bLockoutState);
	}
	else
	{
		UE_LOG(LogParty, Warning, TEXT("[%s]: Missing party state during lockout call"), *InPartyId.ToString());
	}
}

void UParty::CreatePartyInternal(const FUniqueNetId& InUserId, const FPartyConfiguration& InPartyConfig, const FOnCreatePartyComplete& InCompletionDelegate)
{
	UWorld* World = GetWorld();
	IOnlinePartyPtr PartyInt = Online::GetPartyInterface(World);
	if (PartyInt.IsValid())
	{
		PartyInt->CreateParty(InUserId, InPartyConfig, InCompletionDelegate);
	}
	else
	{
		FString ErrorStr(TEXT("No party interface during CreateParty()"));
		InCompletionDelegate.ExecuteIfBound(InUserId, InUserId, ECreatePartyCompletionResult::UnknownClientFailure);
	}
}

void UParty::JoinPartyInternal(const FUniqueNetId& InUserId, const FPartyDetails& InPartyDetails, const FOnJoinPartyComplete& InCompletionDelegate)
{
	IOnlinePartyPtr PartyInt = Online::GetPartyInterface(GetWorld());
	if (PartyInt.IsValid())
	{
		if (InPartyDetails.IsValid())
		{
			const FOnlinePartyId& PartyId = *InPartyDetails.GetPartyId();
			// High level party data check
			UPartyGameState* PartyState = GetParty(PartyId);
			// Interface level party data check should not be out of sync
			const TSharedPtr<FOnlinePartyInfo>& PartyInfo = PartyInt->GetPartyInfo(InUserId, PartyId);
			if (!PartyState)
			{
				if (!PartyInfo.IsValid())
				{
					PartyInt->JoinParty(InUserId, *(InPartyDetails.PartyJoinInfo), InCompletionDelegate);
				}
				else
				{
					UE_LOG(LogParty, Verbose, TEXT("Already joining party %s, not joining again."), *InPartyDetails.GetPartyId()->ToString());
					InCompletionDelegate.ExecuteIfBound(InUserId, *InPartyDetails.GetPartyId(), EJoinPartyCompletionResult::AlreadyJoiningParty, 0);
				}
			}
			else
			{
				ensure(PartyInfo.IsValid());
				UE_LOG(LogParty, Verbose, TEXT("Already in party %s, not joining again."), *InPartyDetails.GetPartyId()->ToString());
				InCompletionDelegate.ExecuteIfBound(InUserId, *InPartyDetails.GetPartyId(), EJoinPartyCompletionResult::AlreadyInParty, 0);
			}
		}
		else
		{
			UE_LOG(LogParty, Verbose, TEXT("Invalid party details, cannot join. Details: %s"), *InPartyDetails.ToString());
			InCompletionDelegate.ExecuteIfBound(InUserId, *InPartyDetails.GetPartyId(), EJoinPartyCompletionResult::JoinInfoInvalid, 0);
		}
	}
	else
	{
		UE_LOG(LogParty, Verbose, TEXT("No party interface during JoinParty()"));
		InCompletionDelegate.ExecuteIfBound(InUserId, *InPartyDetails.GetPartyId(), EJoinPartyCompletionResult::UnknownClientFailure, 0);
	}
}

void UParty::LeavePartyInternal(const FUniqueNetId& InUserId, const FOnlinePartyId& InPartyId, const FOnLeavePartyComplete& InCompletionDelegate)
{
	IOnlinePartyPtr PartyInt = Online::GetPartyInterface(GetWorld());
	if (PartyInt.IsValid() && InPartyId.IsValid())
	{
		UPartyGameState* PartyState = GetParty(InPartyId);
		if (PartyState)
		{
			PartyState->HandleLeavingParty();
			PartyInt->LeaveParty(InUserId, InPartyId, InCompletionDelegate);
		}
		else
		{
			FString ErrorStr(TEXT("Party not found in LeaveParty()"));
			InCompletionDelegate.ExecuteIfBound(InUserId, InUserId, ELeavePartyCompletionResult::UnknownClientFailure);
		}
	}
	else
	{
		FString ErrorStr(TEXT("No party interface during LeaveParty()"));
		InCompletionDelegate.ExecuteIfBound(InUserId, InUserId, ELeavePartyCompletionResult::UnknownClientFailure);
	}
}

void UParty::CreateParty(const FUniqueNetId& InUserId, const FPartyConfiguration& InPartyConfig, const FOnCreatePartyComplete& InCompletionDelegate)
{
	FOnCreatePartyComplete CompletionDelegate;
	CompletionDelegate.BindUObject(this, &ThisClass::OnCreatePartyComplete, InCompletionDelegate);
	CreatePartyInternal(InUserId, InPartyConfig, CompletionDelegate);
}

void UParty::OnCreatePartyComplete(const FUniqueNetId& LocalUserId, const FOnlinePartyId& InPartyId, const ECreatePartyCompletionResult Result, FOnCreatePartyComplete InCompletionDelegate)
{
	bool bWasSuccessful = Result == ECreatePartyCompletionResult::Succeeded;
	UE_LOG(LogParty, Display, TEXT("OnCreatePartyComplete() %s %d %s"), *InPartyId.ToString(), bWasSuccessful, ToString(Result));
	if (bWasSuccessful)
	{
		UWorld* World = GetWorld();
		IOnlinePartyPtr PartyInt = Online::GetPartyInterface(World);
		if (PartyInt.IsValid())
		{
			TSharedPtr<FOnlinePartyInfo> PartyInfo = PartyInt->GetPartyInfo(LocalUserId, InPartyId);
			if (PartyInfo.IsValid())
			{
				UPartyGameState* NewParty = NewObject<UPartyGameState>(this, PartyGameStateClass);

				// Add right away so future delegate broadcasts have this available
				JoinedParties.Add(InPartyId.ToString(), NewParty);

				// Initialize and trigger delegates
				NewParty->InitFromCreate(LocalUserId, PartyInfo);
			}
			else
			{
				UE_LOG(LogParty, Warning, TEXT("Invalid PartyInfo when creating party %s"), *InPartyId.ToString());
				bWasSuccessful = false;
			}
		}
	}

	InCompletionDelegate.ExecuteIfBound(LocalUserId, InPartyId, Result);
}

void UParty::JoinParty(const FUniqueNetId& InUserId, const FPartyDetails& InPartyDetails, const FOnJoinPartyComplete& InCompletionDelegate)
{
	FOnJoinPartyComplete CompletionDelegate;
	CompletionDelegate.BindUObject(this, &ThisClass::OnJoinPartyComplete, InCompletionDelegate);
	JoinPartyInternal(InUserId, InPartyDetails, CompletionDelegate);
}

void UParty::OnJoinPartyComplete(const FUniqueNetId& LocalUserId, const FOnlinePartyId& InPartyId, EJoinPartyCompletionResult Result, int32 DeniedResultCode, FOnJoinPartyComplete CompletionDelegate)
{
	UE_LOG(LogParty, Display, TEXT("OnJoinPartyComplete() %s %s."), *InPartyId.ToString(), ToString(Result));

	EJoinPartyCompletionResult LocalResult = Result;
	FString ErrorString;
	if (Result == EJoinPartyCompletionResult::Succeeded)
	{
		UWorld* World = GetWorld();
		IOnlinePartyPtr PartyInt = Online::GetPartyInterface(World);
		if (PartyInt.IsValid())
		{
			TSharedPtr<FOnlinePartyInfo> PartyInfo = PartyInt->GetPartyInfo(LocalUserId, InPartyId);
			if (PartyInfo.IsValid())
			{
				UPartyGameState* NewParty = NewObject<UPartyGameState>(this, PartyGameStateClass);

				// Add right away so future delegate broadcasts have this available
				JoinedParties.Add(InPartyId.ToString(), NewParty);

				// Initialize and trigger delegates
				NewParty->InitFromJoin(LocalUserId, PartyInfo);
			}
			else
			{
				LocalResult = EJoinPartyCompletionResult::UnknownClientFailure;
				ErrorString = FString::Printf(TEXT("Invalid PartyInfo when joining party %s"), *InPartyId.ToString());
			}
		}
	}
	else
	{
		ErrorString = ToString(LocalResult);
	}

	if (LocalResult != EJoinPartyCompletionResult::Succeeded)
	{
		UE_LOG(LogParty, Warning, TEXT("%s"), *ErrorString);
	}

	int32 OutDeniedResultCode = (LocalResult == EJoinPartyCompletionResult::NotApproved) ? DeniedResultCode : 0;

	CompletionDelegate.ExecuteIfBound(LocalUserId, InPartyId, LocalResult, OutDeniedResultCode);
}

void UParty::LeaveParty(const FUniqueNetId& InUserId, const FOnlinePartyId& InPartyId, const FOnLeavePartyComplete& InCompletionDelegate)
{
	FOnLeavePartyComplete CompletionDelegate;
	CompletionDelegate.BindUObject(this, &ThisClass::OnLeavePartyComplete, InCompletionDelegate);
	LeavePartyInternal(InUserId, InPartyId, CompletionDelegate);
}

void UParty::OnLeavePartyComplete(const FUniqueNetId& LocalUserId, const FOnlinePartyId& InPartyId, const ELeavePartyCompletionResult Result, FOnLeavePartyComplete CompletionDelegate)
{
	UE_LOG(LogParty, Display, TEXT("OnLeavePartyComplete() %s %d %s."), *InPartyId.ToString(), Result == ELeavePartyCompletionResult::Succeeded, ToString(Result));

	UPartyGameState* PartyState = GetParty(InPartyId);
	if (PartyState)
	{
		PartyState->HandleRemovedFromParty(EMemberExitedReason::Left);
		JoinedParties.Remove(InPartyId.ToString());
	}
	else
	{
		UE_LOG(LogParty, Warning, TEXT("OnLeavePartyComplete: Missing party state %s"), *InPartyId.ToString());
	}
	
	CompletionDelegate.ExecuteIfBound(LocalUserId, InPartyId, Result);
}

void UParty::GetDefaultPersistentPartySettings(EPartyType& PartyType, bool& bLeaderFriendsOnly, bool& bLeaderInvitesOnly)
{
	PartyType = EPartyType::Public;
	bLeaderInvitesOnly = false;
	bLeaderFriendsOnly = false;
}

void UParty::GetPersistentPartyConfiguration(FPartyConfiguration& PartyConfig)
{
	EPartyType PartyType = EPartyType::Public;
	bool bLeaderInvitesOnly = false;
	bool bLeaderFriendsOnly = false;
	GetDefaultPersistentPartySettings(PartyType, bLeaderFriendsOnly, bLeaderInvitesOnly);

	bool bIsPrivate = (PartyType == EPartyType::Private);

	PartyConfig.Permissions = bIsPrivate ? EPartyPermissions::Private : EPartyPermissions::Public;
	PartyConfig.bIsAcceptingMembers = bIsPrivate ? false : true;
	PartyConfig.bShouldPublishToPresence = !bIsPrivate;
	PartyConfig.bCanNonLeaderPublishToPresence = !bIsPrivate && !bLeaderFriendsOnly;
	PartyConfig.bCanNonLeaderInviteToParty = !bLeaderInvitesOnly;

	PartyConfig.MaxMembers = DefaultMaxPartySize;
	PartyConfig.bShouldRemoveOnDisconnection = true;
}

void UParty::CreatePersistentParty(const FUniqueNetId& InUserId, const FOnCreatePartyComplete& InCompletionDelegate)
{
	if (PersistentPartyId.IsValid())
	{
		UE_LOG(LogParty, Warning, TEXT("Existing persistent party %s found when creating a new one."), *PersistentPartyId->ToString());
	}

	PersistentPartyId = nullptr;

	FPartyConfiguration PartyConfig;
	GetPersistentPartyConfiguration(PartyConfig);

	FOnCreatePartyComplete CompletionDelegate;
	CompletionDelegate.BindUObject(this, &ThisClass::OnCreatePersistentPartyComplete, InCompletionDelegate);
	CreatePartyInternal(InUserId, PartyConfig, CompletionDelegate);
}

void UParty::OnCreatePersistentPartyComplete(const FUniqueNetId& LocalUserId, const FOnlinePartyId& InPartyId, const ECreatePartyCompletionResult Result, FOnCreatePartyComplete CompletionDelegate)
{
	bool bWasSuccessful = Result == ECreatePartyCompletionResult::Succeeded;
	if (bWasSuccessful)
	{
		PersistentPartyId = InPartyId.AsShared();
	}

	OnCreatePartyComplete(LocalUserId, InPartyId, Result, CompletionDelegate);

	if (bWasSuccessful)
	{
		UPartyGameState* PersistentParty = GetPersistentParty();
		if (PersistentParty)
		{
			EPartyType PartyType = EPartyType::Public;
			bool bLeaderInvitesOnly = false;
			bool bLeaderFriendsOnly = false;
			GetDefaultPersistentPartySettings(PartyType, bLeaderFriendsOnly, bLeaderInvitesOnly);

			PersistentParty->SetPartyType(PartyType, bLeaderFriendsOnly, bLeaderInvitesOnly);

			TSharedPtr<const FUniqueNetId> PartyLeaderPtr = PersistentParty->GetPartyLeader();
			if (PartyLeaderPtr.IsValid())
			{
				FUniqueNetIdRepl PartyLeader(PartyLeaderPtr);
				UpdatePersistentPartyLeader(PartyLeader);
			}
			else
			{
				UE_LOG(LogParty, Warning, TEXT("OnJoinPersistentPartyComplete [%s]: Failed to update party leader"), *PersistentPartyId->ToString());
			}
		}
		else
		{
			UE_LOG(LogParty, Warning, TEXT("OnJoinPersistentPartyComplete [%s]: Failed to find party state object"), *PersistentPartyId->ToString());
		}
	}
}

void UParty::JoinPersistentParty(const FUniqueNetId& InUserId, const FPartyDetails& InPartyDetails, const FOnJoinPartyComplete& InCompletionDelegate)
{
	if (PersistentPartyId.IsValid())
	{
		UE_LOG(LogParty, Warning, TEXT("Existing persistent party %s found when joining a new one."), *PersistentPartyId->ToString());
	}

	PersistentPartyId = nullptr;

	FOnJoinPartyComplete CompletionDelegate;
	CompletionDelegate.BindUObject(this, &ThisClass::OnJoinPersistentPartyComplete, InCompletionDelegate);
	JoinPartyInternal(InUserId, InPartyDetails, CompletionDelegate);
}

void UParty::OnJoinPersistentPartyComplete(const FUniqueNetId& LocalUserId, const FOnlinePartyId& InPartyId, EJoinPartyCompletionResult Result, int32 DeniedResultCode, FOnJoinPartyComplete CompletionDelegate)
{
	if (Result == EJoinPartyCompletionResult::Succeeded)
	{
		PersistentPartyId = InPartyId.AsShared();
	}

	OnJoinPartyComplete(LocalUserId, InPartyId, Result, DeniedResultCode, CompletionDelegate);

	if (Result == EJoinPartyCompletionResult::Succeeded)
	{
		UPartyGameState* PersistentParty = GetPersistentParty();
		if (PersistentParty)
		{
			TSharedPtr<const FUniqueNetId> PartyLeaderPtr = PersistentParty->GetPartyLeader();
			if (PartyLeaderPtr.IsValid())
			{
				FUniqueNetIdRepl PartyLeader(PartyLeaderPtr);
				UpdatePersistentPartyLeader(PartyLeader);
			}
			else
			{
				UE_LOG(LogParty, Warning, TEXT("OnJoinPersistentPartyComplete [%s]: Failed to update party leader"), *PersistentPartyId->ToString());
			}
		}
		else
		{
			UE_LOG(LogParty, Warning, TEXT("OnJoinPersistentPartyComplete [%s]: Failed to find party state object"), PersistentPartyId.IsValid() ? *PersistentPartyId->ToString() : TEXT("INVALID"));
		}
	}
	else
	{
		if (Result != EJoinPartyCompletionResult::AlreadyJoiningParty)
		{
			UWorld* World = GetWorld();
			if (World)
			{
				// Try to get back to a good state
				FTimerDelegate HandleJoinPartyFailure;
				HandleJoinPartyFailure.BindUObject(this, &ThisClass::HandleJoinPersistentPartyFailure);
				World->GetTimerManager().SetTimerForNextTick(HandleJoinPartyFailure);
			}
		}
		else
		{
			UE_LOG(LogParty, Verbose, TEXT("OnJoinPersistentPartyComplete [%s]: already joining party."), PersistentPartyId.IsValid() ? *PersistentPartyId->ToString() : TEXT("INVALID"));
		}
	}
}

void UParty::HandleJoinPersistentPartyFailure()
{
	RestorePersistentPartyState();
}

void UParty::LeavePersistentParty(const FUniqueNetId& InUserId, const FOnLeavePartyComplete& InCompletionDelegate)
{
	FOnLeavePartyComplete CompletionDelegate;
	CompletionDelegate.BindUObject(this, &ThisClass::OnLeavePersistentPartyComplete, InCompletionDelegate);

	if (PersistentPartyId.IsValid())
	{
		if (!bLeavingPersistentParty)
		{
			bLeavingPersistentParty = true;
			LeavePartyInternal(InUserId, *PersistentPartyId, CompletionDelegate);
		}
		else
		{
			LeavePartyCompleteDelegates.Add(InCompletionDelegate);
		}
	}
	else
	{
		UE_LOG(LogParty, Warning, TEXT("No party during LeavePersistentParty()"));
		// mark this as true so the ensure works in the delegate
		bLeavingPersistentParty = true;
		CompletionDelegate.ExecuteIfBound(InUserId, InUserId, ELeavePartyCompletionResult::UnknownClientFailure);
	}
}

void UParty::OnLeavePersistentPartyComplete(const FUniqueNetId& LocalUserId, const FOnlinePartyId& InPartyId, const ELeavePartyCompletionResult Result, FOnLeavePartyComplete CompletionDelegate)
{
	ensure(bLeavingPersistentParty);
	ensure(!PersistentPartyId.IsValid() || *PersistentPartyId == InPartyId);
	bLeavingPersistentParty = false;

	// This might be the last reference of the party id, transfer the shared pointer for this brief period of time
	// PersistentPartyId is expected to be null now as far as external code is concerned
	TSharedPtr<const FOnlinePartyId> TempPartyId = PersistentPartyId.IsValid() ? PersistentPartyId : nullptr;
	PersistentPartyId = nullptr;
	OnLeavePartyComplete(LocalUserId, InPartyId, Result, CompletionDelegate);

	TArray<FOnLeavePartyComplete> DelegatesCopy = LeavePartyCompleteDelegates;
	LeavePartyCompleteDelegates.Empty();

	// fire delegates for any/all LeavePersistentParty calls made while this one was in flight
	for (const FOnLeavePartyComplete& ExtraDelegate : DelegatesCopy)
	{
		ExtraDelegate.ExecuteIfBound(LocalUserId, InPartyId, Result);
	}
}

void UParty::RestorePersistentPartyState()
{
	if (!bLeavingPersistentParty)
	{
		UWorld* World = GetWorld();
		IOnlinePartyPtr PartyInt = Online::GetPartyInterface(World);
		if (PartyInt.IsValid())
		{
			UGameInstance* GameInstance = GetGameInstance();
			check(GameInstance);

			TSharedPtr<const FUniqueNetId> LocalUserId = GameInstance->GetPrimaryPlayerUniqueId();
			check(LocalUserId.IsValid() && LocalUserId->IsValid());

			UPartyGameState* PersistentParty = GetPersistentParty();

			// Check for existing party and create a new one if necessary
			bool bFoundExistingPersistentParty = PersistentParty ? true : false;
			if (bFoundExistingPersistentParty)
			{
				// In a party already, make sure the UI is aware of its state
				if (PersistentParty->ResetForFrontend())
				{
					OnPartyResetForFrontend().Broadcast(PersistentParty);
				}
				else
				{
					// There was an issue resetting the party, so leave 
					LeaveAndRestorePersistentParty();
				}
			}
			else
			{
				PersistentPartyId = nullptr;

				// Create a new party
				FOnCreatePartyComplete CompletionDelegate;
				CreatePersistentParty(*LocalUserId, CompletionDelegate);
			}
		}
	}
	else
	{
		UE_LOG(LogParty, Verbose, TEXT("Can't RestorePersistentPartyState while leaving party, ignoring"));
	}
}

void UParty::UpdatePersistentPartyLeader(const FUniqueNetIdRepl& NewPartyLeader)
{
}

bool UParty::IsInParty(TSharedPtr<const FOnlinePartyId>& PartyId) const
{
	bool bFoundParty = false;

	UWorld* World = GetWorld();
	check(World);

	IOnlinePartyPtr PartyInt = Online::GetPartyInterface(World);
	if (PartyInt.IsValid())
	{
		UGameInstance* GameInstance = GetGameInstance();
		check(GameInstance);

		TSharedPtr<const FUniqueNetId> LocalUserId = GameInstance->GetPrimaryPlayerUniqueId();
		if (ensure(LocalUserId.IsValid() && LocalUserId->IsValid()))
		{
			TArray<TSharedRef<const FOnlinePartyId>> LocalJoinedParties;
			PartyInt->GetJoinedParties(*LocalUserId, LocalJoinedParties);
			for (const auto& JoinedParty : LocalJoinedParties)
			{
				if (*JoinedParty == *PartyId)
				{
					bFoundParty = true;
					break;
				}
			}
		}
	}

	return bFoundParty;
}

void UParty::KickFromPersistentParty()
{
	TSharedPtr<const FOnlinePartyId> LocalPersistentPartyId = GetPersistentPartyId();
	UPartyGameState* PersistentParty = GetPersistentParty();
	if (LocalPersistentPartyId.IsValid() && PersistentParty)
	{
		if (PersistentParty->GetPartySize() > 1)
		{
			UGameInstance* GameInstance = GetGameInstance();
			check(GameInstance);

			TSharedPtr<const FUniqueNetId> LocalUserId = GameInstance->GetPrimaryPlayerUniqueId();
			if (ensure(LocalUserId.IsValid() && LocalUserId->IsValid()))
			{
				// Leave the party (restored in frontend)
				FOnLeavePartyComplete CompletionDelegate;
				LeavePersistentParty(*LocalUserId, CompletionDelegate);
			}
		}
		else
		{
			// Just block new joining until back in the frontend
			PersistentParty->SetAcceptingMembers(false, EJoinPartyDenialReason::Busy);
		}
	}
}

void UParty::LeaveAndRestorePersistentParty()
{
	if (!bLeavingPersistentParty)
	{
		bLeavingPersistentParty = true;
		UWorld* World = GetWorld();

		FTimerDelegate HandleLobbyConnectionAttemptFailed;
		HandleLobbyConnectionAttemptFailed.BindUObject(this, &ThisClass::LeaveAndRestorePersistentPartyInternal);
		World->GetTimerManager().SetTimerForNextTick(HandleLobbyConnectionAttemptFailed);
	}
	else
	{
		UE_LOG(LogParty, Verbose, TEXT("Already leaving persistent party, ignoring"));
	}
}

void UParty::LeaveAndRestorePersistentPartyInternal()
{
	UGameInstance* GameInstance = GetGameInstance();
	check(GameInstance);

	TSharedPtr<const FUniqueNetId> PrimaryUserId = GameInstance->GetPrimaryPlayerUniqueId();
	check(PrimaryUserId.IsValid() && PrimaryUserId->IsValid());

	FOnLeavePartyComplete CompletionDelegate;
	CompletionDelegate.BindUObject(this, &ThisClass::OnLeavePersistentPartyAndRestore);

	// Unset this here, wouldn't be here if we were unable to call LeaveAndRestorePersistentParty, it will be reset inside
	ensure(bLeavingPersistentParty);
	bLeavingPersistentParty = false;
	LeavePersistentParty(*PrimaryUserId, CompletionDelegate);
}

void UParty::OnLeavePersistentPartyAndRestore(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId, const ELeavePartyCompletionResult Result)
{
	UE_LOG(LogParty, Display, TEXT("OnLeavePersistentPartyAndRestore [%s] Success: %d %s"), *PartyId.ToString(), Result == ELeavePartyCompletionResult::Succeeded, ToString(Result));
	ensure(bLeavingPersistentParty);
	bLeavingPersistentParty = false;
	RestorePersistentPartyState();
}

bool UParty::ProcessPendingPartyInvites()
{
	if (PendingPartyInvite.IsValid() && PendingPartyInvite->IsValid())
	{
		// Copy the values out because PendingPartyInvite is going to be remade inside this function
		TSharedRef<const FUniqueNetId> LocalUserId = PendingPartyInvite->LocalUserId;
		FPartyDetails PartyDetails = PendingPartyInvite->PartyDetails;
		FOnJoinPartyComplete JoinCompleteDelegate = PendingPartyInvite->JoinCompleteDelegate;
		HandlePendingInvite(*LocalUserId, PartyDetails, JoinCompleteDelegate);
		return true;
	}

	return false;
}

void UParty::HandlePendingInvite(const FUniqueNetId& LocalUserId, const FPartyDetails& PartyDetails, const FOnJoinPartyComplete& JoinCompleteDelegate)
{
	if (LocalUserId.IsValid() && PartyDetails.IsValid())
	{
		// Setup the invite for processing
		PendingPartyInvite = MakeShareable(new FPendingPartyInvite(LocalUserId.AsShared(), PartyDetails, JoinCompleteDelegate));

		UWorld* World = GetWorld();
		check(World);

		JoinCompleteDelegate.ExecuteIfBound(LocalUserId, *PartyDetails.GetPartyId(), EJoinPartyCompletionResult::UnknownClientFailure, 0);
	}
}

UWorld* UParty::GetWorld() const
{
	UGameInstance* GameInstance = Cast<UGameInstance>(GetOuter());
	if (GameInstance)
	{
		return GameInstance->GetWorld();
	}
	
	return nullptr;
}

UGameInstance* UParty::GetGameInstance() const
{
	return Cast<UGameInstance>(GetOuter());
}

#undef LOCTEXT_NAMESPACE
