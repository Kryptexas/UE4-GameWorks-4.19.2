// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FriendsFontStyle.h"
#include "FriendsComboStyle.h"

#include "FriendsListStyle.generated.h"

/**
 * Interface for the services manager.
 */
USTRUCT()
struct FRIENDSANDCHAT_API FFriendsListStyle
	: public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	// Default Constructor
	FFriendsListStyle() { }

	// Default Destructor
	virtual ~FFriendsListStyle() { }

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
	static const FFriendsListStyle& GetDefault();

	// Friends List Style

	UPROPERTY()
	FFriendsFontStyle FriendsNormalFontStyle;
	FFriendsListStyle& SetNormalFriendsFontStyle(const FFriendsFontStyle& FontStyle);

	UPROPERTY()
	FButtonStyle GlobalChatButtonStyle;
	FFriendsListStyle& SetGlobalChatButtonStyle(const FButtonStyle& ButtonStyle);

	UPROPERTY()
	FFriendsComboStyle FriendsComboStyle;
	FFriendsListStyle& SetFriendsComboStyle(const FFriendsComboStyle& FriendsComboStyle);

	UPROPERTY(EditAnywhere, Category = Appearance)
	FScrollBarStyle ScrollBarStyle;
	FFriendsListStyle& SetScrollbarStyle(const FScrollBarStyle& InScrollBarStyle);

	UPROPERTY()
	FSlateColor ButtonInvertedForegroundColor;
	FFriendsListStyle& SetButtonInvertedForegroundColor(const FSlateColor& Value);

	UPROPERTY()
	FSlateColor ButtonForegroundColor;
	FFriendsListStyle& SetButtonForegroundColor(const FSlateColor& Value);

	UPROPERTY(EditAnywhere, Category = Appearance)
	FMargin BorderPadding;
	FFriendsListStyle& SetBorderPadding(const FMargin& InBorderPadding);

	UPROPERTY()
	FFriendsFontStyle NormalFont;
	FFriendsListStyle& SetNormalFont(const FFriendsFontStyle& FontStyle);

	/** Friends List Open Button style */
	UPROPERTY()
	FButtonStyle FriendListOpenButtonStyle;
	FFriendsListStyle& SetFriendsListOpenButtonStyle(const FButtonStyle& ButtonStyle);

	/** Friends General Purpose Button style */
	UPROPERTY()
	FButtonStyle FriendGeneralButtonStyle;
	FFriendsListStyle& SetFriendGeneralButtonStyle(const FButtonStyle& ButtonStyle);

	/** Friends List Action Button style */
	UPROPERTY()
	FButtonStyle FriendListActionButtonStyle;
	FFriendsListStyle& SetFriendListActionButtonStyle(const FButtonStyle& ButtonStyle);

	/** Friends List Critical Button style */
	UPROPERTY()
	FButtonStyle FriendListCriticalButtonStyle;
	FFriendsListStyle& SetFriendsListCriticalButtonStyle(const FButtonStyle& ButtonStyle);

	/** Friends List Emphasis Button style */
	UPROPERTY()
	FButtonStyle FriendListEmphasisButtonStyle;
	FFriendsListStyle& SetFriendsListEmphasisButtonStyle(const FButtonStyle& ButtonStyle);

	UPROPERTY()
	FButtonStyle FriendListItemButtonStyle;
	FFriendsListStyle& SetFriendsListItemButtonStyle(const FButtonStyle& ButtonStyle);

	UPROPERTY()
	FButtonStyle FriendListItemButtonSimpleStyle;
	FFriendsListStyle& SetFriendsListItemButtonSimpleStyle(const FButtonStyle& ButtonStyle);

	/** Friends List Close button style */
	UPROPERTY()
	FButtonStyle FriendListCloseButtonStyle;
	FFriendsListStyle& SetFriendsListClosedButtonStyle(const FButtonStyle& ButtonStyle);

	/** Add Friend Close button style */
	UPROPERTY()
	FButtonStyle AddFriendCloseButtonStyle;
	FFriendsListStyle& SetAddFriendCloseButtonStyle(const FButtonStyle& ButtonStyle);

	/** Optional content for the Add Friend button */
	UPROPERTY()
	FSlateBrush AddFriendButtonContentBrush;
	FFriendsListStyle& SetAddFriendButtonContentBrush(const FSlateBrush& BrushStyle);

	/** Optional content for the Add Friend button (hovered) */
	UPROPERTY()
	FSlateBrush AddFriendButtonContentHoveredBrush;
	FFriendsListStyle& SetAddFriendButtonContentHoveredBrush(const FSlateBrush& BrushStyle);

	/** Friend Image brush style */
	UPROPERTY(EditAnywhere, Category = Appearance)
	FSlateBrush FriendImageBrush;
	FFriendsListStyle& SetFriendImageBrush(const FSlateBrush& BrushStyle);

	/** Fortnite Image brush style */
	UPROPERTY(EditAnywhere, Category = Appearance)
	FSlateBrush FortniteImageBrush;
	FFriendsListStyle& SetFortniteImageBrush(const FSlateBrush& BrushStyle);

	/** Launcher Image brush style */
	UPROPERTY(EditAnywhere, Category = Appearance)
	FSlateBrush LauncherImageBrush;
	FFriendsListStyle& SetLauncherImageBrush(const FSlateBrush& BrushStyle);

	/** UnrealTournament Image brush style */
	UPROPERTY(EditAnywhere, Category = Appearance)
	FSlateBrush UTImageBrush;
	FFriendsListStyle& SetUTImageBrush(const FSlateBrush& BrushStyle);

	/** Friends Add Icon */
	UPROPERTY(EditAnywhere, Category = Appearance)
	FSlateBrush FriendsAddImageBrush;
	FFriendsListStyle& SetFriendsAddImageBrush(const FSlateBrush& BrushStyle);

	/** Offline brush style */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush OfflineBrush;
	FFriendsListStyle& SetOfflineBrush(const FSlateBrush& InOffLine);

	/** Online brush style */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush OnlineBrush;
	FFriendsListStyle& SetOnlineBrush(const FSlateBrush& InOnLine);

	/** Away brush style */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush AwayBrush;
	FFriendsListStyle& SetAwayBrush(const FSlateBrush& AwayBrush);

	/** Window background style */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush Background;
	FFriendsListStyle& SetBackgroundBrush(const FSlateBrush& InBackground);

	/** Friend container header */
	UPROPERTY(EditAnywhere, Category = Appearance)
	FSlateBrush FriendContainerHeader;
	FFriendsListStyle& SetFriendContainerHeader(const FSlateBrush& InFriendContainerHeader);

	/** Friend list header */
	UPROPERTY(EditAnywhere, Category = Appearance)
	FSlateBrush FriendListHeader;
	FFriendsListStyle& SetFriendListHeader(const FSlateBrush& InFriendListHeader);

	/** Friend user header background */
	UPROPERTY(EditAnywhere, Category = Appearance)
	FSlateBrush FriendUserHeaderBackground;
	FFriendsListStyle& SetFriendUserHeaderBackground(const FSlateBrush& InFriendUserHeaderBackground);

	/** Friends window background */
	UPROPERTY(EditAnywhere, Category = Appearance)
	FSlateBrush FriendsContainerBackground;
	FFriendsListStyle& SetFriendContainerBackground(const FSlateBrush& InFriendContainerBackground);

	UPROPERTY(EditAnywhere, Category = Appearance)
	FLinearColor FriendListActionFontColor;
	FFriendsListStyle& SetFriendListActionFontColor(const FLinearColor& InColor);

	UPROPERTY(EditAnywhere, Category = Appearance)
	FEditableTextBoxStyle AddFriendEditableTextStyle;
	FFriendsListStyle& SetAddFriendEditableTextStyle(const FEditableTextBoxStyle& InEditableTextStyle);

	UPROPERTY(EditAnywhere, Category = Appearance)
	FVector2D UserPresenceImageSize;
	FFriendsListStyle& SetUserPresenceImageSize(const FVector2D& InUserPresenceImageSize);

	UPROPERTY(EditAnywhere, Category = Appearance)
	FVector2D AddFriendButtonSize;
	FFriendsListStyle& SetAddFriendButtonSize(const FVector2D& Value);

	UPROPERTY(EditAnywhere, Category = Appearance)
	FMargin UserHeaderPadding;
	FFriendsListStyle& SetUserHeaderPadding(const FMargin& InUserHeaderPadding);

	UPROPERTY(EditAnywhere, Category = Appearance)
	float FriendsListWidth;
	FFriendsListStyle& SetFriendsListWidth(const float FriendsListLength);

	UPROPERTY(EditAnywhere, Category = Appearance)
	bool HasUserHeader;
	FFriendsListStyle& SetHasUserHeader(bool InHasUserHeader);

// Clan Settings

	UPROPERTY(EditAnywhere, Category = Appearance)
	FSlateBrush ClanDetailsBrush;
	FFriendsListStyle& SetClanDetailsBrush(const FSlateBrush& Brush);

	UPROPERTY(EditAnywhere, Category = Appearance)
	FSlateBrush ClanMembersBrush;
	FFriendsListStyle& SetClanMembersBrush(const FSlateBrush& Brush);
};

