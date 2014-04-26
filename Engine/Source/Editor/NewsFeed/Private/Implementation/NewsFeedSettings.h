// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	NewsFeedSettings.h: Declares the UNewsFeedSettings class.
=============================================================================*/

#pragma once

#include "NewsFeedSettings.generated.h"


/**
 * Enumerates sources for news feed data.
 */
UENUM()
enum ENewsFeedSource
{
	/**
	 * Fetch the news feed from the CDN.
	 */
	NEWSFEED_Cdn,

	/**
	 * Fetch the news feed from the local file system (for testing purposes only).
	 */
	NEWSFEED_Local,

	/**
	 * Fetch the news feed with MCP (not implemented yet).
	 */
	NEWSFEED_Mcp,
};


/**
 * Holds the settings for the news feed.
 */
UCLASS(config=EditorGameAgnostic)
class UNewsFeedSettings
	: public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/**
	 * The URL at which the news feed data files are located when using the Source=Cdn.
	 */
	UPROPERTY(config)
	FString CdnSourceUrl;

	/**
	 * The path to the local data files when using Source=Local.
	 */
	UPROPERTY(config)
	FString LocalSourcePath;

	/**
	 * The source from which to fetch the news feed data.
	 *
	 * Use Local for testing, NEWSFEED_Cdn for production.
	 */
	UPROPERTY(config)
	TEnumAsByte<ENewsFeedSource> Source;

public:

	/**
	 * The maximum number of news items to show.
	 */
	UPROPERTY(config, EditAnywhere, Category=Display)
	int32 MaxItemsToShow;

	/**
	 * List of news feed items that have been marked as read.
	 */
	UPROPERTY(config)
	TArray<FGuid> ReadItems;

	/**
	 * Whether to show only unread news feed items.
	 */
	UPROPERTY(config, EditAnywhere, Category=Display)
	bool ShowOnlyUnreadItems;

public:

	/**
	 * Returns an event delegate that is executed when a setting has changed.
	 *
	 * @return The delegate.
	 */
	DECLARE_EVENT_OneParam(UNewsFeedSettings, FSettingChangedEvent, FName /*PropertyName*/);
	FSettingChangedEvent& OnSettingChanged( ) { return SettingChangedEvent; }

protected:

	// Begin UObject overrides

	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent ) OVERRIDE;

	// End UObject overrides

private:

	// Holds an event delegate that is executed when a setting has changed.
	FSettingChangedEvent SettingChangedEvent;
};
