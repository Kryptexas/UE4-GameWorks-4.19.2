// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_BlendSpace : public FAssetTypeActions_AnimationAsset
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_BlendSpace", "Blend Space"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(255,168,111); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UBlendSpace::StaticClass(); }
	virtual bool CanFilter() OVERRIDE { return true; }
};