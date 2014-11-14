// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "ChatItemViewModel.h"
#include "ChatViewModel.h"

class FChatViewModelImpl
	: public FChatViewModel
{
public:

	virtual void SetFocus() override
	{
		OnChatListSetFocus().Broadcast();
	}

	virtual void SetEntryBarVisibility(EVisibility Visibility)
	{
		ChatEntryVisibility = Visibility;
	}

	virtual EVisibility GetEntryBarVisibility() const
	{
		return ChatEntryVisibility;
	}

	virtual void SetFontOverrideColor(FSlateColor InOverrideColor) override
	{
		OverrideColor = InOverrideColor;
	}

	virtual void SetOverrideColorActive(bool bSet) override
	{
		bUseOverrideColor = bSet;
	}

	virtual bool GetOverrideColorSet() override
	{
		return bUseOverrideColor;
	}

	virtual FSlateColor GetFontOverrideColor() const override
	{
		return OverrideColor;
	}

	virtual TArray< TSharedRef<class FChatItemViewModel > >& GetFilteredChatList() override
	{
		return FilteredChatLists;
	}

	virtual void SetInGameUI(bool bIsInGame) override
	{
		bInGame = bIsInGame;
		if(bIsInGame)
		{
			SetViewChannel(EChatMessageType::Party);
			SetAllowGlobalChat(false);
		}
		FilterChatList();
	}

	virtual FReply  HandleSelectionChanged(TSharedRef<FChatItemViewModel> ItemSelected) override
	{
		SetChatChannel(ItemSelected->GetMessageType(), ItemSelected->GetFriendID().ToString());
		if(SelectedChatChannel == EChatMessageType::Whisper)
		{
			RecentPlayerList.AddUnique(ItemSelected->GetFriendID().ToString());
		}

		return FReply::Handled();
	}

	virtual FText GetViewGroupText() const override
	{
		return EChatMessageType::ToText(SelectedViewChannel);
	}

	virtual FText GetChatGroupText() const override
	{
		return SelectedChatChannel == EChatMessageType::Whisper && SelectedFriend.IsValid() ? FText::FromString(SelectedFriend->GetName()) : EChatMessageType::ToText(SelectedChatChannel);
	}

	virtual void EnumerateChatChannelOptionsList(TArray<EChatMessageType::Type>& OUTChannelType) override
	{
		OUTChannelType.Add(EChatMessageType::Global);
		OUTChannelType.Add(EChatMessageType::Party);
	}

	virtual void SetChatChannel(const EChatMessageType::Type NewOption, FString InSelectedFriend) override
	{
		// To Do - set the chat channel. Disabled for now
//		SelectedFriend = FText::FromString(InSelectedFriend);
		SelectedChatChannel = NewOption;
	}

	virtual void SetViewChannel(const EChatMessageType::Type NewOption) override
	{
		SelectedViewChannel = NewOption;
		SelectedChatChannel = NewOption;
		FilterChatList();
	}

	virtual void SendMessage(const FText NewMessage) override
	{
		switch(SelectedChatChannel)
		{
			case EChatMessageType::Whisper:
			{
				if (SelectedFriend.IsValid())
				{
					MessageManager.Pin()->SendPrivateMessage(SelectedFriend->GetUniqueID().Get(), NewMessage.ToString());
					TSharedPtr< FFriendChatMessage > ChatItem = MakeShareable(new FFriendChatMessage());
					if (SelectedFriend.IsValid())
					{
						ChatItem->FromName = FText::FromString(SelectedFriend->GetName());
					}
					else
					{
						ChatItem->FromName = FText::FromString("Unknown");
					}

					ChatItem->Message = NewMessage;
					ChatItem->MessageType = EChatMessageType::Whisper;
					ChatItem->MessageTimeText = FText::AsTime(FDateTime::UtcNow());
					ChatItem->bIsFromSelf = true;
					ChatLists.Add(FChatItemViewModelFactory::Create(ChatItem.ToSharedRef(), SharedThis(this)));
					FilterChatList();
				}
			}
			break;
			case EChatMessageType::Global:
			{
				//@todo samz - send message to specific room (empty room name will send to all rooms)
				MessageManager.Pin()->SendRoomMessage(FString(), NewMessage.ToString());
			}
			break;
			case EChatMessageType::Party:
			{
				OnNewtworkMessageSentEvent().Broadcast(NewMessage.ToString());
			}
			break;
		}

		// Callback to let some UI know to stay active
		OnChatMessageCommitted().Broadcast();
	}

	virtual void SetTimeDisplayTransparency(const float TimeTransparency)
	{
		TimeDisplayTransaprency = TimeTransparency;
	}

	virtual const float GetTimeTransparency() const
	{
		return TimeDisplayTransaprency;
	}

	virtual EChatMessageType::Type GetChatChannelType() const
	{
		return SelectedChatChannel;
	}
	
	virtual const TArray<FString>& GetRecentOptions() const override
	{
		return RecentPlayerList;
	}

	virtual void SetChatFriend(TSharedPtr<IFriendItem> ChatFriend) override
	{
		SelectedFriend = ChatFriend;
		SelectedChatChannel = EChatMessageType::Whisper;
	}

	virtual EVisibility GetSScrollBarVisibility() const override
	{
		return bInGame ? EVisibility::Collapsed : EVisibility::Visible;
	}

	virtual bool IsGlobalChatEnabled() const override
	{
		return bAllowGlobalChat;
	}

	virtual void SetAllowGlobalChat(bool bAllow) override
	{
		bAllowGlobalChat = bAllow;
		FilterChatList();
	}

	DECLARE_DERIVED_EVENT(FChatViewModelImpl , IChatViewModel::FChatListUpdated, FChatListUpdated);
	virtual FChatListUpdated& OnChatListUpdated() override
	{
		return ChatListUpdatedEvent;
	}

	DECLARE_DERIVED_EVENT(FChatViewModelImpl , FChatViewModel::FChatListSetFocus, FChatListSetFocus);
	virtual FChatListSetFocus& OnChatListSetFocus() override
	{
		return ChatListSetFocusEvent;
	}

	DECLARE_DERIVED_EVENT(FChatViewModelImpl, IChatViewModel::FOnFriendsChatMessageCommitted, FOnFriendsChatMessageCommitted)
	virtual FOnFriendsChatMessageCommitted& OnChatMessageCommitted() override
	{
		return ChatMessageCommittedEvent;
	}

	DECLARE_DERIVED_EVENT(FChatViewModelImpl, IChatViewModel::FOnFriendsSendNetworkMessageEvent, FOnFriendsSendNetworkMessageEvent)
	virtual FOnFriendsSendNetworkMessageEvent& OnNewtworkMessageSentEvent() override
	{
		return FriendsSendNetworkMessageEvent;
	}

private:
	void Initialize()
	{
		MessageManager.Pin()->OnChatMessageRecieved().AddSP(this, &FChatViewModelImpl::HandleMessageReceived);
		FilterChatList();
	}

	void FilterChatList()
	{
		if(!IsGlobalChatEnabled())
		{
			FilteredChatLists.Empty();
			for (const auto& ChatItem : ChatLists)
			{
				if(ChatItem->GetMessageType() != EChatMessageType::Global)
				{
					FilteredChatLists.Add(ChatItem);
				}
			}
		}
		else
		{
			FilteredChatLists = ChatLists;
		}
		ChatListUpdatedEvent.Broadcast();
	}

	void HandleMessageReceived(const TSharedRef<FFriendChatMessage> NewMessage)
	{
		if(bAllowGlobalChat || NewMessage->MessageType != EChatMessageType::Global)
		{
			ChatLists.Add(FChatItemViewModelFactory::Create(NewMessage, SharedThis(this)));
			FilterChatList();
		}
	}

	void HandleSetFocus()
	{
		OnChatListSetFocus().Broadcast();
	}

	FChatViewModelImpl(const TSharedRef<FFriendsMessageManager>& MessageManager)
		: SelectedViewChannel(EChatMessageType::Global)
		, SelectedChatChannel(EChatMessageType::Global)
		, MessageManager(MessageManager)
		, TimeDisplayTransaprency(0.f)
		, bUseOverrideColor(false)
		, bInGame(false)
		, bAllowGlobalChat(true)
	{
	}

private:
	TArray<TSharedRef<FChatItemViewModel> > ChatLists;
	TArray<TSharedRef<FChatItemViewModel> > FilteredChatLists;
	FChatListUpdated ChatListUpdatedEvent;
	FChatListSetFocus ChatListSetFocusEvent;
	FOnFriendsChatMessageCommitted ChatMessageCommittedEvent;
	FOnFriendsSendNetworkMessageEvent FriendsSendNetworkMessageEvent;

	EVisibility ChatEntryVisibility;
	TArray<FString> RecentPlayerList;
	TSharedPtr<IFriendItem> SelectedFriend;

	EChatMessageType::Type SelectedViewChannel;
	EChatMessageType::Type SelectedChatChannel;
	TWeakPtr<FFriendsMessageManager> MessageManager;
	float TimeDisplayTransaprency;
	FSlateColor OverrideColor;
	bool bUseOverrideColor;
	bool bInGame;
	bool bAllowGlobalChat;

private:
	friend FChatViewModelFactory;
};

TSharedRef< FChatViewModel > FChatViewModelFactory::Create(
	const TSharedRef<FFriendsMessageManager>& MessageManager
	)
{
	TSharedRef< FChatViewModelImpl > ViewModel(new FChatViewModelImpl(MessageManager));
	ViewModel->Initialize();
	return ViewModel;
}