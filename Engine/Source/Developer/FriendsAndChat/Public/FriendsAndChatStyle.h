// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FriendsListStyle.h"
#include "FriendsComboStyle.h"
#include "FriendsChatStyle.h"
#include "FriendsChatChromeStyle.h"
#include "FriendsFontStyle.h"

#include "FriendsAndChatStyle.generated.h"

/**
 * Interface for the services manager.
 */
USTRUCT()
struct FRIENDSANDCHAT_API FFriendsAndChatStyle
	: public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	// Default Constructor
	FFriendsAndChatStyle() { }

	// Default Destructor
	virtual ~FFriendsAndChatStyle() { }

	/**
	 * Override widget style function.
	 */
	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const override { }

	// Holds the widget type name
	static const FName TypeName;

	/**
	 * Get the type name.
	 * @return the type name
	 */
	virtual const FName GetTypeName() const override { return TypeName; };

	/**
	 * Get the default style.
	 * @return the default style
	 */
	static const FFriendsAndChatStyle& GetDefault();

	// Common Style

	/** Window edging style */
	UPROPERTY(EditAnywhere, Category = Appearance)
	FSlateBrush WindowEdgingBrush;
	FFriendsAndChatStyle& SetWindowEdgingBrush(const FSlateBrush& Value);

	UPROPERTY(EditAnywhere, Category = Appearance)
	FMargin BorderPadding;
	FFriendsAndChatStyle& SetBorderPadding(const FMargin& InBorderPadding);

	UPROPERTY(EditAnywhere, Category = Appearance)
	FScrollBarStyle ScrollBarStyle;
	FFriendsAndChatStyle& SetScrollbarStyle(const FScrollBarStyle& InScrollBarStyle);

	UPROPERTY(EditAnywhere, Category = Appearance)
	FWindowStyle WindowStyle;
	FFriendsAndChatStyle& SetWindowStyle(const FWindowStyle& InStyle);

	/** SFriendActions Action Button style */
	UPROPERTY()
	FButtonStyle ActionButtonStyle;
	FFriendsAndChatStyle& SetActionButtonStyle(const FButtonStyle& ButtonStyle);

	UPROPERTY()
	FFriendsFontStyle FriendsNormalFontStyle;
	FFriendsAndChatStyle& SetNormalFriendsFontStyle(const FFriendsFontStyle& FontStyle);

	UPROPERTY()
	FFriendsListStyle FriendsListStyle;
	FFriendsAndChatStyle& SetFriendsListStyle(const FFriendsListStyle& InFriendsListStyle);

	UPROPERTY()
	FFriendsComboStyle FriendsComboStyle;
	FFriendsAndChatStyle& SetFriendsComboStyle(const FFriendsComboStyle& InFriendsListStyle);

	UPROPERTY()
	FFriendsChatStyle FriendsChatStyle;
	FFriendsAndChatStyle& SetFriendsChatStyle(const FFriendsChatStyle& InFriendsChatStyle);

	UPROPERTY()
	FFriendsChatChromeStyle FriendsChatChromeStyle;
	FFriendsAndChatStyle& SetFriendsChatChromeStyle(const FFriendsChatChromeStyle& InFriendsChatChromeStyle);
};

/** Manages the style which provides resources for the rich text widget. */
class FRIENDSANDCHAT_API FFriendsAndChatModuleStyle
{
public:

	static void Initialize(FFriendsAndChatStyle FriendStyle);

	static void Shutdown();

	/** reloads textures used by slate renderer */
	static void ReloadTextures();

	/** @return The Slate style set for the Friends and chat module */
	static const ISlateStyle& Get();

	static FName GetStyleSetName();

	static TSharedPtr<class FFriendsFontStyleService> GetStyleService();

private:

	static TSharedRef< class FSlateStyleSet > Create(FFriendsAndChatStyle FriendStyle);

private:

	static TSharedPtr< class FSlateStyleSet > FriendsAndChatModuleStyleInstance;
	static TSharedPtr< class FFriendsFontStyleService > FriendsFontStyleService;
};
