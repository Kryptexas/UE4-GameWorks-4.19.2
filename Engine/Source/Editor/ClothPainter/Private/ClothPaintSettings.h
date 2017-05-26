// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MeshPaintSettings.h"
#include "Delegates/Delegate.h"

#include "ClothPaintSettings.generated.h"

class UClothingAsset;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnClothingAssetSelectionChangedMulticaster, UClothingAsset*, int32, int32);
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

	UClothPainterSettings()
		: UMeshPaintSettings()
		, ViewMin(0.0f)
		, ViewMax(100.0f)
	{}

	float GetViewMin()
	{
		// Zero is reserved, but conceptually we should allow it as that's an
		// implementation detail the user is unlikely to care about
		return FMath::Clamp(ViewMin, SMALL_NUMBER, MAX_flt);
	}

	float GetViewMax()
	{
		return ViewMax;
	}

	// Delegates to communicate with objects concerned with the settings changing
	FOnClothingAssetSelectionChangedMulticaster OnAssetSelectionChanged;

	/** Array of Clothing assets */
	UPROPERTY()
	TArray<UClothingAsset*> ClothingAssets;

protected:
	/** When painting float/1D values, this is considered the zero or black point */
	UPROPERTY(EditAnywhere, Category = View)
	float ViewMin;

	/** When painting float/1D values, this is considered the one or white point */
	UPROPERTY(EditAnywhere, Category = View)
	float ViewMax;
};
