// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

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
	void OnAnimDoubleClicked(const FAssetData& AssetData);

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

protected:

	// Pointer back to persona tool that owns us
	TWeakPtr<class FPersona> PersonaPtr;

	// Set of tags to prevent creating details view columns for (infrequently used)
	TSet<FName> AssetRegistryTagsToIgnore;
};