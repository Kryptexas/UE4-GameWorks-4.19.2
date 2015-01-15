// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ProceduralFoliageBroadphase.h"
#include "ProceduralFoliageInstance.h"
#include "ProceduralFoliageTile.generated.h"

struct FProceduralFoliageInstance;
struct FProceduralFoliageOverlap;
class UProceduralFoliageComponent;
class UProceduralFoliage;
class UFoliageType_InstancedStaticMesh;

UCLASS()
class FOLIAGE_API UProceduralFoliageTile : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	UProceduralFoliage* ProceduralFoliage;

	void Simulate(UProceduralFoliage* ProceduralFoliage, const int32 MaxNumSteps = -1);
	void RemoveInstances();
	void RemoveInstance(FProceduralFoliageInstance* Inst);

	/** Takes a local AABB and finds all the instances in it. The instances are returned in the space local to the AABB. That is, an instance in the bottom left AABB will have a coordinate of (0,0) */
	void GetInstancesInAABB(const FBox2D& LocalAABB, TArray<FProceduralFoliageInstance* >& OutInstances, bool bOnTheBorder) const;
	void CreateInstancesToSpawn(TArray<FProceduralFoliageInstance>& OutInstances, const FTransform& WorldTM, UWorld* World, const float HalfHeight) const;
	void AddInstances(const TArray<FProceduralFoliageInstance*>& NewInstances, const FTransform& LocalTM, const FBox2D& InnerLocalAABB);

	virtual void BeginDestroy() override;

	void InitSimulation(UProceduralFoliage* ProceduralFoliage);

	void InstancesToArray();
private:
	
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
	
private:

	TSet<FProceduralFoliageInstance*> PendingRemovals;
	TSet<FProceduralFoliageInstance*> Instances;

	UPROPERTY()
	TArray<FProceduralFoliageInstance> InstancesArray;

	int32 SimulationStep;
	FProceduralFoliageBroadphase Broadphase;	

	FRandomStream RandomStream;

private:
	float GetRandomGaussian();
	FVector GetSeedOffset(const UFoliageType_InstancedStaticMesh* Type, float MinDistance);
};