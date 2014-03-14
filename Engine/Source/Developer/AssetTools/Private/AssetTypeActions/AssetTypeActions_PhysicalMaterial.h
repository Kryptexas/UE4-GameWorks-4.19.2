// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_PhysicalMaterial : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_PhysicalMaterial", "Physical Material"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(200,192,128); }
	virtual UClass* GetSupportedClass() const OVERRIDE;
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Physics; }
};