// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class FFriendsViewModel;

class SFriendsContainer : public SUserWidget
{
public:

	SLATE_USER_ARGS(SFriendsContainer)
		: _Method(EPopupMethod::UseCurrentWindow)
		, _MaxFriendNameLength(254)
	 { }
	SLATE_ARGUMENT(const FFriendsAndChatStyle*, FriendStyle)
	SLATE_ARGUMENT(const FFriendsFontStyle*, FontStyle)
	SLATE_ARGUMENT(EPopupMethod, Method)
	SLATE_ARGUMENT(int32, MaxFriendNameLength)
	SLATE_END_ARGS()

	virtual void Construct(const FArguments& InArgs, const TSharedRef<FFriendsViewModel>& ViewModel) = 0;
};
