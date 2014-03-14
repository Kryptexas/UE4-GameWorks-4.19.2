// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_AnimMontage : public FAssetTypeActions_AnimationAsset
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AnimMontage", "Animation Montage"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(100,100,255); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UAnimMontage::StaticClass(); }
	virtual bool CanFilter() OVERRIDE { return true; }
};