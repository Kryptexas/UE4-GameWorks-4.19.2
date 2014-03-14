// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_TextureLightProfile : public FAssetTypeActions_Texture
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_TextureLightProfile", "Texture Light Profile"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(255,0,0); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UTextureLightProfile::StaticClass(); }
	virtual bool CanFilter() OVERRIDE { return true; }
};