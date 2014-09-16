// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Components/NiagaraComponent.h"
#include "Engine/NiagaraScript.h"
#include "VectorVM.h"
#include "ParticleHelper.h"
#include "Particles/ParticleResources.h"
#include "Engine/NiagaraConstants.h"
#include "Engine/NiagaraEffectRenderer.h"
#include "MeshBatch.h"
#include "SceneUtils.h"

DECLARE_CYCLE_STAT(TEXT("Tick"),STAT_NiagaraTick,STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Simulate"),STAT_NiagaraSimulate,STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Spawn + Kill"),STAT_NiagaraSpawnAndKill,STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Gen Verts"),STAT_NiagaraGenerateVertices,STATGROUP_Niagara);
DECLARE_DWORD_COUNTER_STAT(TEXT("NumParticles"),STAT_NiagaraNumParticles,STATGROUP_Niagara);


DEFINE_LOG_CATEGORY_STATIC(LogNiagaraComponent, All, All);


FNiagaraSceneProxy::FNiagaraSceneProxy(const UNiagaraComponent* InComponent)
		:	FPrimitiveSceneProxy(InComponent)
{
	if (InComponent->RenderModuleType == Sprites)
	{
		EffectRenderer = new NiagaraEffectRendererSprites(InComponent, this);
	}
	else  if (InComponent->RenderModuleType == Ribbon)
	{
		EffectRenderer = new NiagaraEffectRendererRibbon(InComponent, this);
	}

}

FNiagaraSceneProxy::~FNiagaraSceneProxy()
{
	ReleaseRenderThreadResources();
}

/** Called on render thread to assign new dynamic data */
void FNiagaraSceneProxy::SetDynamicData_RenderThread(FNiagaraDynamicDataBase* NewDynamicData)
{
	EffectRenderer->SetDynamicData_RenderThread(NewDynamicData);
	return;
}


void FNiagaraSceneProxy::ReleaseRenderThreadResources()
{
	EffectRenderer->ReleaseRenderThreadResources();
	return;
}

// FPrimitiveSceneProxy interface.
void FNiagaraSceneProxy::CreateRenderThreadResources()
{
	EffectRenderer->CreateRenderThreadResources();
	return;
}

void FNiagaraSceneProxy::OnActorPositionChanged()
{
	//WorldSpacePrimitiveUniformBuffer.ReleaseResource();
}

void FNiagaraSceneProxy::OnTransformChanged()
{
	//WorldSpacePrimitiveUniformBuffer.ReleaseResource();
}

void FNiagaraSceneProxy::PreRenderView(const FSceneViewFamily* ViewFamily, const uint32 VisibilityMap, int32 FrameNumber)
{
	EffectRenderer->PreRenderView(ViewFamily, VisibilityMap, FrameNumber);
	return;

}
		  
void FNiagaraSceneProxy::DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View) 
{
	EffectRenderer->DrawDynamicElements(PDI, View);
	return;
}

FPrimitiveViewRelevance FNiagaraSceneProxy::GetViewRelevance(const FSceneView* View)
{
	return EffectRenderer->GetViewRelevance(View);
}
/*
virtual bool CanBeOccluded() const override
{
	return !MaterialRelevance.bDisableDepthTest;
}
*/

uint32 FNiagaraSceneProxy::GetMemoryFootprint() const
{ 
	return (sizeof(*this) + GetAllocatedSize()); 
}

uint32 FNiagaraSceneProxy::GetAllocatedSize() const
{ 
	uint32 DynamicDataSize = 0;
	return FPrimitiveSceneProxy::GetAllocatedSize() + EffectRenderer->GetDynamicDataSize();
}


void FNiagaraSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	EffectRenderer->GetDynamicMeshElements(Views, ViewFamily, VisibilityMap, Collector);
}




namespace ENiagaraVectorAttr
{
	enum Type
	{
		Position,
		Velocity,
		Color,
		Rotation,
		RelativeTime,
		MaxVectorAttribs
	};
}


/**
 * A niagara particle simulation.
 */
