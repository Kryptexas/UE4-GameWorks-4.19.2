// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_AnimationAsset : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AnimationAsset", "AnimationAsset"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(80,123,72); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UAnimationAsset::StaticClass(); }
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const OVERRIDE { return true; }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) OVERRIDE;
	virtual bool CanFilter() OVERRIDE { return false; }
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Animation; }

private:
	/** Handler to fill the retarget submenu */
	void FillRetargetMenu( FMenuBuilder& MenuBuilder, const TArray<UObject*> InObjects );

	/** Handler for when FindSkeleton is selected */
	void ExecuteFindSkeleton(TArray<TWeakObjectPtr<UAnimationAsset>> Objects);

	/** Context menu item handler for changing the supplied assets skeletons */ 
	void RetargetAssets(const TArray<UObject*> InAnimBlueprints, bool bDuplicateAssets);
};