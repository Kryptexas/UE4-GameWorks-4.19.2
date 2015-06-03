// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "IFriendList.h"
#include "DefaultFriendList.h"
#include "RecentPlayerList.h"
#include "FriendInviteList.h"

class FFriendListFactoryFactory;

class FFriendListFactory
	: public IFriendListFactory
{
public:
	TSharedRef<IFriendList> Create(EFriendsDisplayLists::Type ListType)
	{
		switch(ListType)
		{
			case EFriendsDisplayLists::GameInviteDisplay :
			{
				return FFriendInviteListFactory::Create(FriendsAndChatManager.Pin().ToSharedRef());
			}
			break;
			case EFriendsDisplayLists::RecentPlayersDisplay :
			{
				return FRecentPlayerListFactory::Create(FriendsAndChatManager.Pin().ToSharedRef());
			}
			break;
		}
		return FDefaultFriendListFactory::Create(ListType, FriendsAndChatManager.Pin().ToSharedRef());
	}

	virtual ~FFriendListFactory() {}

private:

	FFriendListFactory(const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager)
	: FriendsAndChatManager(FriendsAndChatManager)
	{ }

private:

	TWeakPtr<FFriendsAndChatManager> FriendsAndChatManager;
	friend FFriendListFactoryFactory;
};


TSharedRef<IFriendListFactory> FFriendListFactoryFactory::Create(const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager)
{
	TSharedRef<FFriendListFactory> Factory = MakeShareable(
		new FFriendListFactory(FriendsAndChatManager));

	return Factory;
}