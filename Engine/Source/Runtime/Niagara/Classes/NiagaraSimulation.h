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
	virtual ~FNiagaraSimulation();

	void Init(const FNiagaraEmitterHandle& InEmitterHandle, FName EffectInstanceName);
	void ResetSimulation();
	void ReInitSimulation();
	void ReBindDataInterfaces();
	void ReInitDataInterfaces();

	/** Called after all emitters in an effect have been initialized, allows emitters to access information from one another. */
	void PostResetSimulation();

	void PreTick();
	void Tick(float DeltaSeconds);

	uint32 CalculateEventSpawnCount(const FNiagaraEventScriptProperties &EventHandlerProps, TArray<int32> &EventSpawnCounts, FNiagaraDataSet *EventSet);
	void TickEvents(float DeltaSeconds);

	void PostProcessParticles();

	FBox GetBounds() const { return CachedBounds; }

	FNiagaraDataSet &GetData()	{ return Data; }

	FNiagaraParameters &GetUpdateConstants()	{ return ExternalUpdateConstants; }
	FNiagaraParameters &GetSpawnConstants() { return ExternalSpawnConstants; }

	NiagaraEffectRenderer *GetEffectRenderer()	{ return EffectRenderer; }
	
	bool IsEnabled()const 	{ return bIsEnabled;  }
	
	/** Set whether or not this simulation is enabled*/
	void SetEnabled(bool bEnabled) { bIsEnabled = bEnabled; }

	/** Sets whether or not this simulation's data interfaces are valid. */
	void SetDataInterfacesProperlySetup(bool bEnabled) { bDataInterfacesEnabled = bEnabled; }
	
	/** Create a new NiagaraEffectRenderer. The old renderer is not immediately deleted, but instead put in the ToBeRemoved list.*/
	void NIAGARA_API UpdateEffectRenderer(ERHIFeatureLevel::Type FeatureLevel, TArray<NiagaraEffectRenderer*>& ToBeRemovedList);

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

	/** Gets the simulation time constants which are defined externally by the event script. */
	FNiagaraParameters& GetExternalEventConstants() { return ExternalUpdateConstants; }

	/** Gets the simulation time data sources which are defined externally by the spawn script. */
	TArray<FNiagaraScriptDataInterfaceInfo>& GetExternalSpawnDataInterfaceInfo() { return ExternalSpawnDataInterfaces; }

	/** Gets the simulation time data sources which are defined externally by the update script. */
	TArray<FNiagaraScriptDataInterfaceInfo>& GetExternalUpdateDataInterfaceInfo() { return ExternalUpdateDataInterfaces; }

	/** Gets the simulation time data sources which are defined externally by the event script. */
	TArray<FNiagaraScriptDataInterfaceInfo>& GetExternalEventDataInterfaceInfo() { return ExternalEventHandlerDataInterfaces; }

	/** If you modify the GetExternalSpawnDataInterfaceInfo array, make sure that you call this function to cause the data to be reset appropriately.*/
	void DirtyExternalSpawnDataInterfaceInfo() { ExternalSpawnFunctionTable.Empty(); }

	/** If you modify the GetExternalUpdateDataInterfaceInfo array, make sure that you call this function to cause the data to be reset appropriately.*/
	void DirtyExternalUpdateDataInterfaceInfo() { ExternalUpdateFunctionTable.Empty(); }

	/** If you modify the GetExternalEventDataInterfaceInfo array, make sure that you call this function to cause the data to be reset appropriately.*/
	void DirtyExternalEventDataInterfaceInfo() { ExternalEventHandlerFunctionTable.Empty(); }
	
	/** Tell the renderer thread that we're done with the Niagara renderer on this simulation.*/
	void ClearRenderer();

	/** Get the current spawn rate for the emitter.*/
	float GetSpawnRate() const { return SpawnRate; }
	
	/** Set the current spawn rate for the emitter.*/
	void SetSpawnRate(float InSpawnRate) { SpawnRate = InSpawnRate; }
	
	/** Helper accessor defining the string name of the spawn rate variable.*/
	static FString NIAGARA_API GetEmitterSpawnRateInternalVarName();

	/** Helper accessor defining the string name of the emitter enabed variable.*/
	static FString NIAGARA_API GetEmitterEnabledInternalVarName();

	FBox GetCachedBounds() { return CachedBounds; }

