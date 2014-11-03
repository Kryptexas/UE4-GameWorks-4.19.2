// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "ChatItemViewModel.h"
#include "ChatViewModel.h"

class FChatViewModelImpl
	: public FChatViewModel
{
public:

	virtual TArray< TSharedRef<class FChatItemViewModel > >& GetFilteredChatList() override
	{
		return FilteredChatLists;
	}

	virtual void HandleSelectionChanged(TSharedPtr<FChatItemViewModel> ItemSelected, ESelectInfo::Type SelectInfo) override
	{
		if( ItemSelected.IsValid())
		{
			SetChatChannel(ItemSelected->GetMessageType());
			if(SelectedChatChannel == EChatMessageType::Whisper)
			{
				SelectedFriend = ItemSelected->GetFriendID();
			}
		}
	}

	virtual FText GetViewGroupText() const override
	{
		return EChatMessageType::ToText(SelectedViewChannel);
	}

	virtual FText GetChatGroupText() const override
	{
		return SelectedChatChannel == EChatMessageType::Whisper ? SelectedFriend : EChatMessageType::ToText(SelectedChatChannel);
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

	virtual void SetViewChannel(const EChatMessageType::Type NewOption) override
	{
		SelectedViewChannel = NewOption;
		SelectedChatChannel = NewOption;
		FilterChatList();
	}

	virtual void SendMessage(const FText NewMessage) override
	{
		TSharedPtr< FChatMessage > FriendItem = MakeShareable(new FChatMessage());
		FriendItem->FriendID = SelectedFriend;
		FriendItem->Message = NewMessage;
		FriendItem->MessageType = SelectedChatChannel;
		FriendItem->MessageTimeText = FText::AsTime(FDateTime::Now());
		FriendItem->bIsFromSelf = true;
		ChatLists.Add(FChatItemViewModelFactory::Create(FriendItem.ToSharedRef()));
		FilterChatList();
	}

	DECLARE_DERIVED_EVENT(FChatViewModelImpl , FChatViewModel::FChatListUpdated, FChatListUpdated);
	virtual FChatListUpdated& OnChatListUpdated() override
	{
		return ChatListUpdatedEvent;
	}

	~FChatViewModelImpl()
	{
		Uninitialize();
	}


private:
	void Initialize()
	{
		// Create some fake data. TODO: Hook up to chat message discovery once OSS system is ready. Probably just registed for callbacks
		TSharedPtr< FChatMessage > FriendItem = MakeShareable(new FChatMessage());
		FriendItem->FriendID = FText::FromString("Bob");
		FriendItem->Message = FText::FromString("How are you");
		FriendItem->MessageType = EChatMessageType::Whisper;
		FriendItem->MessageTimeText = FText::AsTime(FDateTime::Now());
		FriendItem->bIsFromSelf = false;
		ChatLists.Add(FChatItemViewModelFactory::Create(FriendItem.ToSharedRef()));

		TSharedPtr< FChatMessage > FriendItem1 = MakeShareable(new FChatMessage());
		FriendItem1->FriendID = FText::FromString("Dave");
		FriendItem1->Message = FText::FromString("Join the game test test test test test test test!");
		FriendItem1->MessageType = EChatMessageType::Party;
		FriendItem1->MessageTimeText = FText::AsTime(FDateTime::Now());
		FriendItem1->bIsFromSelf = false;
		ChatLists.Add(FChatItemViewModelFactory::Create(FriendItem1.ToSharedRef()));

		TSharedPtr< FChatMessage > FriendItem2 = MakeShareable(new FChatMessage());
		FriendItem2->FriendID = FText::FromString("JB");
		FriendItem2->Message = FText::FromString("Hello");
		FriendItem2->MessageType = EChatMessageType::Global;
		FriendItem2->MessageTimeText = FText::AsTime(FDateTime::Now());
		FriendItem2->bIsFromSelf = false;
		ChatLists.Add(FChatItemViewModelFactory::Create(FriendItem2.ToSharedRef()));

		FilterChatList();
	}

	void Uninitialize()
	{
	}


	void FilterChatList()
	{
		FilteredChatLists.Empty();
		for (const auto& ChatItem : ChatLists)
		{
			if(ChatItem->GetMessageType() <= SelectedViewChannel)
			{
				FilteredChatLists.Add(ChatItem);
			}
		}
		ChatListUpdatedEvent.Broadcast();
	}

	FChatViewModelImpl()
		: SelectedViewChannel(EChatMessageType::Global)
		, SelectedChatChannel(EChatMessageType::Global)
		, SelectedFriend(FText::GetEmpty())
	{
	}

private:
	friend FChatViewModelFactory;

	TArray<TSharedRef<FChatItemViewModel> > ChatLists;
	TArray<TSharedRef<FChatItemViewModel> > FilteredChatLists;
	FChatListUpdated ChatListUpdatedEvent;

	EChatMessageType::Type SelectedViewChannel;
	EChatMessageType::Type SelectedChatChannel;
	FText SelectedFriend;
};

TSharedRef< FChatViewModel > FChatViewModelFactory::Create()
{
	TSharedRef< FChatViewModelImpl > ViewModel(new FChatViewModelImpl());
	ViewModel->Initialize();
	return ViewModel;
}