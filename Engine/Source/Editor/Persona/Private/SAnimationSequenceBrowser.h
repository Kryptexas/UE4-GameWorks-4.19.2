// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "AssetData.h"

//////////////////////////////////////////////////////////////////////////
// SAnimationSequenceBrowser

class SAnimationSequenceBrowser : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAnimationSequenceBrowser)
		: _Persona()
		{}

		SLATE_ARGUMENT(TSharedPtr<class FPersona>, Persona)
	SLATE_END_ARGS()
public:
	void Construct(const FArguments& InArgs);

	void OnAnimSelected(const FAssetData& AssetData);
	void OnRequestOpenAsset(const FAssetData& AssetData, bool bFromHistory);

	/** Delegate that handles creation of context menu */
	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets);

	/** Delegate to handle "Go To" context menu option 
		Sends selected assets to be highlighted in active Content Browser */
	void OnGoToInContentBrowser(TArray<FAssetData> ObjectsToSync);

	/** Delegate to handle "Save" context menu option */
	void SaveSelectedAssets(TArray<FAssetData> ObjectsToSave) const;

	/** Delegate to handle enabling the "Save" context menu option */
	bool CanSaveSelectedAssets(TArray<FAssetData> ObjectsToSave) const;

	/** Delegate to handle compression context menu option 
		Applies a chosen compression method to the selected assets */
	void OnApplyCompression(TArray<FAssetData> SelectedAssets);

	/** Delegate to handle Export FBX context menu option */
	void OnExportToFBX(TArray<FAssetData> SelectedAssets);

	/** This will allow duplicate the current object, and remap to new skeleton 
	 *	Only allowed for AnimSequence 
	 */
	void OnCreateCopy(TArray<FAssetData> Selected);

protected:
	bool CanShowColumnForAssetRegistryTag(FName AssetType, FName TagName) const;
	
	/** Populate supplied OutPackages with the packages for the supplied Assets array */
	void GetSelectedPackages(const TArray<FAssetData>& Assets, TArray<UPackage*>& OutPackages) const;

	/** Adds the supplied asset to the asset history */
	void AddAssetToHistory(const FAssetData& AssetData);

	void CacheOriginalAnimAssetHistory();

	/** Single step forward in history */
	FReply OnGoForwardInHistory();

	/** Single step back in history */
	FReply OnGoBackInHistory();

	/** Jumps immediately to an index in the history if valid */
	void GoToHistoryIndex(int32 InHistoryIdx);

	/** Returns TRUE if stepping backward in history is allowed */
	bool CanStepBackwardInHistory() const;

	/** Returns TRUE if stepping forward in history is allowed */
	bool CanStepForwardInHistory() const;

	/**
	 * Mouse down callback to display a history menu
	 *
	 * @param InMenuAnchor		This is the anchor the menu will use for positioning
	 */
	FReply OnMouseDownHisory( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, TWeakPtr< SMenuAnchor > InMenuAnchor );

	/** 
	 * Callback to create the history menu.
	 *
	 * @param bInBackHistory		TRUE if the back history is requested, FALSE if the forward history is
	 *
	 * @return						The menu widget displaying all available history
	 */
	TSharedRef<SWidget> CreateHistoryMenu(bool bInBackHistory) const;

protected:

	// Pointer back to persona tool that owns us
	TWeakPtr<class FPersona> PersonaPtr;

	// Set of tags to prevent creating details view columns for (infrequently used)
	TSet<FName> AssetRegistryTagsToIgnore;

	// List of recently opened assets
	TArray<FAssetData> AssetHistory;

	// Current position in the asset history
	int32 CurrentAssetHistoryIndex;

	// Max assets to save in history
	static const int32 MaxAssetsHistory;

	// Track if we have tried to cache the first asset we were playing
	bool bTriedToCacheOrginalAsset;
};