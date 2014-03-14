// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "VectorVM.h"
#include "ParticleHelper.h"
#include "Particles/ParticleResources.h"

DECLARE_CYCLE_STAT(TEXT("Tick"),STAT_NiagaraTick,STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Simulate"),STAT_NiagaraSimulate,STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Spawn + Kill"),STAT_NiagaraSpawnAndKill,STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Gen Verts"),STAT_NiagaraGenerateVertices,STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("PreRenderView"),STAT_NiagaraPreRenderView,STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Render"),STAT_NiagaraRender,STATGROUP_Niagara);
DECLARE_DWORD_COUNTER_STAT(TEXT("NumParticles"),STAT_NiagaraNumParticles,STATGROUP_Niagara);

/** Struct used to pass dynamic data from game thread to render thread */
struct FNiagaraDynamicData
{
	/** Vertex data */
	TArray<FParticleSpriteVertex> VertexData;
};

/**
 * Scene proxy for drawing niagara particle simulations.
 */
class FNiagaraSceneProxy : public FPrimitiveSceneProxy
{
public:

	FNiagaraSceneProxy(const UNiagaraComponent* InComponent)
		:	FPrimitiveSceneProxy(InComponent)
		,	DynamicData(NULL)
	{
		Material = InComponent->Material;
		if (!Material || !Material->CheckMaterialUsage(MATUSAGE_ParticleSprites))
		{
			Material = UMaterial::GetDefaultMaterial(MD_Surface);
		}
		check(Material);
		MaterialRelevance = Material->GetRelevance();
	}

	~FNiagaraSceneProxy()
	{
		ReleaseRenderThreadResources();
	}

	/** Called on render thread to assign new dynamic data */
	void SetDynamicData_RenderThread(FNiagaraDynamicData* NewDynamicData)
	{
		check(IsInRenderingThread());

		if(DynamicData)
		{
			delete DynamicData;
			DynamicData = NULL;
		}

		DynamicData = NewDynamicData;
	}

private:
	void ReleaseRenderThreadResources()
	{
		VertexFactory.ReleaseResource();
		SpriteUniformBuffer.SafeRelease();
		PerViewUniformBuffers.Empty();
		WorldSpacePrimitiveUniformBuffer.ReleaseResource();
	}

	// FPrimitiveSceneProxy interface.
	virtual void CreateRenderThreadResources() OVERRIDE
	{
		VertexFactory.InitResource();

		FParticleSpriteUniformParameters UniformParameters;
		UniformParameters.AxisLockRight = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
		UniformParameters.AxisLockUp = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
		UniformParameters.RotationScale = 1.0f;
		UniformParameters.RotationBias = 0.0f;
		UniformParameters.TangentSelector = FVector4(1.0f, 0.0f, 0.0f, 0.0f);
		UniformParameters.InvDeltaSeconds = 0.0f;
		UniformParameters.SubImageSize = FVector4(1.0f,1.0f,1.0f,1.0f);
		UniformParameters.NormalsType = 0;
		UniformParameters.NormalsSphereCenter = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
		UniformParameters.NormalsCylinderUnitDirection = FVector4(0.0f, 0.0f, 1.0f, 0.0f);
		SpriteUniformBuffer = FParticleSpriteUniformBufferRef::CreateUniformBufferImmediate(UniformParameters, UniformBuffer_MultiUse);
		VertexFactory.SetSpriteUniformBuffer(SpriteUniformBuffer);
	}

	virtual void OnActorPositionChanged() OVERRIDE
	{
		WorldSpacePrimitiveUniformBuffer.ReleaseResource();
	}

	virtual void OnTransformChanged() OVERRIDE
	{
		WorldSpacePrimitiveUniformBuffer.ReleaseResource();
	}

