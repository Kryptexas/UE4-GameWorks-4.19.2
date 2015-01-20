// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Async/Async.h"
#include "ProceduralFoliageInstance.h"
#include "ProceduralFoliageComponent.generated.h"

class UProceduralFoliage;
class AProceduralFoliageLevelInfo;

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

	UPROPERTY(Category = "ProceduralFoliage", BlueprintReadWrite, EditAnywhere, meta=(DisplayName="ProceduralFoliage Asset") )
	UProceduralFoliage* ProceduralFoliage;

	void SpawnProceduralContent();
	void RemoveProceduralContent();
	const FGuid& GetProceduralGuid() const { return ProceduralGuid;  }

private:
	void SpawnTiles();
	void SpawnInstances(const TArray<FProceduralFoliageInstance>& ProceduralFoliageInstances);
	void MergeTiles();

private:
	UPROPERTY()
	FGuid ProceduralGuid;

	UPROPERTY(transient)
	TArray<UProceduralFoliageTile*> WorkTiles;

	TArray<TSharedFuture<const UProceduralFoliageTile*> > MergedTiles;
};