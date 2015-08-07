// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "OSSScheduler.h"

#include "FriendsAndChatAnalytics.h"

const float CHAT_ANALYTICS_INTERVAL = 5 * 60.0f;  // 5 min

class FOSSSchedulerImpl
	: public FOSSScheduler
{
public:

	virtual void Login(IOnlineSubsystem* InOnlineSub, bool bInIsGame, int32 LocalUserID) override
	{
		// Clear existing data
		Logout();
		
		LocalControllerIndex = LocalUserID;
		
		if (InOnlineSub)
		{
			OnlineSub = InOnlineSub;
		}
		else
		{
			OnlineSub = IOnlineSubsystem::Get(TEXT("MCP"));
		}

		if (OnlineSub != nullptr &&
			OnlineSub->GetUserInterface().IsValid() &&
			OnlineSub->GetIdentityInterface().IsValid())
		{
			OnlineIdentity = OnlineSub->GetIdentityInterface();
			OnPresenceUpdatedCompleteDelegate = IOnlinePresence::FOnPresenceTaskCompleteDelegate::CreateSP(this, &FOSSSchedulerImpl::OnPresenceUpdated);
		}

		if (UpdateFriendsTickerDelegate.IsBound() == false)
		{
			UpdateFriendsTickerDelegate = FTickerDelegate::CreateSP(this, &FOSSSchedulerImpl::Tick);
		}

		UpdateFriendsTickerDelegateHandle = FTicker::GetCoreTicker().AddTicker(UpdateFriendsTickerDelegate);
	}

	virtual void Logout() override
	{
		OnlineSub = nullptr;
		OnlineIdentity = nullptr;
		// flush before removing the analytics provider
		Analytics->FireEvent_FlushChatStats();

		if (UpdateFriendsTickerDelegate.IsBound())
		{
			FTicker::GetCoreTicker().RemoveTicker(UpdateFriendsTickerDelegateHandle);
		}
	}

	bool IsLoggedIn() const override
	{
		return OnlineSub != nullptr && OnlineIdentity.IsValid();
	}

	void SetOnline() override
	{
		SetUserIsOnline(EOnlinePresenceState::Online);
	}

	void SetAway() override
	{
		SetUserIsOnline(EOnlinePresenceState::Away);
	}

	virtual EOnlinePresenceState::Type GetOnlineStatus() override
	{
		if (OnlineSub != nullptr &&
			OnlineIdentity.IsValid())
		{
			TSharedPtr<FOnlineUserPresence> Presence;
			TSharedPtr<const FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(0);
			if (UserId.IsValid())
			{
				OnlineSub->GetPresenceInterface()->GetCachedPresence(*UserId, Presence);
				if (Presence.IsValid())
				{
					return Presence->Status.State;
				}
			}
		}
		return EOnlinePresenceState::Offline;
	}

	virtual FString GetUserClientId() const override
	{
		FString Result;
		if (OnlineSub != nullptr &&
			OnlineIdentity.IsValid())
		{
			TSharedPtr<FOnlineUserPresence> Presence;
			TSharedPtr<const FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(0);
			if (UserId.IsValid())
			{
				GetPresenceInterface()->GetCachedPresence(*UserId, Presence);
				if (Presence.IsValid())
				{
					const FVariantData* ClientId = Presence->Status.Properties.Find(DefaultClientIdKey);
					if (ClientId != nullptr && ClientId->GetType() == EOnlineKeyValuePairDataType::String)
					{
						ClientId->GetValue(Result);
					}
				}
			}
		}
		return Result;
	}

	virtual TSharedPtr<class FFriendsAndChatAnalytics> GetAnalytics() const override
	{
		return Analytics;
	}

	virtual IOnlineFriendsPtr GetFriendsInterface() const override
	{
		return OnlineSub->GetFriendsInterface();
	}

	virtual IOnlineUserPtr GetUserInterface() const override
	{
		return OnlineSub->GetUserInterface();
	}

	virtual IOnlineIdentityPtr GetOnlineIdentity() const override
	{
		return OnlineIdentity;
	}

	virtual IOnlinePartyPtr GetPartyInterface() const override
	{
		return OnlineSub->GetPartyInterface();
	}

	virtual IOnlineSessionPtr GetSessionInterface() const override
	{
		return OnlineSub->GetSessionInterface();
	}

	virtual IOnlinePresencePtr GetPresenceInterface() const override
	{
		return OnlineSub->GetPresenceInterface();
	}

	virtual IOnlineChatPtr GetChatInterface() const override
	{
		return OnlineSub->GetChatInterface();
	}

	virtual FChatRoomId GetPartyChatRoomId() const override
	{
		TSharedPtr<const FUniqueNetId> UserId = GetOnlineIdentity()->GetUniquePlayerId(LocalControllerIndex);
		FChatRoomId PartyChatRoomId;
		TArray< TSharedRef<const FOnlinePartyId> > OutPartyIds;
		if (UserId.IsValid())
		{
			IOnlinePartyPtr PartyInterface = GetPartyInterface();
			if (PartyInterface.IsValid()
				&& PartyInterface->GetJoinedParties(*UserId, OutPartyIds) == true
				&& OutPartyIds.Num() > 0)
			{
				TSharedPtr<FOnlinePartyInfo> PartyInfo = PartyInterface->GetPartyInfo(*UserId, *(OutPartyIds[0]));
				if (PartyInfo.IsValid())
				{
					PartyChatRoomId = PartyInfo->RoomId; // @todo EN need to identify the primary game party consistently when multiple parties exist
				}
			}
		}
		return PartyChatRoomId;
	}

	virtual void SetUserIsOnline(EOnlinePresenceState::Type OnlineState) override
	{
		if (OnlineSub != nullptr &&
			OnlineIdentity.IsValid())
		{
			TSharedPtr<const FUniqueNetId> UserId = OnlineIdentity->GetUniquePlayerId(0);
			if (UserId.IsValid())
			{
				TSharedPtr<FOnlineUserPresence> CurrentPresence;
				OnlineSub->GetPresenceInterface()->GetCachedPresence(*UserId, CurrentPresence);
				FOnlineUserPresenceStatus NewStatus;
				if (CurrentPresence.IsValid())
				{
					NewStatus = CurrentPresence->Status;
				}
				NewStatus.State = OnlineState;
				OnlineSub->GetPresenceInterface()->SetPresence(*UserId, NewStatus, OnPresenceUpdatedCompleteDelegate);
			}
		}
	}

	virtual TSharedPtr<const FUniqueNetId> GetGameSessionId() const override
	{
		if (GetOnlineIdentity().IsValid() && GetSessionInterface().IsValid())
		{
			const FNamedOnlineSession* GameSession = GetSessionInterface()->GetNamedSession(GameSessionName);
			if (GameSession != nullptr)
			{
				TSharedPtr<FOnlineSessionInfo> UserSessionInfo = GameSession->SessionInfo;
				if (UserSessionInfo.IsValid())
				{
					return GetOnlineIdentity()->CreateUniquePlayerId(UserSessionInfo->GetSessionId().ToString());
				}
			}
		}
		return nullptr;
	}

	virtual bool IsFriendInSameSession(const TSharedPtr< const IFriendItem >& FriendItem) const override
	{
		TSharedPtr<const FUniqueNetId> MySessionId = GetGameSessionId();
		bool bMySessionValid = MySessionId.IsValid() && MySessionId->IsValid();

		TSharedPtr<const FUniqueNetId> FriendSessionId = FriendItem->GetGameSessionId();
		bool bFriendSessionValid = FriendSessionId.IsValid() && FriendSessionId->IsValid();

		return (bMySessionValid && bFriendSessionValid && (*FriendSessionId == *MySessionId));
	}

	virtual bool IsFriendInSameParty(const TSharedPtr< const IFriendItem >& FriendItem) const override
	{
		if (GetOnlineIdentity().IsValid() && GetPartyInterface().IsValid())
		{
			TSharedPtr<const FUniqueNetId> LocalUserId = GetOnlineIdentity()->GetUniquePlayerId(LocalControllerIndex);
			if (LocalUserId.IsValid())
			{
				IOnlinePartyPtr PartyInt = GetPartyInterface();
				if (PartyInt.IsValid())
				{
					TArray<TSharedRef<const FOnlinePartyId>> JoinedParties;
					if (PartyInt->GetJoinedParties(*LocalUserId, JoinedParties))
					{
						if (JoinedParties.Num() > 0)
						{
							TSharedPtr<IOnlinePartyJoinInfo> FriendPartyJoinInfo = GetPartyJoinInfo(FriendItem);
							if (FriendPartyJoinInfo.IsValid())
							{
								for (auto PartyId : JoinedParties)
								{
									if (*PartyId == *FriendPartyJoinInfo->GetPartyId())
									{
										return true;
									}
								}
							}
							else
							{
								// Party might be private, search manually
								TSharedRef<const FUniqueNetId> FriendUserId = FriendItem->GetUniqueID();
								for (auto PartyId : JoinedParties)
								{
									TSharedPtr<FOnlinePartyMember> PartyMemberId = PartyInt->GetPartyMember(*LocalUserId, *PartyId, *FriendUserId);
									if (PartyMemberId.IsValid())
									{
										return true;
									}
								}
							}
						}
					}
				}
			}
		}

		return false;
	}

	virtual TSharedPtr<IOnlinePartyJoinInfo> GetPartyJoinInfo(const TSharedPtr<const IFriendItem>& FriendItem) const override
	{
		TSharedPtr<IOnlinePartyJoinInfo> Result;

		if (GetOnlineIdentity().IsValid() && GetPartyInterface().IsValid())
		{
			TSharedPtr<const FUniqueNetId> UserId = GetOnlineIdentity()->GetUniquePlayerId(LocalControllerIndex);
			if (UserId.IsValid())
			{
				Result = GetPartyInterface()->GetAdvertisedParty(*UserId, *FriendItem->GetUniqueID());
			}
		}
		return Result;
	}

	void OnPresenceUpdated(const FUniqueNetId& UserId, const bool bWasSuccessful)
	{
	}

	virtual void RequestOSS(FOnRequestOSSAccepted OnRequestOSSAcceptedDelegate) override
	{
		RequestDelegates.Add(OnRequestOSSAcceptedDelegate);
	}

	virtual void ReleaseOSS() override
	{
		bOSSInUse = false;
	}

	bool Tick(float Delta)
	{
		FlushChatAnalyticsCountdown -= Delta;
		if (FlushChatAnalyticsCountdown <= 0)
		{
			Analytics->FireEvent_FlushChatStats();
			// Reset countdown for new update
			FlushChatAnalyticsCountdown = CHAT_ANALYTICS_INTERVAL;
		}
		ProcessOSSRequests();
		return true;
	}

	void ProcessOSSRequests()
	{
		if (!bOSSInUse && RequestDelegates.Num() > 0)
		{
			bOSSInUse = true;
			RequestDelegates[0].Execute();
			RequestDelegates.RemoveAt(0);
		}
	}

private:


	FOSSSchedulerImpl(TSharedPtr<FFriendsAndChatAnalytics> InAnalytics)
		: OnlineIdentity(nullptr)
		, OnlineSub(nullptr)
		, Analytics(InAnalytics)
		, bOSSInUse(false)
		, FlushChatAnalyticsCountdown(CHAT_ANALYTICS_INTERVAL)
	{
	}

	// Holds the Online identity
	IOnlineIdentityPtr OnlineIdentity;
	// Holds the Online Subsystem
	IOnlineSubsystem* OnlineSub;

	// Delegate to owner presence updated
	IOnlinePresence::FOnPresenceTaskCompleteDelegate OnPresenceUpdatedCompleteDelegate;

	TSharedPtr<FFriendsAndChatAnalytics> Analytics;

	int32 LocalControllerIndex;

	TArray<FOnRequestOSSAccepted> RequestDelegates;
	bool bOSSInUse;

	// Holds the ticker delegate
	FTickerDelegate UpdateFriendsTickerDelegate;
	FDelegateHandle UpdateFriendsTickerDelegateHandle;
	float FlushChatAnalyticsCountdown;

	friend FOSSSchedulerFactory;
};

TSharedRef< FOSSScheduler > FOSSSchedulerFactory::Create(TSharedPtr<FFriendsAndChatAnalytics> Analytics)
{
	TSharedRef< FOSSSchedulerImpl > OSSScheduler(new FOSSSchedulerImpl(Analytics));
	return OSSScheduler;
}