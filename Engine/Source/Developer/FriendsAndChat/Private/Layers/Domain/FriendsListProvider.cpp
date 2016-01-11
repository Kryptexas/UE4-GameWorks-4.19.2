// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "IFriendList.h"
#include "DefaultFriendList.h"
#include "RecentPlayerList.h"
#include "FriendInviteList.h"
#include "ClanMemberList.h"

class FFriendListFactoryFactory;

class FFriendListFactory
	: public IFriendListFactory
{
public:
	virtual TSharedRef<IFriendList> Create(EFriendsDisplayLists::Type ListType) override
	{
		switch(ListType)
		{
			case EFriendsDisplayLists::GameInviteDisplay :
			{
				return FFriendInviteListFactory::Create(FriendViewModelFactory, FriendsAndChatManager.Pin().ToSharedRef());
			}
			break;
			case EFriendsDisplayLists::RecentPlayersDisplay :
			{
				return FRecentPlayerListFactory::Create(FriendViewModelFactory, FriendsAndChatManager.Pin().ToSharedRef());
			}
			break;
		}
		return FDefaultFriendListFactory::Create(ListType, FriendViewModelFactory, FriendsAndChatManager.Pin().ToSharedRef());
	}

	virtual TSharedRef<IFriendList> Create(TSharedRef<IClanInfo> ClanInfo) override
	{
		return FClanMemberListFactory::Create(ClanInfo, FriendViewModelFactory, FriendsAndChatManager.Pin().ToSharedRef());
	}

	virtual ~FFriendListFactory() {}

private:

	FFriendListFactory(
	const TSharedRef<IFriendViewModelFactory>& InFriendViewModelFactory,
	const TSharedRef<FFriendsAndChatManager>& InFriendsAndChatManager)
		: FriendViewModelFactory(InFriendViewModelFactory)
		, FriendsAndChatManager(InFriendsAndChatManager)
	{ }

private:

	const TSharedRef<IFriendViewModelFactory> FriendViewModelFactory;
	TWeakPtr<FFriendsAndChatManager> FriendsAndChatManager;
	friend FFriendListFactoryFactory;
};

TSharedRef<IFriendListFactory> FFriendListFactoryFactory::Create(
	const TSharedRef<IFriendViewModelFactory>& FriendViewModelFactory,
	const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager)
{
	TSharedRef<FFriendListFactory> Factory = MakeShareable(
		new FFriendListFactory(FriendViewModelFactory, FriendsAndChatManager));

	return Factory;
}