// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"

const FName FFriendsListStyle::TypeName( TEXT("FFriendsListStyle") );

const FFriendsListStyle& FFriendsListStyle::GetDefault()
{
	static FFriendsListStyle Default;
	return Default;
}

FFriendsListStyle& FFriendsListStyle::SetGlobalChatButtonStyle(const FButtonStyle& ButtonStyle)
{
	GlobalChatButtonStyle = ButtonStyle;
	return *this;
}

FFriendsListStyle& FFriendsListStyle::SetButtonInvertedForegroundColor(const FSlateColor& Value)
{
	ButtonInvertedForegroundColor = Value;
	return *this;
}

FFriendsListStyle& FFriendsListStyle::SetButtonForegroundColor(const FSlateColor& Value)
{
	ButtonForegroundColor = Value;
	return *this;
}

FFriendsListStyle& FFriendsListStyle::SetFriendItemButtonStyle(const FButtonStyle& ButtonStyle)
{
	FriendListOpenButtonStyle = ButtonStyle;
	return *this;
}

FFriendsListStyle& FFriendsListStyle::SetActionMenuArrowBrush(const FSlateBrush& BrushStyle)
{
	ActionMenuArrowBrush = BrushStyle;
	return *this;
}

FFriendsListStyle& FFriendsListStyle::SetFriendsListOpenButtonStyle(const FButtonStyle& ButtonStyle)
{
	FriendListOpenButtonStyle = ButtonStyle;
	return *this;
}

/** Friends General Purpose Button style */
FFriendsListStyle& FFriendsListStyle::SetFriendGeneralButtonStyle(const FButtonStyle& ButtonStyle)
{
	FriendGeneralButtonStyle = ButtonStyle;
	return *this;
}

/** Friends List Action Button style */
FFriendsListStyle& FFriendsListStyle::SetFriendListActionButtonStyle(const FButtonStyle& ButtonStyle)
{
	FriendListActionButtonStyle = ButtonStyle;
	return *this;
}

/** Friends List Critical Button style */
FFriendsListStyle& FFriendsListStyle::SetFriendsListCriticalButtonStyle(const FButtonStyle& ButtonStyle)
{
	FriendListCriticalButtonStyle = ButtonStyle;
	return *this;
}

/** Friends List Emphasis Button style */
FFriendsListStyle& FFriendsListStyle::SetFriendsListEmphasisButtonStyle(const FButtonStyle& ButtonStyle)
{
	FriendListEmphasisButtonStyle = ButtonStyle;
	return *this;
}

/** Friends List Close button style */
FFriendsListStyle& FFriendsListStyle::SetFriendsListClosedButtonStyle(const FButtonStyle& ButtonStyle)
{
	FriendListCloseButtonStyle = ButtonStyle;
	return *this;
}

/** Add Friend Close button style */
FFriendsListStyle& FFriendsListStyle::SetAddFriendCloseButtonStyle(const FButtonStyle& ButtonStyle)
{
	AddFriendCloseButtonStyle = ButtonStyle;
	return *this;
}

/** Optional content for the Add Friend button */
FFriendsListStyle& FFriendsListStyle::SetAddFriendButtonContentBrush(const FSlateBrush& BrushStyle)
{
	AddFriendButtonContentBrush = BrushStyle;
	return *this;
}

/** Optional content for the Add Friend button (hovered) */
FFriendsListStyle& FFriendsListStyle::SetAddFriendButtonContentHoveredBrush(const FSlateBrush& BrushStyle)
{
	AddFriendButtonContentHoveredBrush = BrushStyle;
	return *this;
}

/** Friend Image brush style */
FFriendsListStyle& FFriendsListStyle::SetFriendImageBrush(const FSlateBrush& BrushStyle)
{
	FriendImageBrush = BrushStyle;
	return *this;
}

/** Fortnite Image brush style */
FFriendsListStyle& FFriendsListStyle::SetFortniteImageBrush(const FSlateBrush& BrushStyle)
{
	FortniteImageBrush = BrushStyle;
	return *this;
}

FFriendsListStyle& FFriendsListStyle::SetFortniteDevImageBrush(const FSlateBrush& BrushStyle)
{
	FortniteDevImageBrush = BrushStyle;
	return *this;
}

FFriendsListStyle& FFriendsListStyle::SetFortniteTestImageBrush(const FSlateBrush& BrushStyle)
{
	FortniteTestImageBrush = BrushStyle;
	return *this;
}