	virtual void PreRenderView(const FSceneViewFamily* ViewFamily, const uint32 VisibilityMap, int32 FrameNumber) OVERRIDE
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraPreRenderView);

		check(DynamicData && DynamicData->VertexData.Num());

		int32 SizeInBytes = DynamicData->VertexData.GetTypeSize() * DynamicData->VertexData.Num();
		DynamicVertexAllocation = FGlobalDynamicVertexBuffer::Get().Allocate(SizeInBytes);
		if (DynamicVertexAllocation.IsValid())
		{
			// Copy the vertex data over.
			FMemory::Memcpy(DynamicVertexAllocation.Buffer, DynamicData->VertexData.GetData(), SizeInBytes);
			VertexFactory.SetInstanceBuffer(
				DynamicVertexAllocation.VertexBuffer,
				DynamicVertexAllocation.VertexOffset,
				sizeof(FParticleSpriteVertex),
				true
				);
			VertexFactory.SetDynamicParameterBuffer(NULL,0,0,true);

			// Compute the per-view uniform buffers.
			const int32 NumViews = ViewFamily->Views.Num();
			uint32 ViewBit = 1;
			for (int32 ViewIndex = 0; ViewIndex < NumViews; ++ViewIndex, ViewBit <<= 1)
			{
				FParticleSpriteViewUniformBufferRef* SpriteViewUniformBufferPtr = new(PerViewUniformBuffers) FParticleSpriteViewUniformBufferRef();
				if (VisibilityMap & ViewBit)
				{
					FParticleSpriteViewUniformParameters UniformParameters;
					UniformParameters.MacroUVParameters = FVector4(0.0f,0.0f,1.0f,1.0f);
					*SpriteViewUniformBufferPtr = FParticleSpriteViewUniformBufferRef::CreateUniformBufferImmediate(UniformParameters, UniformBuffer_SingleUse);
				}
			}

			// Update the primitive uniform buffer if needed.
			if (!WorldSpacePrimitiveUniformBuffer.IsInitialized())
			{
				FPrimitiveUniformShaderParameters PrimitiveUniformShaderParameters = GetPrimitiveUniformShaderParameters(
					FMatrix::Identity,
					GetActorPosition(),
					GetBounds(),
					GetLocalBounds(),
					ReceivesDecals()
					);
				WorldSpacePrimitiveUniformBuffer.SetContents(PrimitiveUniformShaderParameters);
				WorldSpacePrimitiveUniformBuffer.InitResource();
			}
		}
	}
		  
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View) OVERRIDE
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraRender);

		check(DynamicData && DynamicData->VertexData.Num());

		const bool bIsWireframe = View->Family->EngineShowFlags.Wireframe;
		FMaterialRenderProxy* MaterialRenderProxy = Material->GetRenderProxy(IsSelected(),IsHovered());

		if (DynamicVertexAllocation.IsValid()
			&& (bIsWireframe || !PDI->IsMaterialIgnored(MaterialRenderProxy)))
		{
			FMeshBatch MeshBatch;
			MeshBatch.VertexFactory = &VertexFactory;
			MeshBatch.CastShadow = CastsDynamicShadow();
			MeshBatch.bUseAsOccluder = false;
			MeshBatch.ReverseCulling = IsLocalToWorldDeterminantNegative();
			MeshBatch.Type = PT_TriangleList;
			MeshBatch.DepthPriorityGroup = GetDepthPriorityGroup(View);
			if (bIsWireframe)
			{
				MeshBatch.MaterialRenderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(IsSelected(),IsHovered());
			}
			else
			{
				MeshBatch.MaterialRenderProxy = MaterialRenderProxy;
			}

			FMeshBatchElement& MeshElement = MeshBatch.Elements[0];
			MeshElement.IndexBuffer = &GParticleIndexBuffer;
			MeshElement.FirstIndex = 0;
			MeshElement.NumPrimitives = 2;
			MeshElement.NumInstances = DynamicData->VertexData.Num();
			MeshElement.MinVertexIndex = 0;
			MeshElement.MaxVertexIndex = MeshElement.NumInstances * 4 - 1;
			MeshElement.PrimitiveUniformBufferResource = &WorldSpacePrimitiveUniformBuffer;

			int32 ViewIndex = View->Family->Views.Find(View);
			check(PerViewUniformBuffers.IsValidIndex(ViewIndex) && PerViewUniformBuffers[ViewIndex]);
			VertexFactory.SetSpriteViewUniformBuffer(PerViewUniformBuffers[ViewIndex]);

			DrawRichMesh(
				PDI, 
				MeshBatch, 
				FLinearColor(1.0f, 0.0f, 0.0f),	//WireframeColor,
				FLinearColor(1.0f, 1.0f, 0.0f),	//LevelColor,
				FLinearColor(1.0f, 1.0f, 1.0f),	//PropertyColor,		
				this,
				GIsEditor && (View->Family->EngineShowFlags.Selection) ? IsSelected() : false,
				bIsWireframe
				);
		}

		/*
		for(int32 i=0; i<DynamicData->VertexData.Num(); i++)
		{
			FParticleSpriteVertex& Vertex = DynamicData->VertexData[i];
			PDI->DrawPoint(Vertex.Position, Vertex.Color, 3.f, SDPG_World);
		}
		*/
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)  OVERRIDE
	{
		FPrimitiveViewRelevance Result;
		bool bHasDynamicData = DynamicData && DynamicData->VertexData.Num() > 0;

		Result.bDrawRelevance = bHasDynamicData && IsShown(View) && View->Family->EngineShowFlags.Particles;
		Result.bShadowRelevance = bHasDynamicData && IsShadowCast(View);
		Result.bDynamicRelevance = bHasDynamicData;
		Result.bNeedsPreRenderView = Result.bDrawRelevance || Result.bShadowRelevance;
		if (bHasDynamicData && View->Family->EngineShowFlags.Bounds)
		{
			Result.bOpaqueRelevance = true;
		}
		MaterialRelevance.SetPrimitiveViewRelevance(Result);

		return Result;
	}

	virtual bool CanBeOccluded() const OVERRIDE
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	virtual uint32 GetMemoryFootprint() const OVERRIDE 
	{ 
		return (sizeof(*this) + GetAllocatedSize()); 
	}

	uint32 GetAllocatedSize() const 
	{ 
		uint32 DynamicDataSize = 0;
		if (DynamicData)
		{
			DynamicDataSize = sizeof(FNiagaraDynamicData) + DynamicData->VertexData.GetAllocatedSize();
		}
		return FPrimitiveSceneProxy::GetAllocatedSize() + DynamicDataSize;
	}

