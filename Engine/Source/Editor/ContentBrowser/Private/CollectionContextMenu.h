// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FCollectionContextMenu : public TSharedFromThis<FCollectionContextMenu>
{
public:
	/** Constructor */
	FCollectionContextMenu(const TWeakPtr<SCollectionView>& InCollectionView);

	/** Bind menu selection commands to the command list */
	void BindCommands(TSharedPtr< FUICommandList > InCommandList);

	/** Makes the collection list context menu widget */
	TSharedPtr<SWidget> MakeCollectionListContextMenu(TSharedPtr< FUICommandList > InCommandList);

	/** Makes the new collection submenu */
	void MakeNewCollectionSubMenu(FMenuBuilder& MenuBuilder);

	/** Update the source control flag the 'CanExecute' functions rely on */
	void UpdateProjectSourceControl();

	/** Handler for when a collection is selected in the "New" menu */
	void ExecuteNewCollection(ECollectionShareType::Type CollectionType);

	/** Handler for when "Rename Collection" is selected */
	void ExecuteRenameCollection();

	/** Handler for when "Destroy Collection" is selected */
	void ExecuteDestroyCollection();

	/** Handler for when "Destroy Collection" is confirmed */
	FReply ExecuteDestroyCollectionConfirmed(TArray<TSharedPtr<FCollectionItem>> CollectionList);

	/** Handler to check to see if "New Collection" can be executed */
	bool CanExecuteNewCollection(ECollectionShareType::Type CollectionType) const;

	/** Handler to check to see if "Rename Collection" can be executed */
	bool CanExecuteRenameCollection() const;

	/** Handler to check to see if "Destroy Collection" can be executed */
	bool CanExecuteDestroyCollection(bool bAnyManagedBySCC) const;

private:
	TWeakPtr<SCollectionView> CollectionView;

	/** Flag caching whether the project is under source control */
	bool bProjectUnderSourceControl;
};
