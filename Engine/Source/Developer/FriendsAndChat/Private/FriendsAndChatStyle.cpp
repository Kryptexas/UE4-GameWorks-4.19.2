// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"


const FName FFriendsAndChatStyle::TypeName( TEXT("FFriendsAndChatStyle") );


FFriendsAndChatStyle& FFriendsAndChatStyle::SetSearchButtonStyle( const FButtonStyle& InButtonStyle)
{
	SearchButtonStyle = InButtonStyle; 
	return *this; 
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendsListOpenButtonStyle(const FButtonStyle& ButtonStyle)
{
	FriendListOpenButtonStyle = ButtonStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendsListClosedButtonStyle(const FButtonStyle& ButtonStyle)
{
	FriendListCloseButtonStyle = ButtonStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetAddFriendCloseButtonStyle(const FButtonStyle& InAddFriendCloseButtonStyle)
{
	AddFriendCloseButtonStyle = InAddFriendCloseButtonStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetAddFriendButtonStyle(const FButtonStyle& InButtonStyle)
{
	AddFriendButtonStyle = InButtonStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendsListActionButtonStyle(const FButtonStyle& ButtonStyle)
{
	FriendListActionButtonStyle = ButtonStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendsListCriticalButtonStyle(const FButtonStyle& ButtonStyle)
{
	FriendListCriticalButtonStyle = ButtonStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendsListEmphasisButtonStyle(const FButtonStyle& ButtonStyle)
{
	FriendListEmphasisButtonStyle = ButtonStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendsListComboButtonStyle(const FButtonStyle& ButtonStyle)
{
	FriendListComboButtonStyle = ButtonStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendsListItemButtonStyle(const FButtonStyle& ButtonStyle)
{
	FriendListItemButtonStyle = ButtonStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendsListItemButtonSimpleStyle(const FButtonStyle& ButtonStyle)
{
	FriendListItemButtonSimpleStyle = ButtonStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetTitleBarBrush(const FSlateBrush& Brush)
{
	TitleBarBrush = Brush;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendImageBrush(const FSlateBrush& Brush)
{
	FriendImageBrush = Brush;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFortniteImageBrush(const FSlateBrush& Brush)
{
	FortniteImageBrush = Brush;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetLauncherImageBrush(const FSlateBrush& Brush)
{
	LauncherImageBrush = Brush;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendsComboDropdownImageBrush(const FSlateBrush& Brush)
{
	FriendsComboDropdownImageBrush = Brush;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendsAddImageBrush(const FSlateBrush& Brush)
{
	FriendsAddImageBrush = Brush;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendsCalloutBrush(const FSlateBrush& Brush)
{
	FriendsCalloutBrush = Brush;
	return *this;
}


FFriendsAndChatStyle& FFriendsAndChatStyle::SetOfflineBrush(const FSlateBrush& InOffLine)
{
	OfflineBrush = InOffLine; 
	return *this; 
}


FFriendsAndChatStyle& FFriendsAndChatStyle::SetOnlineBrush(const FSlateBrush& InOnLine)
{
	OnlineBrush = InOnLine; 
	return *this; 
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetAwayBrush(const FSlateBrush& InAwayBrush)
{
	AwayBrush = InAwayBrush; 
	return *this; 
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetBackgroundBrush(const FSlateBrush& InBackground)
{
	Background = InBackground;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendContainerHeader(const FSlateBrush& InFriendContainerHeader)
{
	FriendContainerHeader = InFriendContainerHeader;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendListHeader(const FSlateBrush& InFriendListHeader)
{
	FriendListHeader = InFriendListHeader;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendItemSelected(const FSlateBrush& InFriendItemSelected)
{
	FriendItemSelected = InFriendItemSelected;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendContainerBackground(const FSlateBrush& InFriendContainerBackground)
{
	FriendContainerBackground = InFriendContainerBackground;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetAddFriendEditBorder(const FSlateBrush& InAddFriendEditBorder)
{
	AddFriendEditBorder = InAddFriendEditBorder;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetTextStyle(const FTextBlockStyle& InTextStle)
{
	TextStyle = InTextStle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFontStyle(const FSlateFontInfo& InFontStyle)
{
	FriendsFontStyle = InFontStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFontStyleBold(const FSlateFontInfo& InFontStyle)
{
	FriendsFontStyleBold = InFontStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFontStyleSmall(const FSlateFontInfo& FontStyle)
{
	FriendsFontStyleSmall = FontStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFontStyleSmallBold(const FSlateFontInfo& FontStyle)
{
	FriendsFontStyleSmallBold = FontStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetChatFontStyleEntry(const FSlateFontInfo& InFontStyle)
{
	ChatFontStyleEntry = InFontStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetDefaultFontColor(const FLinearColor& InFontColor)
{
	DefaultFontColor = InFontColor;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetDefaultChatColor(const FLinearColor& InFontColor)
{
	DefaultChatColor = InFontColor;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetWhisplerChatColor(const FLinearColor& InFontColor)
{
	WhisplerChatColor = InFontColor;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetPartyChatColor(const FLinearColor& InFontColor)
{
	PartyChatColor = InFontColor;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetNetworkChatColor(const FLinearColor& InFontColor)
{
	NetworkChatColor = InFontColor;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetChatGlobalBrush(const FSlateBrush& Brush)
{
	ChatGlobalBrush = Brush;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetChatPartyBrush(const FSlateBrush& Brush)
{
	ChatPartyBrush = Brush;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetChatWhisperBrush(const FSlateBrush& Brush)
{
	ChatWhisperBrush = Brush;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetAddFriendEditableTextStyle(const FEditableTextBoxStyle& InEditableTextStyle)
{
	AddFriendEditableTextStyle = InEditableTextStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetChatEditableTextStyle(const FEditableTextBoxStyle& InEditableTextStyle)
{
	ChatEditableTextStyle = InEditableTextStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendCheckboxStyle(const FCheckBoxStyle& InFriendCheckboxStyle)
{
	FriendCheckboxStyle = InFriendCheckboxStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetButtonPadding(const FVector2D& Padding)
{
	ButtonPadding = Padding;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetBorderPadding(const FMargin& Padding)
{
	BorderPadding = Padding;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendsListWidth(const float InFriendsListWidth)
{
	FriendsListWidth = InFriendsListWidth;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetChatBackgroundBrush(const FSlateBrush& InChatBackgroundBrush)
{
	ChatBackgroundBrush = InChatBackgroundBrush;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetChatChannelsBackgroundBrush(const FSlateBrush& InChatChannelsBackgroundBrush)
{
	ChatChannelsBackgroundBrush = InChatChannelsBackgroundBrush;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetChatOptionsBackgroundBrush(const FSlateBrush& InChatOptionsBackgroundBrush)
{
	ChatOptionsBackgroundBrush = InChatOptionsBackgroundBrush;
	return *this;
}

const FFriendsAndChatStyle& FFriendsAndChatStyle::GetDefault()
{
	static FFriendsAndChatStyle Default;
	return Default;
}

