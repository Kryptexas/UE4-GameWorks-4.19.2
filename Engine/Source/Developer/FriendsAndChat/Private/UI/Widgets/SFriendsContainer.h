// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FFriendsViewModel;

class SFriendsContainer : public SUserWidget
{
public:

	SLATE_USER_ARGS(SFriendsContainer)
		: _Method(SMenuAnchor::UseCurrentWindow)
	 { }
	SLATE_ARGUMENT(const FFriendsAndChatStyle*, FriendStyle)
	SLATE_ARGUMENT(SMenuAnchor::EMethod, Method)
	SLATE_EVENT(FOnClicked, OnCloseClicked)
	SLATE_EVENT(FOnClicked, OnMinimizeClicked)
	SLATE_END_ARGS()

	virtual void Construct(const FArguments& InArgs, const TSharedRef<FFriendsViewModel>& ViewModel) = 0;
};
