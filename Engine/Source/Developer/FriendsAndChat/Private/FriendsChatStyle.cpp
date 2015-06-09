// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"

const FName FFriendsChatStyle::TypeName( TEXT("FFriendsChatStyle") );

const FFriendsChatStyle& FFriendsChatStyle::GetDefault()
{
	static FFriendsChatStyle Default;
	return Default;
}

FFriendsChatStyle& FFriendsChatStyle::SetChatEntryHeight(float Value)
{
	ChatEntryHeight = Value;
	return *this;
}

FFriendsChatStyle& FFriendsChatStyle::SetGlobalChatHeaderBrush(const FSlateBrush& Value)
{
	GlobalChatHeaderBrush = Value;
	return *this;
}

FFriendsChatStyle& FFriendsChatStyle::SetChatContainerBackground(const FSlateBrush& InChatContainerBackground)
{
	ChatContainerBackground = InChatContainerBackground;
	return *this;
}

FFriendsChatStyle& FFriendsChatStyle::SetTextStyle(const FTextBlockStyle& InTextStle)
{
	TextStyle = InTextStle;
	return *this;
}

FFriendsChatStyle& FFriendsChatStyle::SetDefaultChatColor(const FLinearColor& InFontColor)
{
	DefaultChatColor = InFontColor;
	return *this;
}

FFriendsChatStyle& FFriendsChatStyle::SetWhisplerChatColor(const FLinearColor& InFontColor)
{
	WhisplerChatColor = InFontColor;
	return *this;
}

FFriendsChatStyle& FFriendsChatStyle::SetGameChatColor(const FLinearColor& InFontColor)
{
	GameChatColor = InFontColor;
	return *this;
}

FFriendsChatStyle& FFriendsChatStyle::SetPartyChatColor(const FLinearColor& InFontColor)
{
	PartyChatColor = InFontColor;
	return *this;
}


FFriendsChatStyle& FFriendsChatStyle::SetChatGlobalBrush(const FSlateBrush& Brush)
{
	ChatGlobalBrush = Brush;
	return *this;
}

FFriendsChatStyle& FFriendsChatStyle::SetChatGameBrush(const FSlateBrush& Brush)
{
	ChatGameBrush = Brush;
	return *this;
}

FFriendsChatStyle& FFriendsChatStyle::SetChatPartyBrush(const FSlateBrush& Brush)
{
	ChatPartyBrush = Brush;
	return *this;
}

FFriendsChatStyle& FFriendsChatStyle::SetChatWhisperBrush(const FSlateBrush& Brush)
{
	ChatWhisperBrush = Brush;
	return *this;
}

FFriendsChatStyle& FFriendsChatStyle::SetChatInvalidBrush(const FSlateBrush& Brush)
{
	ChatInvalidBrush = Brush;
	return *this;
}

FFriendsChatStyle& FFriendsChatStyle::SetChatEntryTextStyle(const FEditableTextBoxStyle& InEditableTextStyle)
{
	ChatEntryTextStyle = InEditableTextStyle;
	return *this;
}

FFriendsChatStyle& FFriendsChatStyle::SetChatDisplayTextStyle(const FEditableTextBoxStyle& InEditableTextStyle)
{
	ChatDisplayTextStyle = InEditableTextStyle;
	return *this;
}

FFriendsChatStyle& FFriendsChatStyle::SetScrollBorderStyle(const FScrollBorderStyle& InScrollBorderStyle)
{
	ScrollBorderStyle = InScrollBorderStyle;
	return *this;
}

FFriendsChatStyle& FFriendsChatStyle::SetChatBackgroundBrush(const FSlateBrush& InChatBackgroundBrush)
{
	ChatBackgroundBrush = InChatBackgroundBrush;
	return *this;
}

FFriendsChatStyle& FFriendsChatStyle::SetChatFooterBrush(const FSlateBrush& Value)
{
	ChatFooterBrush = Value;
	return *this;
}

FFriendsChatStyle& FFriendsChatStyle::SetChatChannelsBackgroundBrush(const FSlateBrush& InChatChannelsBackgroundBrush)
{
	ChatChannelsBackgroundBrush = InChatChannelsBackgroundBrush;
	return *this;
}

FFriendsChatStyle& FFriendsChatStyle::SetQuickSettingsBrush(const FSlateBrush& Brush)
{
	QuickSettingsBrush = Brush;
	return *this;
}

FFriendsChatStyle& FFriendsChatStyle::SetChatSettingsBrush(const FSlateBrush& Brush)
{
	ChatSettingsBrush = Brush;
	return *this;
}
