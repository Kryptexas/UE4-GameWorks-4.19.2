// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "ChatItemViewModel.h"
#include "ChatViewModel.h"

class FChatItemViewModelImpl
	: public FChatItemViewModel
{
public:

	virtual FText GetMessage() override
	{
		return ChatMessage->Message;
	}

	virtual const EChatMessageType::Type GetMessageType() const override
	{
		return ChatMessage->MessageType;
	}

	virtual FText GetMessageTypeText() override
	{
		return EChatMessageType::ToText(ChatMessage->MessageType);
	}

	virtual FText GetFriendID() override
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("Username"), ChatMessage->FromName);
		const FText DisplayName = FText::Format(NSLOCTEXT("FChatItemViewModel", "DisplayName", "{Username}: "), Args);
		return DisplayName;
	}

	virtual FText GetMessageTime() override
	{
		return ChatMessage->MessageTimeText;
	}

	const bool IsFromSelf() const override
	{
		return ChatMessage->bIsFromSelf;
	}

	virtual const float GetFadeAmountColor() const override
	{
		return Owner->GetTimeTransparency();
	}

private:

	FChatItemViewModelImpl(TSharedRef<FFriendChatMessage> ChatMessage, TSharedPtr<FChatViewModel> Owner)
	: ChatMessage(ChatMessage)
	, Owner(Owner)
	{
	}

private:
	TSharedRef<FFriendChatMessage> ChatMessage;
	TSharedPtr<FChatViewModel> Owner;

private:
	friend FChatItemViewModelFactory;
};

TSharedRef< FChatItemViewModel > FChatItemViewModelFactory::Create(
	const TSharedRef<FFriendChatMessage>& ChatMessage
	, TSharedRef<FChatViewModel> Owner)
{
	TSharedRef< FChatItemViewModelImpl > ViewModel(new FChatItemViewModelImpl(ChatMessage, Owner));
	return ViewModel;
}