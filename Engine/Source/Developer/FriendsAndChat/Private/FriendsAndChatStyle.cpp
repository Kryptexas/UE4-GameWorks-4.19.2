// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"


const FName FFriendsAndChatStyle::TypeName( TEXT("FFriendsAndChatStyle") );


FFriendsAndChatStyle& FFriendsAndChatStyle::SetCloseButtonStyle( const FButtonStyle& InButtonStyle)
{
	CloseButtonStyle = InButtonStyle; 
	return *this; 
}


FFriendsAndChatStyle& FFriendsAndChatStyle::SetMinimizeStyle( const FButtonStyle& InButtonStyle)
{
	MinimizeButtonStyle = InButtonStyle; 
	return *this;
}


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

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendsListActionButtonStyle(const FButtonStyle& ButtonStyle)
{
	FriendListActionButtonStyle = ButtonStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendsListClosedButtonStyle(const FButtonStyle& ButtonStyle)
{
	FriendListCloseButtonStyle = ButtonStyle;
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


FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendsComboDropdownImageBrush(const FSlateBrush& Brush)
{
	FriendsComboDropdownImageBrush = Brush;
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


FFriendsAndChatStyle& FFriendsAndChatStyle::SetBackgroundBrush(const FSlateBrush& InBackground)
{
	Background = InBackground;
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

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFontStyleSmall(const FSlateFontInfo& FontStyle)
{
	FriendsFontStyleSmall = FontStyle;
	return *this;
}


FFriendsAndChatStyle& FFriendsAndChatStyle::SetMenuSetColor(const FLinearColor& InSetColor)
{
	MenuSetColor = InSetColor;
	return *this;
}


FFriendsAndChatStyle& FFriendsAndChatStyle::SetMenuUnsetColor(const FLinearColor& InUnsetColor)
{
	MenuUnsetColor = InUnsetColor;
	return *this;
}


FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendItemStyle(const FComboButtonStyle& InFriendItemStyle)
{
	FriendItemStyle = InFriendItemStyle;
	return *this;
}


const FFriendsAndChatStyle& FFriendsAndChatStyle::GetDefault()
{
	static FFriendsAndChatStyle Default;
	return Default;
}
