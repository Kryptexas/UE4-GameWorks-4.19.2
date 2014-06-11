// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_World : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_World", "Level"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(255, 156, 0); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UWorld::StaticClass(); }
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const OVERRIDE { return false; }
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) OVERRIDE;
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Basic; }
};