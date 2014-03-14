// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_SlateBrush : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_SlateBrush", "Slate Brush"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(105, 165, 60); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return USlateBrushAsset::StaticClass(); }
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Misc; }
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const OVERRIDE { return false; }
};