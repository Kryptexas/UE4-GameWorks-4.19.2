// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "FriendViewModel.h"
#include "FriendsNavigationService.h"
#include "GameAndPartyService.h"
#include "FriendsService.h"

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
			TSharedPtr<IFriendItem> ExistingFriend = FriendsService->FindUser(*FriendItem->GetUniqueID());
			if (!ExistingFriend.IsValid())
			{
				Actions.Add(EFriendActionType::SendFriendRequest);
			}
			if (FriendItem->IsOnline() && (GamePartyService->IsInJoinableGameSession() || GamePartyService->IsInJoinableParty()))
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
						else if (GamePartyService->JoinGameAllowed(GetClientId()) && FriendItem->IsInParty())
						{
							Actions.Add(EFriendActionType::JoinGame);
						}
					}
					if (FriendItem->IsOnline() && FriendItem->CanInvite())
					{
						const bool bIsJoinableGame = GamePartyService->IsInJoinableGameSession() && !GamePartyService->IsFriendInSameSession(FriendItem);
						const bool bIsJoinableParty = GamePartyService->IsInJoinableParty() && !GamePartyService->IsFriendInSameParty(FriendItem);
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
		bool bIsFriendInSameSession = GamePartyService->IsFriendInSameSession(FriendItem);

		return FriendItem->GetInviteStatus() != EInviteStatus::Accepted
			|| FriendItem->IsGameJoinable()
			|| (GamePartyService->IsInJoinableGameSession() && !bIsFriendInSameSession);
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
				if (GamePartyService->JoinGameAllowed(GetClientId()))
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

		if (GamePartyService->IsInLauncher())
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
		return GamePartyService->IsInGameSession();
	}

	virtual bool IsInActiveParty() const override
	{
		return GamePartyService->IsInActiveParty();
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
		FriendsService->DeleteFriend(FriendItem, Reason);
	}

	void AcceptFriend() const
	{
		FriendsService->AcceptFriend(FriendItem);
	}

	void SendFriendRequest() const
	{
		FriendsService->RequestFriend(FText::FromString(FriendItem->GetName()));
	}

	void InviteToGame()
	{
		GamePartyService->SendGameInvite(FriendItem);
	}

	void JoinGame()
	{
		if (FriendItem->IsGameRequest() || FriendItem->IsGameJoinable())
		{
			GamePartyService->AcceptGameInvite(FriendItem);
		}
		else if (GamePartyService->JoinGameAllowed(GetClientId()) && FriendItem->CanJoinParty())
		{
			GamePartyService->AcceptGameInvite(FriendItem);
		}
	}

	void RejectGame()
	{
		if (FriendItem->IsGameRequest())
		{
			GamePartyService->RejectGameInvite(FriendItem);
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
		const TSharedRef<FFriendsService>& InFriendsService,
		const TSharedRef<FGameAndPartyService>& InGamePartyService
		)
		: FriendItem(InFriendItem)
		, NavigationService(InNavigationService)
		, GamePartyService(InGamePartyService)
		, FriendsService(InFriendsService)
	{
	}

private:
	FString NameNoSpaces;
	TSharedRef<IFriendItem> FriendItem;
	TSharedRef<FFriendsNavigationService> NavigationService;
	TSharedRef<FGameAndPartyService> GamePartyService;
	TSharedRef<FFriendsService> FriendsService;

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
			new FFriendViewModelImpl(FriendItem, NavigationService, FriendsService, GamePartyService));
		ViewModel->Initialize();
		return ViewModel;
	}

	virtual ~FFriendViewModelFactoryImpl(){}

private:

	FFriendViewModelFactoryImpl(
	const TSharedRef<FFriendsNavigationService>& InNavigationService,
	const TSharedRef<FFriendsService>& InFriendsService,
	const TSharedRef<FGameAndPartyService>& InGamePartyService
	)
		: NavigationService(InNavigationService)
		, FriendsService(InFriendsService)
		, GamePartyService(InGamePartyService)
	{ }

private:

	const TSharedRef<FFriendsNavigationService> NavigationService;
	const TSharedRef<FFriendsService> FriendsService;
	const TSharedRef<FGameAndPartyService> GamePartyService;
	friend FFriendViewModelFactoryFactory;
};

TSharedRef<IFriendViewModelFactory> FFriendViewModelFactoryFactory::Create(
	const TSharedRef<FFriendsNavigationService>& NavigationService,
	const TSharedRef<FFriendsService>& FriendsService,
	const TSharedRef<FGameAndPartyService>& GamePartyService
	)
{
	TSharedRef<FFriendViewModelFactoryImpl> Factory = MakeShareable(
		new FFriendViewModelFactoryImpl(NavigationService, FriendsService, GamePartyService));

	return Factory;
}

#undef LOCTEXT_NAMESPACE
