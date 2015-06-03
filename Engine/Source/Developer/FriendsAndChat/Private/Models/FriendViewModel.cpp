// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "FriendViewModel.h"

#define LOCTEXT_NAMESPACE "FriendsAndChat"

class FFriendViewModelImpl
	: public FFriendViewModel
{
public:

	virtual void EnumerateActions(TArray<EFriendActionType::Type>& Actions, bool bFromChat = false) override
	{
		bool bIsFriendInSameSession = FriendsAndChatManager.Pin()->IsFriendInSameSession(FriendItem);

		if(FriendItem->IsGameRequest())
		{
			if (FriendItem->IsGameJoinable())
			{
				Actions.Add(EFriendActionType::JoinGame);
			}
			Actions.Add(EFriendActionType::RejectGame);
		}
		else if(FriendItem->IsPendingAccepted())
		{
			Actions.Add(EFriendActionType::Updating);
		}
		else if (FriendItem->GetListType() == EFriendsDisplayLists::RecentPlayersDisplay)
		{
			TSharedPtr<IFriendItem> ExistingFriend = FriendsAndChatManager.Pin()->FindUser(*FriendItem->GetUniqueID());
			if (!ExistingFriend.IsValid())
			{
				Actions.Add(EFriendActionType::SendFriendRequest);
			}
			if (FriendsAndChatManager.Pin()->IsInJoinableGameSession() && FriendItem->IsOnline())
			{
				Actions.Add(EFriendActionType::InviteToGame);
			}
		}
		else
		{
			switch (FriendItem->GetInviteStatus())
			{
				case EInviteStatus::Accepted :
				{
					if (FriendItem->IsOnline() && !bIsFriendInSameSession && FriendItem->IsGameJoinable())
					{
						if(CanPerformAction(EFriendActionType::JoinGame))
						{
							Actions.Add(EFriendActionType::JoinGame);
						}
					}
					if (FriendItem->IsOnline() && !bIsFriendInSameSession && FriendItem->CanInvite() && FriendsAndChatManager.Pin()->IsInJoinableGameSession())
					{
						Actions.Add(EFriendActionType::InviteToGame);
					}
					if(!bFromChat)
					{
						if (FriendItem->IsOnline())
						{
							Actions.Add(EFriendActionType::Chat);
						}
						Actions.Add(EFriendActionType::RemoveFriend);
					}
				}
				break;
				case EInviteStatus::PendingInbound :
				{
					Actions.Add(EFriendActionType::AcceptFriendRequest);
					Actions.Add(EFriendActionType::IgnoreFriendRequest);
				}
				break;
				case EInviteStatus::PendingOutbound :
				{
					Actions.Add(EFriendActionType::CancelFriendRequest);
				}
				break;
			default:
				Actions.Add(EFriendActionType::SendFriendRequest);
				break;
			}
		}
	}
	
	virtual const bool HasChatAction() const override
	{
		bool bIsFriendInSameSession = FriendsAndChatManager.Pin()->IsFriendInSameSession(FriendItem);

		return FriendItem->GetInviteStatus() != EInviteStatus::Accepted
			|| FriendItem->IsGameJoinable()
			|| (FriendsAndChatManager.Pin()->IsInJoinableGameSession() && !bIsFriendInSameSession);
	}

	virtual void PerformAction(const EFriendActionType::Type ActionType) override
	{
		switch(ActionType)
		{
			case EFriendActionType::AcceptFriendRequest : 
			{
				AcceptFriend();
				break;
			}
			case EFriendActionType::RemoveFriend :
			case EFriendActionType::IgnoreFriendRequest :
			case EFriendActionType::BlockFriend :
			case EFriendActionType::RejectFriendRequest:
			case EFriendActionType::CancelFriendRequest:
			{
				RemoveFriend(EFriendActionType::ToText(ActionType).ToString());
				break;
			}
			case EFriendActionType::SendFriendRequest : 
			{
				SendFriendRequest();
				break;
			}
			case EFriendActionType::InviteToGame : 
			{
				InviteToGame();
				break;
			}
			case EFriendActionType::JoinGame : 
			{
				JoinGame();
				break;
			}
			case EFriendActionType::RejectGame:
			{
				RejectGame();
				break;
			}
			case EFriendActionType::Chat:
			{
				StartChat();
				break;
			}
		}
		SetPendingAction(EFriendActionType::MAX_None);
	}

	virtual void SetPendingAction(EFriendActionType::Type PendingAction) override
	{
		FriendItem->SetPendingAction(PendingAction);
	}

	virtual EFriendActionType::Type GetPendingAction() override
	{
		return FriendItem->GetPendingAction();
	}

	virtual bool CanPerformAction(const EFriendActionType::Type ActionType) override
	{
		switch (ActionType)
		{
			case EFriendActionType::JoinGame:
			{
				return FriendsAndChatManager.Pin()->JoinGameAllowed(GetClientId());
			}
			case EFriendActionType::AcceptFriendRequest:
			case EFriendActionType::RemoveFriend:
			case EFriendActionType::IgnoreFriendRequest:
			case EFriendActionType::BlockFriend:
			case EFriendActionType::RejectFriendRequest:
			case EFriendActionType::CancelFriendRequest:
			case EFriendActionType::SendFriendRequest:
			case EFriendActionType::InviteToGame:
			case EFriendActionType::RejectGame:
			case EFriendActionType::Chat:
			default:
			{
				return true;
			}
		}
	}

	virtual FText GetJoinGameDisallowReason() const override
	{
		static const FText InLauncher = LOCTEXT("GameJoinFail_InLauncher", "Please ensure Fortnite is installed and up to date");
		static const FText InSession = LOCTEXT("GameJoinFail_InSession", "Quit to the Main Menu to join a friend's game");

		if(FriendsAndChatManager.Pin()->IsInLauncher())
		{
			return InLauncher;
		}
		return InSession;
	}

	~FFriendViewModelImpl()
	{
		Uninitialize();
	}

	virtual const FString GetName() const override
	{
		return FriendItem->GetName();
	}

	virtual FText GetFriendLocation() const override
	{
		return FriendItem.IsValid() ? FriendItem->GetFriendLocation() : FText::GetEmpty();
	}

	virtual bool IsOnline() const override
	{
		return FriendItem.IsValid() ? FriendItem->IsOnline() : false;
	}

	virtual bool IsInGameSession() const override
	{
		return FriendsAndChatManager.Pin()->IsInGameSession();
	}

	virtual bool IsInActiveParty() const override
	{
		return FriendsAndChatManager.Pin()->IsInActiveParty();
	}

	virtual const EOnlinePresenceState::Type GetOnlineStatus() const override
	{
		return FriendItem.IsValid() ? FriendItem->GetOnlineStatus() : EOnlinePresenceState::Offline;
	}

	virtual const FString GetClientId() const override
	{
		return FriendItem.IsValid() ? FriendItem->GetClientId() : FString();
	}

private:

	void RemoveFriend(const FString& Action) const
	{
		FriendsAndChatManager.Pin()->DeleteFriend(FriendItem, Action);
	}

	void AcceptFriend() const
	{
		FriendsAndChatManager.Pin()->AcceptFriend( FriendItem );
	}

	void SendFriendRequest() const
	{
		if (FriendItem.IsValid())
		{
			FriendsAndChatManager.Pin()->RequestFriend(FText::FromString(FriendItem->GetName()));
		}
	}

	void InviteToGame()
	{
		if (FriendItem.IsValid())
		{
			FriendsAndChatManager.Pin()->SendGameInvite(FriendItem);
		}
	}

	void JoinGame()
	{
		if (FriendItem.IsValid() &&
			(FriendItem->IsGameRequest() || FriendItem->IsGameJoinable()))
		{
			FriendsAndChatManager.Pin()->AcceptGameInvite(FriendItem);
		}
	}

	void RejectGame()
	{
		if (FriendItem.IsValid() &&
			FriendItem->IsGameRequest())
		{
			FriendsAndChatManager.Pin()->RejectGameInvite(FriendItem);
		}
	}

	void StartChat()
	{
		if (FriendItem.IsValid() && FriendItem->GetOnlineFriend().IsValid() && FriendItem->IsOnline())
		{
			FriendsAndChatManager.Pin()->SetChatFriend(FriendItem);
		}
	}

private:
	void Initialize()
	{
	}

	void Uninitialize()
	{
	}

	FFriendViewModelImpl(
		const TSharedRef<IFriendItem>& FriendItem,
		const TSharedRef<class FFriendsAndChatManager>& FriendsAndChatManager
		)
		: FriendItem(FriendItem)
		, FriendsAndChatManager(FriendsAndChatManager)
	{
	}

private:
	TSharedPtr<IFriendItem> FriendItem;
	TWeakPtr<FFriendsAndChatManager> FriendsAndChatManager;

private:
	friend FFriendViewModelFactory;
};

TSharedRef< FFriendViewModel > FFriendViewModelFactory::Create(
	const TSharedRef<IFriendItem>& FriendItem,
	const TSharedRef<class FFriendsAndChatManager>& FriendsAndChatManager
	)
{
	TSharedRef< FFriendViewModelImpl > ViewModel(new FFriendViewModelImpl(FriendItem, FriendsAndChatManager));
	ViewModel->Initialize();
	return ViewModel;
}

#undef LOCTEXT_NAMESPACE
