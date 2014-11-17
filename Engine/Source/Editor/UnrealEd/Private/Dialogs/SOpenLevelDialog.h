// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"
#include "Developer/AssetTools/Public/IAssetTypeActions.h"
#include "Editor/ContentBrowser/Public/ContentBrowserDelegates.h"

class FAssetData;

class SOpenLevelDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SOpenLevelDialog ){}
		
	SLATE_END_ARGS()

	/** Opens this dialog in a default window */
	static void CreateAndShowOpenLevelDialog(const FEditorFileUtils::FOnLevelsChosen& InOnLevelsChosen, bool bAllowMultipleSelection);

	SOpenLevelDialog();

	virtual void Construct(const FArguments& InArgs, const FEditorFileUtils::FOnLevelsChosen& InOnLevelsChosen, bool bAllowMultipleSelection);

private:

	/** Handler for when a path is selected in the path view */
	void HandlePathSelected(const FString& NewPath);

	/** Returns true if the open button can be clicked */
	bool IsOpenButtonEnabled() const;

	/** Handler for when the open button is clicked */
	FReply OnOpenClicked();

	/** Handler for when the cancel button is clicked */
	FReply OnCancelClicked();

	/* Handler for when the asset selection changes */
	void OnAssetSelected(const FAssetData& AssetData);

	/* Handler for when a level was selected from the asset picker */
	void OnAssetsActivated(const TArray<FAssetData>& SelectedAssets, EAssetTypeActivationMethod::Type ActivationType);

	/** Fires the levels chosen delegate and closes the window */
	void ChooseLevels(const TArray<FAssetData>& SelectedLevels);

	/** Closes this dialog */
	void CloseDialog();

private:

	/** Used to update the asset view after it has been created */
	FSetARFilterDelegate SetFilterDelegate;

	/** Used to get the currently selected assets */
	FGetCurrentSelectionDelegate GetCurrentSelectionDelegate;

	/** Executed when levels have been selected */
	FEditorFileUtils::FOnLevelsChosen OnLevelsChosen;

	/** The most recently selected assets */
	TArray<FAssetData> LastSelectedAssets;

	/** If true, more than one level may be selected */
	bool bConfiguredToAllowMultipleSelection;
};
