// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "ChatItemViewModel.h"
#include "ChatViewModel.h"
#include "OnlineChatInterface.h"

// Holds how many message to display
static const int32 MessageDisplay = 200;

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

		if (OnNetworkMessageSentEvent().IsBound())
		{
			SetViewChannel(EChatMessageType::Party);
		}
		else
		{
			SetViewChannel(EChatMessageType::Global);
		}
		
		SetAllowGlobalChat(!bIsInGame);
		FilterChatList();
	}

	virtual FReply HandleSelectionChanged(TSharedRef<FChatItemViewModel> ItemSelected) override
	{
		if(ItemSelected->GetMessageType() == EChatMessageType::Whisper)
		{
			if(ItemSelected->GetMessageItem()->SenderId.IsValid())
			{
				TSharedPtr< IFriendItem > ExistingFriend = FFriendsAndChatManager::Get()->FindUser(*ItemSelected->GetMessageItem()->SenderId);
				if(ExistingFriend.IsValid() && ExistingFriend->GetInviteStatus() == EInviteStatus::Accepted)
				{
					SetWhisperChannel(GetRecentFriend(ItemSelected->GetFriendName(), ItemSelected->GetMessageItem()->SenderId));
				}
			}
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
		return SelectedFriend.IsValid() ? SelectedFriend->FriendName : EChatMessageType::ToText(SelectedChatChannel);
	}

	virtual void EnumerateChatChannelOptionsList(TArray<EChatMessageType::Type>& OUTChannelType) override
	{
		OUTChannelType.Add(EChatMessageType::Global);
		if (OnNetworkMessageSentEvent().IsBound() && FFriendsAndChatManager::Get()->IsInGameSession())
		{
			OUTChannelType.Add(EChatMessageType::Party);
		}
	}

	virtual void EnumerateFriendOptions(TArray<EFriendActionType::Type>& OUTActionList) override
	{
		// Enumerate actions
		if(SelectedFriend.IsValid() && SelectedFriend->UserID.IsValid())
		{
			if(SelectedFriend->ViewModel.IsValid())
			{
				SelectedFriend->ViewModel->EnumerateActions(OUTActionList, true);
			}
			else
			{
				OUTActionList.Add(EFriendActionType::SendFriendRequest);
			}
		}
		else
		{
			OUTActionList.Add(EFriendActionType::SendFriendRequest);
		}
	}

	virtual void PerformFriendAction(EFriendActionType::Type ActionType) override
	{
		if(SelectedFriend.IsValid())
		{
			if(SelectedFriend->ViewModel.IsValid())
			{
				SelectedFriend->ViewModel->PerformAction(ActionType);
			}
			else if(ActionType == EFriendActionType::SendFriendRequest)
			{
				FFriendsAndChatManager::Get()->RequestFriend(SelectedFriend->FriendName);
				CancelAction();
			}
			else if(ActionType == EFriendActionType::InviteToGame)
			{
				if(SelectedFriend->UserID.IsValid())
				{
					FFriendsAndChatManager::Get()->SendGameInvite(*SelectedFriend->UserID.Get());
				}
				else if(SelectedFriend->SelectedMessage.IsValid() && SelectedFriend->SelectedMessage->MessageRef.IsValid())
				{
					FFriendsAndChatManager::Get()->SendGameInvite(SelectedFriend->SelectedMessage->MessageRef->GetUserId());
				}
			}
		}
	}

	virtual void CancelAction() 
	{
		bHasActionPending = false;
		SelectedFriend.Reset();
	}

	virtual void SetChatChannel(const EChatMessageType::Type NewOption) override
	{
		bHasActionPending = false;
		SelectedChatChannel = NewOption;
		SelectedFriend.Reset();
	}

	virtual void SetWhisperChannel(const TSharedPtr<FSelectedFriend> InFriend) override
	{
		SelectedChatChannel = EChatMessageType::Whisper;
		SelectedFriend = InFriend;
		if(SelectedFriend->UserID.IsValid())
		{
			SelectedFriend->ViewModel = FFriendsAndChatManager::Get()->GetFriendViewModel(*SelectedFriend->UserID.Get());
		}
		SelectedFriend->MessageType = EChatMessageType::Whisper;
		bHasActionPending = false;
	}

	virtual void SetViewChannel(const EChatMessageType::Type NewOption) override
	{
		SelectedViewChannel = NewOption;
		SelectedChatChannel = NewOption;
		FilterChatList();
		bHasActionPending = false;
	}

	virtual void SetChannelUserClicked(const TSharedRef<FFriendChatMessage> ChatItemSelected) override
	{
		bool bFoundUser = false;
		TSharedPtr< IFriendItem > ExistingFriend = NULL;

		if(ChatItemSelected->SenderId.IsValid())
		{
			ExistingFriend = FFriendsAndChatManager::Get()->FindUser(*ChatItemSelected->SenderId.Get());
		}
		else if(ChatItemSelected->MessageRef.IsValid())
		{
			ExistingFriend = FFriendsAndChatManager::Get()->FindUser(ChatItemSelected->MessageRef->GetUserId());
		}

		if(ExistingFriend.IsValid() && ExistingFriend->GetInviteStatus() == EInviteStatus::Accepted)
		{
			bFoundUser = true;
			SetWhisperChannel(GetRecentFriend(FText::FromString(ExistingFriend->GetName()), ExistingFriend->GetUniqueID()));
		}

		if(!bFoundUser)
		{
			SetChatChannel(ChatItemSelected->MessageType);
			SelectedFriend = MakeShareable(new FSelectedFriend());
			SelectedFriend->FriendName = ChatItemSelected->FromName;
			SelectedFriend->MessageType = ChatItemSelected->MessageType;
			SelectedFriend->ViewModel = nullptr;
			SelectedFriend->SelectedMessage = ChatItemSelected;
			bHasActionPending = true;
			bAllowJoinGame = FFriendsAndChatManager::Get()->IsInJoinableGameSession();
		}
	}

	virtual bool SendMessage(const FText NewMessage) override
	{
		bool bSuccess = false;
		if(!NewMessage.IsEmpty())
		{
			switch(SelectedChatChannel)
			{
				case EChatMessageType::Whisper:
				{
					if (SelectedFriend.IsValid() && SelectedFriend->UserID.IsValid())
					{
						TSharedPtr< FFriendChatMessage > ChatItem = MakeShareable(new FFriendChatMessage());
						ChatItem->FromName = SelectedFriend->FriendName;
						ChatItem->Message = NewMessage;
						ChatItem->MessageType = EChatMessageType::Whisper;
						ChatItem->MessageTimeText = FText::AsTime(FDateTime::UtcNow());
						ChatItem->bIsFromSelf = true;
						ChatItem->SenderId = SelectedFriend->UserID;

						if (MessageManager.Pin()->SendPrivateMessage(*SelectedFriend->UserID.Get(), ChatItem))
						{
							bSuccess = true;
						}

						FFriendsAndChatManager::Get()->GetAnalytics().RecordPrivateChat(SelectedFriend->UserID->ToString());
					}
				}
				break;
				case EChatMessageType::Global:
				{
					if (IsGlobalChatEnabled())
					{
						//@todo samz - send message to specific room (empty room name will send to all rooms)
						bSuccess = MessageManager.Pin()->SendRoomMessage(FString(), NewMessage.ToString());

						FFriendsAndChatManager::Get()->GetAnalytics().RecordChannelChat(TEXT("Global"));
					}	
				}
				break;
				case EChatMessageType::Party:
				{
					OnNetworkMessageSentEvent().Broadcast(NewMessage.ToString());
					bSuccess = true;

					FFriendsAndChatManager::Get()->GetAnalytics().RecordChannelChat(TEXT("Party"));
				}
				break;
			}
		}
		// Callback to let some UI know to stay active
		OnChatMessageCommitted().Broadcast();
		return bSuccess;
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

	virtual const EVisibility GetTextEntryVisibility() override
	{
		if(GetEntryBarVisibility() == EVisibility::Visible)
		{
			return bHasActionPending ? EVisibility::Collapsed : EVisibility::Visible;
		}
		return EVisibility::Collapsed;
	}

	virtual const EVisibility GetConfirmationVisibility() override
	{
		if(GetEntryBarVisibility() == EVisibility::Visible)
		{
			return bHasActionPending ? EVisibility::Visible : EVisibility::Collapsed;
		}
		return EVisibility::Collapsed;
	}

	virtual EVisibility GetScrollbarVisibility() const override
	{
		return bInGame ? EVisibility::Collapsed : EVisibility::Visible;
	}

	virtual EVisibility GetInviteToGameVisibility() const override
	{
		return bAllowJoinGame ? EVisibility::Visible : EVisibility::Collapsed;
	}

	virtual bool IsGlobalChatEnabled() const override
	{
		return bAllowGlobalChat;
	}

	virtual const bool IsChatHidden() override
	{
		return FilteredChatLists.Num() == 0 || (bInGame && GetOverrideColorSet());
	}

	virtual bool HasValidSelectedFriend() const override
	{
		return SelectedFriend.IsValid();
	}

	virtual bool HasFriendChatAction() const override
	{
		if(SelectedFriend.IsValid() && SelectedFriend->ViewModel.IsValid())
		{
			return SelectedFriend->ViewModel->HasChatAction();
		}
		return true;
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
	virtual FOnFriendsSendNetworkMessageEvent& OnNetworkMessageSentEvent() override
	{
		return FriendsSendNetworkMessageEvent;
	}

private:
	void Initialize()
	{
		FFriendsAndChatManager::Get()->OnFriendsListUpdated().AddSP(this, &FChatViewModelImpl::UpdateSelectedFriendsViewModel);
		FFriendsAndChatManager::Get()->OnChatFriendSelected().AddSP(this, &FChatViewModelImpl::HandleChatFriendSelected);
		MessageManager.Pin()->OnChatMessageRecieved().AddSP(this, &FChatViewModelImpl::HandleMessageReceived);

		for( const auto& Message : MessageManager.Pin()->GetMessageList())
		{
			HandleMessageReceived(Message.ToSharedRef());
		}
	}

	void FilterChatList()
	{
		bool bAddedItem = true;
		if(!IsGlobalChatEnabled())
		{
			FilteredChatLists.Empty();
			bAddedItem = false;
			for (const auto& ChatItem : ChatLists)
			{
				if(ChatItem->GetMessageType() != EChatMessageType::Global)
				{
					FilteredChatLists.Add(ChatItem);
					bAddedItem = true;
				}
			}
		}
		else
		{
			FilteredChatLists = ChatLists;
		}

		if(bAddedItem)
		{
			ChatListUpdatedEvent.Broadcast();
		}
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
		if(IsGlobalChatEnabled() || NewMessage->MessageType != EChatMessageType::Global)
		{
			ChatLists.Add(FChatItemViewModelFactory::Create(NewMessage, SharedThis(this)));
			if(ChatLists.Num() > MessageDisplay)
			{
				ChatLists.RemoveAt(0);
			}
		}
		FilterChatList();
	}

	void HandleChatFriendSelected(TSharedPtr<IFriendItem> ChatFriend)
	{
		if(ChatFriend.IsValid())
		{
			SetWhisperChannel(GetRecentFriend(FText::FromString(ChatFriend->GetName()), ChatFriend->GetUniqueID()));
		}
	}

	void HandleSetFocus()
	{
		OnChatListSetFocus().Broadcast();
	}

	TSharedRef<FSelectedFriend> GetRecentFriend(const FText Username, TSharedPtr<FUniqueNetId> UniqueID)
	{
		TSharedPtr<FSelectedFriend> NewFriend = FindFriend(UniqueID);
		if (!NewFriend.IsValid())
		{
			NewFriend = MakeShareable(new FSelectedFriend());
			NewFriend->FriendName = Username;
			NewFriend->UserID = UniqueID;
			RecentPlayerList.AddUnique(NewFriend);
		}
		return NewFriend.ToSharedRef();
	}

	void UpdateSelectedFriendsViewModel()
	{
		if( SelectedFriend.IsValid())
		{
			if(SelectedFriend->UserID.IsValid())
			{
				SelectedFriend->ViewModel = FFriendsAndChatManager::Get()->GetFriendViewModel(*SelectedFriend->UserID.Get());
			}
			else if(SelectedFriend->SelectedMessage.IsValid() && SelectedFriend->SelectedMessage->MessageRef.IsValid())
			{
				SelectedFriend->ViewModel = FFriendsAndChatManager::Get()->GetFriendViewModel(SelectedFriend->SelectedMessage->MessageRef->GetUserId());
			}
		}
		bHasActionPending = false;
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
		, bHasActionPending(false)
		, bAllowJoinGame(false)
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
	bool bHasActionPending;
	bool bAllowJoinGame;

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