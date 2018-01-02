// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
NiagaraEmitterInstance.h: Niagara emitter simulation class
==============================================================================*/
#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
#include "NiagaraCommon.h"
#include "NiagaraDataSet.h"
#include "NiagaraEvents.h"
#include "NiagaraCollision.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraEmitter.h"
#include "NiagaraScriptExecutionContext.h"

class FNiagaraSystemInstance;
class NiagaraRenderer;
struct FNiagaraEmitterHandle;
class UNiagaraParameterCollection;
class UNiagaraParameterCollectionInstance;

/**
* A Niagara particle simulation.
*/
struct FNiagaraEmitterInstance
{

private:
	struct FNiagaraBurstInstance
	{
		float Time;
		uint32 NumberToSpawn;
	};

public:
	explicit FNiagaraEmitterInstance(FNiagaraSystemInstance* InParentSystemInstance);
	bool bDumpAfterEvent;
	virtual ~FNiagaraEmitterInstance();

	void Init(int32 InEmitterIdx, FName SystemInstanceName);

	void ResetSimulation();

	/** Called after all emitters in an System have been initialized, allows emitters to access information from one another. */
	void PostInitSimulation();

	void DirtyDataInterfaces();
	
	/** Replaces the binding for a single parameter colleciton instance. If for example the component begins to override the global instance. */
	//void RebindParameterCollection(UNiagaraParameterCollectionInstance* OldInstance, UNiagaraParameterCollectionInstance* NewInstance);
	void BindParameters();
	void UnbindParameters();

	void PreTick();
	void Tick(float DeltaSeconds);
	void PostProcessParticles();
	bool HandleCompletion(bool bForce=false);

	FORCEINLINE bool ShouldTick()const { return ExecutionState == ENiagaraExecutionState::Active || GetNumParticles() > 0; }

	uint32 CalculateEventSpawnCount(const FNiagaraEventScriptProperties &EventHandlerProps, TArray<int32> &EventSpawnCounts, FNiagaraDataSet *EventSet);

	NIAGARA_API FBox CalculateDynamicBounds();

	FNiagaraDataSet &GetData()	{ return *ParticleDataSet; }

	int32 GetEmitterRendererNum() const {
		return EmitterRenderer.Num();
	}

	NiagaraRenderer *GetEmitterRenderer(int32 Index)	{ return EmitterRenderer[Index]; }

	FORCEINLINE bool IsDisabled()const { return ExecutionState == ENiagaraExecutionState::Disabled; }
	FORCEINLINE bool IsComplete()const { return ExecutionState == ENiagaraExecutionState::Complete || ExecutionState == ENiagaraExecutionState::Disabled; }
	
	/** Sets the error state. */
	void SetError(bool bInError) { bError = bInError; }
		
	/** Create a new NiagaraRenderer. The old renderer is not immediately deleted, but instead put in the ToBeRemoved list.*/
	void NIAGARA_API UpdateEmitterRenderer(ERHIFeatureLevel::Type FeatureLevel, TArray<NiagaraRenderer*>& ToBeAddedList, TArray<NiagaraRenderer*>& ToBeRemovedList);

	FORCEINLINE int32 GetNumParticles()const	{ return ParticleDataSet->GetNumInstances(); }

	NIAGARA_API const FNiagaraEmitterHandle& GetEmitterHandle() const;

	FNiagaraSystemInstance* GetParentSystemInstance()const	{ return ParentSystemInstance; }

	float NIAGARA_API GetTotalCPUTime();
	int	NIAGARA_API GetTotalBytesUsed();
	
	ENiagaraExecutionState NIAGARA_API GetExecutionState()	{ return ExecutionState; }
	void NIAGARA_API SetExecutionState(ENiagaraExecutionState InState);

	FNiagaraDataSet* GetDataSet(FNiagaraDataSetID SetID);

	/** Tell the renderer thread that we're done with the Niagara renderer on this simulation.*/
	void ClearRenderer();

	FBox GetBounds();

	FNiagaraScriptExecutionContext& GetSpawnExecutionContext() { return SpawnExecContext; }
	FNiagaraScriptExecutionContext& GetUpdateExecutionContext() { return UpdateExecContext; }
	TArray<FNiagaraScriptExecutionContext>& GetEventExecutionContexts() { return EventExecContexts; }

	TArray<FNiagaraSpawnInfo>& GetSpawnInfo() { return SpawnInfos; }

	NIAGARA_API bool IsReadyToRun() const;

private:

	void CheckForErrors();

	/** The index of our emitter in our parent system instance. */
	int32 EmitterIdx;

	/* The age of the emitter*/
	float Age;
	/* how many loops this emitter has gone through */
	uint32 Loops;
	
	/* Some error has occurred so stop ticking or rendering. Better to just kill the emitter? */
	uint32 bError : 1;
	
	/** Typical resets must be deferred until the tick as the RT could still be using the current buffer. */
	uint32 bResetPending : 1;

	/* Seconds taken to process everything (including rendering) */
	float CPUTimeMS;
	/* Emitter tick state */
	ENiagaraExecutionState ExecutionState;
	/* Emitter bounds */
	FBox CachedBounds;

	/** Array of all spawn info driven by our owning emitter script. */
	TArray<FNiagaraSpawnInfo> SpawnInfos;

	FNiagaraScriptExecutionContext SpawnExecContext;
	FNiagaraScriptExecutionContext UpdateExecContext;
	FNiagaraComputeExecutionContext GPUExecContext;
	TArray<FNiagaraScriptExecutionContext> EventExecContexts;

	FNiagaraParameterDirectBinding<float> SpawnIntervalBinding;
	FNiagaraParameterDirectBinding<float> InterpSpawnStartBinding;

	FNiagaraParameterDirectBinding<float> SpawnEmitterAgeBinding;
	FNiagaraParameterDirectBinding<float> UpdateEmitterAgeBinding;
	TArray<FNiagaraParameterDirectBinding<float>> EventEmitterAgeBindings;

	FNiagaraParameterDirectBinding<FNiagaraBool> SpawnEmitterLocalSpaceBinding;
	FNiagaraParameterDirectBinding<FNiagaraBool> UpdateEmitterLocalSpaceBinding;
	TArray<FNiagaraParameterDirectBinding<FNiagaraBool>> EventEmitterLocalSpaceBindings;

	FNiagaraParameterDirectBinding<int32> SpawnExecCountBinding;
	FNiagaraParameterDirectBinding<int32> UpdateExecCountBinding;
	TArray<FNiagaraParameterDirectBinding<int32>> EventExecCountBindings;

	/** particle simulation data. Must be a shared ref as various things on the RT can have direct ref to it. */
	FNiagaraDataSet* ParticleDataSet;

	TArray<NiagaraRenderer*> EmitterRenderer;
	FNiagaraSystemInstance *ParentSystemInstance;

	TArray<FNiagaraDataSet*> UpdateScriptEventDataSets;
	TArray<FNiagaraDataSet*> SpawnScriptEventDataSets;
	TMap<FNiagaraDataSetID, FNiagaraDataSet*> DataSetMap;
	

	FName OwnerSystemInstanceName;

#if WITH_EDITORONLY_DATA
	bool CheckAttributesForRenderer(int32 Index);
#endif

	FNiagaraDataSetAccessor<FVector> PositionAccessor;
	FNiagaraDataSetAccessor<FVector2D> SizeAccessor;
	FNiagaraDataSetAccessor<FVector> MeshScaleAccessor;
};
