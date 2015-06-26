// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "FriendsFontStyleService.h"
#include "ChatItemViewModel.h"
#include "ChatViewModel.h"
#include "FriendViewModel.h"
#include "OnlineChatInterface.h"
#include "FriendsNavigationService.h"
#include "FriendsChatMarkupService.h"
#include "ChatTipViewModel.h"
#include "ChatSettingsService.h"

#define LOCTEXT_NAMESPACE ""

class FChatViewModelImpl
	: public FChatViewModel
{
public:

	// Begin FChatViewModel interface
	virtual FText GetChatGroupText(bool ShowWhisperFriendsName) const override
	{
		return SelectedFriend.IsValid() && ShowWhisperFriendsName ? SelectedFriend->DisplayName : EChatMessageType::ToText(GetChatChannelType());
	}

	virtual TArray< TSharedRef<FChatItemViewModel > >& GetMessages() override
	{
		return FilteredMessages;
	}

	virtual int32 GetMessageCount() const override
	{
		return FilteredMessages.Num();
	}

	virtual TSharedPtr<FFriendViewModel> GetFriendViewModel(const TSharedPtr<const FUniqueNetId> UniqueID, const FText Username) override
	{
		return FriendsAndChatManager.Pin()->GetFriendViewModel(UniqueID, Username);
	}

	virtual TSharedPtr<FFriendViewModel> GetFriendViewModel(const FString InUserID, const FText Username) override
	{
		return FriendsAndChatManager.Pin()->GetFriendViewModel(InUserID, Username);
	}

	virtual void SetChannelFlags(uint8 ChatFlags) override
	{
		ChatChannelFlags = ChatFlags;
		bHasActionPending = false;
		SelectedFriend.Reset();
	}

	virtual bool IsChannelSet(const EChatMessageType::Type InChannel) override
	{
		return (InChannel & ChatChannelFlags) != 0;
	}

	virtual void ToggleChannel(const EChatMessageType::Type InChannel) override
	{
		ChatChannelFlags ^= InChannel;
	}

	virtual void SetOutgoingMessageChannel(const EChatMessageType::Type InChannel) override
	{
		OutgoingMessageChannel = InChannel;
		if(InChannel == EChatMessageType::Whisper && IsWhisperFriendSet())
		{
			OutGoingChannelText = FText::Format(LOCTEXT("WhisperChannelText", "{0} {1}"), EChatMessageType::ToText(OutgoingMessageChannel), SelectedFriend->DisplayName);
		}
		else
		{
			OutGoingChannelText = EChatMessageType::ToText(OutgoingMessageChannel);
		}
	}

	virtual EChatMessageType::Type GetOutgoingChatChannel() const override
	{
		return OutgoingMessageChannel;
	}
	
	virtual FText GetOutgoingChannelText() const override
	{
		return OutGoingChannelText;
	}

	virtual TSharedRef<FChatTipViewModel> GetChatTipViewModel() const
	{
		return FChatTipViewModelFactory::Create(MarkupService);
	}

	virtual void SetWhisperFriend(const TSharedPtr<FSelectedFriend> InFriend) override
	{
		SelectedFriend = InFriend;
		if (SelectedFriend->UserID.IsValid())
		{
			SelectedFriend->ViewModel = FriendsAndChatManager.Pin()->GetFriendViewModel(SelectedFriend->UserID, SelectedFriend->DisplayName);
		}
		SelectedFriend->MessageType = EChatMessageType::Whisper;
		bHasActionPending = false;

		// Re set the channel to rebuild the channel text
		if(OutgoingMessageChannel == EChatMessageType::Whisper)
		{
			SetOutgoingMessageChannel(EChatMessageType::Whisper);
		}
	}

	virtual bool IsWhisperFriendSet() const override
	{
		return SelectedFriend.IsValid();
	}

	virtual bool IsChatConnected() const override
	{
		// Are we online
		bool bConnected = FriendsAndChatManager.Pin()->GetOnlineStatus() != EOnlinePresenceState::Offline;

		// Is our friend online
		if (GetChatChannelType() == EChatMessageType::Whisper && SelectedFriend.IsValid())
		{ 
			if(SelectedFriend->ViewModel.IsValid())
			{
				bConnected &= SelectedFriend->ViewModel->IsOnline();
			}
			else
			{
				bConnected = false;
			}
		}
		return bConnected;
	}

	virtual FText GetChatDisconnectText() const override
	{
		if (FriendsAndChatManager.Pin()->GetOnlineStatus() == EOnlinePresenceState::Offline)
		{
			return NSLOCTEXT("ChatViewModel", "LostConnection", "Unable to chat while offline.");
		}
		if (GetChatChannelType() == EChatMessageType::Whisper && SelectedFriend.IsValid())
		{
			if(	SelectedFriend->ViewModel.IsValid() && !SelectedFriend->ViewModel->IsOnline())
			{
				return FText::Format(NSLOCTEXT("ChatViewModel", "FriendOffline", "{0} is now offline."), SelectedFriend->DisplayName);
			}
			else if(SelectedFriend.IsValid())
			{
				return FText::Format(NSLOCTEXT("ChatViewModel", "FriendUnavailable", "Unable to send whisper to {0}."), SelectedFriend->DisplayName);
			}
			else
			{
				return NSLOCTEXT("ChatViewModel", "FriendUnavailable", "Unable to send whisper.");
			}
		}
		return FText::GetEmpty();
	}

	virtual bool SendMessage(const FText NewMessage, const FText PlainText) override
	{
		bool bSuccess = true;
		if (!AllowMarkup() || !MarkupService->ValidateSlashMarkup(NewMessage.ToString(), PlainText.ToString()))
		{
			if(!PlainText.IsEmptyOrWhitespace())
			{
				if(!PlainText.IsEmpty())
				{
					switch (OutgoingMessageChannel)
					{
						case EChatMessageType::Whisper:
						{
							if (SelectedFriend.IsValid() && SelectedFriend->UserID.IsValid())
							{
								if (MessageManager.Pin()->SendPrivateMessage(SelectedFriend->UserID, PlainText))
								{
									bSuccess = true;
								}
							}
						}
						break;
						case EChatMessageType::Party:
						{
							TSharedPtr<const FOnlinePartyId> PartyRoomId = FriendsAndChatManager.Pin()->GetPartyChatRoomId();
							if (FriendsAndChatManager.Pin()->IsInActiveParty()
								&& PartyRoomId.IsValid())
							{
								//@todo will need to support multiple party channels eventually, hardcoded to first party for now
								bSuccess = MessageManager.Pin()->SendRoomMessage((*PartyRoomId).ToString(), NewMessage.ToString());

								FriendsAndChatManager.Pin()->GetAnalytics().RecordChannelChat(TEXT("Party"));
							}
						}
						break;
						case EChatMessageType::Global:
						{
								//@todo samz - send message to specific room (empty room name will send to all rooms)
								bSuccess = MessageManager.Pin()->SendRoomMessage(FString(), PlainText.ToString());
								FriendsAndChatManager.Pin()->GetAnalytics().RecordChannelChat(TEXT("Global"));
						}
					}
				}
			}
		}

		ChatDisplayService->MessageCommitted();
		MarkupService->CloseChatTips();

		return bSuccess;
	}

	virtual EChatMessageType::Type GetChatChannelType() const override
	{
		if ((ChatChannelFlags ^ EChatMessageType::Global) == 0)
		{
			return EChatMessageType::Global;
		}
		if ((ChatChannelFlags ^ EChatMessageType::Game) == 0)
		{
			return EChatMessageType::Game;
		}
		if ((ChatChannelFlags ^ EChatMessageType::Whisper) == 0)
		{
			return EChatMessageType::Whisper;
		}
		return EChatMessageType::Custom;
	}
	// End FChatViewModel interface

	/*
	 * Set Selected Friend that we want outgoing messages to whisper to
	 * @param Username Friend Name
	 * @param UniqueID Friends Network ID
	 */
	void SetWhisperFriend(const FText Username, TSharedPtr<const FUniqueNetId> UniqueID)
	{
		TSharedPtr<FSelectedFriend> NewFriend = FindFriend(UniqueID);
		if (!NewFriend.IsValid())
		{
			NewFriend = MakeShareable(new FSelectedFriend());
			NewFriend->DisplayName = Username;
			NewFriend->UserID = UniqueID;
			SetWhisperFriend(NewFriend);
		}
	}

	/*
	 * Get Selected whisper friend
	 * @return FSelectedFriend item
	 */
	const TSharedPtr<FSelectedFriend> GetWhisperFriend() const
	{
		return SelectedFriend;
	}

protected:

	/*
	* Filter received message to see if this chat should display it
	* @param Message The message to filter
	* @return True if passed filter check
	*/
	virtual bool FilterMessage(const TSharedRef<FFriendChatMessage>& Message)
	{
		bool Changed = false;

		// Set outgoing whisper recipient if not set
		if (GetChatChannelType() == EChatMessageType::Whisper && Message->MessageType == EChatMessageType::Whisper && !SelectedFriend.IsValid())
		{
			if (Message->bIsFromSelf)
			{
				SetWhisperFriend(Message->ToName, Message->RecipientId);
			}
			else
			{ 
				SetWhisperFriend(Message->FromName, Message->SenderId);
			}
		}

		// Always show an outgoing message on the channel it was sent from
		if (Message->bIsFromSelf && IsActive())
		{
			AggregateMessage(Message);
			Changed = true;
		}
		else if (IsChannelSet(Message->MessageType))
		{
			AggregateMessage(Message);
			Changed = true;
		}
		return Changed;
	}

	/*
	 * Add Message to chat
	 * @param Message The message to filter
	 */
	void AggregateMessage(const TSharedRef<FFriendChatMessage>& Message)
	{
		FilteredMessages.Add(FChatItemViewModelFactory::Create(Message));
	}

	/*
	 * Check for new messages
	 */
	void RefreshMessages() override
	{
		const TArray<TSharedRef<FFriendChatMessage>>& Messages = MessageManager.Pin()->GetMessages();
		FilteredMessages.Empty();

		bool AddedItem = false;
		for (const auto& Message : Messages)
		{
			AddedItem |= FilterMessage(Message);
		}

		if (AddedItem)
		{
			OnChatListUpdated().Broadcast();
		}
	}

private:	

	/*
	 * Find a friend
	 * @param UniqueID Friends Network ID
	 * @return FSelectedFriend item
	 */
	TSharedPtr<FSelectedFriend> FindFriend(TSharedPtr<const FUniqueNetId> UniqueID)
	{
		// ToDo - Make this nicer NickD
		for( const auto& ExistingFriend : RecentPlayerList)
		{
			if(ExistingFriend->UserID == UniqueID)
			{
				return	ExistingFriend;
			}
		}
		return nullptr;
	}

	/*
	 * Deal with new message arriving
	 * @param NewMessage Message
	 */
	void HandleMessageReceived(const TSharedRef<FFriendChatMessage> NewMessage)
	{
		if (FilterMessage(NewMessage))
		{
			OnChatListUpdated().Broadcast();
		}
	}

	void HandleChatFriendSelected(TSharedRef<IFriendItem> ChatFriend)
	{
		SetWhisperFriend(FText::FromString(ChatFriend->GetName()), ChatFriend->GetUniqueID());
		OnChatListUpdated().Broadcast();
	}

	void HandleViewChangedEvent(EChatMessageType::Type NewChannel)
	{
		// Clear Chat Text here after navigation
	}

	void HandleChatInputUpdated()
	{
		OnTextValidated().Broadcast();
	}

	/*
	 * Get a Friend
	 * @param Username Friends Name
	 * @param UniqueID Friends Network ID
	 * @return FSelectedFriend item
	 */
	TSharedRef<FSelectedFriend> GetRecentFriend(const FText Username, TSharedPtr<const FUniqueNetId> UniqueID)
	{
		TSharedPtr<FSelectedFriend> NewFriend = FindFriend(UniqueID);
		if (!NewFriend.IsValid())
		{
			NewFriend = MakeShareable(new FSelectedFriend());
			NewFriend->DisplayName = Username;
			NewFriend->UserID = UniqueID;
			RecentPlayerList.AddUnique(NewFriend);
		}
		return NewFriend.ToSharedRef();
	}

	/*
	 * Fill in View model for current Selected friend
	 */
	void UpdateSelectedFriendsViewModel()
	{
		if( SelectedFriend.IsValid())
		{
			if(SelectedFriend->UserID.IsValid())
			{
				SelectedFriend->ViewModel = FriendsAndChatManager.Pin()->GetFriendViewModel(SelectedFriend->UserID, SelectedFriend->DisplayName);
				bHasActionPending = false;
			}
			else if(SelectedFriend->SelectedMessage.IsValid() && SelectedFriend->SelectedMessage->GetSenderID().IsValid())
			{
				SelectedFriend->ViewModel = FriendsAndChatManager.Pin()->GetFriendViewModel(SelectedFriend->SelectedMessage->GetSenderID(), SelectedFriend->SelectedMessage->GetSenderName());
				bHasActionPending = false;
			}
		}
	}

	
public:

	// Begin ChatDisplayOptionsViewModel interface
	virtual void SetCaptureFocus(bool bInCaptureFocus) override
	{
		bCaptureFocus = bInCaptureFocus;
	}

	virtual void SetCurve(FCurveHandle InFadeCurve) override
	{
		FadeCurve = InFadeCurve;
	}

	virtual bool ShouldCaptureFocus() const override
	{
		return bCaptureFocus;
	}

	virtual float GetTimeTransparency() const override
	{
		return FadeCurve.GetLerp();
	}

	virtual EVisibility GetTextEntryVisibility() const override
	{
		return ChatDisplayService->GetEntryBarVisibility();
	}

	virtual EVisibility GetBackgroundVisibility() const override
	{
		return EVisibility::Visible;
	}

	virtual EVisibility GetTipVisibility() const override
	{
		return MarkupService->ChatTipAvailable() ? EVisibility::Visible : EVisibility::Collapsed;
	}

	virtual EVisibility GetChatListVisibility() const override
	{
		if (bAllowFade)
		{
			return ChatDisplayService->GetChatListVisibility();
		}
		return EVisibility::Visible;
	}

	virtual void ToggleChatTipVisibility() override
	{
		MarkupService->ToggleDisplayChatTips();
	}

	virtual void ValidateChatInput(const FText Message, const FText PlainText)
	{
		if (AllowMarkup())
		{
			MarkupService->SetInputText(Message.ToString(), PlainText.ToString());
			if (!Message.IsEmpty())
			{
				ChatDisplayService->ChatEntered();
			}
		}
	}

	virtual FText GetValidatedInput()
	{
		return MarkupService->GetValidatedInput();
	}

	virtual void SetIsActive(bool InIsActive)
	{
		bIsActive = InIsActive;
	}

	virtual void SetInParty(bool bInPartySetting) override
	{
		// ToDo NickD - Move this to display services
// 		if (bInParty != bInPartySetting)
// 		{
// 			if (bInPartySetting)
// 			{
// 				// Entered a party.  Change the selected chat channel to party if we're not in a game and not whispering
// 				if (SelectedChatChannel != EChatMessageType::Whisper && !bInGame)
// 				{
// 					SetViewChannel(EChatMessageType::Party);
// 				}
// 			}
// 			else
// 			{
// 				// Left a party.  If Party is selected for chat, fall back to global.  Note having party selected implies we're not in a game.
// 				if (SelectedChatChannel == EChatMessageType::Party)
// 				{
// 					SetViewChannel(EChatMessageType::Global);
// 				}
// 			}
// 			bInParty = bInPartySetting;
// 		}
	}

	virtual bool AllowMarkup() override
	{
		return true;
	}
	
	virtual bool MultiChat() override
	{
		return true;
	}

	virtual void SetFocus() override
	{
		ChatDisplayService->SetFocus();
	}

	virtual float GetWindowOpacity() override
	{
		return WindowOpacity;
	}

	virtual bool IsActive() const override
	{
		return bIsActive;
	}

	virtual void EnumerateChatChannelOptionsList(TArray<EChatMessageType::Type>& OUTChannelType) override
	{
		if (bAllowGlobalChat)
		{
			OUTChannelType.Add(EChatMessageType::Global);
		}

		if(SelectedFriend.IsValid())
		{
			OUTChannelType.Add(EChatMessageType::Whisper);
		}
	}

	virtual void SetChatSettingsService(TSharedPtr<IChatSettingsService> InChatSettingsService) override
	{
		ChatSettingsService = InChatSettingsService;
		bAllowFade = ChatSettingsService->GetFlagOption(EChatSettingsType::NeverFadeMessages);
		WindowOpacity = ChatSettingsService->GetSliderValue(EChatSettingsType::WindowOpacity);
		uint8 FontSize = ChatSettingsService->GetRadioOptionIndex(EChatSettingsType::FontSize);
		FFriendsAndChatModuleStyle::GetStyleService()->SetUserFontSize(EChatFontType::Type(FontSize));
		ChatSettingsService->OnChatSettingStateUpdated().AddSP(this, &FChatViewModelImpl::HandleSettingsChanged);
		ChatSettingsService->OnChatSettingRadioStateUpdated().AddSP(this, &FChatViewModelImpl::HandleRadioSettingsChanged);
		ChatSettingsService->OnChatSettingValueUpdated().AddSP(this, &FChatViewModelImpl::HandleValueSettingsChanged);
	}

	virtual void HandleSettingsChanged(EChatSettingsType::Type OptionType, bool NewState)
	{
		if (OptionType == EChatSettingsType::NeverFadeMessages)
		{
			bAllowFade = !NewState;
		}
	}

	virtual void HandleRadioSettingsChanged(EChatSettingsType::Type OptionType, uint8 Index)
	{
		if (OptionType == EChatSettingsType::FontSize)
		{
			FFriendsAndChatModuleStyle::GetStyleService()->SetUserFontSize(EChatFontType::Type(Index));
			OnSettingsUpdated().Broadcast();
		}
	}

	virtual void HandleValueSettingsChanged(EChatSettingsType::Type OptionType, float Value)
	{
		if (OptionType == EChatSettingsType::WindowOpacity)
		{
			WindowOpacity = Value;
		}
	}

	virtual FReply HandleChatKeyEntry(const FKeyEvent& KeyEvent) override
	{
		return AllowMarkup() ? MarkupService->HandleChatKeyEntry(KeyEvent) : FReply::Unhandled();
	}

	DECLARE_DERIVED_EVENT(FChatViewModelImpl, FChatViewModel::FChatListSetFocus, FChatListSetFocus);
	virtual FChatListSetFocus& OnChatListSetFocus() override
	{
		return ChatListSetFocusEvent;
	}

	DECLARE_DERIVED_EVENT(FChatViewModelImpl, FChatViewModel::FChatTextValidatedEvent, FChatTextValidatedEvent);
	virtual FChatTextValidatedEvent& OnTextValidated() override
	{
		return ChatTextValidatedEvent;
	}

	DECLARE_DERIVED_EVENT(FChatViewModelImpl, FChatViewModel::FChatListUpdated, FChatListUpdated)
	virtual FChatListUpdated& OnChatListUpdated() override
	{
		return ChatListUpdatedEvent;
	}

	DECLARE_DERIVED_EVENT(FChatViewModelImpl, FChatViewModel::FChatSettingsUpdated, FChatSettingsUpdated)
	virtual FChatSettingsUpdated& OnSettingsUpdated() override
	{
		return ChatSettingsUpdatedEvent;
	}

	// End ChatDisplayOptionsViewModel Interface

private:

	void HandleChatLisUpdated()
	{
		OnChatListUpdated().Broadcast();
	}

	void HandleSetFocus()
	{
		OnChatListSetFocus().Broadcast();
	}

protected:

	FChatViewModelImpl(const TSharedRef<FFriendsMessageManager>& InMessageManager,
		const TSharedRef<FFriendsNavigationService>& InNavigationService,
		const TSharedRef<FFriendsChatMarkupService>& InMarkupService,
		const TSharedRef<IChatDisplayService>& InChatDisplayService,
		const TSharedRef<FFriendsAndChatManager>& InFriendsAndChatManager)
		: ChatChannelFlags(0)
		, OutgoingMessageChannel(EChatMessageType::Custom)
		, MessageManager(InMessageManager)
		, NavigationService(InNavigationService)
		, MarkupService(InMarkupService)
		, ChatDisplayService(InChatDisplayService)
		, FriendsAndChatManager(InFriendsAndChatManager)
		, bIsActive(false)
		, bInGame(true)
		, bHasActionPending(false)
		, bAllowJoinGame(false)
		, bAllowFade(true)
		, bUseOverrideColor(false)
		, bAllowGlobalChat(true)
		, bCaptureFocus(false)
		, WindowOpacity(1.0f)
	{
	}

	void Initialize(bool bBindNavService)
	{
		FriendsAndChatManager.Pin()->OnFriendsListUpdated().AddSP(this, &FChatViewModelImpl::UpdateSelectedFriendsViewModel);
		if (bBindNavService)
		{
			NavigationService->OnChatViewChanged().AddSP(this, &FChatViewModelImpl::HandleViewChangedEvent);
			NavigationService->OnChatFriendSelected().AddSP(this, &FChatViewModelImpl::HandleChatFriendSelected);
		}
		MessageManager.Pin()->OnChatMessageRecieved().AddSP(this, &FChatViewModelImpl::HandleMessageReceived);
		ChatDisplayService->OnChatListSetFocus().AddSP(this, &FChatViewModelImpl::HandleSetFocus);
		MarkupService->OnValidateInputReady().AddSP(this, &FChatViewModelImpl::HandleChatInputUpdated);
		RefreshMessages();
	}

private:

	// The Channels we are subscribed too
	uint8 ChatChannelFlags;

	// The Current outgoing channel we are sending messages on
	EChatMessageType::Type OutgoingMessageChannel;

	TWeakPtr<FFriendsMessageManager> MessageManager;
	TSharedRef<FFriendsNavigationService> NavigationService;
	TSharedRef<FFriendsChatMarkupService> MarkupService;
	TSharedRef<IChatDisplayService> ChatDisplayService;
	TWeakPtr<FFriendsAndChatManager> FriendsAndChatManager;
	TSharedPtr<IChatSettingsService> ChatSettingsService;

	bool bIsActive;
	bool bInGame;
	bool bInParty;
	bool bHasActionPending;
	bool bAllowJoinGame;

	TArray<TSharedRef<FChatItemViewModel> > FilteredMessages;
	TArray<TSharedPtr<FSelectedFriend> > RecentPlayerList;
	TSharedPtr<FSelectedFriend> SelectedFriend;

	// Display Options
	bool bAllowFade;
	bool bUseOverrideColor;
	bool bAllowGlobalChat;
	bool bCaptureFocus;
	bool bDisplayChatTip;
	float WindowOpacity;

	FText OutGoingChannelText;
	// Holds the fade in curve
	FCurveHandle FadeCurve;
	FChatListSetFocus ChatListSetFocusEvent;
	FChatTextValidatedEvent ChatTextValidatedEvent;
	FChatListUpdated ChatListUpdatedEvent;
	FChatSettingsUpdated ChatSettingsUpdatedEvent;

	EVisibility ChatEntryVisibility;
	FSlateColor OverrideColor;

private:
	friend FChatViewModelFactory;
};


