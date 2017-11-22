#pragma once

#include "FlexParticleEmitterInstance.h"

/*-----------------------------------------------------------------------------
FFlexGPUParticleEmitterInstance
-----------------------------------------------------------------------------*/

struct FFlexGPUParticleEmitterInstance
{
	FFlexGPUParticleEmitterInstance(struct FFlexParticleEmitterInstance* InOwner);
	virtual ~FFlexGPUParticleEmitterInstance();

	void Tick(float DeltaSeconds, bool bSuppressSpawning, FRenderResource* FlexSimulationResource);
	void DestroyParticles(int32 Start, int32 Count, bool bShrink = true);
	void DestroyAllParticles(int32 ParticlesPerTile, bool bFreeParticleIndices);

	void AllocParticleIndices(int32 Count);
	void FreeParticleIndices(int32 Start, int32 Count);

	int32 CreateNewParticles(int32 NewStart, int32 NewCount);
	void DestroyNewParticles(int32 NewStart, int32 NewCount);
	void InitNewParticle(int32 NewIndex, int32 RegularIndex);
	void SetNewParticle(int32 NewIndex, const FVector& Position, const FVector& Velocity, float RelativeTime, float TimeScale, float InitialSize);

	FRenderResource* CreateSimulationResource();

	static void FillSimulationParams(FRenderResource* FlexSimulationResource, FFlexGPUParticleSimulationParameters& SimulationParams);


	FFlexParticleEmitterInstance* Owner;

	/* For each allocated tile there are TILE_SIZE * TILE_SIZE indices. If -1 it means index is currently unused */
	TArray<int32> FlexParticleIndices;

	TArray<int32> NewFlexParticleIndices;

	struct FParticleData
	{
		float RelativeTime;
		float TimeScale;
		float InitialSize;
	};
	TArray<FParticleData> ParticleDataArray;
};
