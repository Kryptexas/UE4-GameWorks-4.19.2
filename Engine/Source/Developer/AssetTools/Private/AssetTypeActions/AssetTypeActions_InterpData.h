// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_InterpData : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_InterpData", "Matinee Data"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(255,128,0); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UInterpData::StaticClass(); }
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Misc; }
};