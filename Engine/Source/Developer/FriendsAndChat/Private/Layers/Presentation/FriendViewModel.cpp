// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "FriendViewModel.h"
#include "FriendsNavigationService.h"

#define LOCTEXT_NAMESPACE "FriendsAndChat"

class FFriendViewModelFactoryFactory;
class FFriendViewModelFactoryImpl;

class FFriendViewModelImpl
	: public FFriendViewModel
{
public:

	virtual void EnumerateActions(TArray<EFriendActionType::Type>& Actions, bool bFromChat = false) override
	{
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
			if (FriendItem->IsOnline() && (FriendsAndChatManager.Pin()->IsInJoinableGameSession() || FriendsAndChatManager.Pin()->IsInJoinableParty()))
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
					if (FriendItem->IsOnline() && (FriendItem->IsGameJoinable() || FriendItem->IsInParty()))
					{
						if(CanPerformAction(EFriendActionType::JoinGame))
						{
							Actions.Add(EFriendActionType::JoinGame);
						}
						else if(FriendsAndChatManager.Pin()->JoinGameAllowed(GetClientId()) && FriendItem->IsInParty())
						{
							Actions.Add(EFriendActionType::JoinGame);
						}
					}
					if (FriendItem->IsOnline() && FriendItem->CanInvite())
					{
						const bool bIsJoinableGame = FriendsAndChatManager.Pin()->IsInJoinableGameSession() && !FriendsAndChatManager.Pin()->IsFriendInSameSession(FriendItem);
						const bool bIsJoinableParty = FriendsAndChatManager.Pin()->IsInJoinableParty() && !FriendsAndChatManager.Pin()->IsFriendInSameParty(FriendItem);
						if (bIsJoinableGame || bIsJoinableParty)
						{
							Actions.Add(EFriendActionType::InviteToGame);
						}
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
				RemoveFriend(ActionType);
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
				if(FriendsAndChatManager.Pin()->JoinGameAllowed(GetClientId()))
				{
					if(FriendItem->IsInParty())
					{
						return FriendItem->CanJoinParty();
					}
					return true;
				}
				return false;
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

	virtual const FString GetNameNoSpaces() const override
	{
		return NameNoSpaces;
	}

	virtual FText GetFriendLocation() const override
	{
		return FriendItem->GetFriendLocation();
	}

	virtual bool IsOnline() const override
	{
		return FriendItem->IsOnline();
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
		return FriendItem->GetOnlineStatus();
	}

	virtual const FString GetClientId() const override
	{
		return FriendItem->GetClientId();
	}

	virtual TSharedRef<IFriendItem> GetFriendItem() const override
	{
		return FriendItem;
	}

private:

	void RemoveFriend(EFriendActionType::Type Reason) const
	{
		FriendsAndChatManager.Pin()->DeleteFriend(FriendItem, Reason);
	}

	void AcceptFriend() const
	{
		FriendsAndChatManager.Pin()->AcceptFriend( FriendItem );
	}

	void SendFriendRequest() const
	{
		FriendsAndChatManager.Pin()->RequestFriend(FText::FromString(FriendItem->GetName()));
	}

	void InviteToGame()
	{
		FriendsAndChatManager.Pin()->SendGameInvite(FriendItem);
	}

	void JoinGame()
	{
		if (FriendItem->IsGameRequest() || FriendItem->IsGameJoinable())
		{
			FriendsAndChatManager.Pin()->AcceptGameInvite(FriendItem);
		}
		else if(FriendsAndChatManager.Pin()->JoinGameAllowed(GetClientId()) && FriendItem->CanJoinParty())
		{
			FriendsAndChatManager.Pin()->AcceptGameInvite(FriendItem);
		}
	}

	void RejectGame()
	{
		if (FriendItem->IsGameRequest())
		{
			FriendsAndChatManager.Pin()->RejectGameInvite(FriendItem);
		}
	}

	void StartChat()
	{
		// Inform navigation system
		NavigationService->SetOutgoingChatFriend(FriendItem);
	}

private:
	void Initialize()
	{
		NameNoSpaces = FriendItem->GetName().Replace(TEXT(" "), TEXT(""));
	}

	void Uninitialize()
	{
	}

	FFriendViewModelImpl(
		const TSharedRef<IFriendItem>& InFriendItem,
		const TSharedRef<FFriendsNavigationService>& InNavigationService,
		const TSharedRef<class FFriendsAndChatManager>& InFriendsAndChatManager
		)
		: FriendItem(InFriendItem)
		, NavigationService(InNavigationService)
		, FriendsAndChatManager(InFriendsAndChatManager)
	{
	}

private:
	FString NameNoSpaces;
	TSharedRef<IFriendItem> FriendItem;
	TSharedRef<FFriendsNavigationService> NavigationService;
	TWeakPtr<FFriendsAndChatManager> FriendsAndChatManager;

private:
	friend FFriendViewModelFactoryImpl;
};

class FFriendViewModelFactoryImpl
	: public IFriendViewModelFactory
{
public:
	virtual TSharedRef<FFriendViewModel> Create(const TSharedRef<IFriendItem>& FriendItem) override
	{
		TSharedRef<FFriendViewModelImpl> ViewModel = MakeShareable(
			new FFriendViewModelImpl(FriendItem, NavigationService, FriendsAndChatManager.Pin().ToSharedRef()));
		ViewModel->Initialize();
		return ViewModel;
	}

	virtual ~FFriendViewModelFactoryImpl(){}

private:

	FFriendViewModelFactoryImpl(
	const TSharedRef<FFriendsNavigationService>& InNavigationService,
	const TSharedRef<FFriendsAndChatManager>& InFriendsAndChatManager)
		: NavigationService(InNavigationService)
		, FriendsAndChatManager(InFriendsAndChatManager)
	{ }

private:

	const TSharedRef<FFriendsNavigationService> NavigationService;
	TWeakPtr<FFriendsAndChatManager> FriendsAndChatManager;
	friend FFriendViewModelFactoryFactory;
};

TSharedRef<IFriendViewModelFactory> FFriendViewModelFactoryFactory::Create(
	const TSharedRef<FFriendsNavigationService>& NavigationService,
	const TSharedRef<class FFriendsAndChatManager>& FriendsAndChatManager)
{
	TSharedRef<FFriendViewModelFactoryImpl> Factory = MakeShareable(
		new FFriendViewModelFactoryImpl(NavigationService, FriendsAndChatManager));

	return Factory;
}

#undef LOCTEXT_NAMESPACE
