// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "ContentBrowserDelegates.h"
#include "Runtime/AssetRegistry/Public/ARFilter.h"
#include "Developer/CollectionManager/Public/CollectionManagerTypes.h"
#include "IFilter.h"
#include "FilterCollection.h"
#include "AssetThumbnail.h"

typedef const FAssetData& AssetFilterType;
typedef TFilterCollection<AssetFilterType> AssetFilterCollectionType;

/** The view modes used in SAssetView */
namespace EAssetViewType
{
	enum Type
	{
		List,
		Tile,
		Column,

		MAX
	};
}

/** A struct containing details about how the asset picker should behave */
struct FAssetPickerConfig
{
	/** The selection mode the picker should use */
	ESelectionMode::Type SelectionMode;

	/** A pointer to an existing delegate which the AssetView will register a function which clears the current selection */
	FClearSelectionDelegate* ClearSelectionDelegate;

	/** An array of pointers to existing delegates which the AssetView will register a function which returns the current selection */
	TArray<FGetCurrentSelectionDelegate*> GetCurrentSelectionDelegates;

	/** A pointer to an existing delegate which the AssetView will register a function which increments the selection */
	FAdjustSelectionDelegate* AdjustSelectionDelegate;

	/** The asset registry filter to use to cull results */
	FARFilter Filter;

	/** Dynamic filters that get applied after the backend and frontend filters*/
	TSharedRef< AssetFilterCollectionType > ExtraFilters;

	/** The text to highlight on assets */
	TAttribute< FText > HighlightedText;

	/** The label visibility on asset items */
	TAttribute< EVisibility > LabelVisibility;

	/** The contents of the label on the thumbnail */
	EThumbnailLabel::Type ThumbnailLabel;

	/** The default scale for thumbnails. [0-1] range */
	TAttribute< float > ThumbnailScale;
	FOnThumbnailScaleChanged ThumbnailScaleChanged;

	/** Only display results in these collections */
	TArray<FCollectionNameType> Collections;

	/** The asset that should be initially selected */
	FAssetData InitialAssetSelection;

	/** The delegate that fires when an asset was selected */
	FOnAssetSelected OnAssetSelected;

	/** The delegate that fires when an asset is dragged */
	FOnAssetDragged OnAssetDragged;

	/** The delegate that fires when an asset is clicked on */
	FOnAssetClicked OnAssetClicked;

	/** The delegate that fires when an asset is double clicked */
	FOnAssetDoubleClicked OnAssetDoubleClicked;

	/** The delegate that fires when an asset has enter pressed while selected */
	FOnAssetEnterPressed OnAssetEnterPressed;

	/** The delegate that fires when any number of assets are activated */
	FOnAssetsActivated OnAssetsActivated;

	/** The delegate that fires when an asset is right clicked and a context menu is requested */
	FOnGetAssetContextMenu OnGetAssetContextMenu;

	/** If more detailed filtering is required than simply Filter, this delegate will get fired for every asset to determine if it should be culled. */
	FOnShouldFilterAsset OnShouldFilterAsset;

	/** This delegate will be called in Details view when a new asset registry searchable tag is encountered, to
	    determine if it should be displayed or not.  If it returns true or isn't bound, the tag will be displayed normally. */
	FOnShouldDisplayAssetTag OnAssetTagWantsToBeDisplayed;

	/** The delegate that fires to construct the tooltip for the specified asset */
	FConstructToolTipForAsset ConstructToolTipForAsset;

	/** The default view mode */
	EAssetViewType::Type InitialAssetViewType;

	/** The text to show when there are no assets to show */
	TAttribute< FText > AssetShowWarningText;

	/** If true, the search box will gain focus when the asset picker is created */
	bool bFocusSearchBoxWhenOpened;

	/** If true, a "None" item will always appear at the top of the list */
	bool bAllowNullSelection;

	/** If true, show the bottom toolbar which shows # of assets selected, view mode buttons, etc... */
	bool bShowBottomToolbar;

	/** If false, auto-hide the search bar above */
	bool bAutohideSearchBar;

	/** Whether to allow dragging of items */
	bool bAllowDragging;

	/** Indicates if this view is allowed to show classes */
	bool bCanShowClasses;

	/** Indicates if the 'Show Folders' option should be visible */
	bool bCanShowFolders;

	/** Indicates if the 'Show Only Assets In Selection' option should be visible */
	bool bCanShowOnlyAssetsInSelectedFolders;

	/** Indicates if the 'Real-Time Thumbnails' option should be visible */
	bool bCanShowRealTimeThumbnails;

	/** Indicates if the 'Show Developers' option should be visible */
	bool bCanShowDevelopersFolder;