class FNiagaraSimulation
{
public:
	explicit FNiagaraSimulation(const UNiagaraComponent* InComponent)
		: SpawnRate(InComponent->SpawnRate)
		, Component(*InComponent)
		, UpdateScript(*InComponent->UpdateScript)
		, SpawnScript(InComponent->SpawnScript)
		, ConstantTable(InComponent->UpdateScript->ConstantTable)
		, SpawnRemainder(0.0f)
		, CachedBounds(ForceInit)
	{
		check(InComponent->UpdateScript && UpdateScript.ByteCode.Num());

		// make room for constants
		while (ConstantTable.Num() < 10)	
		{
			ConstantTable.Add( FVector4(0.0f, 0.0f, 0.0f, 0.0f) );
		}
	}

	void Tick(float DeltaSeconds)
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraTick);

		const int32 NumAttributes = UpdateScript.Attributes.Num();

		// Cache the ComponentToWorld transform.
		CachedComponentToWorld = Component.GetComponentToWorld();
		
		Data.SwapBuffers();

		// Figure out how many we will spawn.
		int32 NumToSpawn = CalcNumToSpawn(DeltaSeconds);
		int32 MaxNewParticles = Data.GetNumParticles() + NumToSpawn;

		// Remember the stride of the original data.
		int32 PrevNumVectorsPerAttribute = Data.GetParticleAllocation();

		Data.Allocate( MaxNewParticles );
		// Simulate particles forward by DeltaSeconds.
		UpdateParticles(
			DeltaSeconds,
			Data.GetPreviousBuffer(),
			PrevNumVectorsPerAttribute,
			Data.GetCurrentBuffer(),
			MaxNewParticles,
			Data.GetNumParticles()
			);

		SpawnAndKillParticles(
			NumToSpawn
			);

		DECLARE_DWORD_COUNTER_STAT(TEXT("NumParticles"), STAT_NiagaraNumParticles, STATGROUP_Niagara);
		INC_DWORD_STAT_BY(STAT_NiagaraNumParticles, Data.GetNumParticles());

	}

	FBox GetBounds() const { return CachedBounds; }


	FNiagaraEmitterParticleData &GetData()	{ return Data; }

	void SetConstant(int Idx, const FVector4 &Value)
	{
		check(Idx < ConstantTable.Num());
		ConstantTable[Idx] = Value;
	}

