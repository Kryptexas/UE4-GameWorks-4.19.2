// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	FriendsAndChatStyle.h: Declares the FFriendsAndChatStyle interface.
=============================================================================*/
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
	FFriendsAndChatStyle(){}

	// Default Destructor
	virtual ~FFriendsAndChatStyle() {}

	/**
	 * Override widget style function.
	 */
	virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const OVERRIDE
	{}

	// Holds the widget type name
	static const FName TypeName;

	/**
	 * Get the type name.
	 * @return the type name
	 */
	virtual const FName GetTypeName() const OVERRIDE { return TypeName; };

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

	/** Toggle checkbox Style */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FCheckBoxStyle CheckBoxToggleStyle;
	FFriendsAndChatStyle& SetCheckboxToggleStyle(const FCheckBoxStyle& InCheckboxStyle);

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
