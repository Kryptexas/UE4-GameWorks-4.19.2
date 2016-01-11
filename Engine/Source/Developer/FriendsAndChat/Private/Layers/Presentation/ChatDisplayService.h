// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class FChatDisplayService
	: public IChatDisplayService
	, public TSharedFromThis<FChatDisplayService>
{
public:
	virtual ~FChatDisplayService() {}
};

/**
 * Creates the implementation for an FChatDisplayService.
 *
 * @return the newly created FChatDisplayService implementation.
 */
FACTORY(TSharedRef< FChatDisplayService >, FChatDisplayService, 
	const TSharedRef<class FFriendsAndChatManager>& FFriendsAndChatManager,
	bool FadeChatList, bool FadeChatEntry, float ListFadeTime, float EntryFadeTime);