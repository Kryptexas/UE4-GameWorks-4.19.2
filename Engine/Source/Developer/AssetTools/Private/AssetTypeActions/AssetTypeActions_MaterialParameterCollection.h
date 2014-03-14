// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_MaterialParameterCollection : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MaterialParameterCollection", "Material Parameter Collection"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(0, 192, 0); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UMaterialParameterCollection::StaticClass(); }
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::MaterialsAndTextures; }
};