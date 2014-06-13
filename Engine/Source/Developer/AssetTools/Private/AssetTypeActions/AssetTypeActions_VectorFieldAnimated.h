// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_VectorFieldAnimated : public FAssetTypeActions_VectorField
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_VectorFieldAnimated", "Animated Vector Field"); }
	virtual UClass* GetSupportedClass() const override { return UVectorFieldAnimated::StaticClass(); }
	virtual bool CanFilter() override { return true; }
};