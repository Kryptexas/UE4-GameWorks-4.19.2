// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "ChatItemViewModel.h"
#include "ChatViewModel.h"
#include "FriendViewModel.h"
#include "OnlineChatInterface.h"

class FChatViewModelImpl
	: public FChatViewModel
{
public:
	virtual TArray< TSharedRef<FChatItemViewModel > >& GetMessages() override
	{
		return FilteredMessages;
	}

	virtual FReply HandleSelectionChanged(TSharedRef<FChatItemViewModel> ItemSelected) override
	{
		if(ItemSelected->GetMessageType() == EChatMessageType::Whisper)
		{
			TSharedPtr<const FUniqueNetId> FriendID = ItemSelected->IsFromSelf() ? ItemSelected->GetRecipientID() : ItemSelected->GetSenderID();
			FText FriendName = ItemSelected->IsFromSelf() ? ItemSelected->GetRecipientName() : ItemSelected->GetSenderName();
			if (FriendID.IsValid())
			{
				TSharedPtr< IFriendItem > ExistingFriend = FriendsAndChatManager.Pin()->FindUser(*FriendID);
				if(ExistingFriend.IsValid() && ExistingFriend->GetInviteStatus() == EInviteStatus::Accepted)
				{
					SetWhisperChannel(GetRecentFriend(FriendName, FriendID));
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
		return SelectedFriend.IsValid() ? SelectedFriend->DisplayName : EChatMessageType::ToText(SelectedChatChannel);
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
				FriendsAndChatManager.Pin()->RequestFriend(SelectedFriend->DisplayName);
				CancelAction();
			}
			else if(ActionType == EFriendActionType::InviteToGame)
			{
				if(SelectedFriend->UserID.IsValid())
				{
					FriendsAndChatManager.Pin()->SendGameInvite(*SelectedFriend->UserID.Get());
				}
				else if(SelectedFriend->SelectedMessage.IsValid() && SelectedFriend->SelectedMessage->GetSenderID().IsValid())
				{
					FriendsAndChatManager.Pin()->SendGameInvite(*SelectedFriend->SelectedMessage->GetSenderID().Get());
				}
			}
			else if (ActionType == EFriendActionType::Whisper)
			{
				TSharedPtr<IFriendItem> FriendItem = FriendsAndChatManager.Pin()->FindUser(*SelectedFriend->UserID);
				if (FriendItem.IsValid())
				{
					FriendsAndChatManager.Pin()->SetChatFriend(FriendItem);
				}				
				CancelAction();
			}
			else if (ActionType == EFriendActionType::AcceptFriendRequest)
			{
				TSharedPtr<IFriendItem> FriendItem = FriendsAndChatManager.Pin()->FindUser(*SelectedFriend->UserID);
				if (FriendItem.IsValid())
				{
					FriendsAndChatManager.Pin()->AcceptFriend(FriendItem);
				}
				CancelAction();
			}
			else if (ActionType == EFriendActionType::IgnoreFriendRequest)
			{
				TSharedPtr<IFriendItem> FriendItem = FriendsAndChatManager.Pin()->FindUser(*SelectedFriend->UserID);
				if (FriendItem.IsValid())
				{
					FriendsAndChatManager.Pin()->DeleteFriend(FriendItem, EFriendActionType::ToText(EFriendActionType::IgnoreFriendRequest).ToString());
				}
				CancelAction();
			}
			else if (ActionType == EFriendActionType::CancelFriendRequest)
			{
				TSharedPtr<IFriendItem> FriendItem = FriendsAndChatManager.Pin()->FindUser(*SelectedFriend->UserID);
				if (FriendItem.IsValid())
				{
					FriendsAndChatManager.Pin()->DeleteFriend(FriendItem, EFriendActionType::ToText(EFriendActionType::CancelFriendRequest).ToString());
				}
				CancelAction();
			}
		}
	}

	virtual void CancelAction() 
	{
		bHasActionPending = false;
		SelectedFriend.Reset();
	}

	virtual void LockChatChannel(bool bLocked) override
	{
		bLockedChannel = bLocked;
		RefreshMessages();
	}

	virtual bool IsChatChannelLocked() const override
	{
		return bLockedChannel;
	}
	
	virtual void SetChatChannel(const EChatMessageType::Type NewOption) override
	{
		if (!bLockedChannel)
		{
			bHasActionPending = false;
			SelectedChatChannel = NewOption;
			if (NewOption == EChatMessageType::Global && !bIsDisplayingGlobalChat)
			{
				SetDisplayGlobalChat(true);
			}
			SelectedFriend.Reset();
		}
	}

	virtual void SetWhisperChannel(const TSharedPtr<FSelectedFriend> InFriend) override
	{
		if (!bLockedChannel)
		{
			SelectedChatChannel = EChatMessageType::Whisper;
			SelectedFriend = InFriend;
			if (SelectedFriend->UserID.IsValid())
			{
				SelectedFriend->ViewModel = FriendsAndChatManager.Pin()->GetFriendViewModel(*SelectedFriend->UserID.Get());
			}
			SelectedFriend->MessageType = EChatMessageType::Whisper;
			bHasActionPending = false;
		}
	}

	virtual void SetViewChannel(const EChatMessageType::Type NewOption) override
	{
		if (!bLockedChannel)
		{
		    SelectedViewChannel = NewOption;
		    SelectedChatChannel = NewOption;
			RefreshMessages();
		    bHasActionPending = false;
	    }
	}

	virtual const EChatMessageType::Type GetChatChannel() const override
	{
		return SelectedChatChannel;
	}

	virtual bool IsChatChannelValid() const override
	{
		return SelectedChatChannel != EChatMessageType::Global || bEnableGlobalChat;
	}

	virtual bool IsChatConnected() const override
	{
		// Are we online
		bool bConnected = FriendsAndChatManager.Pin()->GetOnlineStatus() != EOnlinePresenceState::Offline;

		// Is our friend online
		if (SelectedChatChannel == EChatMessageType::Whisper)
		{ 
			if(SelectedFriend.IsValid() &&
			SelectedFriend->ViewModel.IsValid())
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
		if (SelectedChatChannel == EChatMessageType::Whisper )
		{
			if( SelectedFriend.IsValid() && 
			SelectedFriend->ViewModel.IsValid() &&
			!SelectedFriend->ViewModel->IsOnline())
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

	virtual void SetChannelUserClicked(const TSharedRef<FChatItemViewModel> ChatItemSelected) override
	{
		if(ChatItemSelected->IsFromSelf() && (ChatItemSelected->GetMessageType() == EChatMessageType::Global || ChatItemSelected->GetMessageType() == EChatMessageType::Party))
		{
			SetChatChannel(ChatItemSelected->GetMessageType());
		}
		else
		{
			bool bFoundUser = false;
			TSharedPtr< IFriendItem > ExistingFriend = NULL;

			const TSharedPtr<const FUniqueNetId> ChatID = ChatItemSelected->IsFromSelf() ? ChatItemSelected->GetRecipientID() : ChatItemSelected->GetSenderID();
			if(ChatID.IsValid())
			{
				ExistingFriend = FriendsAndChatManager.Pin()->FindUser(*ChatID.Get());
			}

			if(ExistingFriend.IsValid() && ExistingFriend->GetInviteStatus() == EInviteStatus::Accepted && !bLockedChannel)
			{
				bFoundUser = true;
				SetWhisperChannel(GetRecentFriend(FText::FromString(ExistingFriend->GetName()), ExistingFriend->GetUniqueID()));
			}			
			if (ExistingFriend.IsValid() && ExistingFriend->GetInviteStatus() == EInviteStatus::Accepted && !ExistingFriend->IsOnline() )
			{
				/* Its our friend but they are offline */
				bFoundUser = true;
			}
			if (ExistingFriend.IsValid() && ExistingFriend->GetInviteStatus() == EInviteStatus::Accepted && ExistingFriend->IsOnline() && !bIsDisplayingGlobalChat && bLockedChannel && !bAllowJoinGame)
			{
				/* Its our friend they are online but this is already a whisper chat window with just them and we cant invite them to join our game */
				bFoundUser = true;
			}

			if(!bFoundUser)
			{
				SetChatChannel(ChatItemSelected->GetMessageType());
				SelectedFriend = MakeShareable(new FSelectedFriend());
				SelectedFriend->DisplayName = ChatItemSelected->GetSenderName();
				SelectedFriend->MessageType = ChatItemSelected->GetMessageType();
				SelectedFriend->ViewModel = nullptr;
				SelectedFriend->SelectedMessage = ChatItemSelected;
				if (ExistingFriend.IsValid())
				{
					SelectedFriend->UserID = ExistingFriend->GetUniqueID();
				}
				bHasActionPending = true;
				bAllowJoinGame = FriendsAndChatManager.Pin()->IsInJoinableGameSession();
			}
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
						if (MessageManager.Pin()->SendPrivateMessage(SelectedFriend->UserID, NewMessage))
						{
							bSuccess = true;
						}

						FriendsAndChatManager.Pin()->GetAnalytics().RecordPrivateChat(SelectedFriend->UserID->ToString());
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
					FString GlobalRoomId;
					if (IsDisplayingGlobalChat()
						&& FriendsAndChatManager.Pin()->GetGlobalChatRoomId(GlobalRoomId))
					{
						//@todo will need to support multiple global channels eventually, roomname hardcoded for now
						bSuccess = MessageManager.Pin()->SendRoomMessage(GlobalRoomId, NewMessage.ToString());
						
						FriendsAndChatManager.Pin()->GetAnalytics().RecordChannelChat(TEXT("Global"));
					}
				}
				break;
			}
		}
		return bSuccess;
	}

	virtual EChatMessageType::Type GetChatChannelType() const
	{
		return SelectedChatChannel;
	}
	
	virtual const TArray<TSharedPtr<FSelectedFriend> >& GetRecentOptions() const override
	{
		return RecentPlayerList;
	}

	virtual EVisibility GetFriendRequestVisibility() const override
	{
		if (SelectedFriend.IsValid() && SelectedFriend->UserID.IsValid())
		{
			TSharedPtr<IFriendItem> FriendItem = FriendsAndChatManager.Pin()->FindUser(*SelectedFriend->UserID);
			if (FriendItem.IsValid())
			{
				return EVisibility::Collapsed;
			}
		}
		return EVisibility::Visible;
	}

	virtual EVisibility GetAcceptFriendRequestVisibility() const override
	{
		if (SelectedFriend.IsValid() && SelectedFriend->UserID.IsValid())
		{
			TSharedPtr<IFriendItem> FriendItem = FriendsAndChatManager.Pin()->FindUser(*SelectedFriend->UserID);
			if (FriendItem.IsValid() && 
				FriendItem->GetInviteStatus() == EInviteStatus::PendingInbound && 
				(!FriendItem->IsPendingAccepted() || !FriendItem->IsPendingDelete()))
			{
				return EVisibility::Visible;
			}
		}

		return EVisibility::Collapsed;
	}

	virtual EVisibility GetIgnoreFriendRequestVisibility() const override
	{
		if (SelectedFriend.IsValid() && SelectedFriend->UserID.IsValid())
		{
			TSharedPtr<IFriendItem> FriendItem = FriendsAndChatManager.Pin()->FindUser(*SelectedFriend->UserID);
			if (FriendItem.IsValid() &&
				FriendItem->GetInviteStatus() == EInviteStatus::PendingInbound &&
				(!FriendItem->IsPendingAccepted() || !FriendItem->IsPendingDelete()))
			{
				return EVisibility::Visible;
			}
		}

		return EVisibility::Collapsed;
	}

	virtual EVisibility GetCancelFriendRequestVisibility() const override
	{
		if (SelectedFriend.IsValid() && SelectedFriend->UserID.IsValid())
		{
			TSharedPtr<IFriendItem> FriendItem = FriendsAndChatManager.Pin()->FindUser(*SelectedFriend->UserID);
			if (FriendItem.IsValid() &&
				FriendItem->GetInviteStatus() == EInviteStatus::PendingOutbound &&
				(!FriendItem->IsPendingAccepted() || !FriendItem->IsPendingDelete()))
			{
				return EVisibility::Visible;
			}
		}

		return EVisibility::Collapsed;
	}

	virtual EVisibility GetInviteToGameVisibility() const override
	{
		return bAllowJoinGame ? EVisibility::Visible : EVisibility::Collapsed;
	}

	virtual EVisibility GetOpenWhisperVisibility() const override
	{
		if (bIsDisplayingGlobalChat && SelectedFriend.IsValid() && SelectedFriend->UserID.IsValid())
		{
			TSharedPtr<IFriendItem> FriendItem = FriendsAndChatManager.Pin()->FindUser(*SelectedFriend->UserID);			
			if (FriendItem.IsValid() && FriendItem->GetInviteStatus() == EInviteStatus::Accepted && FriendItem->IsOnline())
			{
				return EVisibility::Visible;
			}
		}
		return EVisibility::Collapsed;
	}

	virtual bool IsDisplayingGlobalChat() const override
	{
		return bEnableGlobalChat && bIsDisplayingGlobalChat;
	}

	virtual bool IsGlobalChatEnabled() const override
	{
		return bEnableGlobalChat;
	}

	virtual bool HasValidSelectedFriend() const override
	{
		return SelectedFriend.IsValid();
	}

	virtual bool HasFriendChatAction() const override
	{
		if (SelectedFriend.IsValid() && 
			SelectedFriend->ViewModel.IsValid())
		{
			return SelectedFriend->ViewModel->HasChatAction();
		}
		return false;
	}

	virtual bool HasActionPending() const override
	{
		return bHasActionPending;
	}

	virtual void SetDisplayGlobalChat(bool bAllow) override
	{
		bIsDisplayingGlobalChat = bAllow;
		TSharedPtr<const FUniqueNetId> LocalUserId = FriendsAndChatManager.Pin()->GetLocalUserId();
		if (LocalUserId.IsValid())
		{
			FriendsAndChatManager.Pin()->GetAnalytics().RecordToggleChat(*LocalUserId, TEXT("Global"), bIsDisplayingGlobalChat, TEXT("Social.Chat.Toggle"));
		}
		
		RefreshMessages();
	}

	virtual void EnableGlobalChat(bool bEnable) override
	{
		bEnableGlobalChat = bEnable;
		RefreshMessages();
	}

	virtual void SetInGame(bool bInGameSetting) override
	{
		if (bInGame != bInGameSetting)
		{
			if (bInGameSetting)
			{
				// Entered a game.  Always change the selected chat channel to game chat
				SetViewChannel(EChatMessageType::Game);
			}
			else
			{
				// Left a game.  Fall back to party or global.
				if (bInParty)
				{
					SetViewChannel(EChatMessageType::Party);
				}
				else
				{
					SetViewChannel(EChatMessageType::Global);
				}
			}
			bInGame = bInGameSetting;
		}
	}

	virtual void SetInParty(bool bInPartySetting) override
	{
		if (bInParty != bInPartySetting)
		{
			if (bInPartySetting)
			{
				// Entered a party.  Change the selected chat channel to party if we're not in a game and not whispering
				if (SelectedChatChannel != EChatMessageType::Whisper && !bInGame)
				{
					SetViewChannel(EChatMessageType::Party);
				}
			}
			else
			{
				// Left a party.  If Party is selected for chat, fall back to global.  Note having party selected implies we're not in a game.
				if (SelectedChatChannel == EChatMessageType::Party)
				{
					SetViewChannel(EChatMessageType::Global);
				}
			}
			bInParty = bInPartySetting;
		}
	}

	DECLARE_DERIVED_EVENT(FChatViewModelImpl , FChatViewModel::FChatListUpdated, FChatListUpdated);
	virtual FChatListUpdated& OnChatListUpdated() override
	{
		return ChatListUpdatedEvent;
	}

private:
	void Initialize()
	{
		FriendsAndChatManager.Pin()->OnFriendsListUpdated().AddSP(this, &FChatViewModelImpl::UpdateSelectedFriendsViewModel);
		FriendsAndChatManager.Pin()->OnChatFriendSelected().AddSP(this, &FChatViewModelImpl::HandleChatFriendSelected);
		MessageManager.Pin()->OnChatMessageRecieved().AddSP(this, &FChatViewModelImpl::HandleMessageReceived);
		RefreshMessages();
	}

	void RefreshMessages()
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

	bool FilterMessage(const TSharedRef<FFriendChatMessage>& Message)
	{
		bool Changed = false;

		if (!IsDisplayingGlobalChat() || bLockedChannel)
		{
			if (Message->MessageType != EChatMessageType::Global)
			{
				// If channel is locked only show chat from current friend
				if (bLockedChannel && SelectedFriend.IsValid() && !bIsDisplayingGlobalChat)
				{
					if ((Message->bIsFromSelf && Message->RecipientId == SelectedFriend->UserID) || Message->SenderId == SelectedFriend->UserID)
					{
						AggregateMessage(Message);
						Changed = true;
					}
				}

				// If its not locked show all chat
				if (!bLockedChannel)
				{
					AggregateMessage(Message);
					Changed = true;
				}
			}
			else
			{
				// If channel is locked only show global chat
				if (bLockedChannel && bIsDisplayingGlobalChat)
				{
					AggregateMessage(Message);
					Changed = true;
				}
			}
		}
		else
		{
			AggregateMessage(Message);
			Changed = true;
		}

		return Changed;
	}

	void AggregateMessage(const TSharedRef<FFriendChatMessage>& Message)
	{
		// Agregate messages have been disabled in Fortnite. This will be replaced with Chat 2.0 Nick Davies
		FilteredMessages.Add(FChatItemViewModelFactory::Create(Message));
	}

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

	void HandleMessageReceived(const TSharedRef<FFriendChatMessage> NewMessage)
	{
		if(IsDisplayingGlobalChat() || NewMessage->MessageType != EChatMessageType::Global)
		{
			if (FilterMessage(NewMessage))
			{
				OnChatListUpdated().Broadcast();
			}
		}
	}

	void HandleChatFriendSelected(TSharedPtr<IFriendItem> ChatFriend)
	{
		if(ChatFriend.IsValid())
		{
			SetWhisperChannel(GetRecentFriend(FText::FromString(ChatFriend->GetName()), ChatFriend->GetUniqueID()));
		}
		else if (bIsDisplayingGlobalChat)
		{
			SetChatChannel(EChatMessageType::Global);
		}
	}

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

	void UpdateSelectedFriendsViewModel()
	{
		if( SelectedFriend.IsValid())
		{
			if(SelectedFriend->UserID.IsValid())
			{
				SelectedFriend->ViewModel = FriendsAndChatManager.Pin()->GetFriendViewModel(*SelectedFriend->UserID.Get());
				bHasActionPending = false;
			}
			else if(SelectedFriend->SelectedMessage.IsValid() && SelectedFriend->SelectedMessage->GetSenderID().IsValid())
			{
				SelectedFriend->ViewModel = FriendsAndChatManager.Pin()->GetFriendViewModel(*SelectedFriend->SelectedMessage->GetSenderID().Get());
				bHasActionPending = false;
			}
		}
	}

	FChatViewModelImpl(const TSharedRef<FFriendsMessageManager>& MessageManager, const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager)
		: SelectedViewChannel(EChatMessageType::Global)
		, SelectedChatChannel(EChatMessageType::Global)
		, MessageManager(MessageManager)
		, FriendsAndChatManager(FriendsAndChatManager)
		, bInGame(false)
		, bIsDisplayingGlobalChat(false)
		, bEnableGlobalChat(true)
		, bHasActionPending(false)
		, bAllowJoinGame(false)
		, bLockedChannel(false)
	{
	}

private:
	EChatMessageType::Type SelectedViewChannel;
	EChatMessageType::Type SelectedChatChannel;
	TWeakPtr<FFriendsMessageManager> MessageManager;
	TWeakPtr<FFriendsAndChatManager> FriendsAndChatManager;
	bool bInGame;
	bool bInParty;
	/** Whether global chat is currently switched on and displayed  */
	bool bIsDisplayingGlobalChat;
	/** Whether global chat mode is enabled/disabled - if disabled it can't be selected and isn't an option in the UI */
	bool bEnableGlobalChat;
	bool bHasActionPending;
	bool bAllowJoinGame;
	bool bLockedChannel;

	TArray<TSharedRef<FChatItemViewModel> > FilteredMessages;
	TArray<TSharedPtr<FSelectedFriend> > RecentPlayerList;
	TSharedPtr<FSelectedFriend> SelectedFriend;
	FChatListUpdated ChatListUpdatedEvent;

private:
	friend FChatViewModelFactory;
};

TSharedRef< FChatViewModel > FChatViewModelFactory::Create(
	const TSharedRef<FFriendsMessageManager>& MessageManager,
	const TSharedRef<FFriendsAndChatManager>& FriendsAndChatManager
	)
{
	TSharedRef< FChatViewModelImpl > ViewModel(new FChatViewModelImpl(MessageManager, FriendsAndChatManager));
	ViewModel->Initialize();
	return ViewModel;
}
