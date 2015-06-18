// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
NiagaraSimulation.h: Niagara emitter simulation class
==============================================================================*/
#pragma once

#include "NiagaraScript.h"
#include "NiagaraComponent.h"
#include "NiagaraSimulation.generated.h"

DECLARE_CYCLE_STAT(TEXT("Tick"), STAT_NiagaraTick, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Simulate"), STAT_NiagaraSimulate, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Spawn"), STAT_NiagaraSpawn, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Kill"), STAT_NiagaraKill, STATGROUP_Niagara);


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


USTRUCT()
struct FNiagaraEmitterScriptProperties
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Script")
	UNiagaraScript *Script;

	UPROPERTY(EditAnywhere, Category = "Script", meta = (ShowOnlyInnerProperties))
	FNiagaraConstants ExternalConstants;

	void Init(UNiagaraEmitterProperties* EmitterProps)
	{
		if (Script)
		{
			ExternalConstants.Init(EmitterProps, this);
		}
		else
		{
			ExternalConstants.Empty();
		}
	}
};

//This struct now only exists for backwards compatibility and should be removed once effects are updated.
USTRUCT()
struct FDeprecatedNiagaraEmitterProperties
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	FString Name;
	UPROPERTY()
	bool bIsEnabled;
	UPROPERTY()
	float SpawnRate;
	UPROPERTY()
	UNiagaraScript *UpdateScript;
	UPROPERTY()
	UNiagaraScript *SpawnScript;
	UPROPERTY()
	UMaterial *Material;
	UPROPERTY()
	TEnumAsByte<EEmitterRenderModuleType> RenderModuleType;
	UPROPERTY()
	float StartTime;
	UPROPERTY()
	float EndTime;
	UPROPERTY()
	class UNiagaraEffectRendererProperties *RendererProperties;
	UPROPERTY()
	FNiagaraConstantMap ExternalConstants;		// these are the update script constants from the effect editor; will be added to the emitter's constant map
	UPROPERTY()
	FNiagaraConstantMap ExternalSpawnConstants;		// these are the spawn script constants from the effect editor; will be added to the emitter's constant map
	UPROPERTY()
	int32 NumLoops;
};

/** 
 *	UNiagaraEmitterProperties stores the attributes of an FNiagaraSimulation
 *	that need to be serialized and are used for its initialization 
 */
UCLASS(MinimalAPI)
class UNiagaraEmitterProperties : public UObject
{
	GENERATED_UCLASS_BODY()
public:
	void Init()
	{
		SpawnScriptProps.Init(this);
		UpdateScriptProps.Init(this);
	}

	void InitFromOldStruct(FDeprecatedNiagaraEmitterProperties& OldStruct);

	//Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//End UObject Interface

	UPROPERTY(EditAnywhere, Category = "Emitter")
	FString EmitterName;
	UPROPERTY(EditAnywhere, Category = "Emitter")
	bool bIsEnabled;
	UPROPERTY(EditAnywhere, Category = "Emitter")
	float SpawnRate;
	UPROPERTY(EditAnywhere, Category = "Emitter")
	UMaterial *Material;
	UPROPERTY(EditAnywhere, Category = "Emitter")
	TEnumAsByte<EEmitterRenderModuleType> RenderModuleType;

	UPROPERTY(EditAnywhere, Category = "Emitter")
	float StartTime;
	UPROPERTY(EditAnywhere, Category = "Emitter")
	float EndTime;

	UPROPERTY()
	class UNiagaraEffectRendererProperties *RendererProperties;

	UPROPERTY(EditAnywhere, Category = "Emitter")
	int32 NumLoops;

	UPROPERTY(EditAnywhere, Category = "Emitter", meta = (ShowOnlyInnerProperties))
	FNiagaraEmitterScriptProperties UpdateScriptProps;

	UPROPERTY(EditAnywhere, Category = "Emitter", meta = (ShowOnlyInnerProperties))
	FNiagaraEmitterScriptProperties SpawnScriptProps;
};

/**
* A Niagara particle simulation.
*/
struct FNiagaraSimulation
{
public:
	explicit FNiagaraSimulation(TWeakObjectPtr<UNiagaraEmitterProperties> InProps, UNiagaraEffect* Effect);
	FNiagaraSimulation(TWeakObjectPtr<UNiagaraEmitterProperties> Props, UNiagaraEffect* Effect, ERHIFeatureLevel::Type InFeatureLevel);
	virtual ~FNiagaraSimulation()
	{}

	NIAGARA_API void Init();

	void Tick(float DeltaSeconds);


	FBox GetBounds() const { return CachedBounds; }


	FNiagaraEmitterParticleData &GetData()	{ return Data; }

	void SetConstants(const FNiagaraConstantMap &InMap)	{ Constants = InMap; }
	FNiagaraConstantMap &GetConstants()	{ return Constants; }

	NiagaraEffectRenderer *GetEffectRenderer()	{ return EffectRenderer; }
	
	bool IsEnabled()const 	{ return bIsEnabled;  }

	void NIAGARA_API SetRenderModuleType(EEmitterRenderModuleType Type, ERHIFeatureLevel::Type FeatureLevel);

	int32 GetNumParticles()	{ return Data.GetNumParticles(); }

	TWeakObjectPtr<UNiagaraEmitterProperties> GetProperties()	{ return Props; }
	void SetProperties(TWeakObjectPtr<UNiagaraEmitterProperties> InProps);

	UNiagaraEffect *GetParentEffect()	{ return ParentEffect; }

	float NIAGARA_API GetTotalCPUTime();
	int	NIAGARA_API GetTotalBytesUsed();

	void NIAGARA_API SetEnabled(bool bEnabled);

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
private:
	TWeakObjectPtr<UNiagaraEmitterProperties> Props;		// points to an entry in the array of FNiagaraProperties stored in the EffectInstance (itself pointing to the effect's properties)

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
	UNiagaraEffect *ParentEffect;

	/** Calc number to spawn */
	int32 CalcNumToSpawn(float DeltaSeconds)
	{
		if (TickState == NTS_Dead || TickState == NTS_Dieing || TickState == NTS_Suspended)
		{
			return 0;
		}

		float FloatNumToSpawn = SpawnRemainder + (DeltaSeconds * Props->SpawnRate);
		int32 NumToSpawn = FMath::FloorToInt(FloatNumToSpawn);
		SpawnRemainder = FloatNumToSpawn - NumToSpawn;
		return NumToSpawn;
	}
	
	/** Runs a script in the VM over a specific range of particles. */
	void RunVMScript(UNiagaraScript* Script, EUnusedAttributeBehaviour UnusedAttribBehaviour);
	void RunVMScript(UNiagaraScript* Script, EUnusedAttributeBehaviour UnusedAttribBehaviour, uint32 StartParticle);
	void RunVMScript(UNiagaraScript* Script, EUnusedAttributeBehaviour UnusedAttribBehaviour, uint32 StartParticle, uint32 NumParticles);

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

	bool CheckAttriubtesForRenderer();
};
