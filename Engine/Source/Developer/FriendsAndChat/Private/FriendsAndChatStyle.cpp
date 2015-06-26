// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"

#include "FriendsFontStyleService.h"
const FName FFriendsAndChatStyle::TypeName( TEXT("FFriendsAndChatStyle") );

FFriendsAndChatStyle& FFriendsAndChatStyle::SetNormalFriendsFontStyle(const FFriendsFontStyle& FontStyle)
{
	FriendsNormalFontStyle = FontStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendsListStyle(const FFriendsListStyle& InFriendsListStyle)
{
	FriendsListStyle = InFriendsListStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendsComboStyle(const FFriendsComboStyle& InFriendsComboStyle)
{
	FriendsComboStyle = InFriendsComboStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendsChatStyle(const FFriendsChatStyle& InFriendsChatStyle)
{
	FriendsChatStyle = InFriendsChatStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetFriendsChatChromeStyle(const FFriendsChatChromeStyle& InFriendsChatChromeStyle)
{
	FriendsChatChromeStyle = InFriendsChatChromeStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetWindowEdgingBrush(const FSlateBrush& Value)
{
	WindowEdgingBrush = Value;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetBorderPadding(const FMargin& Padding)
{
	BorderPadding = Padding;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetScrollbarStyle(const FScrollBarStyle& InScrollBarStyle)
{
	ScrollBarStyle = InScrollBarStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetWindowStyle(const FWindowStyle& InStyle)
{
	WindowStyle = InStyle;
	return *this;
}

FFriendsAndChatStyle& FFriendsAndChatStyle::SetActionButtonStyle(const FButtonStyle& ButtonStyle)
{
	ActionButtonStyle = ButtonStyle;
	return *this;
}

const FFriendsAndChatStyle& FFriendsAndChatStyle::GetDefault()
{
	static FFriendsAndChatStyle Default;
	return Default;
}

/**
	Module style set
*/
TSharedPtr< FSlateStyleSet > FFriendsAndChatModuleStyle::FriendsAndChatModuleStyleInstance = NULL;
TSharedPtr< FFriendsFontStyleService > FFriendsAndChatModuleStyle::FriendsFontStyleService = NULL;

void FFriendsAndChatModuleStyle::Initialize(FFriendsAndChatStyle FriendStyle)
{
	if ( !FriendsAndChatModuleStyleInstance.IsValid() )
	{
		FriendsAndChatModuleStyleInstance = Create(FriendStyle);
		FSlateStyleRegistry::RegisterSlateStyle( *FriendsAndChatModuleStyleInstance );

		FriendsFontStyleService = FFriendsFontStyleServiceFactory::Create();
		FriendsFontStyleService->SetFontStyles(FriendStyle.FriendsNormalFontStyle);
	}
}

void FFriendsAndChatModuleStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle( *FriendsAndChatModuleStyleInstance );
	ensure( FriendsAndChatModuleStyleInstance.IsUnique() );
	FriendsAndChatModuleStyleInstance.Reset();
}

FName FFriendsAndChatModuleStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("FriendsAndChat"));
	return StyleSetName;
}

TSharedPtr<FFriendsFontStyleService> FFriendsAndChatModuleStyle::GetStyleService()
{
	if (!FriendsFontStyleService.IsValid())
	{
		FriendsFontStyleService = FFriendsFontStyleServiceFactory::Create();
	}
	return FriendsFontStyleService;
}

TSharedRef< FSlateStyleSet > FFriendsAndChatModuleStyle::Create(FFriendsAndChatStyle FriendStyle)
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("FriendsAndChatStyle"));

	const FTextBlockStyle DefaultText = FTextBlockStyle(FriendStyle.FriendsChatStyle.TextStyle)
		.SetFont(FriendStyle.FriendsNormalFontStyle.FriendsFontSmall);

	// Name Style
	const FTextBlockStyle GlobalChatFont = FTextBlockStyle(DefaultText)
		.SetFont(FriendStyle.FriendsNormalFontStyle.FriendsFontNormalBold)
		.SetColorAndOpacity(FriendStyle.FriendsChatStyle.GlobalChatColor);

	const FTextBlockStyle GameChatFont = FTextBlockStyle(DefaultText)
		.SetFont(FriendStyle.FriendsNormalFontStyle.FriendsFontNormalBold)
		.SetColorAndOpacity(FriendStyle.FriendsChatStyle.GameChatColor);

	const FTextBlockStyle PartyChatFont = FTextBlockStyle(DefaultText)
		.SetFont(FriendStyle.FriendsNormalFontStyle.FriendsFontNormalBold)
		.SetColorAndOpacity(FriendStyle.FriendsChatStyle.PartyChatColor);

	const FTextBlockStyle WhisperChatFont = FTextBlockStyle(DefaultText)
		.SetFont(FriendStyle.FriendsNormalFontStyle.FriendsFontNormalBold)
		.SetColorAndOpacity(FriendStyle.FriendsChatStyle.WhisplerChatColor);

	const FButtonStyle UserNameButton = FButtonStyle()
		.SetNormal(FSlateNoResource())
		.SetPressed(FSlateNoResource())
		.SetHovered(FSlateNoResource());

	const FHyperlinkStyle GlobalChatHyperlink = FHyperlinkStyle()
		.SetUnderlineStyle(UserNameButton)
		.SetTextStyle(GlobalChatFont)
		.SetPadding(FMargin(0.0f));

	const FHyperlinkStyle GameChatHyperlink = FHyperlinkStyle()
		.SetUnderlineStyle(UserNameButton)
		.SetTextStyle(GameChatFont)
		.SetPadding(FMargin(0.0f));

	const FHyperlinkStyle PartyChatHyperlink = FHyperlinkStyle()
		.SetUnderlineStyle(UserNameButton)
		.SetTextStyle(PartyChatFont)
		.SetPadding(FMargin(0.0f));

	const FHyperlinkStyle WhisperChatHyperlink = FHyperlinkStyle()
		.SetUnderlineStyle(UserNameButton)
		.SetTextStyle(WhisperChatFont)
		.SetPadding(FMargin(0.0f));

	const FHyperlinkStyle DefaultChatHyperlink = FHyperlinkStyle()
		.SetUnderlineStyle(UserNameButton)
		.SetTextStyle(DefaultText)
		.SetPadding(FMargin(0.0f));

	Style->Set("UserNameTextStyle.Default", DefaultText);

	Style->Set("UserNameTextStyle.GlobalHyperlink", GlobalChatHyperlink);
	Style->Set("UserNameTextStyle.GameHyperlink", GameChatHyperlink);
	Style->Set("UserNameTextStyle.PartyHyperlink", GameChatHyperlink);
	Style->Set("UserNameTextStyle.Whisperlink", WhisperChatHyperlink);
	Style->Set("UserNameTextStyle.DefaultHyperlink", DefaultChatHyperlink);
	Style->Set("UserNameTextStyle.GlobalTextStyle", GlobalChatFont);
	Style->Set("UserNameTextStyle.GameTextStyle", GameChatFont);
	Style->Set("UserNameTextStyle.PartyTextStyle", PartyChatFont);
	Style->Set("UserNameTextStyle.WhisperTextStyle", WhisperChatFont);

	Style->Set("MessageBreak", FTextBlockStyle(DefaultText)
		.SetFont(FSlateFontInfo(
		FriendStyle.FriendsNormalFontStyle.FriendsFontSmall.FontObject,
		6,
		FriendStyle.FriendsNormalFontStyle.FriendsFontSmall.TypefaceFontName
		)));

	Style->Set("GlobalChatIcon", FInlineTextImageStyle()
		.SetImage(FriendStyle.FriendsChatStyle.ChatGlobalBrush)
		.SetBaseline(0));

	Style->Set("WhisperChatIcon", FInlineTextImageStyle()
		.SetImage(FriendStyle.FriendsChatStyle.ChatWhisperBrush)
		.SetBaseline(0));

	Style->Set("PartyChatIcon", FInlineTextImageStyle()
		.SetImage(FriendStyle.FriendsChatStyle.ChatGameBrush)
		.SetBaseline(0));

	return Style;
}

void FFriendsAndChatModuleStyle::ReloadTextures()
{
	FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
}

const ISlateStyle& FFriendsAndChatModuleStyle::Get()
{
	return *FriendsAndChatModuleStyleInstance;
}