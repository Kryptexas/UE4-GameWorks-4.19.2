// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "ChatItemViewModel.h"

class FChatItemViewModelImpl
	: public FChatItemViewModel
{
public:

	~FChatItemViewModelImpl()
	{
		Uninitialize();
	}

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
		return ChatMessage->FriendID;
	}

	virtual FText GetMessageTime() override
	{
		return ChatMessage->MessageTimeText;
	}

	virtual FSlateColor GetDisplayColor() override
	{
		// TODO: Add chat colors to style
		switch(ChatMessage->MessageType)
		{
			case EChatMessageType::Global: return FLinearColor::White; break;
			case EChatMessageType::Whisper: return FLinearColor::Yellow; break;
			case EChatMessageType::Party: return FLinearColor::Green; break;
			default:
			return FLinearColor::Gray;
		}
	}

	const bool IsFromSelf() const override
	{
		return ChatMessage->bIsFromSelf;
	}

private:
	void Initialize()
	{
		
	}

	void Uninitialize()
	{
	}

	FChatItemViewModelImpl(TSharedRef<FChatMessage> ChatMessage)
	: ChatMessage(ChatMessage)
	{
	}

private:
	TSharedRef<FChatMessage> ChatMessage;


private:
	friend FChatItemViewModelFactory;
};

TSharedRef< FChatItemViewModel > FChatItemViewModelFactory::Create(
	const TSharedRef<FChatMessage>& ChatMessage)
{
	TSharedRef< FChatItemViewModelImpl > ViewModel(new FChatItemViewModelImpl(ChatMessage));
	ViewModel->Initialize();
	return ViewModel;
}