// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NewAssetContextMenu.h"

class FPathContextMenu : public TSharedFromThis<FPathContextMenu>
{
public:
	/** Constructor */
	FPathContextMenu(const TWeakPtr<SWidget>& InParentContent);

	/** Sets the handler for when new assets are requested */
	void SetOnNewAssetRequested(const FNewAssetContextMenu::FOnNewAssetRequested& InOnNewAssetRequested);

	/** Makes the asset tree context menu extender */
	TSharedRef<FExtender> MakePathViewContextMenuExtender(const TArray<FString>& InSelectedPaths);

	/** Makes the asset tree context menu widget */
	void MakePathViewContextMenu(FMenuBuilder& MenuBuilder);

	/** Makes the new asset submenu */
	void MakeNewAssetSubMenu(FMenuBuilder& MenuBuilder);

	/** Makes the set color submenu */
	void MakeSetColorSubMenu(FMenuBuilder& MenuBuilder);

	/** Handler for when "Migrate Folder" is selected */
	void ExecuteMigrateFolder();

	/** Handler for when "Explore" is selected */
	void ExecuteExplore();

	/** Handler for when reset color is selected */
	void ExecuteResetColor();

	/** Handler for when new or set color is selected */
	void ExecutePickColor();

	/** Handler for when "Save" is selected */
	void ExecuteSaveFolder();

	/** Handler for when "Delete" is selected */
	void ExecuteDeleteFolder();

	/** Handler for when "ReferenceViewer" is selected */
	void ExecuteReferenceViewer();

	/** Handler for when "Delete" is selected and the delete was confirmed */
	FReply ExecuteDeleteFolderConfirmed();

	/** Handler for when "Checkout from source control" is selected */
	void ExecuteSCCCheckOut();

	/** Handler for when "Open for Add to source control" is selected */
	void ExecuteSCCOpenForAdd();

	/** Handler for when "Checkin to source control" is selected */
	void ExecuteSCCCheckIn();

	/** Handler for when "Connect to source control" is selected */
	void ExecuteSCCConnect() const;

	/** Handler to check to see if "Checkout from source control" can be executed */
	bool CanExecuteSCCCheckOut() const;

	/** Handler to check to see if "Open for Add to source control" can be executed */
	bool CanExecuteSCCOpenForAdd() const;

	/** Handler to check to see if "Checkin to source control" can be executed */
	bool CanExecuteSCCCheckIn() const;

	/** Handler to check to see if "Connect to source control" can be executed */
	bool CanExecuteSCCConnect() const;	

private:
	/** Initializes some variable used to in "CanExecute" checks that won't change at runtime or are too expensive to check every frame. */
	void CacheCanExecuteVars();

	/** Returns a list of names of packages in all selected paths in the sources view */
	void GetPackageNamesInSelectedPaths(TArray<FString>& OutPackageNames) const;

	/** Gets the first selected path, if it exists */
	FString GetFirstSelectedPath() const;

	/** Checks to see if any of the selected paths use custom colors */
	bool SelectedHasCustomColors() const;

	/** Callback when the color picker dialog has been closed */
	void NewColorComplete(const TSharedRef<SWindow>& Window);

	/** Callback when the color is picked from the set color submenu */
	FReply OnColorClicked( const FLinearColor InColor );

	/** Resets the colors of the selected paths */
	void ResetColors();

private:
	TArray<FString> SelectedPaths;
	TWeakPtr<SWidget> ParentContent;
	FNewAssetContextMenu::FOnNewAssetRequested OnNewAssetRequested;

	/** Cached SCC CanExecute vars */
	bool bCanExecuteSCCCheckOut;
	bool bCanExecuteSCCOpenForAdd;
	bool bCanExecuteSCCCheckIn;
};