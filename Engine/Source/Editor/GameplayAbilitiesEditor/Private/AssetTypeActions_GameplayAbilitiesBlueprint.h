// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetTypeActions_Base.h"

//TODO Use base class FAssetTypeActions_Blueprint later
class FAssetTypeActions_GameplayAbilitiesBlueprint : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override { return FColor(255,255,255); }
	virtual UClass* GetSupportedClass() const override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual uint32 GetCategories() override { return EAssetTypeCategories::Blueprint | EAssetTypeCategories::Gameplay; }
	// End IAssetTypeActions Implementation

private:
	/** Returns true if the blueprint is data only */
	bool ShouldUseDataOnlyEditor(const UBlueprint* Blueprint) const;
};