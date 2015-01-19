// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ProceduralFoliageComponent.generated.h"

class UProceduralFoliage;

UCLASS(BlueprintType)
class FOLIAGE_API UProceduralFoliageComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

	/** The number of ProceduralFoliage tiles to place along the x-axis */
	UPROPERTY(Category = "ProceduralFoliage", BlueprintReadWrite, EditAnywhere)
	int32 TilesX;

	/** The number of ProceduralFoliage tiles to place along the y-axis */
	UPROPERTY(Category = "ProceduralFoliage", BlueprintReadWrite, EditAnywhere)
	int32 TilesY;

	/** The extent of the volume above and below. Instances will only be placed on geometry within this volume*/
	UPROPERTY(Category = "ProceduralFoliage", BlueprintReadWrite, EditAnywhere)
	float HalfHeight;

	/** The overlap in Cms between two tiles. */
	UPROPERTY(Category = "ProceduralFoliage", BlueprintReadWrite, EditAnywhere)
	float Overlap;

	UPROPERTY(Category = "ProceduralFoliage", BlueprintReadWrite, EditAnywhere, meta=(DisplayName="ProceduralFoliage Asset") )
	UProceduralFoliage* ProceduralFoliage;

	void SpawnProceduralContent();

private:
	void SpawnTiles();
};