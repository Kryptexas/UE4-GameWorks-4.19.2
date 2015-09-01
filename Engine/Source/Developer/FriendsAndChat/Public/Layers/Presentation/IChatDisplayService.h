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
	 * Get Visibility of the Chat Header Bar (Tabs)
	 */
	virtual EVisibility GetChatHeaderVisibiliy() const = 0;

	/*
	 * Get the chat window visibility
	 */
	virtual EVisibility GetChatWindowVisibiliy() const = 0;

	/*
	 * Get the minimized chat window visibility
	 */
	virtual EVisibility GetChatMinimizedVisibility() const = 0;

	/*
	 * Get Visibility of the chat list
	 */
	virtual EVisibility GetChatListVisibility() const = 0;

	/*
	 * Sets if background fading is enabled
	 */
	virtual void SetFadeBackgroundEnabled(bool bEnabled) = 0;

	/*
	 * Returns True if background fading is enabled at all
	 */
	virtual bool IsFadeBackgroundEnabled() const = 0;

	/*
	 * Get Visibility of the chat list background
	 */
	virtual EVisibility GetBackgroundVisibility() const = 0;

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

	/*
	 * Returns True if chat minimizing is enabled at all
	 */
	virtual bool IsChatMinimizeEnabled() = 0;

	/*
	 * Returns True if chat auto minimize is enabled.
	 */
	virtual bool IsChatAutoMinimizeEnabled() = 0;

	/*
	 * Toggle chat minimized
	 */
	virtual void ToggleChatMinimized() = 0;

	/*
	 * Returns True if chat window is minimized currently
	 */
	virtual bool IsChatMinimized() const = 0;

	virtual void SetTabVisibilityOverride(EVisibility TabVisibility) = 0;

	virtual void ClearTabVisibilityOverride() = 0;

	virtual void SetEntryVisibilityOverride(EVisibility EntryVisibility) = 0;

	virtual void ClearEntryVisibilityOverride() = 0;

	virtual void SetBackgroundVisibilityOverride(EVisibility BackgroundVisibility) = 0;

	virtual void ClearBackgroundVisibilityOverride() = 0;

	virtual void SetChatListVisibilityOverride(EVisibility ChatVisibility) = 0;

	virtual void ClearChatListVisibilityOverride() = 0;

	virtual void SetActiveTab(TWeakPtr<class FChatViewModel> ActiveTab) = 0;

	virtual void SendGameMessage(const FString& GameMessage) = 0;

	// Should auto release
	virtual bool ShouldAutoRelease() = 0;

	// Set if we should release focus after a message is sent
	virtual void SetAutoReleaseFocus(bool AutoRelease) = 0;

	DECLARE_EVENT(IChatDisplayService, FOnFriendsChatMessageCommitted)
	virtual FOnFriendsChatMessageCommitted& OnChatMessageCommitted() = 0;

	DECLARE_EVENT_OneParam(IChatDisplayService, FOnFriendsSendNetworkMessageEvent, /*struct*/ const FString& /* the message */)
	virtual FOnFriendsSendNetworkMessageEvent& OnNetworkMessageSentEvent() = 0;

	DECLARE_EVENT(IChatDisplayService, FOnFocusReleasedEvent)
	virtual FOnFocusReleasedEvent& OnFocuseReleasedEvent() = 0;

	DECLARE_EVENT(IChatDisplayService, FChatListUpdated)
	virtual FChatListUpdated& OnChatListUpdated() = 0;

	DECLARE_EVENT(IChatDisplayService, FChatListSetFocus)
	virtual FChatListSetFocus& OnChatListSetFocus() = 0;
};
