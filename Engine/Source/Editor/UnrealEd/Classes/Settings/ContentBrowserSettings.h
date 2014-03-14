// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ContentBrowserSettings.h: Declares the UContentBrowserSettings class.
=============================================================================*/

#pragma once


#include "ContentBrowserSettings.generated.h"


/**
 * Implements the Level Editor's loading and saving settings.
 */
UCLASS(config=EditorUserSettings)
class UNREALED_API UContentBrowserSettings
	: public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/** The number of objects to load at once in the Content Browser before displaying a warning about loading many assets */
	UPROPERTY(EditAnywhere, config, Category=ContentBrowser, meta=(DisplayName = "Assets to Load at Once Before Warning"))
	int32 NumObjectsToLoadBeforeWarning;

	/** Whether to show only assets in selected folders in the Content Browser, excluding sub-folders. Note that this always the case when 'Display Folders' is enabled. */
	UPROPERTY(config)
	bool ShowOnlyAssetsInSelectedFolders;

	/** Whether to render thumbnails for loaded assets in real-time in the Content Browser */
	UPROPERTY(config)
	bool RealTimeThumbnails;

	/** Whether to display folders in the assets view of the content browser. Note that this implies 'Show Only Assets in Selected Folders'. */
	UPROPERTY(config)
	bool DisplayFolders;

public:

	/** Sets whether we are allowed to display the engine folder or not, optional flag for setting override instead */
	void SetDisplayEngineFolder( bool bInDisplayEngineFolder, bool bOverride = false )
	{ 
		bOverride ? OverrideDisplayEngineFolder = bInDisplayEngineFolder : DisplayEngineFolder = bInDisplayEngineFolder;
	}
	/** Gets whether we are allowed to display the engine folder or not, optional flag ignoring the override */
	bool GetDisplayEngineFolder( bool bExcludeOverride = false ) const
	{ 
		return ( ( bExcludeOverride ? false : OverrideDisplayEngineFolder ) | DisplayEngineFolder );
	}

	/** Sets whether we are allowed to display the developers folder or not, optional flag for setting override instead */
	void SetDisplayDevelopersFolder( bool bInDisplayDevelopersFolder, bool bOverride = false )
	{ 
		bOverride ? OverrideDisplayDevelopersFolder = bInDisplayDevelopersFolder : DisplayDevelopersFolder = bInDisplayDevelopersFolder;
	}

	/** Gets whether we are allowed to display the developers folder or not, optional flag ignoring the override */
	bool GetDisplayDevelopersFolder( bool bExcludeOverride = false ) const
	{ 
		return ( ( bExcludeOverride ? false : OverrideDisplayDevelopersFolder ) | DisplayDevelopersFolder );
	}

	/**
	 * Returns an event delegate that is executed when a setting has changed.
	 *
	 * @return The delegate.
	 */
	DECLARE_EVENT_OneParam(UContentBrowserSettings, FSettingChangedEvent, FName /*PropertyName*/);
	FSettingChangedEvent& OnSettingChanged( ) { return SettingChangedEvent; }

protected:

	// Begin UObject overrides

	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent ) OVERRIDE;

	// End UObject overrides

private:

	/** Whether to display the engine folder in the assets view of the content browser. */
	UPROPERTY(config)
	bool DisplayEngineFolder;

	/** If true, overrides the DisplayEngine setting */
	bool OverrideDisplayEngineFolder;

	/** Whether to display the developers folder in the path view of the content browser */
	UPROPERTY(config)
	bool DisplayDevelopersFolder;

	/** If true, overrides the DisplayDev setting */
	bool OverrideDisplayDevelopersFolder;

	// Holds an event delegate that is executed when a setting has changed.
	FSettingChangedEvent SettingChangedEvent;
};
