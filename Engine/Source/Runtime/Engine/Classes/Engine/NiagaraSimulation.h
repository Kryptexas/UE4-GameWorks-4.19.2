// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
NiagaraSimulation.h: Niagara emitter simulation class
==============================================================================*/
#pragma once

#include "NiagaraScript.h"
#include "Components/NiagaraComponent.h"
#include "Runtime/VectorVM/Public/VectorVM.h"
#include "NiagaraSimulation.generated.h"

DECLARE_CYCLE_STAT(TEXT("Tick"), STAT_NiagaraTick, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Simulate"), STAT_NiagaraSimulate, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Spawn + Kill"), STAT_NiagaraSpawnAndKill, STATGROUP_Niagara);


class NiagaraEffectRenderer;

UENUM()
enum EEmitterRenderModuleType
{
	RMT_None = 0,
	RMT_Sprites,
	RMT_Ribbon,
	RMT_Trails,
	RMT_Meshes
};


/** 
 *	FniagaraEmitterProperties stores the attributes of an FNiagaraSimulation
 *	that need to be serialized and are used for its initialization 
 */
USTRUCT()
struct FNiagaraEmitterProperties
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Emitter data")
	float SpawnRate;

	UPROPERTY(EditAnywhere, Category = "Emitter data")
	UNiagaraScript *UpdateScript;
	UPROPERTY(EditAnywhere, Category = "Emitter data")
	UNiagaraScript *SpawnScript;

	UPROPERTY(EditAnywhere, Category = "Emitter data")
	UMaterial *Material;
};



/**
* A niagara particle simulation.
*/
class FNiagaraSimulation
{
public:
	explicit FNiagaraSimulation();
	virtual ~FNiagaraSimulation()
	{}

	void Tick(float DeltaSeconds);


	FBox GetBounds() const { return CachedBounds; }


	FNiagaraEmitterParticleData &GetData()	{ return Data; }

	void SetConstants(const FNiagaraConstantMap &InMap)	{ Constants = InMap; }
	NiagaraEffectRenderer *GetEffectRenderer()	{ return EffectRenderer;  }
	
	void SetUpdateScript(UNiagaraScript *InScript)	{ UpdateScript = InScript;  }
	void SetSpawnScript(UNiagaraScript *InScript)	{ SpawnScript = InScript; }

	bool IsEnabled()	{ return bIsEnabled;  }
	void SetEnabled(bool bInEnabled)	{ bIsEnabled = bInEnabled;  }

	void SetSpawnRate(float InRate)	{ SpawnRate = InRate; }
	float GetSpawnRate()		{ return SpawnRate;  }

	EEmitterRenderModuleType GetRenderModuleType()	{ return RenderModuleType; }
	ENGINE_API void SetRenderModuleType(EEmitterRenderModuleType Type, ERHIFeatureLevel::Type FeatureLevel);

	int32 GetNumParticles()	{ return Data.GetNumParticles(); }
private:
	float Age;
	bool bIsEnabled;
	/** Temporary stuff for the prototype. */
	float SpawnRate;
	EEmitterRenderModuleType RenderModuleType;

	/** The particle update script. */
	UNiagaraScript *UpdateScript;
	/** The particle spawn script. */
	UNiagaraScript *SpawnScript;
	/** Local constant set. */
	FNiagaraConstantMap Constants;
	/** particle simulation data */
	FNiagaraEmitterParticleData Data;
	/** Keep partial particle spawns from last frame */
	float SpawnRemainder;
	/** The cached ComponentToWorld transform. */
	FTransform CachedComponentToWorld;
	/** Cached bounds. */
	FBox CachedBounds;

	NiagaraEffectRenderer *EffectRenderer;

	/** Calc number to spawn */
	int32 CalcNumToSpawn(float DeltaSeconds)
	{
		float FloatNumToSpawn = SpawnRemainder + (DeltaSeconds * SpawnRate);
		int32 NumToSpawn = FMath::FloorToInt(FloatNumToSpawn);
		SpawnRemainder = FloatNumToSpawn - NumToSpawn;
		return NumToSpawn;
	}

	/** Run VM to update particle positions */
	void UpdateParticles(
		float DeltaSeconds,
		FVector4* PrevParticles,
		int32 PrevNumVectorsPerAttribute,
		FVector4* Particles,
		int32 NumVectorsPerAttribute,
		int32 NumParticles
		);

	int32 SpawnAndKillParticles(int32 NumToSpawn);


	/** Spawn a new particle at this index */
	int32 SpawnParticles(int32 NumToSpawn);

	/** Util to move a particle */
	void MoveParticleToIndex(int32 SrcIndex, int32 DestIndex)
	{
		FVector4 *SrcPtr = Data.GetCurrentBuffer() + SrcIndex;
		FVector4 *DestPtr = Data.GetCurrentBuffer() + DestIndex;

		for (int32 AttrIndex = 0; AttrIndex < Data.GetNumAttributes(); AttrIndex++)
		{
			*DestPtr = *SrcPtr;
			DestPtr += Data.GetParticleAllocation();
			SrcPtr += Data.GetParticleAllocation();
		}
	}

};
