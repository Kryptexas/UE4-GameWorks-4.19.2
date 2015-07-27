// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BaseTextLayoutMarshaller.h"

class FChatTextLayoutMarshaller
	: public FBaseTextLayoutMarshaller
{
public:
	virtual ~FChatTextLayoutMarshaller() {}

	virtual void AddUserNameHyperlinkDecorator(const TSharedRef<class FHyperlinkDecorator>& InNameHyperlinkDecorator) = 0;
	virtual void AddChannelNameHyperlinkDecorator(const TSharedRef<class FHyperlinkDecorator>& InChannelHyperlinkDecorator) = 0;
	virtual bool AppendMessage(const TSharedPtr<class FChatItemViewModel>& NewMessage, bool GroupText) = 0;
	virtual bool AppendLineBreak() = 0;
};

/**
 * Creates the implementation for an FChatDisplayService.
 *
 * @return the newly created FChatDisplayService implementation.
 */
FACTORY(TSharedRef< FChatTextLayoutMarshaller >, FChatTextLayoutMarshaller,
	const FFriendsAndChatStyle* const InDecoratorStyleSet,
	bool InIsMultiChat);