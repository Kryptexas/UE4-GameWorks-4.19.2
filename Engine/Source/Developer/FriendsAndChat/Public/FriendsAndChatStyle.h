// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

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

	/** Close button style */
	UPROPERTY()
	FButtonStyle CloseButtonStyle;
	FFriendsAndChatStyle& SetCloseButtonStyle( const FButtonStyle& InButtonStyle);

	/** Minimize button style */
	UPROPERTY()
	FButtonStyle MinimizeButtonStyle;
	FFriendsAndChatStyle& SetMinimizeStyle( const FButtonStyle& InButtonStyle);

	/** Search button style */
	UPROPERTY()
	FButtonStyle SearchButtonStyle;
	FFriendsAndChatStyle& SetSearchButtonStyle( const FButtonStyle& InButtonStyle);

	/** Friends List Open Button style */
	UPROPERTY()
	FButtonStyle FriendListOpenButtonStyle;
	FFriendsAndChatStyle& SetFriendsListOpenButtonStyle(const FButtonStyle& ButtonStyle);

	/** Friends List Action Button style */
	UPROPERTY()
	FButtonStyle FriendListActionButtonStyle;
	FFriendsAndChatStyle& SetFriendsListActionButtonStyle(const FButtonStyle& ButtonStyle);

	/** Friends List Close button style */
	UPROPERTY()
	FButtonStyle FriendListCloseButtonStyle;
	FFriendsAndChatStyle& SetFriendsListClosedButtonStyle(const FButtonStyle& ButtonStyle);

	/** Title Bar brush style */
	UPROPERTY(EditAnywhere, Category = Appearance)
	FSlateBrush TitleBarBrush;
	FFriendsAndChatStyle& SetTitleBarBrush(const FSlateBrush& BrushStyle);

	/** Friend Image brush style */
	UPROPERTY(EditAnywhere, Category = Appearance)
	FSlateBrush FriendImageBrush;
	FFriendsAndChatStyle& SetFriendImageBrush(const FSlateBrush& BrushStyle);

	/** Friend combo dropdown Image */
	UPROPERTY(EditAnywhere, Category = Appearance)
	FSlateBrush FriendsComboDropdownImageBrush;
	FFriendsAndChatStyle& SetFriendsComboDropdownImageBrush(const FSlateBrush& BrushStyle);

	/** Offline brush style */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush OfflineBrush;
	FFriendsAndChatStyle& SetOfflineBrush(const FSlateBrush& InOffLine);

	/** Online brush style */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush OnlineBrush;
	FFriendsAndChatStyle& SetOnlineBrush(const FSlateBrush& InOnLine);

	/** Window background style */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush Background;
	FFriendsAndChatStyle& SetBackgroundBrush(const FSlateBrush& InBackground);

	/** Text Style */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FTextBlockStyle TextStyle;
	FFriendsAndChatStyle& SetTextStyle(const FTextBlockStyle& InTextStyle);

	/** Font Style */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateFontInfo FriendsFontStyle;
	FFriendsAndChatStyle& SetFontStyle(const FSlateFontInfo& InFontStyle);

	/** Font Style */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateFontInfo FriendsFontStyleSmall;
	FFriendsAndChatStyle& SetFontStyleSmall(const FSlateFontInfo& InFontStyle);

	/** Menu set color */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FLinearColor MenuSetColor;
	FFriendsAndChatStyle& SetMenuSetColor(const FLinearColor& InSetColor);

	/** Menu Unset color */
	FLinearColor MenuUnsetColor;
	FFriendsAndChatStyle& SetMenuUnsetColor(const FLinearColor& InUnsetColor);

	/** Friend List Item Style */
	FComboButtonStyle FriendItemStyle;
	FFriendsAndChatStyle& SetFriendItemStyle(const FComboButtonStyle& InFriendItemStyle);
};
