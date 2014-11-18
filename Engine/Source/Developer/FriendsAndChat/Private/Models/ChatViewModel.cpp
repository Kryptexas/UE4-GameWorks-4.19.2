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

	virtual FReply HandleSelectionChanged(TSharedRef<FChatItemViewModel> ItemSelected) override
	{
		if(ItemSelected->GetMessageType() == EChatMessageType::Whisper)
		{
			TSharedPtr<FSelectedFriend> NewFriend = FindFriend(ItemSelected->GetMessageItem()->SenderId);
			if(!NewFriend.IsValid())
			{
				NewFriend = MakeShareable(new FSelectedFriend());
				NewFriend->FriendName = ItemSelected->GetFriendID();
				NewFriend->UserID = ItemSelected->GetMessageItem()->SenderId;
				RecentPlayerList.AddUnique(NewFriend);
			}
			SetWhisperChannel(NewFriend);
		}
		else
		{
			SetChatChannel(ItemSelected->GetMessageType());
		}

		return FReply::Handled();
	}

	virtual FText GetViewGroupText() const override
	{
		return EChatMessageType::ToText(SelectedViewChannel);
	}

	virtual FText GetChatGroupText() const override
	{
		return SelectedChatChannel == EChatMessageType::Whisper && SelectedFriend.IsValid() ? SelectedFriend->FriendName : EChatMessageType::ToText(SelectedChatChannel);
	}

	virtual void EnumerateChatChannelOptionsList(TArray<EChatMessageType::Type>& OUTChannelType) override
	{
		OUTChannelType.Add(EChatMessageType::Global);
		OUTChannelType.Add(EChatMessageType::Party);
	}

	virtual void SetChatChannel(const EChatMessageType::Type NewOption) override
	{
		SelectedChatChannel = NewOption;
	}

	virtual void SetWhisperChannel(const TSharedPtr<FSelectedFriend> InFriend) override
	{
		SelectedChatChannel = EChatMessageType::Whisper;
		SelectedFriend = InFriend;
	}

	virtual void SetViewChannel(const EChatMessageType::Type NewOption) override
	{
		SelectedViewChannel = NewOption;
		SelectedChatChannel = NewOption;
		FilterChatList();
	}

	virtual void SendMessage(const FText NewMessage) override
	{
		if(!NewMessage.IsEmpty())
		{
			switch(SelectedChatChannel)
			{
				case EChatMessageType::Whisper:
				{
					if (SelectedFriend.IsValid() && SelectedFriend->UserID.IsValid())
					{
						MessageManager.Pin()->SendPrivateMessage(*SelectedFriend->UserID.Get(), NewMessage.ToString());
						TSharedPtr< FFriendChatMessage > ChatItem = MakeShareable(new FFriendChatMessage());
						ChatItem->FromName = SelectedFriend->FriendName;
						ChatItem->Message = NewMessage;
						ChatItem->MessageType = EChatMessageType::Whisper;
						ChatItem->MessageTimeText = FText::AsTime(FDateTime::UtcNow());
						ChatItem->bIsFromSelf = true;
						ChatItem->SenderId = SelectedFriend->UserID;
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
	
	virtual const TArray<TSharedPtr<FSelectedFriend> >& GetRecentOptions() const override
	{
		return RecentPlayerList;
	}

	virtual void SetChatFriend(TSharedPtr<IFriendItem> ChatFriend) override
	{
		if(ChatFriend.IsValid())
		{
			TSharedPtr<FSelectedFriend> NewFriend = FindFriend(ChatFriend->GetUniqueID());
			if(!NewFriend.IsValid())
			{
				NewFriend = MakeShareable(new FSelectedFriend());
				NewFriend->FriendName = FText::FromString(ChatFriend->GetName());
				NewFriend->UserID = ChatFriend->GetUniqueID();
				RecentPlayerList.AddUnique(NewFriend);
			}
			SetWhisperChannel(NewFriend);
		}
	}

	virtual EVisibility GetSScrollBarVisibility() const override
	{
		return bInGame ? EVisibility::Collapsed : EVisibility::Visible;
	}

	virtual bool IsGlobalChatEnabled() const override
	{
		return bAllowGlobalChat;
	}

	virtual void SetCaptureFocus(bool bInCaptureFocus) override
	{
		bCaptureFocus = bInCaptureFocus;
	}

	virtual const bool ShouldCaptureFocus() const override
	{
		return bCaptureFocus;
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

	TSharedPtr<FSelectedFriend> FindFriend(TSharedPtr<FUniqueNetId> UniqueID)
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

	void HandleMessageReceived(const TSharedRef<FFriendChatMessage> NewMessage)
	{
		ChatLists.Add(FChatItemViewModelFactory::Create(NewMessage, SharedThis(this)));
		FilterChatList();
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
		, bCaptureFocus(false)
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
	TArray<TSharedPtr<FSelectedFriend> > RecentPlayerList;
	TSharedPtr<FSelectedFriend> SelectedFriend;

	EChatMessageType::Type SelectedViewChannel;
	EChatMessageType::Type SelectedChatChannel;
	TWeakPtr<FFriendsMessageManager> MessageManager;
	float TimeDisplayTransaprency;
	FSlateColor OverrideColor;
	bool bUseOverrideColor;
	bool bInGame;
	bool bAllowGlobalChat;
	bool bCaptureFocus;

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