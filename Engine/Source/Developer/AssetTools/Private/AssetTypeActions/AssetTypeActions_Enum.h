// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_Enum : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_Enum", "Enumeration"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(255, 200, 200); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UUserDefinedEnum::StaticClass(); }
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Misc; }

	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) OVERRIDE;
};
