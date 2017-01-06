// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
NiagaraSimulation.h: Niagara emitter simulation class
==============================================================================*/
#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
#include "NiagaraCommon.h"
#include "NiagaraDataSet.h"
#include "NiagaraEvents.h"
#include "NiagaraCollision.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraEmitterProperties.h"

class FNiagaraEffectInstance;
class NiagaraEffectRenderer;
struct FNiagaraEmitterHandle;

/**
* A Niagara particle simulation.
*/
struct FNiagaraSimulation
{

private:
	struct FNiagaraBurstInstance
	{
		float Time;
		uint32 NumberToSpawn;
	};

public:
	explicit FNiagaraSimulation(FNiagaraEffectInstance* InParentEffectInstance);
	bool bDumpAfterEvent;
	virtual ~FNiagaraSimulation()
	{
		//TODO: Clean up renderer pointer.
	}

	void Init(const FNiagaraEmitterHandle& InEmitterHandle, FName EffectInstanceName);
	void ResetSimulation();

	/** Called after all emitters in an effect have been initialized, allows emitters to access information from one another. */
	void PostResetSimulation();

	void PreTick();
	void Tick(float DeltaSeconds);

	uint32 CalculateEventSpawnCount(const FNiagaraEventScriptProperties &EventHandlerProps, TArray<int32> &EventSpawnCounts, FNiagaraDataSet *EventSet);
	void TickEvents(float DeltaSeconds);

	void KillParticles();

	FBox GetBounds() const { return CachedBounds; }

	FNiagaraDataSet &GetData()	{ return Data; }

	FNiagaraParameters &GetUpdateConstants()	{ return ExternalUpdateConstants; }
	FNiagaraParameters &GetSpawnConstants() { return ExternalSpawnConstants; }

	NiagaraEffectRenderer *GetEffectRenderer()	{ return EffectRenderer; }
	
	bool IsEnabled()const 	{ return bIsEnabled;  }

	void NIAGARA_API UpdateEffectRenderer(ERHIFeatureLevel::Type FeatureLevel);

	int32 GetNumParticles()	{ return Data.GetNumInstances(); }

	const FNiagaraEmitterHandle& GetEmitterHandle() const { return *EmitterHandle; }

	FNiagaraEffectInstance* GetParentEffectInstance()	{ return ParentEffectInstance; }

	float NIAGARA_API GetTotalCPUTime();
	int	NIAGARA_API GetTotalBytesUsed();

	float GetAge() { return Age; }
	int32 GetLoopCount()	{ return Loops; }
	void LoopRestart()	
	{ 
		Age = 0.0f;
		Loops++;
		SetTickState(NTS_Running);
	}

	ENiagaraTickState NIAGARA_API GetTickState()	{ return TickState; }
	void NIAGARA_API SetTickState(ENiagaraTickState InState)	{ TickState = InState; }

	FNiagaraDataSet* GetDataSet(FNiagaraDataSetID SetID);

	void SpawnBurst(uint32 Count) { SpawnRemainder += Count; }

	/** Gets the simulation time constants which are defined externally by the spawn script. */
	FNiagaraParameters& GetExternalSpawnConstants() { return ExternalSpawnConstants; }

	/** Gets the simulation time constants which are defined externally by the update script. */
	FNiagaraParameters& GetExternalUpdateConstants() { return ExternalUpdateConstants; }

private:
	void InitInternal();

private:
	/* A handle to the emitter this simulation is simulating. */
	const FNiagaraEmitterHandle* EmitterHandle;

	/* The age of the emitter*/
	float Age;
	/* how many loops this emitter has gone through */
	uint32 Loops;
	/* If false, don't tick or render*/
	bool bIsEnabled;
	/* Seconds taken to process everything (including rendering) */
	float CPUTimeMS;
	/* Emitter tick state */
	ENiagaraTickState TickState;
	
	/** Local constant set. */
	FNiagaraParameters ExternalSpawnConstants;
	FNiagaraParameters ExternalUpdateConstants;
	FNiagaraParameters ExternalEventHandlerConstants;

	TArray<FNiagaraBurstInstance> BurstInstances;
	int32 CurrentBurstInstanceIndex;

	/** particle simulation data */
	FNiagaraDataSet Data;
	/** Keep partial particle spawns from last frame */
	float SpawnRemainder;
	/** The cached ComponentToWorld transform. */
	FTransform CachedComponentToWorld;
	/** Cached bounds. */
	FBox CachedBounds;

	NiagaraEffectRenderer *EffectRenderer;
	FNiagaraEffectInstance *ParentEffectInstance;

	TArray<FNiagaraDataSet*> UpdateScriptEventDataSets;
	TMap<FNiagaraDataSetID, FNiagaraDataSet*> DataSetMap;
	
	FNiagaraCollisionBatch CollisionBatch;

	FNiagaraVariable EmitterAge;
	FNiagaraVariable DeltaTime;
	FNiagaraVariable ExecCount;
	FNiagaraVariable SpawnRateParam;
	FNiagaraVariable SpawnRateInvParam;
	FNiagaraVariable SpawnRemainderParam;

	FName OwnerEffectInstanceName;
	
	/** Calc number to spawn */
	int32 CalcNumToSpawn(float DeltaSeconds);
	
	/** Runs a script in the VM over a specific range of particles. */
	void RunVMScript(const FNiagaraEmitterScriptProperties& ScriptProps, EUnusedAttributeBehaviour UnusedAttribBehaviour, FNiagaraParameters &Parameters);
	void RunVMScript(const FNiagaraEmitterScriptProperties& ScriptProps, EUnusedAttributeBehaviour UnusedAttribBehaviour, uint32 StartParticle, FNiagaraParameters &Parameters);
	void RunVMScript(const FNiagaraEmitterScriptProperties& ScriptProps, EUnusedAttributeBehaviour UnusedAttribBehaviour, uint32 StartParticle, uint32 NumParticles, FNiagaraParameters &Parameters);
	void RunEventHandlerScript(const FNiagaraEmitterScriptProperties& ScriptProps, FNiagaraParameters &Parameters, FNiagaraDataSet *InEventDataSet, uint32 StartParticle, uint32 NumParticles, uint32 EventIndex);
	bool CheckAttributesForRenderer();
};
