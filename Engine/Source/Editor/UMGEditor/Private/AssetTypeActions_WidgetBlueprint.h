// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

//TODO Use base class FAssetTypeActions_Blueprint later
class FAssetTypeActions_WidgetBlueprint : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_WidgetBlueprint", "Widget Blueprint"); }
	virtual FColor GetTypeColor() const override { return FColor(255,255,255); }
	virtual UClass* GetSupportedClass() const override { return UWidgetBlueprint::StaticClass(); }
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual uint32 GetCategories() override { return EAssetTypeCategories::UI; }
	// End IAssetTypeActions Implementation

private:
	/** Returns true if the blueprint is data only */
	bool ShouldUseDataOnlyEditor(const UBlueprint* Blueprint) const;
};