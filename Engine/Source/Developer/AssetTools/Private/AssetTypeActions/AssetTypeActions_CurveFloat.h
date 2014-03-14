// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_CurveFloat : public FAssetTypeActions_Curve
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_CurveFloat", "Float Curve"); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UCurveFloat::StaticClass(); }
	virtual bool CanFilter() OVERRIDE { return true; }
};