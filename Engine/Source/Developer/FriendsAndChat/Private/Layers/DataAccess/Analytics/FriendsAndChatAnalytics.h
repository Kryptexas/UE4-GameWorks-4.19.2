// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

class IAnalyticsProvider;

class FFriendsAndChatAnalytics
{
public:

	virtual ~FFriendsAndChatAnalytics() { }

	/**
	 * Update provider to use for capturing events
	 */
	virtual void SetProvider(const TSharedPtr<IAnalyticsProvider>& InAnalyticsProvider) = 0;

	/*
	 * @EventName Social.GameInvite.Accept
	 *
	 * @Trigger Fires whenever a game invite is accepted
	 *
	 * @Type Static
	 *
	 * @EventParam User (string) The local user ID
	 * @EventParam Friend (string) The inviter ID
	 * @EventParam ClientId (string) the client ID
	 * @EventParam Status (string) presence status
	 *
	 * @Comments 
	 *
	 */
	virtual void FireEvent_AcceptGameInvite(const FUniqueNetId& LocalUserId, const FUniqueNetId& ToUser) const = 0;

	/*
	 * @EventName Social.GameInvite.Reject
	 *
	 * @Trigger Fires whenever a game invite is rejected
	 *
	 * @Type Static
	 *
	 * @EventParam User (string) The local user ID
	 * @EventParam Friend (string) The inviter ID
	 * @EventParam ClientId (string) the client ID
	 * @EventParam Status (string) presence status
	 *
	 * @Comments 
	 *
	 */
	virtual void FireEvent_RejectGameInvite(const FUniqueNetId& LocalUserId, const FUniqueNetId& ToUser) const = 0;

	/*
	 * @EventName Social.GameInvite.Send
	 *
	 * @Trigger Fires whenever a game invite is sent
	 *
	 * @Type Static
	 *
	 * @EventParam User (string) The local user ID
	 * @EventParam Friend (string) The invitee ID
	 * @EventParam ClientId (string) the client ID
	 * @EventParam Status (string) presence status
	 *
	 * @Comments 
	 *
	 */
	virtual void FireEvent_SendGameInvite(const FUniqueNetId& LocalUserId, const FUniqueNetId& ToUser) const = 0;

	/*
	 * @EventName Social.AddFriend
	 *
	 * @Trigger Fires whenever we add a friend
	 *
	 * @Type Static
	 *
	 * @EventParam User (string) The local user ID
	 * @EventParam Friend (string) The invitee ID
	 * @EventParam FriendName(string) The friend name
	 * @EventParam Result (string) The find friend result
	 * @EventParam bRecentPlayer (bool) If this was from a recent player list
	 * @EventParam ClientId (string) the client ID
	 * @EventParam Status (string) presence status
	 *
	 * @Comments 
	 *
	 */
	virtual void FireEvent_AddFriend(const FUniqueNetId& LocalUserId, const FString& FriendName, const FUniqueNetId& FriendId, EFindFriendResult::Type Result, bool bRecentPlayer) const = 0;


	/*
	 * @EventName Social.FriendAction.Accept
	 *
	 * @Trigger Fires whenever a user accepts a friend request
	 *
	 * @Type Static
	 *
	 * @EventParam User (string) The local user ID
	 * @EventParam Friend (string) The invitee ID
	 * @EventParam ClientId (string) the client ID
	 * @EventParam Status (string) presence status
	 *
	 * @Comments 
	 *
	 */
	virtual void FireEvent_FriendRequestAccepted(const FUniqueNetId& LocalUserId, const IFriendItem& Friend) const = 0;

	/*
	 * @EventName Social.FriendRemoved
	 *
	 * @Trigger Fires whenever a friend is removed
	 *
	 * @Type Static
	 *
	 * @EventParam User (string) The local user ID
	 * @EventParam Friend (string) The invitee ID
	 * @EventParam RemoveReason (string) The reason a friend was removed
	 * @EventParam ClientId (string) the client ID
	 * @EventParam Status (string) presence status
	 *
	 * @Comments 
	 *
	 */
	virtual void FireEvent_FriendRemoved(const FUniqueNetId& LocalUserId, const IFriendItem& Friend, const FString& RemoveReason) const = 0;

	/*
	 * @EventName Social.Chat.Toggle
	 *
	 * @Trigger Fires whenever toggle global chat on or off
	 *
	 * @Type Static
	 *
	 * @EventParam User (string) The local user ID
	 * @EventParam Channel (string) Channel Opened
	 * @EventParam bEnabled (bool) if enabled
	 * @EventParam ClientId (string) the client ID
	 * @EventParam Status (string) presence status
	 *
	 * @Comments 
	 *
	 */
	virtual void FireEvent_RecordToggleChat(const FUniqueNetId& LocalUserId, const FString& Channel, bool bEnabled) const = 0;

	/**
	 * Record a private chat to a user (aggregates pending FlushChat)
	 */
	virtual void RecordPrivateChat(const FString& ToUser) = 0;
	/**
	 * Record a public chat to a channel (aggregates pending FlushChat)
	 */
	virtual void RecordChannelChat(const FString& ToChannel) = 0;

	/*
	 * @EventName Social.Chat.Counts.2
	 *
	 * @Trigger Fires when the user logs out, and periodically on CHAT_ANALYTICS_INTERVAL. Currently every 5 minutes
	 *
	 * @Type Static
	 *
	 * @EventParam Name(string) the channel or user
	 * @EventParam Type (string) The chat type
	 * @EventParam Count (int) amount of times the event occurred

	 * @Comments 
	 *
	 */
	virtual void FireEvent_FlushChatStats() = 0;
};

FACTORY(TSharedRef<class FFriendsAndChatAnalytics>, FFriendsAndChatAnalytics);
