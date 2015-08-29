// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
/*=============================================================================
	IChatDisplayService.h: Interface for Chat display services.
	The chat display service is used to control a group of chat windows display.
	e.g. Setting chat entry bar to be hidden, and register to listen for list refreshes
=============================================================================*/

#pragma once

class IChatDisplayService
{
public:

	/*
	 * Set the widget to have focus ready for chat input
	 */
	virtual void SetFocus() = 0;

	/*
	 * Notify chat entered - keep entry box visible
	 */
	virtual void ChatEntered() = 0;

	/*
	 * Notify when a message is committed so a consumer can perform some action - e.g. play a sound
	 */
	virtual void MessageCommitted() = 0;

	/*
	 * Get Visibility of the Entry Bar
	 */
	virtual EVisibility GetEntryBarVisibility() const = 0;

	/*
	 * Get Visibility of the chat list
	 */
	virtual EVisibility GetChatListVisibility() const = 0;

	/*
	 * Will this display fade.
	 */
	virtual bool IsFading() const = 0;

	/*
	 * Set if this widget is active - visible and receiving input
	 */
	virtual void SetActive(bool bIsActive) = 0;

	/*
	 * Is this chat widget active
	 */
	virtual bool IsActive() const = 0;


	DECLARE_EVENT(IChatDisplayService, FOnFriendsChatMessageCommitted)
	virtual FOnFriendsChatMessageCommitted& OnChatMessageCommitted() = 0;

	DECLARE_EVENT_OneParam(IChatDisplayService, FOnFriendsSendNetworkMessageEvent, /*struct*/ const FString& /* the message */)
	virtual FOnFriendsSendNetworkMessageEvent& OnNetworkMessageSentEvent() = 0;

	DECLARE_EVENT(IChatDisplayService, FChatListUpdated)
	virtual FChatListUpdated& OnChatListUpdated() = 0;

	DECLARE_EVENT(IChatDisplayService, FChatListSetFocus)
	virtual FChatListSetFocus& OnChatListSetFocus() = 0;
};
