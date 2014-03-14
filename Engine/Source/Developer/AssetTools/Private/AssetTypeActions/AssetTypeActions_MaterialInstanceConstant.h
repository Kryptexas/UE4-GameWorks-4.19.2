// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_MaterialInstanceConstant : public FAssetTypeActions_MaterialInterface
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MaterialInstanceConstant", "Material Instance"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(0,128,0); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UMaterialInstanceConstant::StaticClass(); }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) OVERRIDE;
	virtual bool CanFilter() OVERRIDE { return true; }

private:
	/** Handler for when Edit is selected */
	void ExecuteEdit(TArray<TWeakObjectPtr<UMaterialInstanceConstant>> Objects);

	/** Handler for when FindParent is selected */
	void ExecuteFindParent(TArray<TWeakObjectPtr<UMaterialInstanceConstant>> Objects);
};