private:
	void InitInternal();

	/* A handle to the emitter this simulation is simulating. */
	const FNiagaraEmitterHandle* EmitterHandle;

	/* The age of the emitter*/
	float Age;
	/* how many loops this emitter has gone through */
	uint32 Loops;
	/* If false, don't tick or render*/
	bool bIsEnabled;

	/* If false, the data interfaces are incorrect, so don't tick or render. */
	bool bDataInterfacesEnabled;

	/* If false, the properties or scripts are incorrect, don't tick or render until a reset happens.*/
	bool bHasValidPropertiesAndScripts;

	/* Seconds taken to process everything (including rendering) */
	float CPUTimeMS;
	/* Emitter tick state */
	ENiagaraTickState TickState;
	/* Emitter bounds */
	FBox CachedBounds;


	/* The current spawn rate of the simulation*/
	float SpawnRate;
	
	/** Local constant set. */
	FNiagaraParameters ExternalSpawnConstants;
	FNiagaraParameters ExternalUpdateConstants;
	FNiagaraParameters ExternalEventHandlerConstants;

	/** The complete set of spawn constants used last frame. Used for interpolated spawning.*/
	FNiagaraParameters PrevSpawnConstants;

	/** Local data source sets. */
	TArray<FNiagaraScriptDataInterfaceInfo> ExternalSpawnDataInterfaces;
	TArray<FNiagaraScriptDataInterfaceInfo> ExternalUpdateDataInterfaces;
	TArray<FNiagaraScriptDataInterfaceInfo> ExternalEventHandlerDataInterfaces;

	/** Build local function tables. */
	TArray<FVMExternalFunction> ExternalSpawnFunctionTable;
	TArray<FVMExternalFunction> ExternalUpdateFunctionTable;
	TArray<FVMExternalFunction> ExternalEventHandlerFunctionTable;

	TArray<FNiagaraBurstInstance> BurstInstances;
	int32 CurrentBurstInstanceIndex;

	/** particle simulation data */
	FNiagaraDataSet Data;
	/** Keep partial particle spawns from last frame */
	float SpawnRemainder;
	/** The cached GetComponentTransform() transform. */
	FTransform CachedComponentToWorld;

	NiagaraEffectRenderer *EffectRenderer;
	FNiagaraEffectInstance *ParentEffectInstance;

	TArray<FNiagaraDataSet*> UpdateScriptEventDataSets;
	TArray<FNiagaraDataSet*> SpawnScriptEventDataSets;
	TMap<FNiagaraDataSetID, FNiagaraDataSet*> DataSetMap;
	
	FNiagaraCollisionBatch CollisionBatch;

	FNiagaraVariable EmitterAge;
	FNiagaraVariable DeltaTime;
	FNiagaraVariable InvDeltaTime;
	FNiagaraVariable ExecCount;
	FNiagaraVariable SpawnRateParam;
	FNiagaraVariable SpawnIntervalParam;
	FNiagaraVariable InterpSpawnStartDtParam;

	FName OwnerEffectInstanceName;

	/** Stat IDs for all scopes of each script. */
#if STATS
	TArray<TStatId> SpawnStatScopes;
	TArray<TStatId> UpdateStatScopes;
	TArray<TStatId> EventHandlerStatScopes;
#endif
	
	/** Calc number to spawn */
	int32 CalcNumToSpawn(float DeltaSeconds);
	
	bool CheckAttributesForRenderer();

	bool UpdateFunctionTableInternal(TArray<FNiagaraScriptDataInterfaceInfo>& DataInterfaces, TArray<FVMExternalFunction>& FunctionTable, UNiagaraScript* Script);
};
