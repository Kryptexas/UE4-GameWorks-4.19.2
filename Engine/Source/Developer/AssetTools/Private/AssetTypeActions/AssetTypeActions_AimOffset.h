// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_AimOffset : public FAssetTypeActions_BlendSpace
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AimOffset", "Aim Offset"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(0,162,232); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UAimOffsetBlendSpace::StaticClass(); }
};