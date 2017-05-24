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


UENUM()
enum class EClothPaintTool
{
	/** Brush paint tool to directly paint vertices */
	Brush,
	/** Gradient paint tool to create a gradient between two sets of vertices */
	Gradient
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

	/** Type of paint tool to use*/
	UPROPERTY(EditAnywhere, Category = ClothPainting)
	EClothPaintTool PaintTool;

	/** Value to paint for the currently selected PaintingProperty */
	UPROPERTY(EditAnywhere, Category = ClothPainting)
	float PaintValue;

	/** Value of (green) points defined at the start of the gradient */
	UPROPERTY(EditAnywhere, Category = ClothPainting)
	float GradientStartValue;

	/** Value of (red) points defined at the end of the gradient */
	UPROPERTY(EditAnywhere, Category = ClothPainting)
	float GradientEndValue;

	/** Toggle for using the regular brush size for painting the Start and End points */
	UPROPERTY(EditAnywhere, Category = ClothPainting)
	bool bUseRegularBrushForGradient;

	/** Array of Clothing assets */
	UPROPERTY()
	TArray<UClothingAsset*> ClothingAssets;
};