private:
	UMaterialInterface* Material;
	FNiagaraDynamicData* DynamicData;
	TUniformBuffer<FPrimitiveUniformShaderParameters> WorldSpacePrimitiveUniformBuffer;
	FParticleSpriteVertexFactory VertexFactory;
	FParticleSpriteUniformBufferRef SpriteUniformBuffer;
	TArray<FParticleSpriteViewUniformBufferRef, TInlineAllocator<2> > PerViewUniformBuffers;
	FGlobalDynamicVertexBuffer::FAllocation DynamicVertexAllocation;
	FMaterialRelevance MaterialRelevance;
};

namespace ENiagaraAttr
{
	enum Type
	{
		PositionX,
		PositionY,
		PositionZ,
		VelocityX,
		VelocityY,
		VelocityZ,
		ColorR,
		ColorG,
		ColorB,
		ColorA,
		Rotation,
		RelativeTime,
		MaxAttributes
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
		, ConstantTable(InComponent->UpdateScript->ConstantTable)
		, BufferIndex(0)
		, NumVectorsPerAttribute(0)
		, NumParticles(0)
		, SpawnRemainder(0.0f)
		, CachedBounds(ForceInit)
	{
		check(InComponent->UpdateScript && UpdateScript.ByteCode.Num());
		while (ConstantTable.Num() < 2)
		{
			ConstantTable.Add(0);
		}
	}

