// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/IToolkitHost.h"
#include "AssetTypeActions_Base.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialFunctionMaterialLayer.h"
#include "Materials/MaterialFunctionMaterialLayerBlend.h"

class FMenuBuilder;

class FAssetTypeActions_MaterialFunction : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MaterialFunction", "Material Function"); }
	virtual FColor GetTypeColor() const override { return FColor(0,175,175); }
	virtual UClass* GetSupportedClass() const override { return UMaterialFunction::StaticClass(); }
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const override { return true; }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) override;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) override;
	virtual uint32 GetCategories() override { return EAssetTypeCategories::MaterialsAndTextures; }
	virtual class UThumbnailInfo* GetThumbnailInfo(UObject* Asset) const override;

protected:
	/** Handler for when NewMFI is selected */
	virtual void ExecuteNewMFI(TArray<TWeakObjectPtr<UMaterialFunctionInterface>> Objects);

	/** Handler for when FindMaterials is selected */
	void ExecuteFindMaterials(TArray<TWeakObjectPtr<UMaterialFunctionInterface>> Objects);
	virtual FText GetInstanceText() const { return NSLOCTEXT("AssetTypeActions", "Material_NewMFI", "Create Function Instance"); }

};

class FAssetTypeActions_MaterialFunctionLayer : public FAssetTypeActions_MaterialFunction
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MaterialFunctionMaterialLayer", "Material Layer"); }
	virtual FColor GetTypeColor() const override { return FColor(0,175,175); }
	virtual UClass* GetSupportedClass() const override;
	virtual bool CanFilter() override;
	virtual uint32 GetCategories() override { return EAssetTypeCategories::MaterialsAndTextures; }

private:
	virtual void ExecuteNewMFI(TArray<TWeakObjectPtr<UMaterialFunctionInterface>> Objects) override;
	virtual FText GetInstanceText() const override{ return NSLOCTEXT("AssetTypeActions", "Material_NewMLI", "Create Layer Instance"); }
};


class FAssetTypeActions_MaterialFunctionLayerBlend : public FAssetTypeActions_MaterialFunction
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MaterialFunctionMaterialLayerBlend", "Material Layer Blend"); }
	virtual FColor GetTypeColor() const override { return FColor(0,175,175); }
	virtual UClass* GetSupportedClass() const override;
	virtual bool CanFilter() override;
	virtual uint32 GetCategories() override { return EAssetTypeCategories::MaterialsAndTextures; }

private:
	virtual void ExecuteNewMFI(TArray<TWeakObjectPtr<UMaterialFunctionInterface>> Objects) override;
	virtual FText GetInstanceText() const override { return NSLOCTEXT("AssetTypeActions", "Material_NewMBI", "Create Blend Instance"); }
};
