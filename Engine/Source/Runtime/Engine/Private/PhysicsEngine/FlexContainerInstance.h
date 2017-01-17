// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#if WITH_FLEX

#include "flex.h"
#include "flexExt.h"

#include "PhysXIncludes.h"

// UE types
class UFlexContainer;
class UFlexComponent;
class UFlexAsset;
struct FFlexPhase;
struct IFlexContainerClient;

#if STATS

DECLARE_STATS_GROUP(TEXT("Flex"), STATGROUP_Flex, STATCAT_Advanced);
DECLARE_STATS_GROUP_VERBOSE(TEXT("FlexGpu"), STATGROUP_FlexGpu, STATCAT_Advanced);

enum EFlexStats
{
	// UFlexComponentStats stats
	STAT_Flex_RenderMeshTime,
	STAT_Flex_UpdateBoundsCpu,
	STAT_Flex_ActiveParticleCount,
	STAT_Flex_ActiveMeshActorCount,
	
	// Container stats
	STAT_Flex_DeviceUpdateTime,
	STAT_Flex_SolverUpdateTime,
	STAT_Flex_WaitTime,
	STAT_Flex_GatherCollisionShapes,
	STAT_Flex_UpdateCollisionShapes,
	STAT_Flex_UpdateActors,
	STAT_Flex_ContainerCount,
	STAT_Flex_InstanceCount,
	STAT_Flex_ParticleCount,
	STAT_Flex_SpringCount,
	STAT_Flex_ShapeCount,
	STAT_Flex_StaticConvexCount,
	STAT_Flex_StaticTriangleCount,
	STAT_Flex_ForceFieldCount,
};

#endif


// one container per-phys scene
struct FFlexContainerInstance : public PxDeletionListener
{
	FFlexContainerInstance(UFlexContainer* Template, FPhysScene* Owner);
	virtual ~FFlexContainerInstance();

	int32 CreateParticle(const FVector4& Pos, const FVector& Vel, int32 Phase);
	void DestroyParticle(int32 Index);

	void CopyParticle(int32 Source, int32 Dest);

	// spawns a new instance of an asset into the container
	FlexExtInstance* CreateInstance(const FlexExtAsset* Asset, const FMatrix& Mat, const FVector& Velocity, int32 Phase);
	void DestroyInstance(FlexExtInstance* Asset);

	// convert a phase to the solver format, will allocate a new group if requested
	int32 GetPhase(const FFlexPhase& Phase);

	// returns a cached copy of the PxShape 
	const FlexTriangleMeshId GetTriangleMesh(const PxTriangleMesh* TriMesh);
	const FlexTriangleMeshId GetTriangleMesh(const PxHeightField* HeightField);
	const FlexConvexMeshId GetConvexMesh(const PxConvexMesh* ConvexMesh);

	// kicks off the simulation update and all compute kernels, unmaps particle data, calls unmap()
	void Simulate(float DeltaTime);	
	// starts synchronization phase, should be called after GPU work has finished, calls map()
	void Synchronize();

	// maps particle data, synchronizing with GPU, should only be called by Synchronize()
	void Map();
	// unmaps data, should only be called by Simulate()
	void Unmap();	
	// returns true if data is mapped, if so then reads/writes may occur, otherwise they are illegal
	bool IsMapped();

	// gather and send collision data to the solver
	void UpdateCollisionData();
	// send particle and parameter data to the solver
	void UpdateSimData();

	// register component to receive callbacks 
	void Register(IFlexContainerClient* Comp);
	void Unregister(IFlexContainerClient* Comp);

	// add a radial force for one frame 
	void AddRadialForce(FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff);
	// add a radial impulse for one frame 
	void AddRadialImpulse(FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff, bool bVelChange);

	// helper methods
	void ComputeSteppingParam(float& Dt, int32& NumSubsteps, float& NewLeftOverTime, float DeltaTime) const;
	void DebugDraw();

	int GetActiveParticleCount();
	int GetMaxParticleCount();

	// PxDeletionListener callback used for invalidating shape cache
	virtual void onRelease(const PxBase* observed, void* userData, PxDeletionEventFlag::Enum deletionEvent);

	FlexExtContainer* Container;
	FlexSolver* Solver;
	FlexExtForceFieldCallback* ForceFieldCallback;

	// mapped extensions data
	FlexExtParticleData MappedData;

	// cache the converted triangle meshes
	TMap<void*, FlexTriangleMeshId> TriangleMeshes;
	TMap<void*, FlexConvexMeshId> ConvexMeshes;

	// pointers into the container's mapped memory, only valid during Synchronize(), these are a typed alias for contents of MappedData
	FVector4* Particles;
	FVector4* ParticleRestPositions;
	FVector* Velocities;
	FVector4* Normals;
	int* Phases;

	// copy of particle data
	FlexVector<FVector4> Anisotropy1;
	FlexVector<FVector4> Anisotropy2;
	FlexVector<FVector4> Anisotropy3;
	FlexVector<FVector4> SmoothPositions;

	static const int MaxContactsPerParticle = 6;

	FlexVector<int32> ContactIndices;
	FlexVector<FVector4> ContactVelocities;
	FlexVector<uint32> ContactCounts;
	TArray<bool> ContactCounted;

	FPhysScene* Owner;
	FBoxSphereBounds Bounds;

	TArray<IFlexContainerClient*> Components;

	TWeakObjectPtr<UFlexContainer> TemplateRef;
	UFlexContainer* Template;

	// incrementing group counter used to auto-assign unique groups to rigids
	int GroupCounter;

	FlexVector<FlexCollisionGeometry> ShapeGeometry;
	FlexVector<int32> ShapeFlags;
	FlexVector<FVector4> ShapePositions;
	FlexVector<FQuat> ShapeRotations;
	FlexVector<FVector4> ShapePositionsPrev;
	FlexVector<FQuat> ShapeRotationsPrev;


	TArray<int32> ShapeReportIndices;
	TArray<TWeakObjectPtr<UPrimitiveComponent>> ShapeReportComponents;

	// temporary buffers used during collision shape building
	FlexVector<FVector> TriMeshVerts;
	FlexVector<int32> TriMeshIndices;	
	FlexVector<FVector4> ConvexMeshPlanes;

	TArray<FlexExtForceField> ForceFields;

	float LeftOverTime;
	float AverageDeltaTime;

	static bool sGlobalDebugDraw;

};


#endif //WITH_FLEX