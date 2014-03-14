// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Developer/AssetTools/Public/IAssetTypeActions.h"

/** Called when a collection is selected in the collections view */
DECLARE_DELEGATE_OneParam( FOnCollectionSelected, const struct FCollectionNameType& /*SelectedCollection*/);

/** Called to retrieve the tooltip for the specified asset */
DECLARE_DELEGATE_RetVal_OneParam( TSharedRef< SToolTip >, FConstructToolTipForAsset, const FAssetData& /*Asset*/);

/** Called to check if an asset should be filtered out by external code. Return true to exclude the asset from the view. */
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnShouldFilterAsset, const class FAssetData& /*AssetData*/);

/** Called to check if an asset tag should be display in details view. Return false to exclude the asset from the view. */
DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnShouldDisplayAssetTag, FName /*AssetType*/, FName /*TagName*/);

/** Called to clear the selection of the specified assetdata or all selection if an invalid assetdata is passed */
DECLARE_DELEGATE( FClearSelectionDelegate );

/** Called when thumbnail scale changes and the thumbnail scale is bound to a delegate */
DECLARE_DELEGATE_OneParam( FOnThumbnailScaleChanged, const float /*NewScale*/);

/** Called to retrieve an array of the currently selected asset data */
DECLARE_DELEGATE_RetVal( TArray< FAssetData >, FGetCurrentSelectionDelegate );

/** Called to adjust the selection from the current assetdata, should be +1 to increment or -1 to derement */
DECLARE_DELEGATE_OneParam( FAdjustSelectionDelegate, const int32 /*direction*/ );

/** Called when an asset is selected in the asset view */
DECLARE_DELEGATE_OneParam(FOnAssetSelected, const class FAssetData& /*AssetData*/);

/** Called when the user double clicks, presses enter, or presses space on an asset */
DECLARE_DELEGATE_TwoParams(FOnAssetsActivated, const TArray<FAssetData>& /*ActivatedAssets*/, EAssetTypeActivationMethod::Type /*ActivationMethod*/);

/** Called when an asset has begun being dragged by the user */
DECLARE_DELEGATE_RetVal_OneParam(FReply, FOnAssetDragged, const TArray<class FAssetData>& /*AssetData*/);

/** Called when an asset is clicked on in the asset view */
DECLARE_DELEGATE_OneParam( FOnAssetClicked, const class FAssetData& /*AssetData*/ );

/** Called when an asset is double clicked in the asset view */
DECLARE_DELEGATE_OneParam(FOnAssetDoubleClicked, const class FAssetData& /*AssetData*/);

/** Called when enter is pressed on an asset in the asset view */
DECLARE_DELEGATE_OneParam(FOnAssetEnterPressed, const TArray<FAssetData>& /*SelectedAssets*/);

/** Called when a new folder is starting to be created */
DECLARE_DELEGATE_TwoParams(FOnCreateNewFolder, const FString& /*FolderName*/, const FString& /*FolderPath*/);

/** Called to request the menu when right clicking on an asset */
DECLARE_DELEGATE_RetVal_OneParam(TSharedPtr<SWidget>, FOnGetAssetContextMenu, const TArray<FAssetData>& /*SelectedAssets*/);

/** Called when a path is selected in the path picker */
DECLARE_DELEGATE_OneParam(FOnPathSelected, const FString& /*Path*/);

/** Called when a path is double clicked in the asset view */
DECLARE_DELEGATE_OneParam(FOnPathDoubleClicked, const FString& /*Path*/);

/** Called to request the menu when right clicking on a path */
DECLARE_DELEGATE_RetVal(TSharedRef<FExtender>, FContentBrowserMenuExtender);
DECLARE_DELEGATE_RetVal_OneParam(TSharedRef<FExtender>, FContentBrowserMenuExtender_SelectedAssets, const TArray<FAssetData>& /*SelectedAssets*/);
DECLARE_DELEGATE_RetVal_OneParam(TSharedRef<FExtender>, FContentBrowserMenuExtender_SelectedPaths, const TArray<FString>& /*SelectedPaths*/);

/** Called to request the menu when right clicking on an asset */
DECLARE_DELEGATE_RetVal_ThreeParams(TSharedPtr<SWidget>, FOnGetFolderContextMenu, const TArray<FString>& /*SelectedPaths*/, FContentBrowserMenuExtender_SelectedPaths /*MenuExtender*/, FOnCreateNewFolder /*CreationDelegate*/);