	FAssetPickerConfig()
		: SelectionMode( ESelectionMode::Multi )
		, ClearSelectionDelegate( NULL )
		, AdjustSelectionDelegate( NULL )
		, ExtraFilters( MakeShareable( new AssetFilterCollectionType() ) )
		, HighlightedText()
		, ThumbnailLabel( EThumbnailLabel::ClassName )
		, ThumbnailScale(0.25f) // A reasonable scale
		, ThumbnailScaleChanged()
		, InitialAssetViewType(EAssetViewType::Tile)
		, bFocusSearchBoxWhenOpened(true)
		, bAllowNullSelection(false)
		, bShowBottomToolbar(true)
		, bAutohideSearchBar(false)
		, bAllowDragging(true)
		, bCanShowClasses(true)
		, bCanShowFolders(false)
		, bCanShowOnlyAssetsInSelectedFolders(false)
		, bCanShowRealTimeThumbnails(false)
		, bCanShowDevelopersFolder(false)
	{}
};

/** A struct containing details about how the path picker should behave */
struct FPathPickerConfig
{
	/** The initial path to select. Leave empty to skip initial selection. */
	FString DefaultPath;

	/** The delegate that fires when a path was selected */
	FOnPathSelected OnPathSelected;

	/** The delegate that fires when a path is right clicked and a context menu is requested */
	FContentBrowserMenuExtender_SelectedPaths OnGetPathContextMenuExtender;

	/** If true, the search box will gain focus when the path picker is created */
	bool bFocusSearchBoxWhenOpened;

	/** If false, the context menu will not open when an item is right clicked */
	bool bAllowContextMenu;

	/** If true, will add the path specified in DefaultPath to the tree if it doesn't exist already */
	bool bAddDefaultPath;

	FPathPickerConfig()
		: bFocusSearchBoxWhenOpened(true)
		, bAllowContextMenu(true)
		, bAddDefaultPath(false)
	{}
};

/** A struct containing details about how the collection picker should behave */
struct FCollectionPickerConfig
{
	/** If true, collection buttons will be displayed */
	bool AllowCollectionButtons;

	/** If true, users will be able to access the right-click menu of a collection */
	bool AllowRightClickMenu;

	/** Called when a collection was selected */
	FOnCollectionSelected OnCollectionSelected;

	FCollectionPickerConfig()
		: AllowCollectionButtons(true)
		, AllowRightClickMenu(true)
		, OnCollectionSelected()
	{}
};

/**
 * Content browser module singleton
 */
class IContentBrowserSingleton
{
public:
	/** Virtual destructor */
	virtual ~IContentBrowserSingleton() {}

	/**
	 * Generates an asset picker widget locked to the specified FARFilter.
	 *
	 * @param AssetPickerConfig		A struct containing details about how the asset picker should behave				
	 * @return The asset picker widget
	 */
	virtual TSharedRef<class SWidget> CreateAssetPicker(const FAssetPickerConfig& AssetPickerConfig) = 0;

	/**
	 * Generates a path picker widget.
	 *
	 * @param PathPickerConfig		A struct containing details about how the path picker should behave				
	 * @return The path picker widget
	 */
	virtual TSharedRef<class SWidget> CreatePathPicker(const FPathPickerConfig& PathPickerConfig) = 0;

	/**
	 * Generates a collection picker widget.
	 *
	 * @param CollectionPickerConfig		A struct containing details about how the collection picker should behave				
	 * @return The collection picker widget
	 */
	virtual TSharedRef<class SWidget> CreateCollectionPicker(const FCollectionPickerConfig& CollectionPickerConfig) = 0;

	/** Returns true if there is at least one browser open that is eligible to be a primary content browser */
	virtual bool HasPrimaryContentBrowser() const = 0;

	/** Brings the primary content browser to the front or opens one if it does not exist. */
	virtual void FocusPrimaryContentBrowser() = 0;

	/** Sets up an inline-name for the creation of a new asset in the primary content browser using the specified path and the specified class and/or factory */
	virtual void CreateNewAsset(const FString& DefaultAssetName, const FString& PackagePath, UClass* AssetClass, UFactory* Factory) = 0;

	/** Selects the supplied assets in all content browsers. If bAllowLockedBrowsers is true, even locked browsers may handle the sync. Only set to true if the sync doesn't seem external to the content browser. */
	virtual void SyncBrowserToAssets(const TArray<class FAssetData>& AssetDataList, bool bAllowLockedBrowsers = false) = 0;
	virtual void SyncBrowserToAssets(const TArray<UObject*>& AssetList, bool bAllowLockedBrowsers = false) = 0;

	/** Generates a list of assets that are selected in the primary content browser */
	virtual void GetSelectedAssets(TArray<FAssetData>& SelectedAssets) = 0;
};