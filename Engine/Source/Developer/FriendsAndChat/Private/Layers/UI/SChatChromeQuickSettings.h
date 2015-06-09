// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class SChatChromeQuickSettings : public SUserWidget
{
public:
	SLATE_USER_ARGS(SChatChromeQuickSettings)
	{ }
	SLATE_ARGUMENT(const FFriendsAndChatStyle*, FriendStyle)
	SLATE_END_ARGS()

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	 virtual void Construct(const FArguments& InArgs, TSharedPtr<class FChatSettingsViewModel> SettingsViewModel, const TSharedPtr<class FChatChromeViewModel> ChromeViewModel) = 0;

};
