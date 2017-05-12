// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MeshPaintSettings.h"
#include "Delegates/Delegate.h"

#include "ClothPaintSettings.generated.h"

class UClothingAsset;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnClothingAssetSelectionChangedMulticaster, UClothingAsset*, int32);
typedef FOnClothingAssetSelectionChangedMulticaster::FDelegate FOnClothingAssetSelectionChanged;

UENUM()
enum class EPaintableClothProperty
{
	/** Max distances cloth property */
	MaxDistances,
	/** Backstop cloth property */
	BackstopDistances,
	/** Backstop radius property */
	BackstopRadius
};

UCLASS()
class UClothPainterSettings : public UMeshPaintSettings
{
	GENERATED_BODY()

public:
	// Delegates to communicate with objects concerned with the settings changing
	FOnClothingAssetSelectionChangedMulticaster OnAssetSelectionChanged;

	/** Current Clothing Property which should be visualized and painted */
	UPROPERTY(EditAnywhere, Category = ClothPainting)
	EPaintableClothProperty PaintingProperty;

	/** Array of Clothing assets */
	UPROPERTY()
	TArray<UClothingAsset*> ClothingAssets;
};
