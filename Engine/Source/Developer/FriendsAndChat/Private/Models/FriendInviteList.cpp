// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "FriendInviteList.h"
#include "IFriendItem.h"
#include "FriendViewModel.h"

class FFriendInviteListImpl
	: public FFriendInviteList
{
public:

	virtual int32 GetFriendList(TArray< TSharedPtr<FFriendViewModel> >& OutFriendsList) override
	{
		TArray< TSharedPtr< IFriendItem > > FriendItemList;
		FriendsAndChatManager.Pin()->GetFilteredGameInviteList(FriendItemList);

		FriendItemList.Sort(FCompareGroupByName());
		for(const auto& FriendItem : FriendItemList)
		{
			OutFriendsList.Add(FFriendViewModelFactory::Create(FriendItem.ToSharedRef(), FriendsAndChatManager.Pin().ToSharedRef()));
		}

		return 0;
	}

	DECLARE_DERIVED_EVENT(FFriendInviteList, IFriendList::FFriendsListUpdated, FFriendsListUpdated);
	virtual FFriendsListUpdated& OnFriendsListUpdated() override
	{
		return FriendsListUpdatedEvent;
	}

public:
	FFriendsListUpdated FriendsListUpdatedEvent;

private:
	void Initialize()
	{
		FriendsAndChatManager.Pin()->OnGameInvitesUpdated().AddSP(this, &FFriendInviteListImpl::HandleChatListUpdated);
	}

	void HandleChatListUpdated()
	{
		OnFriendsListUpdated().Broadcast();
	}

	FFriendInviteListImpl(const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager)
		: FriendsAndChatManager(FriendsAndChatManager)
	{}

private:
	TWeakPtr<FFriendsAndChatManager> FriendsAndChatManager;
	friend FFriendInviteListFactory;
};

TSharedRef< FFriendInviteList > FFriendInviteListFactory::Create(const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager)
{
	TSharedRef< FFriendInviteListImpl > ChatList(new FFriendInviteListImpl(FriendsAndChatManager));
	ChatList->Initialize();
	return ChatList;
}