	void Tick(float DeltaSeconds)
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraTick);

		const int32 NumAttributes = UpdateScript.Attributes.Num();

		// Cache the ComponentToWorld transform.
		CachedComponentToWorld = Component.GetComponentToWorld();
		
		// Flip buffers.
		BufferIndex ^= 0x1;

		// Get prev + next data.
		TArray<FVector4>& PrevParticles = ParticleBuffers[BufferIndex ^ 0x1];
		TArray<FVector4>& Particles = ParticleBuffers[BufferIndex];

		// Figure out how many we will spawn.
		int32 NumToSpawn = CalcNumToSpawn(DeltaSeconds);

		// Remember the stride of the original data.
		int32 PrevNumVectorsPerAttribute = NumVectorsPerAttribute;

		// The script updates relative time so we don't know yet which will die
		// without simulation. Allocate for the worst case.
		int32 MaxNewParticles = NumParticles + NumToSpawn;
		NumVectorsPerAttribute = ((MaxNewParticles + 0x3) & ~0x3) >> 2;
		Particles.Reset(NumAttributes * NumVectorsPerAttribute);
		Particles.AddUninitialized(NumAttributes * NumVectorsPerAttribute);

		// Simualte particles forward by DeltaSeconds.
		{
			SCOPE_CYCLE_COUNTER(STAT_NiagaraSimulate);
			UpdateParticles(
				DeltaSeconds,
				PrevParticles.GetTypedData(),
				PrevNumVectorsPerAttribute,
				Particles.GetTypedData(),
				NumVectorsPerAttribute,
				NumParticles
				);
		}

		// Spawn new particles overwriting dead particles and compact the final buffer.
		{
			SCOPE_CYCLE_COUNTER(STAT_NiagaraSpawnAndKill);
			NumParticles = SpawnAndKillParticles(
				Particles.GetTypedData(),
				NumParticles,
				NumToSpawn,
				NumVectorsPerAttribute
				);
		}

		DECLARE_DWORD_COUNTER_STAT(TEXT("NumParticles"),STAT_NiagaraNumParticles,STATGROUP_Niagara);
		INC_DWORD_STAT_BY(STAT_NiagaraNumParticles, NumParticles);
	}

	FNiagaraDynamicData* GetDynamicData()
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraGenerateVertices);
		FNiagaraDynamicData* DynamicData = new FNiagaraDynamicData;
		GenerateVertexData(DynamicData->VertexData);
		return DynamicData;
	}

	FBox GetBounds() const { return CachedBounds; }

