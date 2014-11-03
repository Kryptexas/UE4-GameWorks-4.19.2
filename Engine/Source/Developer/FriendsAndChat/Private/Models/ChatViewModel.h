// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FChatItemViewModel;
namespace EChatMessageType
{
	enum Type : uint8;
}

class FChatViewModel
	: public TSharedFromThis<FChatViewModel>
{
public:
	virtual TArray< TSharedRef<FChatItemViewModel > >& GetFilteredChatList() = 0;
	virtual void HandleSelectionChanged(TSharedPtr<FChatItemViewModel> ItemSelected, ESelectInfo::Type SelectInfo) = 0;
	virtual FText GetViewGroupText() const = 0;
	virtual FText GetChatGroupText() const = 0;
	virtual void EnumerateChatChannelOptionsList(TArray<EChatMessageType::Type>& OUTChannelType) = 0;
	virtual void SetChatChannel(const EChatMessageType::Type NewOption) = 0;
	virtual void SetViewChannel(const EChatMessageType::Type NewOption) = 0;
	virtual void SendMessage(const FText NewMessage) = 0;

	DECLARE_EVENT(FChatViewModel, FChatListUpdated)
	virtual FChatListUpdated& OnChatListUpdated() = 0;

	virtual ~FChatViewModel() {}
};

/**
 * Creates the implementation for an ChatViewModel.
 *
 * @return the newly created ChatViewModel implementation.
 */
FACTORY(TSharedRef< FChatViewModel >, FChatViewModel);