// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_Texture2D : public FAssetTypeActions_Texture
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_Texture2D", "Texture"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(255,0,0); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UTexture2D::StaticClass(); }
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const OVERRIDE { return true; }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE;
	virtual bool CanFilter() OVERRIDE { return true; }
	virtual uint32 GetCategories() OVERRIDE { return FAssetTypeActions_Texture::GetCategories() | EAssetTypeCategories::Basic; }

private:
	/** Handler for when Create Slate Brush is selected */
	void ExecuteCreateSlateBrush(TArray<TWeakObjectPtr<UTexture>> Objects);
};