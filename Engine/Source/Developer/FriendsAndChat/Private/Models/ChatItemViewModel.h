// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Enum for the chat message type.
 */
namespace EChatMessageType
{
	enum Type : uint8
	{
		// Person whisper Item
		Whisper,
		// Party Chat Item
		Party,
		// Global Chat Item
		Global,

	};

	/** @return the FTextified version of the enum passed in */
	inline FText ToText(EChatMessageType::Type Type)
	{
		switch (Type)
		{
			case Global: return NSLOCTEXT("FriendsList","Global", "Global");
			case Whisper: return NSLOCTEXT("FriendsList","Whisper", "Whisper");
			case Party: return NSLOCTEXT("FriendsList","Party", "Party");

			default: return FText::GetEmpty();
		}
	}
};

// Stuct for holding chat message information. Will probably be replaced by OSS version
struct FChatMessage
{
	EChatMessageType::Type MessageType;
	FText FriendID;
	FText Message;
	FText MessageTimeText;
	bool bIsFromSelf;
};

class FChatItemViewModel
	: public TSharedFromThis<FChatItemViewModel>
{
public:
	virtual ~FChatItemViewModel() {}

	virtual FText GetMessage() = 0;
	virtual FText GetMessageTypeText() = 0;
	virtual const EChatMessageType::Type GetMessageType() const =0;
	virtual FSlateColor GetDisplayColor() = 0;
	virtual FText GetFriendID() = 0;
	virtual FText GetMessageTime() = 0;
	virtual const bool IsFromSelf() const = 0;
};

/**
 * Creates the implementation for an ChatItemViewModel.
 *
 * @return the newly created ChaItemtViewModel implementation.
 */
FACTORY(TSharedRef< FChatItemViewModel >, FChatItemViewModel,
	const TSharedRef<struct FChatMessage>& FChatMessage);