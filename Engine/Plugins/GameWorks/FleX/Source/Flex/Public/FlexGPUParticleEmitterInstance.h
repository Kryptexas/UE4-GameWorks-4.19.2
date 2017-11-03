#pragma once

#include "FlexParticleEmitterInstance.h"

/*-----------------------------------------------------------------------------
FFlexGPUParticleEmitterInstance
-----------------------------------------------------------------------------*/

struct FFlexGPUParticleEmitterInstance
{
	FFlexGPUParticleEmitterInstance(struct FFlexParticleEmitterInstance* InOwner, int32 NumParticles);
	virtual ~FFlexGPUParticleEmitterInstance();

	void Tick(float DeltaSeconds, bool bSuppressSpawning);
	void DestroyParticles(int32 Start, int32 Count);
	void DestroyAllParticles(int32 ParticlesPerTile);

	void AllocParticleIndices(int32 Count);
	void FreeParticleIndices(int32 Start, int32 Count);

	int32 CreateNewParticles(int32 NewStart, int32 NewCount);
	void DestroyNewParticles(int32 NewStart, int32 NewCount);
	void InitNewParticle(int32 NewIndex, int32 RegularIndex);
	void SetNewParticle(int32 NewIndex, const FVector& Position, const FVector& Velocity);

	FFlexParticleEmitterInstance* Owner;

	/* For each allocated tile there are TILE_SIZE * TILE_SIZE indices. If -1 it means index is currently unused */
	TArray<int32> FlexParticleIndices;

	TArray<int32> NewFlexParticleIndices;
};
