// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "FriendsNavigationService.h"
#include "FriendsService.h"

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
		if(FriendItem->GetInviteStatus() == EInviteStatus::Accepted)
		{
			ChangeViewChannel(EChatMessageType::Whisper);
			OnChatFriendSelected().Broadcast(FriendItem);
		}
	}

	virtual void SetOutgoingChatFriend(const FUniqueNetId& InUserID) override
	{
		TSharedPtr< IFriendItem > FoundFriend = FriendsService->FindUser(InUserID);
		if(FoundFriend.IsValid())
		{
			SetOutgoingChatFriend(FoundFriend.ToSharedRef());
		}
	}

	virtual bool IsInGame() const override
	{
		return InGame;
	}

	DECLARE_DERIVED_EVENT(FFriendsNavigationServiceImpl, IFriendsNavigationService::FViewChangedEvent, FViewChangedEvent);
	virtual FViewChangedEvent& OnChatViewChanged() override
	{
		return ViewChangedEvent;
	}

	DECLARE_DERIVED_EVENT(FFriendsNavigationServiceImpl, IFriendsNavigationService::FChannelChangedEvent, FChannelChangedEvent);
	virtual FChannelChangedEvent& OnChatChannelChanged() override
	{
		return ChannelChangedEvent;
	}

	DECLARE_DERIVED_EVENT(FFriendsNavigationServiceImpl, IFriendsNavigationService::FFriendSelectedEvent, FFriendSelectedEvent);
	virtual FFriendSelectedEvent& OnChatFriendSelected() override
	{
		return FriendSelectedEvent;
	}

private:

	void Initialize()
	{
	}

	FFriendsNavigationServiceImpl(const TSharedRef<class FFriendsService>& InFriendsService, bool InIsInGame)
		: FriendsService(InFriendsService)
		, InGame(InIsInGame)
	{}

private:
	FViewChangedEvent ViewChangedEvent;
	FChannelChangedEvent ChannelChangedEvent;
	FFriendSelectedEvent FriendSelectedEvent;

	TSharedRef<FFriendsService> FriendsService;
	bool InGame;

	friend FFriendsNavigationServiceFactory;
};

TSharedRef< FFriendsNavigationService > FFriendsNavigationServiceFactory::Create(const TSharedRef<class FFriendsService>& FriendsService, bool InIsInGame)
{
	TSharedRef< FFriendsNavigationServiceImpl > Service = MakeShareable(new FFriendsNavigationServiceImpl(FriendsService, InIsInGame));
	Service->Initialize();
	return Service;
}