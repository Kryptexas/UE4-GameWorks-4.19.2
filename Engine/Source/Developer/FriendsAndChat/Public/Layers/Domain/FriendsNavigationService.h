// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

namespace EChatMessageType
{
	enum Type : uint8;
}

class FFriendsNavigationService
	: public TSharedFromThis<FFriendsNavigationService>
{
public:

	virtual ~FFriendsNavigationService() {}

	virtual void ChangeViewChannel(EChatMessageType::Type ChannelSelected) = 0;
	virtual void ChangeChatChannel(EChatMessageType::Type ChannelSelected) = 0;
	virtual void SetOutgoingChatFriend(TSharedRef<class IFriendItem> FriendItem) = 0;

	/**
	 * Event broadcast when a navigation even is requested.
	 */
	DECLARE_EVENT_OneParam(FFriendsNavigationService, FViewChangedEvent, EChatMessageType::Type  /* Channel Selected */);
	virtual FViewChangedEvent& OnChatViewChanged() = 0;

	/**
	 * Event broadcast when the outgoing chat channel change.
	 */
	DECLARE_EVENT_OneParam(FFriendsNavigationService, FChannelChangedEvent, EChatMessageType::Type  /* Channel Selected */);
	virtual FChannelChangedEvent& OnChatChannelChanged() = 0;

	/**
	 * Event broadcast when the outgoing chat channel change is requested.
	 */
	DECLARE_EVENT_OneParam(FFriendsNavigationService, FFriendSelectedEvent, TSharedRef<class IFriendItem> /*FriendItem*/);
	virtual FFriendSelectedEvent& OnChatFriendSelected() = 0;

};

FACTORY(TSharedRef<FFriendsNavigationService>, FFriendsNavigationService);
