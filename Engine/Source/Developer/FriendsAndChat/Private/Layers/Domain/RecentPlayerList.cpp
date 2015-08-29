// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "RecentPlayerList.h"
#include "IFriendItem.h"
#include "FriendViewModel.h"
#include "FriendsNavigationService.h"

class FRecentPlayerListImpl
	: public FRecentPlayerList
{
public:

	virtual int32 GetFriendList(TArray< TSharedPtr<FFriendViewModel> >& OutFriendsList) override
	{
		TArray< TSharedPtr< IFriendItem > > FriendItemList = FriendsAndChatManager.Pin()->GetRecentPlayerList();
		FriendItemList.Sort(FCompareGroupByName());
		for(const auto& FriendItem : FriendItemList)
		{
			OutFriendsList.Add(FriendViewModelFactory->Create(FriendItem.ToSharedRef()));
		}
		return 0;
	}

	DECLARE_DERIVED_EVENT(FRecentPlayerList, IFriendList::FFriendsListUpdated, FFriendsListUpdated);
	virtual FFriendsListUpdated& OnFriendsListUpdated() override
	{
		return FriendsListUpdatedEvent;
	}

public:
	FFriendsListUpdated FriendsListUpdatedEvent;

private:

	void Initialize()
	{
		FriendsAndChatManager.Pin()->OnFriendsListUpdated().AddSP(this, &FRecentPlayerListImpl::HandleChatListUpdated);
	}

	void HandleChatListUpdated()
	{
		OnFriendsListUpdated().Broadcast();
	}

	FRecentPlayerListImpl(
	const TSharedRef<IFriendViewModelFactory>& InFriendViewModelFactory,
	const TSharedRef<FFriendsAndChatManager>& InFriendsAndChatManager)
		:FriendViewModelFactory(InFriendViewModelFactory)
		, FriendsAndChatManager(InFriendsAndChatManager)
	{
	}

private:
	TSharedRef<IFriendViewModelFactory> FriendViewModelFactory;
	TWeakPtr<FFriendsAndChatManager> FriendsAndChatManager;
	friend FRecentPlayerListFactory;
};

TSharedRef< FRecentPlayerList > FRecentPlayerListFactory::Create(
	const TSharedRef<IFriendViewModelFactory>& FriendViewModelFactory,
	const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager)
{
	TSharedRef< FRecentPlayerListImpl > ChatList(new FRecentPlayerListImpl(FriendViewModelFactory, FriendsAndChatManager));
	ChatList->Initialize();
	return ChatList;
}