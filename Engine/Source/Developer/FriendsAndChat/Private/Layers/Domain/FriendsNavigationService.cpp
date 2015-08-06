// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "FriendsNavigationService.h"

class FFriendsNavigationServiceImpl
	: public FFriendsNavigationService
{
public:

	virtual void ChangeViewChannel(EChatMessageType::Type ChannelSelected) override
	{
		OnChatViewChanged().Broadcast(ChannelSelected);
	}

	virtual void ChangeChatChannel(EChatMessageType::Type ChannelSelected) override
	{
		OnChatChannelChanged().Broadcast(ChannelSelected);
	}

	virtual void SetOutgoingChatFriend(TSharedRef<IFriendItem> FriendItem) override
	{
		OnChatFriendSelected().Broadcast(FriendItem);
	}

	virtual bool IsInGame() const override
	{
		return InGame;
	}

	DECLARE_DERIVED_EVENT(FFriendsNavigationServiceImpl, FFriendsNavigationService::FViewChangedEvent, FViewChangedEvent);
	virtual FViewChangedEvent& OnChatViewChanged() override
	{
		return ViewChangedEvent;
	}

	DECLARE_DERIVED_EVENT(FFriendsNavigationServiceImpl, FFriendsNavigationService::FChannelChangedEvent, FChannelChangedEvent);
	virtual FChannelChangedEvent& OnChatChannelChanged() override
	{
		return ChannelChangedEvent;
	}

	DECLARE_DERIVED_EVENT(FFriendsNavigationServiceImpl, FFriendsNavigationService::FFriendSelectedEvent, FFriendSelectedEvent);
	virtual FFriendSelectedEvent& OnChatFriendSelected() override
	{
		return FriendSelectedEvent;
	}

private:

	void Initialize()
	{
	}

	FFriendsNavigationServiceImpl(bool InIsInGame)
		: InGame(InIsInGame)
	{}

private:
	FViewChangedEvent ViewChangedEvent;
	FChannelChangedEvent ChannelChangedEvent;
	FFriendSelectedEvent FriendSelectedEvent;
	bool InGame;

	friend FFriendsNavigationServiceFactory;
};

TSharedRef< FFriendsNavigationService > FFriendsNavigationServiceFactory::Create(bool InIsInGame)
{
	TSharedRef< FFriendsNavigationServiceImpl > Service = MakeShareable(new FFriendsNavigationServiceImpl(InIsInGame));
	Service->Initialize();
	return Service;
}