private:
	/** Temporary stuff for the prototype. */
	float SpawnRate;

	/** The component. */
	const UNiagaraComponent& Component;
	/** The particle update script. */
	UNiagaraScript& UpdateScript;
	/** The particle spawn script. */
	UNiagaraScript *SpawnScript;
	/** Local constant table. */
	TArray<FVector4> ConstantTable;
	/** particle simulation data */
	FNiagaraEmitterParticleData Data;
	/** Keep partial particle spawns from last frame */
	float SpawnRemainder;
	/** The cached ComponentToWorld transform. */
	FTransform CachedComponentToWorld;
	/** Cached bounds. */
	FBox CachedBounds;


	/* builtin constants are setup in the tick function, but a script may have additional constants 
	 derived from unconnected nodes, so we need to merge those into the simulation constant table */
	void MergeScriptConstants(UNiagaraScript *Script)
	{
		// start at NumBuiltinConstants+1, because index 0 is always the fixed zero constant
		for (int i = NiagaraConstants::NumBuiltinConstants + 1; i < Script->ConstantTable.Num(); i++)
		{
			if (ConstantTable.Num() <= i)
			{
				ConstantTable.Add(FVector4());
			}
			ConstantTable[i] = Script->ConstantTable[i];
		}
	}

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
		)
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraSimulate);

		VectorRegister* InputRegisters[VectorVM::MaxInputRegisters] = { 0 };
		VectorRegister* OutputRegisters[VectorVM::MaxOutputRegisters] = {0};
		const int32 NumAttr = UpdateScript.Attributes.Num();

		check(NumAttr < VectorVM::MaxInputRegisters);
		check(NumAttr < VectorVM::MaxOutputRegisters);

		// Setup input and output registers.
		for (int32 AttrIndex = 0; AttrIndex < NumAttr; ++AttrIndex)
		{
			InputRegisters[AttrIndex] = (VectorRegister*)(PrevParticles + AttrIndex * PrevNumVectorsPerAttribute);
			OutputRegisters[AttrIndex] = (VectorRegister*)(Particles + AttrIndex * NumVectorsPerAttribute);
		}

		// copy script specific constants into the constant table
		MergeScriptConstants(&UpdateScript);

		VectorVM::Exec(
			UpdateScript.ByteCode.GetData(),
			InputRegisters,
			NumAttr,
			OutputRegisters,
			NumAttr,
			ConstantTable.GetData(),
			NumParticles
			);
	}

	int32 SpawnAndKillParticles(int32 NumToSpawn)
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraSpawnAndKill);
		int32 OrigNumParticles = Data.GetNumParticles();
		int32 CurNumParticles = OrigNumParticles;
		CurNumParticles = SpawnParticles(NumToSpawn);

		// run the spawn graph over all new particles
		if (SpawnScript && SpawnScript->ByteCode.Num())
		{
			VectorRegister* InputRegisters[VectorVM::MaxInputRegisters] = { 0 };
			VectorRegister* OutputRegisters[VectorVM::MaxOutputRegisters] = { 0 };
			const int32 NumAttr = SpawnScript->Attributes.Num();
			const int32 NumVectors = NumToSpawn;

			check(NumAttr < VectorVM::MaxInputRegisters);
			check(NumAttr < VectorVM::MaxOutputRegisters);

			FVector4 *NewParticlesStart = Data.GetCurrentBuffer() + OrigNumParticles;

			// Setup input and output registers.
			for (int32 AttrIndex = 0; AttrIndex < NumAttr; ++AttrIndex)
			{
				InputRegisters[AttrIndex] = (VectorRegister*)(NewParticlesStart + AttrIndex * Data.GetParticleAllocation());
				OutputRegisters[AttrIndex] = (VectorRegister*)(NewParticlesStart + AttrIndex * Data.GetParticleAllocation());
			}

			// copy script specific constants into the constant table
			MergeScriptConstants(SpawnScript);

			VectorVM::Exec(
				SpawnScript->ByteCode.GetData(),
				InputRegisters,
				NumAttr,
				OutputRegisters,
				NumAttr,
				ConstantTable.GetData(),
				NumVectors
				);
		}

		// Iterate over looking for dead particles and move from the end of the list to the dead location, compacting in the process
		int32 ParticleIndex = 0;
		const FVector4* ParticleRelativeTimes = Data.GetAttributeData("Age");
		while (ParticleIndex < OrigNumParticles)
		{
			if (ParticleRelativeTimes[ParticleIndex].X > 1.0f)
			{
				// Particle is dead, move one from the end here.
				MoveParticleToIndex(--CurNumParticles, ParticleIndex);
			}
			ParticleIndex++;
		}

		Data.SetNumParticles(CurNumParticles);
		return CurNumParticles;
	}

	/** Spawn a new particle at this index */
	int32 SpawnParticles(int32 NumToSpawn)
	{
		FVector4 *PosPtr = Data.GetAttributeDataWrite("Position");
		FVector4 *VelPtr = Data.GetAttributeDataWrite("Velocity");
		FVector4 *ColPtr = Data.GetAttributeDataWrite("Color");
		FVector4 *RotPtr = Data.GetAttributeDataWrite("Rotation");
		FVector4 *AgePtr = Data.GetAttributeDataWrite("Age");

		// Spawn new Particles at the end of the buffer
		int32 ParticleIndex = Data.GetNumParticles();
		for (int32 i = 0; i < NumToSpawn; i++)
		{
			FVector SpawnLocation = CachedComponentToWorld.GetLocation();
			SpawnLocation.X += FMath::FRandRange(-20.f, 20.f);
			SpawnLocation.Y += FMath::FRandRange(-20.f, 20.f);
			SpawnLocation.Z += FMath::FRandRange(-20.f, 20.f);

			PosPtr[ParticleIndex] = SpawnLocation;
			ColPtr[ParticleIndex] = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
			VelPtr[ParticleIndex] = FVector4(0.0f, 0.0f, 2.0f, 0.0f);
			RotPtr[ParticleIndex] = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
			AgePtr[ParticleIndex] = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
			ParticleIndex++;
		}
		return ParticleIndex;
	}

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

