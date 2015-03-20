// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ProceduralFoliageBroadphase.h"
#include "ProceduralFoliageInstance.h"
#include "InstancedFoliage.h"
#include "ProceduralFoliageTile.generated.h"

struct FProceduralFoliageInstance;
struct FProceduralFoliageOverlap;
class UProceduralFoliageSpawner;
class UFoliageType_InstancedStaticMesh;

UCLASS()
class FOLIAGE_API UProceduralFoliageTile : public UObject
{
	GENERATED_UCLASS_BODY()

	void Simulate(const UProceduralFoliageSpawner* InFoliageSpawner, const int32 RandomSeed, const int32 MaxNumSteps, const int32 InLastCancel);
	void RemoveInstances();
	void RemoveInstance(FProceduralFoliageInstance* Inst);

	/** Takes a local AABB and finds all the instances in it. The instances are returned in the space local to the AABB. That is, an instance in the bottom left AABB will have a coordinate of (0,0) */
	void GetInstancesInAABB(const FBox2D& LocalAABB, TArray<FProceduralFoliageInstance* >& OutInstances, bool bOnTheBorder) const;
	void CreateInstancesToSpawn(TArray<FDesiredFoliageInstance>& OutInstances, const FTransform& WorldTM, const FGuid& ProceduralGuid, const float HalfHeight, const FBodyInstance* VolumeBodyInstance) const;
	void AddInstances(const TArray<FProceduralFoliageInstance*>& NewInstances, const FTransform& LocalTM, const FBox2D& InnerLocalAABB);

	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) override;

	virtual void BeginDestroy() override;

	void InitSimulation(const UProceduralFoliageSpawner* InFoliageSpawner, const int32 RandomSeed);

	void InstancesToArray();

	/** Empty arrays and sets */
	void Empty();
private:
	
	void StartSimulation(const int32 MaxNumSteps, bool bShadeGrowth);
	void StepSimulation();
	void AddRandomSeeds(TArray<FProceduralFoliageInstance*>& OutInstances);

	/** Determines whether the instance will survive all the overlaps, and then kills the appropriate instances. Returns true if the instance survives */
	bool HandleOverlaps(FProceduralFoliageInstance* Instance);

	/** Attempts to create a new instance and resolves any overlaps. Returns the new instance if successful for calling code to add to Instances */
	FProceduralFoliageInstance* NewSeed(const FVector& Location, float Scale, const UFoliageType_InstancedStaticMesh* Type, float InAge, bool bBlocker = false);

	void SpreadSeeds(TArray<FProceduralFoliageInstance*>& NewSeeds);
	void AgeSeeds();

	void MarkPendingRemoval(FProceduralFoliageInstance* ToRemove);
	void FlushPendingRemovals();

	bool UserCancelled() const;
	
private:

	UPROPERTY()
	const UProceduralFoliageSpawner* FoliageSpawner;

	TSet<FProceduralFoliageInstance*> PendingRemovals;
	TSet<FProceduralFoliageInstance*> Instances;

	UPROPERTY()
	TArray<FProceduralFoliageInstance> InstancesArray;

	int32 SimulationStep;
	FProceduralFoliageBroadphase Broadphase;	

	FRandomStream RandomStream;
	bool bSimulateShadeGrowth;

	int32 LastCancel;

private:
	float GetRandomGaussian();
	FVector GetSeedOffset(const UFoliageType_InstancedStaticMesh* Type, float MinDistance);
};