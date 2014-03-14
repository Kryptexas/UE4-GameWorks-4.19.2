// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_MaterialInterface : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MaterialInterface", "Material Interface"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(0,255,0); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UMaterialInterface::StaticClass(); }
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const OVERRIDE { return true; }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE;
	virtual bool CanFilter() OVERRIDE { return false; }
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::MaterialsAndTextures; }
	virtual class UThumbnailInfo* GetThumbnailInfo(UObject* Asset) const OVERRIDE;

private:
	/** Handler for when NewMIC is selected */
	void ExecuteNewMIC(TArray<TWeakObjectPtr<UMaterialInterface>> Objects);
};