private:
	/** Temporary stuff for the prototype. */
	float SpawnRate;

	/** The component. */
	const UNiagaraComponent& Component;
	/** The particle update script. */
	UNiagaraScript& UpdateScript;
	/** Local constant table. */
	TArray<float> ConstantTable;
	/** 16-byte aligned set of attribute data used by vector VM for simulation */
	TArray<FVector4> ParticleBuffers[2];
	/** Current buffer. */
	int32 BufferIndex;
	/** Stride between attributes for the current particle buffer. In sizeof(FVector4). */
	int32 NumVectorsPerAttribute;
	/** Total number of alive particles */
	int32 NumParticles;
	/** Keep partial particle spawns from last frame */
	float SpawnRemainder;
	/** The cached ComponentToWorld transform. */
	FTransform CachedComponentToWorld;
	/** Cached bounds. */
	FBox CachedBounds;

	/** Calc number to spawn */
	int32 CalcNumToSpawn(float DeltaSeconds)
	{
		float FloatNumToSpawn = SpawnRemainder + (DeltaSeconds * SpawnRate);
		int32 NumToSpawn = FMath::Floor(FloatNumToSpawn);
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
		VectorRegister* InputRegisters[VectorVM::MaxInputRegisters] = {0};
		VectorRegister* OutputRegisters[VectorVM::MaxOutputRegisters] = {0};
		const int32 NumAttr = UpdateScript.Attributes.Num();
		const int32 NumVectors = ((NumParticles + 0x3) & ~0x3) >> 2;

		check(NumAttr < VectorVM::MaxInputRegisters);
		check(NumAttr < VectorVM::MaxOutputRegisters);

		// Setup input and output registers.
		for (int32 AttrIndex = 0; AttrIndex < NumAttr; ++AttrIndex)
		{
			InputRegisters[AttrIndex] = (VectorRegister*)(PrevParticles + AttrIndex * PrevNumVectorsPerAttribute);
			OutputRegisters[AttrIndex] = (VectorRegister*)(Particles + AttrIndex * NumVectorsPerAttribute);
		}

		// Setup constant table.
		ConstantTable[0] = 0.0f;
		ConstantTable[1] = DeltaSeconds;

		VectorVM::Exec(
			UpdateScript.ByteCode.GetData(),
			InputRegisters,
			NumAttr,
			OutputRegisters,
			NumAttr,
			ConstantTable.GetData(),
			NumVectors
			);
	}

	/** Init new particles, and write over dead ones */
	int32 SpawnAndKillParticles(
		FVector4* Particles,
		int32 NumParticles, 
		int32 NumToSpawn, 
		int32 NumVectorsPerAttribute
		)
	{
		// Iterate over looking for dead particles and spawning new ones
		int32 ParticleIndex = 0;
		float* ParticleRelativeTimes = (float*)(Particles + ENiagaraAttr::RelativeTime * NumVectorsPerAttribute);
		while (NumToSpawn > 0 && ParticleIndex < NumParticles)
		{
			if (ParticleRelativeTimes[ParticleIndex] > 1.0f)
			{
				// Particle is dead, spawn a new one.
				SpawnParticleAtIndex(Particles, NumVectorsPerAttribute, ParticleIndex);
				NumToSpawn--;
			}
			ParticleIndex++;
		}

		// Spawn any remaining particles.
		while (NumToSpawn > 0)
		{
			SpawnParticleAtIndex(Particles, NumVectorsPerAttribute, ParticleIndex++);
			NumToSpawn--;
			NumParticles++;
		}

		// Compact remaining particles.
		while (ParticleIndex < NumParticles)
		{
			if (ParticleRelativeTimes[ParticleIndex] > 1.0f)
			{
				MoveParticleToIndex(Particles, NumVectorsPerAttribute, --NumParticles, ParticleIndex);
			}
			else
			{
				ParticleIndex++;
			}
		}

		return NumParticles;
	}

	/** Spawn a new particle at this index */
	void SpawnParticleAtIndex(FVector4* Particles, int32 NumVectorsPerAttribute, int32 ParticleIndex)
	{
		FVector SpawnLocation = CachedComponentToWorld.GetLocation();
		SpawnLocation.X += FMath::FRandRange(-50.f, 50.f);

		float* Attr = (float*)Particles + ParticleIndex;
		int32 NumFloatsPerAttribute = NumVectorsPerAttribute * 4;

		*Attr = SpawnLocation.X; Attr += NumFloatsPerAttribute;
		*Attr = SpawnLocation.Y; Attr += NumFloatsPerAttribute;
		*Attr = SpawnLocation.Z; Attr += NumFloatsPerAttribute;
		*Attr = 0.0f; Attr += NumFloatsPerAttribute;
		*Attr = 0.0f; Attr += NumFloatsPerAttribute;
		*Attr = 10.0f; Attr += NumFloatsPerAttribute;
		*Attr = 1.0f; Attr += NumFloatsPerAttribute;
		*Attr = 1.0f; Attr += NumFloatsPerAttribute;
		*Attr = 1.0f; Attr += NumFloatsPerAttribute;
		*Attr = 1.0f; Attr += NumFloatsPerAttribute;
		*Attr = 0.0f; Attr += NumFloatsPerAttribute;
		*Attr = 0.0f;
	}

	/** Util to move a particle */
	void MoveParticleToIndex(FVector4* Particles, int32 NumVectorsPerAttribute, int32 SrcIndex, int32 DestIndex)
	{
		float* SrcPtr = (float*)Particles + SrcIndex;
		float* DestPtr = (float*)Particles + DestIndex;
		int32 NumFloatsPerAttribute = NumVectorsPerAttribute * 4;

		for (int32 AttrIndex = 0; AttrIndex < ENiagaraAttr::MaxAttributes; AttrIndex++)
		{
			*DestPtr = *SrcPtr;
			DestPtr += NumFloatsPerAttribute;
			SrcPtr += NumFloatsPerAttribute;
		}
	}

	/** Update render data buffer from attributes */
	void GenerateVertexData(TArray<FParticleSpriteVertex>& RenderData)
	{
		RenderData.Reset(NumParticles);
		CachedBounds.Init();

		float MaxSize = 0.0f;
		float* Particles = (float*)ParticleBuffers[BufferIndex].GetTypedData();
		int32 AttrStride = NumVectorsPerAttribute * 4;
		for (int32 ParticleIndex = 0; ParticleIndex < NumParticles; ParticleIndex++)
		{
			float* Particle = Particles + ParticleIndex;
		
			FParticleSpriteVertex& NewVertex = *new(RenderData) FParticleSpriteVertex;
			NewVertex.Position = FVector(Particle[AttrStride*ENiagaraAttr::PositionX],Particle[AttrStride*ENiagaraAttr::PositionY],Particle[AttrStride*ENiagaraAttr::PositionZ]);
			//CachedBounds += NewVertex.Position;
			NewVertex.Color = FLinearColor(Particle[AttrStride*ENiagaraAttr::ColorR],Particle[AttrStride*ENiagaraAttr::ColorG],Particle[AttrStride*ENiagaraAttr::ColorB],Particle[AttrStride*ENiagaraAttr::ColorA]);
			NewVertex.OldPosition = NewVertex.Position;
			NewVertex.ParticleId = 0.f;
			NewVertex.RelativeTime = Particle[AttrStride*ENiagaraAttr::RelativeTime];
			NewVertex.Size = FVector2D(20.0f,20.0f);
			//MaxSize = FMath::Max(MaxSize,NewVertex.Size.X);
			//MaxSize = FMath::Max(MaxSize,NewVertex.Size.Y);
			NewVertex.Rotation = Particle[AttrStride*ENiagaraAttr::Rotation];
			NewVertex.SubImageIndex = 0.f;
		}
		//CachedBounds.ExpandBy(MaxSize);
	}
};