// Windowed Mode Impl
class FWindowedChatViewModelImpl : public FChatViewModelImpl
{
private:

	FWindowedChatViewModelImpl(const TSharedRef<FFriendsMessageManager>& MessageManager,
			const TSharedRef<FFriendsNavigationService>& NavigationService,
			const TSharedRef<FFriendsChatMarkupService>& MarkupService,
			const TSharedRef<IChatDisplayService>& ChatDisplayService,
			const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager)
		: FChatViewModelImpl(MessageManager, NavigationService, MarkupService, ChatDisplayService, FriendsAndChatManager)
	{
	}

	// Being FChatViewModelImpl 
	virtual bool FilterMessage(const TSharedRef<FFriendChatMessage>& Message) override
	{
		const TSharedPtr<FSelectedFriend> WhisperFriend = GetWhisperFriend();
		bool Changed = false;

		// Always show an outgoing message on the channel it was sent from
		if (Message->bIsFromSelf && IsActive())
		{
			AggregateMessage(Message);
			Changed = true;
		}
		else if (GetChatChannelType() == EChatMessageType::Whisper && WhisperFriend.IsValid())
		{
			// Make sure whisper is for this window
			if ((Message->bIsFromSelf && Message->RecipientId == WhisperFriend->UserID) || Message->SenderId == WhisperFriend->UserID)
			{
				AggregateMessage(Message);
				Changed = true;
			}
		}
		else
		{
			if (IsChannelSet(Message->MessageType))
			{
				AggregateMessage(Message);
				Changed = true;
			}
		}
		return Changed;
	}

