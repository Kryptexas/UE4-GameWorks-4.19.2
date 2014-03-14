// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_World : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_World", "World"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(255, 156, 0); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UWorld::StaticClass(); }
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const OVERRIDE { return false; }
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Basic; }
};