// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_AnimBlueprint : public FAssetTypeActions_Blueprint
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AnimBlueprint", "Animation Blueprint"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(200,116,0); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UAnimBlueprint::StaticClass(); }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) OVERRIDE;
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Animation; }

protected:

	/** Whether or not we can create derived blueprints */
	virtual bool CanCreateNewDerivedBlueprint() const OVERRIDE;

private:

	/** Handler to fill the retarget submenu */
	void FillRetargetMenu( FMenuBuilder& MenuBuilder, const TArray<UObject*> InObjects );

	/** Handler for when FindSkeleton is selected */
	void ExecuteFindSkeleton(TArray<TWeakObjectPtr<UAnimBlueprint>> Objects);

	/** Context menu item handler for changing the supplied assets skeletons */ 
	void RetargetAssets(const TArray<UObject*> InAnimBlueprints, bool bDuplicateAssets);
};