#pragma once

#include "FlexParticleEmitterInstance.h"

/*-----------------------------------------------------------------------------
FFlexGPUParticleEmitterInstance
-----------------------------------------------------------------------------*/

struct FFlexGPUParticleEmitterInstance
{
	FFlexGPUParticleEmitterInstance(struct FFlexParticleEmitterInstance* InOwner, int32 InParticlesPerTile);
	virtual ~FFlexGPUParticleEmitterInstance();

	void Tick(float DeltaSeconds, bool bSuppressSpawning, FRenderResource* FlexSimulationResource);
	bool ShouldRenderParticles();
	void DestroyTileParticles(int32 TileIndex);
	void DestroyAllParticles(bool bFreeParticleIndices);

	void AllocParticleIndices(int32 TileCount);
	void FreeParticleIndices(int32 TileStart, int32 TileCount);

	int32 CreateNewParticles(int32 NewStart, int32 NewCount);
	void DestroyNewParticles(int32 NewStart, int32 NewCount);
	void AddNewParticle(int32 NewIndex, int32 TileIndex, int32 SubTileIndex);
	void SetNewParticle(int32 NewIndex, const FVector& Position, const FVector& Velocity, float RelativeTime, float TimeScale, float InitialSize);

	FRenderResource* CreateSimulationResource();

	static void FillSimulationParams(FRenderResource* FlexSimulationResource, FFlexGPUParticleSimulationParameters& SimulationParams);


	FFlexParticleEmitterInstance* Owner;
	const int32 ParticlesPerTile;

	int32 NumActiveParticles;

	/* For each allocated tile there are TILE_SIZE * TILE_SIZE indices. If -1 it means index is currently unused */
	TArray<int32> FlexParticleIndices;

	TArray<int32> NewFlexParticleIndices;

	struct FParticleTileData
	{
		int32 NumAddedParticles = 0;
		int32 NumActiveParticles = 0;
	};
	TArray<FParticleTileData> ParticleTiles;

	struct FParticleData
	{
		float RelativeTime;
		float TimeScale;
		float InitialSize;
	};
	TArray<FParticleData> ParticleDataArray;
};