//////////////////////////////////////////////////////////////////////////

UNiagaraComponent::UNiagaraComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;

	SpawnRate = 20.f;
}


void UNiagaraComponent::TickComponent(float DeltaSeconds, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	if (Simulation)
	{
		Simulation->Tick(DeltaSeconds);
		UpdateComponentToWorld();
		MarkRenderDynamicDataDirty();
	}
}

void UNiagaraComponent::OnRegister()
{
	Super::OnRegister();

	ensure(Simulation == NULL);
	if (UpdateScript && UpdateScript->ByteCode.Num() && UpdateScript->Attributes.Num() == ENiagaraAttr::MaxAttributes)
	{
		Simulation = new FNiagaraSimulation(this);
	}
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
		FNiagaraDynamicData* DynamicData = Simulation->GetDynamicData();

		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FSendNiagaraDynamicData,
			FNiagaraSceneProxy*,NiagaraSceneProxy,(FNiagaraSceneProxy*)SceneProxy,
			FNiagaraDynamicData*,DynamicData,DynamicData,
		{
			NiagaraSceneProxy->SetDynamicData_RenderThread(DynamicData);
		});
	}
}

int32 UNiagaraComponent::GetNumMaterials() const
{
	return 1;
}

UMaterialInterface* UNiagaraComponent::GetMaterial(int32 ElementIndex) const
{
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
