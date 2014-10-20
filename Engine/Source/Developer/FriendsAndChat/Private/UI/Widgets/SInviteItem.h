// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SInviteItem : public SUserWidget
{
public:
	SLATE_USER_ARGS(SInviteItem)
	{ }
	SLATE_ARGUMENT(const FFriendsAndChatStyle*, FriendStyle)
	SLATE_END_ARGS()

	/**
	 * Constructs the widget.
	 */
	virtual void Construct( const FArguments& InArgs, const TSharedRef<class FFriendViewModel>& ViewModel ) = 0;
};
