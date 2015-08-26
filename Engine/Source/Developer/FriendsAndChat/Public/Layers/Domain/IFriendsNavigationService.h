// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class IFriendsNavigationService
{
public:

	virtual ~IFriendsNavigationService() {}

	virtual void SetOutgoingChatFriend(TSharedRef<class IFriendItem> FriendItem) = 0;

	virtual void SetOutgoingChatFriend(const FUniqueNetId& InUserID) = 0;

	/**
	 * Event broadcast when a navigation even is requested.
	 */
	DECLARE_EVENT_OneParam(IFriendsNavigationService, FViewChangedEvent, EChatMessageType::Type  /* Channel Selected */);
	virtual FViewChangedEvent& OnChatViewChanged() = 0;

	/**
	 * Event broadcast when the outgoing chat channel change.
	 */
	DECLARE_EVENT_OneParam(IFriendsNavigationService, FChannelChangedEvent, EChatMessageType::Type  /* Channel Selected */);
	virtual FChannelChangedEvent& OnChatChannelChanged() = 0;

	/**
	 * Event broadcast when the outgoing chat channel change is requested.
	 */
	DECLARE_EVENT_OneParam(IFriendsNavigationService, FFriendSelectedEvent, TSharedRef<class IFriendItem> /*FriendItem*/);
	virtual FFriendSelectedEvent& OnChatFriendSelected() = 0;
};

