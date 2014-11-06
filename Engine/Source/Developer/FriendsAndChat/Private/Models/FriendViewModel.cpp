// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "FriendViewModel.h"

class FFriendViewModelImpl
	: public FFriendViewModel
{
public:

	virtual void EnumerateActions(TArray<EFriendActionType::Type>& Actions)
	{
		if(FriendItem->IsPendingAccepted())
		{
			Actions.Add(EFriendActionType::Updating);
		}
		else
		{
			switch (FriendItem->GetInviteStatus())
			{
				case EInviteStatus::Accepted :
				{
					if(FriendItem->IsOnline())
					{
						Actions.Add(EFriendActionType::JoinGame);
					}

					if( FFriendsAndChatManager::Get()->IsInSession())
					{
						Actions.Add(EFriendActionType::InviteToGame);
					}

					// TODO. Move to user is online. Here for testing until Chat is ready
					Actions.Add(EFriendActionType::Chat);

					Actions.Add(EFriendActionType::RemoveFriend);
				}
				break;
				case EInviteStatus::PendingInbound :
				{
					Actions.Add(EFriendActionType::AcceptFriendRequest);
					Actions.Add(EFriendActionType::RejectFriendRequest);
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

	virtual void PerformAction(const EFriendActionType::Type ActionType)
	{
		switch(ActionType)
		{
			case EFriendActionType::AcceptFriendRequest : AcceptFriend(); break;
			case EFriendActionType::RemoveFriend :
			case EFriendActionType::IgnoreFriendRequest :
			case EFriendActionType::BlockFriend :
			case EFriendActionType::RejectFriendRequest:
			case EFriendActionType::CancelFriendRequest:
			{
				RemoveFriend();
			}
			break;
			case EFriendActionType::SendFriendRequest : SendFriendRequest(); break;
			case EFriendActionType::InviteToGame : InviteToGame(); break;
			case EFriendActionType::JoinGame : JoinGame(); break;
			case EFriendActionType::Chat : StartChat(); break;
		}
	}

	~FFriendViewModelImpl()
	{
		Uninitialize();
	}

	virtual FText GetFriendName() const override
	{
		return FText::FromString(FriendItem->GetName());
	}

	virtual FText GetFriendLocation() const override
	{
		return FriendItem.IsValid() ? FriendItem->GetFriendLocation() : FText::GetEmpty();
	}

	virtual bool IsOnline() const override
	{
		return FriendItem.IsValid() ? FriendItem->IsOnline() : false;
	}

private:

	void RemoveFriend() const
	{
		FFriendsAndChatManager::Get()->DeleteFriend( FriendItem );
	}

	void AcceptFriend() const
	{
		FFriendsAndChatManager::Get()->AcceptFriend( FriendItem );
	}

	void SendFriendRequest() const
	{
		if ( FriendItem.IsValid() && FriendItem->GetOnlineFriend().IsValid() )
		{
			FFriendsAndChatManager::Get()->RequestFriend(FText::FromString(FriendItem->GetName()));
		}
	}

	void InviteToGame()
	{
		if ( FriendItem.IsValid() && FriendItem->GetOnlineFriend().IsValid() )
		{
			//FFriendsMessageManager::Get()->InviteFriendToGame( FriendItem->GetOnlineFriend()->GetUserId() );
		}
	}

	void JoinGame()
	{
		if ( FriendItem.IsValid() && FriendItem->GetOnlineFriend().IsValid() )
		{
			//FFriendsMessageManager::Get()->RequestJoinAGame( FriendItem->GetOnlineFriend()->GetUserId() );
		}
	}

	void StartChat()
	{
		if ( FriendItem.IsValid() && FriendItem->GetOnlineFriend().IsValid() )
		{
			FFriendsAndChatManager::Get()->GenerateChatWindow(FriendItem);
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
		const TSharedRef<FFriendStuct>& FriendItem
		)
		: FriendItem(FriendItem)
	{
	}

private:
	TSharedPtr<FFriendStuct> FriendItem;

private:
	friend FFriendViewModelFactory;
};

TSharedRef< FFriendViewModel > FFriendViewModelFactory::Create(
	const TSharedRef<FFriendStuct>& FriendItem
	)
{
	TSharedRef< FFriendViewModelImpl > ViewModel(new FFriendViewModelImpl(FriendItem));
	ViewModel->Initialize();
	return ViewModel;
}