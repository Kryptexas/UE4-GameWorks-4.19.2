// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_AimOffset1D : public FAssetTypeActions_BlendSpace1D
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AimOffset1D", "Aim Offset 1D"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(0,162,232); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UAimOffsetBlendSpace1D::StaticClass(); }
};