/** Launcher Image brush style */
FFriendsListStyle& FFriendsListStyle::SetLauncherImageBrush(const FSlateBrush& BrushStyle)
{
	LauncherImageBrush = BrushStyle;
	return *this;
}

/** UnrealTournament Image brush style */
FFriendsListStyle& FFriendsListStyle::SetUTImageBrush(const FSlateBrush& BrushStyle)
{
	UTImageBrush = BrushStyle;
	return *this;
}

/** Offline brush style */
FFriendsListStyle& FFriendsListStyle::SetOfflineBrush(const FSlateBrush& BrushStyle)
{
	OfflineBrush = BrushStyle;
	return *this;
}

/** Online brush style */
FFriendsListStyle& FFriendsListStyle::SetOnlineBrush(const FSlateBrush& BrushStyle)
{
	OnlineBrush = BrushStyle;
	return *this;
}

/** Away brush style */
FFriendsListStyle& FFriendsListStyle::SetAwayBrush(const FSlateBrush& BrushStyle)
{
	AwayBrush = BrushStyle;
	return *this;
}

/** Window background style */
FFriendsListStyle& FFriendsListStyle::SetBackgroundBrush(const FSlateBrush& BrushStyle)
{
	Background = BrushStyle;
	return *this;
}

/** Friend container header */
FFriendsListStyle& FFriendsListStyle::SetFriendContainerHeader(const FSlateBrush& BrushStyle)
{
	FriendContainerHeader = BrushStyle;
	return *this;
}

/** Friend list header */
FFriendsListStyle& FFriendsListStyle::SetFriendListHeader(const FSlateBrush& BrushStyle)
{
	FriendListHeader = BrushStyle;
	return *this;
}

/** Friends window background */
FFriendsListStyle& FFriendsListStyle::SetFriendContainerBackground(const FSlateBrush& BrushStyle)
{
	FriendsContainerBackground = BrushStyle;
	return *this;
}

FFriendsListStyle& FFriendsListStyle::SetFriendListActionFontColor(const FLinearColor& Color)
{
	FriendListActionFontColor = Color;
	return *this;
}

FFriendsListStyle& FFriendsListStyle::SetAddFriendEditableTextStyle(const FEditableTextBoxStyle& TextStyle)
{
	AddFriendEditableTextStyle = TextStyle;
	return *this;
}

FFriendsListStyle& FFriendsListStyle::SetUserPresenceImageSize(const FVector2D& Size)
{
	UserPresenceImageSize = Size;
	return *this;
}

FFriendsListStyle& FFriendsListStyle::SetAddFriendButtonSize(const FVector2D& Value)
{
	AddFriendButtonSize = Value;
	return *this;
}

FFriendsListStyle& FFriendsListStyle::SetUserHeaderPadding(const FMargin& Margin)
{
	UserHeaderPadding = Margin;
	return *this;
}

FFriendsListStyle& FFriendsListStyle::SetFriendsListWidth(const float InWidth)
{
	FriendsListWidth = InWidth;
	return *this;
}

FFriendsListStyle& FFriendsListStyle::SetHasUserHeader(bool InHasUserHeader)
{
	HasUserHeader = InHasUserHeader;
	return *this;
}

FFriendsListStyle& FFriendsListStyle::SetBackBrush(const FSlateBrush& Brush)
{
	BackBrush = Brush;
	return *this;
}

FFriendsListStyle& FFriendsListStyle::SetSelectedOptionBrush(const FSlateBrush& Brush)
{
	SelectedOptionBrush = Brush;
	return *this;
}

FFriendsListStyle& FFriendsListStyle::SetFilterBrush(const FSlateBrush& Brush)
{
	FilterBrush = Brush;
	return *this;
}

FFriendsListStyle& FFriendsListStyle::SetSettingsBrush(const FSlateBrush& Brush)
{
	SettingsBrush = Brush;
	return *this;
}

FFriendsListStyle& FFriendsListStyle::SetClanDetailsBrush(const FSlateBrush& Brush)
{
	ClanDetailsBrush = Brush;
	return *this;
}

FFriendsListStyle& FFriendsListStyle::SetClanMembersBrush(const FSlateBrush& Brush)
{
	ClanMembersBrush = Brush;
	return *this;
}