//////////////////////////////////////////////////////////////////////////

UNiagaraComponent::UNiagaraComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
	RenderModuleType = ERenderModuleType::Sprites;
	SpawnRate = 20.f;
	Simulation = nullptr;
}


void UNiagaraComponent::TickComponent(float DeltaSeconds, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	if (Simulation)
	{
		EmitterAge += DeltaSeconds;

		Simulation->SetConstant(0, FVector4(0.0f, 0.0f, 0.0f, 0.0f) );	// zero constant
		Simulation->SetConstant(1, FVector4(DeltaSeconds, DeltaSeconds, DeltaSeconds, DeltaSeconds));
		Simulation->SetConstant(2, FVector4(ComponentToWorld.GetTranslation()));
		Simulation->SetConstant(3, FVector4(EmitterAge, EmitterAge, EmitterAge, EmitterAge));
		Simulation->SetConstant(4, FVector4(ComponentToWorld.GetUnitAxis(EAxis::X)) );
		Simulation->SetConstant(5, FVector4(ComponentToWorld.GetUnitAxis(EAxis::Y)) );
		Simulation->SetConstant(6, FVector4(ComponentToWorld.GetUnitAxis(EAxis::Z)) );

		Simulation->Tick(DeltaSeconds);
		UpdateComponentToWorld();
		MarkRenderDynamicDataDirty();
	}
}

void UNiagaraComponent::OnRegister()
{
	Super::OnRegister();

	ensure(Simulation == NULL);
	if (UpdateScript && UpdateScript->ByteCode.Num() && UpdateScript->Attributes.Num() == ENiagaraVectorAttr::MaxVectorAttribs)
	{
		Simulation = new FNiagaraSimulation(this);
	}
	VectorVM::Init();
}


void UNiagaraComponent::OnUnregister()
{
	Super::OnUnregister();

	if(Simulation != NULL)
	{
		delete Simulation;
		Simulation = NULL;
	}
}

void UNiagaraComponent::SendRenderDynamicData_Concurrent()
{
	if (Simulation && SceneProxy)
	{
		FNiagaraSceneProxy *NiagaraProxy = static_cast<FNiagaraSceneProxy*>(SceneProxy);
		FNiagaraDynamicDataBase* DynamicData = NiagaraProxy->GetEffectRenderer()->GenerateVertexData(Simulation->GetData());

		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FSendNiagaraDynamicData,
			NiagaraEffectRenderer*, EffectRenderer, (NiagaraEffectRenderer*)NiagaraProxy->GetEffectRenderer(),
			FNiagaraDynamicDataBase*,DynamicData,DynamicData,
		{
			EffectRenderer->SetDynamicData_RenderThread(DynamicData);
		});
	}
}

int32 UNiagaraComponent::GetNumMaterials() const
{
	return 0;
}

UMaterialInterface* UNiagaraComponent::GetMaterial(int32 ElementIndex) const
{
	return NULL;

	UMaterialInterface* RequestedMaterial = NULL;
	if (ElementIndex == 0)
	{
		RequestedMaterial = Material;
	}
	return RequestedMaterial;
}

void UNiagaraComponent::SetMaterial(int32 ElementIndex, UMaterialInterface* InMaterial)
{
	if (ElementIndex == 0)
	{
		Material = InMaterial;
	}
}

FBoxSphereBounds UNiagaraComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	FBox SimBounds(ForceInit);
	//if (Simulation)
	//{
		//SimBounds = Simulation->GetBounds();
	//}
	//if (!SimBounds.IsValid)
	{
		SimBounds.Min = FVector(-HALF_WORLD_MAX,-HALF_WORLD_MAX,-HALF_WORLD_MAX);
		SimBounds.Max = FVector(+HALF_WORLD_MAX,+HALF_WORLD_MAX,+HALF_WORLD_MAX);
	}
	return FBoxSphereBounds(SimBounds);
}

FPrimitiveSceneProxy* UNiagaraComponent::CreateSceneProxy()
{
	return new FNiagaraSceneProxy(this);
}

#if WITH_EDITOR
void UNiagaraComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FComponentReregisterContext ReregisterContext(this);
}
#endif // WITH_EDITOR
