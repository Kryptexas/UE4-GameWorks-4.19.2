// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class IChatTip
{
public:
	virtual FText GetTipText() = 0;
	virtual bool IsEnabled() = 0;
	virtual bool IsValidForType(EChatMessageType::Type ChatChannel) = 0;
	virtual FReply ExecuteTip() = 0;
	virtual void ExecuteCommand() = 0;
};

class FFriendsChatMarkupService
	: public TSharedFromThis<FFriendsChatMarkupService>
{
public:

	virtual ~FFriendsChatMarkupService() {}

	/**
	 * Add custom slash commands - handled by the owning client.
	 */
	virtual void AddCustomSlashMarkupCommand(TArray<TSharedRef<class ICustomSlashCommand> >& InCustomSlashCommands) = 0;

	/**
	 * Make chat tips unavailable.
	 */
	virtual void CloseChatTips() = 0;

	/**
	 * Validate messages to find slash commands.
	 * @param NewMessage - message to validate
	 * @param PlainText - the unformatted message
	 */
	virtual bool ValidateSlashMarkup(const FString NewMessage, const FString PlainText) = 0;

	/**
	 * Do we have a chat tip available. When a user enters / as the first character of a chat.
	 */
	virtual bool ChatTipAvailable() = 0;

	/**
	 * Receive the chat input entered in a chat window for validation - i.e. find markup options.
	 * @param NewMessage - message to validate
	 * @param PlainText - the unformatted message
	 * @para ForceDisplayTip - force display the tool tip (/) is delayed by default
	 */
	virtual void SetInputText(const FString InputText, const FString PlainText, bool ForceDisplayTip = false) = 0;

	/**
	 * Receive the chat input validated by a chat tip.
	 */
	virtual void SetValidatedText(FString InputText) = 0;

	/**
	 * Return a validated version of the string entered by the user..
	 */
	virtual FText GetValidatedInput() = 0;

	/**
	 * Set the selected friend
	 */
	 virtual void SetSelectedFriend(TSharedPtr<class FFriendViewModel> FriendViewModel) = 0;

	/**
	 * Navigate to channel
	 */
	virtual void NavigateToChannel(EChatMessageType::Type ChatChannel) = 0;

	/**
	 * Process keyboard input for tip actions
	 */
	virtual FReply HandleChatKeyEntry(const FKeyEvent& KeyEvent) = 0;

	/**
	 * Get a list of chat tips e.g. Global. Displayed when the user enters / into the chat box.
	 * @return the array of chat tips.
	 */
	virtual TArray<TSharedRef<IChatTip> >& GetChatTips() = 0;

	/**
	 * return the selected active tip.
	 */
	virtual TSharedPtr<IChatTip> GetActiveTip() = 0;

	/**
	 * the selected markup channel.
	 */
	virtual EChatMessageType::Type GetMarkupChannel() const = 0;

	/**
	 * Event broadcast when a user enters some text.
	 */
	DECLARE_EVENT(FFriendsChatMarkupService, FChatInputUpdated);
	virtual FChatInputUpdated& OnInputUpdated() = 0;

	/**
	 * Event broadcast when a chat tip is selected.
	 */
	DECLARE_EVENT_OneParam(FFriendsChatMarkupService, FChatTipSelected, TSharedRef<IChatTip> /* Chat Tip Selected */);
	virtual FChatTipSelected& OnChatTipSelected() = 0;

	/**
	 * Event broadcast when a validated text entry is ready.
	 */
	DECLARE_EVENT(FFriendsChatMarkupService, FValidatedChatReadyEvent);
	virtual FValidatedChatReadyEvent& OnValidateInputReady() = 0;

	/**
	 * Event broadcast when a message is sent.
	 */
	DECLARE_EVENT(FFriendsChatMarkupService, FChatMessageCommitted)
	virtual FChatMessageCommitted& OnMessageCommitted() = 0;

	/**
	 * Event broadcast when we want to send a network message.
	 */
	DECLARE_EVENT_OneParam(FFriendsChatMarkupService, FSendNetworkMessageEvent, const FString& /*message*/)
	virtual FSendNetworkMessageEvent& OnSendNetworkMessageEvent() = 0;

};

IFACTORY(TSharedRef< FFriendsChatMarkupService >, IFriendsChatMarkupService);

FACTORY(TSharedRef<IFriendsChatMarkupServiceFactory>, FFriendsChatMarkupServiceFactory, 
		const TSharedRef<class IChatCommunicationService >& CommunicationService,
		const TSharedRef<class FFriendsNavigationService>& NavigationService,
		const TSharedRef<IFriendListFactory>& FriendsListFactory,
		const TSharedRef<class FGameAndPartyService>& InGamePartyService);
