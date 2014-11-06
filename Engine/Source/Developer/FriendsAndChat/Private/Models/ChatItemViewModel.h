// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FChatItemViewModel
	: public TSharedFromThis<FChatItemViewModel>
{
public:
	virtual ~FChatItemViewModel() {}

	virtual FText GetMessage() = 0;
	virtual FText GetMessageTypeText() = 0;
	virtual const EChatMessageType::Type GetMessageType() const =0;
	virtual FText GetFriendID() = 0;
	virtual FText GetMessageTime() = 0;
	virtual const bool IsFromSelf() const = 0;
	virtual const float GetFadeAmountColor() const = 0;
};

/**
 * Creates the implementation for an ChatItemViewModel.
 *
 * @return the newly created ChaItemtViewModel implementation.
 */
FACTORY(TSharedRef< FChatItemViewModel >, FChatItemViewModel,
	const TSharedRef<struct FFriendChatMessage>& FFriendChatMessage, TSharedRef<class FChatViewModel> Owner);