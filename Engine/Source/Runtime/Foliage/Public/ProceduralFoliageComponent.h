// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ProceduralFoliageInstance.h"
#include "ProceduralFoliageComponent.generated.h"

class UProceduralFoliageSpawner;
class AProceduralFoliageLevelInfo;
struct FDesiredFoliageInstance;

UCLASS(BlueprintType)
class FOLIAGE_API UProceduralFoliageComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	/** The procedural foliage spawner used to generate foliage instances within this volume. */
	UPROPERTY(Category = "ProceduralFoliage", BlueprintReadWrite, EditAnywhere)
	UProceduralFoliageSpawner* FoliageSpawner;

	/** The amount of overlap between simulation tiles (in cm). */
	UPROPERTY(Category = "ProceduralFoliage", BlueprintReadWrite, EditAnywhere)
	float TileOverlap;

#if WITH_EDITORONLY_DATA
	/** Whether to visualize the tiles used for the foliage spawner simulation */
	UPROPERTY(Category = "ProceduralFoliage", BlueprintReadWrite, EditAnywhere)
	bool bShowDebugTiles;
#endif

	bool SpawnProceduralContent(TArray<FDesiredFoliageInstance>& OutFoliageInstances);
	void RemoveProceduralContent();
	bool HasSpawnedAnyInstances();
	const FGuid& GetProceduralGuid() const { return ProceduralGuid;  }

	UPROPERTY()
	AVolume* SpawningVolume;

	FVector GetWorldPosition() const;
	void GetTilesLayout(int32& MinXIdx, int32& MinYIdx, int32& NumXTiles, int32& NumYTiles, float& HalfHeight) const;

private:
	bool SpawnTiles(TArray<FDesiredFoliageInstance>& OutFoliageInstances);

private:
	UPROPERTY()
	FGuid ProceduralGuid;
};