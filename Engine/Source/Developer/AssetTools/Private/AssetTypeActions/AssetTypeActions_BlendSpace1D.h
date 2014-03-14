// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_BlendSpace1D : public FAssetTypeActions_AnimationAsset
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_BlendSpace1D", "Blend Space 1D"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(255,180,130); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UBlendSpace1D::StaticClass(); }
	virtual bool CanFilter() OVERRIDE { return true; }
};