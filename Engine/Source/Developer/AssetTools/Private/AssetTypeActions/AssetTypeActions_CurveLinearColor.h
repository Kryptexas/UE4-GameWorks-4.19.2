// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_CurveLinearColor : public FAssetTypeActions_Curve
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_CurveLinearColor", "Color Curve"); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UCurveLinearColor::StaticClass(); }
	virtual bool CanFilter() OVERRIDE { return true; }
};