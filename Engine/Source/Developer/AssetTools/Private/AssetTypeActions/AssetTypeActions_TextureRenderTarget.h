// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_TextureRenderTarget : public FAssetTypeActions_Texture
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_TextureRenderTarget", "Texture Render Target"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(255,0,0); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UTextureRenderTarget::StaticClass(); }
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const OVERRIDE { return true; }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE;

private:
	/** Handler for when CreateStatic is selected */
	void ExecuteCreateStatic(TArray<TWeakObjectPtr<UTextureRenderTarget>> Objects);
};