// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FoliageType_InstancedStaticMesh.h"
#include "ProceduralFoliageInstance.h"
#include "ProceduralFoliageSpawner.generated.h"

class UProceduralFoliageTile;


USTRUCT()
struct FProceduralFoliageTypeData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(Category = ProceduralFoliageSimulation, EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UFoliageType_InstancedStaticMesh> Type;

	UPROPERTY(transient)
	UFoliageType_InstancedStaticMesh* TypeInstance;

	UPROPERTY()
	int32 ChangeCount;
};

UCLASS(BlueprintType, Blueprintable)
class FOLIAGE_API UProceduralFoliageSpawner : public UObject
{
	GENERATED_UCLASS_BODY()

	/**The seed used for generating the randomness of the simulation. */
	UPROPERTY(Category = ProceduralFoliageSimulation, EditAnywhere, BlueprintReadOnly)
	int32 RandomSeed;

	/**Cm size of the tile along one axis. The tile is square so the total area is TileSize*TileSize */
	UPROPERTY(Category = ProceduralFoliageSimulation, EditAnywhere, BlueprintReadOnly)
	float TileSize;

	UPROPERTY(Category = ProceduralFoliageSimulation, EditAnywhere, BlueprintReadOnly)
	int32 NumUniqueTiles;

	FThreadSafeCounter LastCancel;

private:
	UPROPERTY(Category = ProceduralFoliageSimulation, EditAnywhere)
	TArray<FProceduralFoliageTypeData> Types;

	UPROPERTY()
	bool bNeedsSimulation;

public:
	UFUNCTION(BlueprintCallable, Category = ProceduralFoliageSimulation)
	void Simulate(int32 NumSteps = -1);

	int32 GetRandomNumber();

	const TArray<FProceduralFoliageTypeData>& GetTypes() const { return Types; }

	/** Returns the instances that need to spawn for a given min,max region */
	void GetInstancesToSpawn(TArray<FProceduralFoliageInstance>& OutInstances, const FVector& Min = FVector(-FLT_MAX, -FLT_MAX, -FLT_MAX), const FVector& Max = FVector(FLT_MAX, FLT_MAX, FLT_MAX) ) const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);
#endif

	virtual void Serialize(FArchive& Ar);

	/** Simulates tiles if needed */
	void SimulateIfNeeded();

	/** Takes a tile index and returns a random tile associated with that index. */
	const UProceduralFoliageTile* GetRandomTile(int32 X, int32 Y);

	/** Creates a temporary empty tile with the appropriate settings created for it. */
	UProceduralFoliageTile* CreateTempTile();

private:
	void CreateProceduralFoliageInstances();

	bool AnyDirty() const;

	void SetClean();
private:
	TArray<TWeakObjectPtr<UProceduralFoliageTile>> PrecomputedTiles;

	FRandomStream RandomStream;
};
