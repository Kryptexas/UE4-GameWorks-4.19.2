// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_AnimComposite : public FAssetTypeActions_AnimationAsset
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AnimComposite", "Animation Composite"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(0,128,192); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UAnimComposite::StaticClass(); }
	virtual bool CanFilter() OVERRIDE { return true; }
};