// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "NiagaraScript.h"
#include "NiagaraCollision.h"
#include "NiagaraEmitterProperties.generated.h"

class UMaterial;
class UNiagaraEmitterProperties;
class UNiagaraEventReceiverEmitterAction;

//TODO: Event action that spawns other whole effects?
//One that calls a BP exposed delegate?

USTRUCT()
struct FNiagaraEventReceiverProperties
{
	GENERATED_BODY()

	FNiagaraEventReceiverProperties()
	: Name(NAME_None)
	, SourceEventGenerator(NAME_None)
	, SourceEmitter(NAME_None)
	{

	}

	FNiagaraEventReceiverProperties(FName InName, FName InEventGenerator, FName InSourceEmitter)
		: Name(InName)
		, SourceEventGenerator(InEventGenerator)
		, SourceEmitter(InSourceEmitter)
	{

	}

	/** The name of this receiver. */
	UPROPERTY(EditAnywhere, Category = "Event Receiver")
	FName Name;

	/** The name of the EventGenerator to bind to. */
	UPROPERTY(EditAnywhere, Category = "Event Receiver")
	FName SourceEventGenerator;

	/** The name of the emitter from which the Event Generator is taken. */
	UPROPERTY(EditAnywhere, Category = "Event Receiver")
	FName SourceEmitter;

	UPROPERTY(EditAnywhere, Instanced, Category = "Event Receiver")
	TArray<UNiagaraEventReceiverEmitterAction*> EmitterActions;
};

USTRUCT()
struct FNiagaraEventGeneratorProperties
{
	GENERATED_BODY()

	FNiagaraEventGeneratorProperties()
	: MaxEventsPerFrame(64)
	{

	}

	FNiagaraEventGeneratorProperties(FNiagaraDataSetProperties &Props, FName InEventGenerator, FName InSourceEmitter)
		: ID(Props.ID.Name)
		, SourceEmitter(InSourceEmitter)
		, SetProps(Props)		
	{

	}

	/** Max Number of Events that can be generated per frame. */
	UPROPERTY(EditAnywhere, Category = "Event Receiver")
	int32 MaxEventsPerFrame; //TODO - More complex allocation so that we can grow dynamically if more space is needed ?

	FName ID;
	FName SourceEmitter;

	UPROPERTY()
	FNiagaraDataSetProperties SetProps;
};


UENUM()
enum class EScriptExecutionMode : uint8
{
	EveryParticle = 0,
	SpawnedParticles,
	SingleParticle
};

UENUM()
enum class EScriptCompileIndices : uint8
{
	SpawnScript = 0,
	UpdateScript,
	EventScript
};

USTRUCT()
struct FNiagaraEmitterScriptProperties
{
	GENERATED_BODY()
	
 	UPROPERTY(EditAnywhere, Category = "Script")
 	UNiagaraScript *Script;

	UPROPERTY(EditAnywhere, EditFixedSize, Category = "Script")
	TArray<FNiagaraEventReceiverProperties> EventReceivers;

	UPROPERTY(EditAnywhere, EditFixedSize, Category = "Script")
	TArray<FNiagaraEventGeneratorProperties> EventGenerators;

	NIAGARA_API void InitDataSetAccess();
};

USTRUCT()
struct FNiagaraEventScriptProperties : public FNiagaraEmitterScriptProperties
{
	GENERATED_BODY()
			
	UPROPERTY(EditAnywhere, Category="Event Handling Type")
	EScriptExecutionMode ExecutionMode;

	UPROPERTY(EditAnywhere, Category = "Event spawning")
	uint32 SpawnNumber;

	UPROPERTY(EditAnywhere, Category = "Event spawning")
	uint32 MaxEventsPerFrame;

	UPROPERTY(EditAnywhere, Category = "Source Emitter")
	FGuid SourceEmitterID;

	UPROPERTY(EditAnywhere, Category = "Source Event")
	FName SourceEventName;
};

/** Represents timed burst of particles in an emitter. */
USTRUCT()
struct FNiagaraEmitterBurst
{
	GENERATED_BODY()

	/** The base time of the burst absolute emitter time. */
	UPROPERTY(EditAnywhere, Category = "Burst")
	float Time;

	/** 
	 * A range around the base time which can be used to randomize burst timing.  The resulting range is 
	 * from Time - (TimeRange / 2) to Time + (TimeRange / 2).
	 */
	UPROPERTY(EditAnywhere, Category = "Burst")
	float TimeRange;

	/** The minimum number of particles to spawn. */
	UPROPERTY(EditAnywhere, Category = "Burst")
	uint32 SpawnMinimum;

	/** The maximum number of particles to spawn. */
	UPROPERTY(EditAnywhere, Category = "Burst")
	uint32 SpawnMaximum;
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
	//Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	void Serialize(FArchive& Ar)override;
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;
	//End UObject Interface

	UPROPERTY(EditAnywhere, Category = "Emitter")
	float SpawnRate;
	UPROPERTY(EditAnywhere, Category = "Emitter")
	bool bLocalSpace;
	UPROPERTY(EditAnywhere, Category = "Render")
	UMaterial *Material;

	UPROPERTY(EditAnywhere, Category = "Emitter")
	float StartTime;
	UPROPERTY(EditAnywhere, Category = "Emitter")
	float EndTime;
	UPROPERTY(EditAnywhere, Category = "Emitter")
	int32 NumLoops;
	UPROPERTY(EditAnywhere, Category = "Emitter")
	ENiagaraCollisionMode CollisionMode;

	UPROPERTY(EditAnywhere, Instanced, Category = "Render")
	class UNiagaraEffectRendererProperties *RendererProperties;

	UPROPERTY()
	FNiagaraEmitterScriptProperties UpdateScriptProps;

	UPROPERTY()
	FNiagaraEmitterScriptProperties SpawnScriptProps;

	UPROPERTY(EditAnywhere, Category="Event Handler")
	FNiagaraEventScriptProperties EventHandlerScriptProps;

	UPROPERTY(EditAnywhere, Category = "Emitter")
	TArray<FNiagaraEmitterBurst> Bursts;

	/** When enabled, this will spawn using interpolated parameter values and perform a partial update at spawn time. This adds significant additional cost for spawning but will produce much smoother spawning for high spawn rates, erratic frame rates and fast moving emitters. */
	UPROPERTY(EditAnywhere, Category = "Emitter")
	uint32 bInterpolatedSpawning : 1;

#if WITH_EDITORONLY_DATA
	/** Adjusted every time that we compile this emitter. Lets us know that we might differ from any cached versions.*/
	UPROPERTY()
	FGuid ChangeId;

	void NIAGARA_API CompileScripts(TArray<ENiagaraScriptCompileStatus>& OutScriptStatuses, TArray<FString>& OutGraphLevelErrorMessages, TArray<FString>& PathNames);
	ENiagaraScriptCompileStatus NIAGARA_API CompileScript(EScriptCompileIndices InScriptToCompile, FString& OutGraphLevelErrorMessages);

	UNiagaraEmitterProperties* MakeRecursiveDeepCopy(UObject* DestOuter) const;
#endif
};


