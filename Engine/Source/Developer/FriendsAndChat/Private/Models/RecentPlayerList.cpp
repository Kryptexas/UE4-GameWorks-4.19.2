// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "RecentPlayerList.h"
#include "IFriendItem.h"
#include "FriendViewModel.h"

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
			OutFriendsList.Add(FFriendViewModelFactory::Create(FriendItem.ToSharedRef(), FriendsAndChatManager.Pin().ToSharedRef()));
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

	void HandleChatListUpdated()
	{
		OnFriendsListUpdated().Broadcast();
	}

	void Initialize()
	{
		FriendsAndChatManager.Pin()->OnFriendsListUpdated().AddSP(this, &FRecentPlayerListImpl::HandleChatListUpdated);
	}

	FRecentPlayerListImpl(const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager)
	: FriendsAndChatManager(FriendsAndChatManager)
	{}

private:
	TWeakPtr<FFriendsAndChatManager> FriendsAndChatManager;
	friend FRecentPlayerListFactory;
};

TSharedRef< FRecentPlayerList > FRecentPlayerListFactory::Create(const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager)
{
	TSharedRef< FRecentPlayerListImpl > ChatList(new FRecentPlayerListImpl(FriendsAndChatManager));
	ChatList->Initialize();
	return ChatList;
}