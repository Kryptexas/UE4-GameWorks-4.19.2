// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_Struct : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_Struct", "Structure"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(0, 0, 255); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UUserDefinedStruct::StaticClass(); }
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Misc; }

	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) OVERRIDE;
};
