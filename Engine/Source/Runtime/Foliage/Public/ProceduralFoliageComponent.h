// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ProceduralFoliageInstance.h"
#include "ProceduralFoliageComponent.generated.h"

class UProceduralFoliage;
class AProceduralFoliageLevelInfo;
struct FDesiredFoliageInstance;

UCLASS(BlueprintType)
class FOLIAGE_API UProceduralFoliageComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	/** The overlap in Cms between two tiles. */
	UPROPERTY(Category = "ProceduralFoliage", BlueprintReadWrite, EditAnywhere)
	float Overlap;

	UPROPERTY(Category = "ProceduralFoliage", BlueprintReadWrite, EditAnywhere, meta=(DisplayName="ProceduralFoliage Asset") )
	UProceduralFoliage* ProceduralFoliage;

#if WITH_EDITORONLY_DATA
	/** The overlap in Cms between two tiles. */
	UPROPERTY(Category = "ProceduralFoliage", BlueprintReadWrite, EditAnywhere)
	bool bHideDebugTiles;
#endif

	bool SpawnProceduralContent(TArray<FDesiredFoliageInstance>& OutFoliageInstances);
	void RemoveProceduralContent();
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