// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_TextureMovie : public FAssetTypeActions_Texture
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_TextureMovie", "Texture Movie"); }
	virtual FColor GetTypeColor() const override { return FColor(255,0,0); }
	virtual UClass* GetSupportedClass() const override { return UTextureMovie::StaticClass(); }
	virtual bool CanFilter() override { return true; }
};