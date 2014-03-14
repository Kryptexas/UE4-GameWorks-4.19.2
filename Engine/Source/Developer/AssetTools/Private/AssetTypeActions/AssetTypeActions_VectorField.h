// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_VectorField : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_VectorField", "Vector Field"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(200,128,128); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UVectorField::StaticClass(); }
	virtual bool CanFilter() OVERRIDE { return false; }
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Misc; }
};