	virtual bool AllowMarkup() override
	{
		return false;
	}

	virtual bool MultiChat() override
	{
		return false;
	}
	
	// End FChatViewModelImpl 


	friend FChatViewModelFactory;
};

// Factory
TSharedRef< FChatViewModel > FChatViewModelFactory::Create(
	const TSharedRef<FFriendsMessageManager>& MessageManager,
	const TSharedRef<FFriendsNavigationService>& NavigationService,
	const TSharedRef<FFriendsChatMarkupService>& MarkupService,
	const TSharedRef<IChatDisplayService>& ChatDisplayService,
	const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager,
	EChatViewModelType::Type ChatType)
{
	if (ChatType == EChatViewModelType::Windowed)
	{
		TSharedRef< FWindowedChatViewModelImpl > ViewModel(new FWindowedChatViewModelImpl(MessageManager, NavigationService, MarkupService, ChatDisplayService, FriendsAndChatManager));
		ViewModel->Initialize(false);
		return ViewModel;
	}
	TSharedRef< FChatViewModelImpl > ViewModel(new FChatViewModelImpl(MessageManager, NavigationService, MarkupService, ChatDisplayService, FriendsAndChatManager));
	ViewModel->Initialize(true);
	return ViewModel;
}

#undef LOCTEXT_NAMESPACE