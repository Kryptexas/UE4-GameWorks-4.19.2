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

	/** Friends List Critical Button style */
	UPROPERTY()
	FButtonStyle FriendListCriticalButtonStyle;
	FFriendsAndChatStyle& SetFriendsListCriticalButtonStyle(const FButtonStyle& ButtonStyle);

	UPROPERTY()
	FButtonStyle FriendListItemButtonStyle;
	FFriendsAndChatStyle& SetFriendsListItemButtonStyle(const FButtonStyle& ButtonStyle);

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

	/** Fortnite Image brush style */
	UPROPERTY(EditAnywhere, Category = Appearance)
	FSlateBrush FortniteImageBrush;
	FFriendsAndChatStyle& SetFortniteImageBrush(const FSlateBrush& BrushStyle);

	/** Launcher Image brush style */
	UPROPERTY(EditAnywhere, Category = Appearance)
	FSlateBrush LauncherImageBrush;
	FFriendsAndChatStyle& SetLauncherImageBrush(const FSlateBrush& BrushStyle);

	/** Friend combo dropdown Image */
	UPROPERTY(EditAnywhere, Category = Appearance)
	FSlateBrush FriendsComboDropdownImageBrush;
	FFriendsAndChatStyle& SetFriendsComboDropdownImageBrush(const FSlateBrush& BrushStyle);

	/** Friend combo callout Image */
	UPROPERTY(EditAnywhere, Category = Appearance)
	FSlateBrush FriendsCalloutBrush;
	FFriendsAndChatStyle& SetFriendsCalloutBrush(const FSlateBrush& BrushStyle);

	/** Offline brush style */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush OfflineBrush;
	FFriendsAndChatStyle& SetOfflineBrush(const FSlateBrush& InOffLine);

	/** Online brush style */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush OnlineBrush;
	FFriendsAndChatStyle& SetOnlineBrush(const FSlateBrush& InOnLine);

	/** Away brush style */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush AwayBrush;
	FFriendsAndChatStyle& SetAwayBrush(const FSlateBrush& AwayBrush);

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

	/** Default Font Color */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FLinearColor DefaultFontColor;
	FFriendsAndChatStyle& SetDefaultFontColor(const FLinearColor& InFontColor);

	UPROPERTY(EditAnywhere, Category=Appearance)
	FLinearColor DefaultChatColor;
	FFriendsAndChatStyle& SetDefaultChatColor(const FLinearColor& InFontColor);

	UPROPERTY(EditAnywhere, Category=Appearance)
	FLinearColor WhisplerChatColor;
	FFriendsAndChatStyle& SetWhisplerChatColor(const FLinearColor& InFontColor);

	UPROPERTY(EditAnywhere, Category=Appearance)
	FLinearColor PartyChatColor;
	FFriendsAndChatStyle& SetPartyChatColor(const FLinearColor& InFontColor);

	UPROPERTY(EditAnywhere, Category=Appearance)
	FLinearColor NetworkChatColor;
	FFriendsAndChatStyle& SetNetworkChatColor(const FLinearColor& InFontColor);

	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush ChatGlobalBrush;
	FFriendsAndChatStyle& SetChatGlobalBrush(const FSlateBrush& Brush);

	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush ChatPartyBrush;
	FFriendsAndChatStyle& SetChatPartyBrush(const FSlateBrush& Brush);

	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush ChatWhisperBrush;
	FFriendsAndChatStyle& SetChatWhisperBrush(const FSlateBrush& Brush);
};
