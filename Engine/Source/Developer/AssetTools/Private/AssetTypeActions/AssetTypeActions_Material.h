// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_Material : public FAssetTypeActions_MaterialInterface
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_Material", "Material"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(0,255,0); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UMaterial::StaticClass(); }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) OVERRIDE;
	virtual bool CanFilter() OVERRIDE { return true; }
	virtual uint32 GetCategories() OVERRIDE { return FAssetTypeActions_MaterialInterface::GetCategories() | EAssetTypeCategories::Basic; }

private:
	/** Handler for when Edit is selected */
	void ExecuteEdit(TArray<TWeakObjectPtr<UMaterial>> Objects);
};