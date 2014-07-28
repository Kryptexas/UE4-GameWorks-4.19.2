// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Components/NiagaraComponent.h"
#include "Engine/NiagaraScript.h"
#include "VectorVM.h"
#include "ParticleHelper.h"
#include "Particles/ParticleResources.h"
#include "Engine/NiagaraConstants.h"

DECLARE_CYCLE_STAT(TEXT("Tick"),STAT_NiagaraTick,STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Simulate"),STAT_NiagaraSimulate,STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Spawn + Kill"),STAT_NiagaraSpawnAndKill,STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Gen Verts"),STAT_NiagaraGenerateVertices,STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("PreRenderView"),STAT_NiagaraPreRenderView,STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Render"),STAT_NiagaraRender,STATGROUP_Niagara);
DECLARE_DWORD_COUNTER_STAT(TEXT("NumParticles"),STAT_NiagaraNumParticles,STATGROUP_Niagara);


DEFINE_LOG_CATEGORY_STATIC(LogNiagaraComponent, All, All);

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
		, VertexFactory(PVFT_Sprite, InComponent->GetWorld()->FeatureLevel)
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
		PerViewUniformBuffers.Empty();
		WorldSpacePrimitiveUniformBuffer.ReleaseResource();
	}

	// FPrimitiveSceneProxy interface.
	virtual void CreateRenderThreadResources() override
	{
		VertexFactory.InitResource();

		UniformParameters.AxisLockRight = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
		UniformParameters.AxisLockUp = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
		UniformParameters.RotationScale = 1.0f;
		UniformParameters.RotationBias = 0.0f;
		UniformParameters.TangentSelector = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
		UniformParameters.InvDeltaSeconds = 0.0f;
		UniformParameters.SubImageSize = FVector4(1.0f,1.0f,1.0f,1.0f);
		UniformParameters.NormalsType = 0;
		UniformParameters.NormalsSphereCenter = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
		UniformParameters.NormalsCylinderUnitDirection = FVector4(0.0f, 0.0f, 1.0f, 0.0f);
		UniformParameters.PivotOffset = FVector2D(0.0f, 0.0f);
	}

	virtual void OnActorPositionChanged() override
	{
		WorldSpacePrimitiveUniformBuffer.ReleaseResource();
	}

	virtual void OnTransformChanged() override
	{
		WorldSpacePrimitiveUniformBuffer.ReleaseResource();
	}

	virtual void PreRenderView(const FSceneViewFamily* ViewFamily, const uint32 VisibilityMap, int32 FrameNumber) override
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
			VertexFactory.SetDynamicParameterBuffer(NULL, 0, 0, true);
			// Compute the per-view uniform buffers.
			const int32 NumViews = ViewFamily->Views.Num();
			uint32 ViewBit = 1;
			for (int32 ViewIndex = 0; ViewIndex < NumViews; ++ViewIndex, ViewBit <<= 1)
			{
				FParticleSpriteUniformBufferRef* SpriteViewUniformBufferPtr = new(PerViewUniformBuffers) FParticleSpriteUniformBufferRef();
				if (VisibilityMap & ViewBit)
				{
					FParticleSpriteUniformParameters PerViewUniformParameters = UniformParameters;
					PerViewUniformParameters.MacroUVParameters = FVector4(0.0f, 0.0f, 1.0f, 1.0f);
					*SpriteViewUniformBufferPtr = FParticleSpriteUniformBufferRef::CreateUniformBufferImmediate(PerViewUniformParameters, UniformBuffer_SingleFrame);
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
					ReceivesDecals(),
					false
					);
				WorldSpacePrimitiveUniformBuffer.SetContents(PrimitiveUniformShaderParameters);
				WorldSpacePrimitiveUniformBuffer.InitResource();
			}
		}
	}
		  
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View) override
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraRender);

		check(DynamicData && DynamicData->VertexData.Num());

		const bool bIsWireframe = View->Family->EngineShowFlags.Wireframe;
		FMaterialRenderProxy* MaterialRenderProxy = Material->GetRenderProxy(IsSelected(),IsHovered());

		if (DynamicVertexAllocation.IsValid()
			&& (bIsWireframe || !PDI->IsMaterialIgnored(MaterialRenderProxy, View->GetFeatureLevel())))
		{
			SCOPED_DRAW_EVENT(NiagaraRender, DEC_SCENE_ITEMS);

			FMeshBatch MeshBatch;
			MeshBatch.VertexFactory = &VertexFactory;
			MeshBatch.CastShadow = CastsDynamicShadow();
			MeshBatch.bUseAsOccluder = false;
			MeshBatch.ReverseCulling = IsLocalToWorldDeterminantNegative();
			MeshBatch.Type = PT_TriangleList;
			MeshBatch.DepthPriorityGroup = GetDepthPriorityGroup(View);
			MeshBatch.LCI = NULL;
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
			VertexFactory.SetSpriteUniformBuffer(PerViewUniformBuffers[ViewIndex]);

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
			PDI->DrawPoint(Vertex.Position, Vertex.Color, 1.0f, SDPG_World);
		}
		*/
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)  override
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

	virtual bool CanBeOccluded() const override
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	virtual uint32 GetMemoryFootprint() const override 
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
	FParticleSpriteUniformParameters UniformParameters;
	TArray<FParticleSpriteUniformBufferRef, TInlineAllocator<2> > PerViewUniformBuffers;
	FGlobalDynamicVertexBuffer::FAllocation DynamicVertexAllocation;
	FMaterialRelevance MaterialRelevance;
};

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
 */n
class FNiagaraSimulation
{
public:
	explicit FNiagaraSimulation(const UNiagaraComponent* InComponent)
		: SpawnRate(InComponent->SpawnRate)
		, Component(*InComponent)
		, UpdateScript(*InComponent->UpdateScript)
		, SpawnScript(InComponent->SpawnScript)
		, ConstantTable(InComponent->UpdateScript->ConstantTable)
		, BufferIndex(0)
		, NumVectorsPerAttribute(0)
		, NumParticles(0)
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
		NumVectorsPerAttribute = MaxNewParticles;
		Particles.Reset(NumAttributes * NumVectorsPerAttribute);
		Particles.AddUninitialized(NumAttributes * NumVectorsPerAttribute);

		// Simulate particles forward by DeltaSeconds.
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

	void AddConstant(FVector4 Value)
	{
		ConstantTable.Add(Value);
	}

	void SetConstant(int Idx, FVector4 Value)
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


	/* builtin constants are setup in the tick function, but a script may have additional constants 
	 derived from unconnected nodes, so we need to merge those into the simulation constant table */
	void MergeScriptConstants(UNiagaraScript *Script)
	{
		// start at NumBuiltinConstants+1, because index 0 is always the fixed zero constant
		for (int i = NiagaraConstants::NumBuiltinConstants + 1; i < Script->ConstantTable.Num(); i++)
		{
			if (ConstantTable.Num() <= i)
				ConstantTable.Add(FVector4());
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
		VectorRegister* InputRegisters[VectorVM::MaxInputRegisters] = {0};
		VectorRegister* OutputRegisters[VectorVM::MaxOutputRegisters] = {0};
		const int32 NumAttr = UpdateScript.Attributes.Num();
		const int32 NumVectors = NumParticles;

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
		int32 OrigNumParticles = NumParticles;
		FVector4 *NewParticles = Particles + NumParticles;

		// Spawn new Particles at the end of the buffer
		for (int32 i = 0; i < NumToSpawn; i++)
		{
			SpawnParticleAtIndex(NewParticles, NumVectorsPerAttribute, i);
			NumParticles++;
		}

		// run the spawn graph over all new particles
		if (SpawnScript && SpawnScript->ByteCode.Num())
		{
			VectorRegister* InputRegisters[VectorVM::MaxInputRegisters] = { 0 };
			VectorRegister* OutputRegisters[VectorVM::MaxOutputRegisters] = { 0 };
			const int32 NumAttr = SpawnScript->Attributes.Num();
			const int32 NumVectors = NumToSpawn;// ((NumParticles + 0x3) & ~0x3);

			check(NumAttr < VectorVM::MaxInputRegisters);
			check(NumAttr < VectorVM::MaxOutputRegisters);

			// Setup input and output registers.
			for (int32 AttrIndex = 0; AttrIndex < NumAttr; ++AttrIndex)
			{
				InputRegisters[AttrIndex] = (VectorRegister*)(NewParticles + AttrIndex * NumVectorsPerAttribute);
				OutputRegisters[AttrIndex] = (VectorRegister*)(NewParticles + AttrIndex * NumVectorsPerAttribute);
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
		FVector4* ParticleRelativeTimes = (Particles + ENiagaraVectorAttr::RelativeTime * NumVectorsPerAttribute);
		while (ParticleIndex < OrigNumParticles)
		{
			if (ParticleRelativeTimes[ParticleIndex].X > 1.0f)
			{
				// Particle is dead, move one from the end here.
				MoveParticleToIndex(Particles, NumVectorsPerAttribute, --NumParticles, ParticleIndex);
			}
			ParticleIndex++;
		}

		return NumParticles;
	}

	/** Spawn a new particle at this index */
	void SpawnParticleAtIndex(FVector4* Particles, int32 NumVectorsPerAttribute, int32 ParticleIndex)
	{
		FVector SpawnLocation = CachedComponentToWorld.GetLocation();
		SpawnLocation.X += FMath::FRandRange(-20.f, 20.f);
		SpawnLocation.Y += FMath::FRandRange(-20.f, 20.f);
		SpawnLocation.Z += FMath::FRandRange(-20.f, 20.f);

		FVector4* Attr = Particles + ParticleIndex;
		Attr[NumVectorsPerAttribute*ENiagaraVectorAttr::Position] = SpawnLocation;
		Attr[NumVectorsPerAttribute*ENiagaraVectorAttr::Color] = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
		Attr[NumVectorsPerAttribute*ENiagaraVectorAttr::RelativeTime] = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
		Attr[NumVectorsPerAttribute*ENiagaraVectorAttr::Rotation] = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
		Attr[NumVectorsPerAttribute*ENiagaraVectorAttr::Velocity] = FVector4(0.0f, 0.0f, 2.0f, 0.0f);
	}

	/** Util to move a particle */
	void MoveParticleToIndex(FVector4* Particles, int32 NumVectorsPerAttribute, int32 SrcIndex, int32 DestIndex)
	{
		FVector4 *SrcPtr = Particles + SrcIndex;
		FVector4 *DestPtr = Particles + DestIndex;

		for (int32 AttrIndex = 0; AttrIndex < ENiagaraVectorAttr::MaxVectorAttribs; AttrIndex++)
		{
			*DestPtr = *SrcPtr;
			DestPtr += NumVectorsPerAttribute;
			SrcPtr += NumVectorsPerAttribute;
		}
	}

	/** Update render data buffer from attributes */
	void GenerateVertexData(TArray<FParticleSpriteVertex>& RenderData)
	{
		RenderData.Reset(NumParticles);
		CachedBounds.Init();


		float MaxSize = 0.0f;
		FVector4* Particles = ParticleBuffers[BufferIndex].GetTypedData();
		int32 AttrStride = NumVectorsPerAttribute;
		for (int32 ParticleIndex = 0; ParticleIndex < NumParticles; ParticleIndex++)
		{
			FVector4* Particle = Particles + ParticleIndex;

			FParticleSpriteVertex& NewVertex = *new(RenderData)FParticleSpriteVertex;
			NewVertex.Position = Particle[AttrStride*ENiagaraVectorAttr::Position];
			NewVertex.OldPosition = NewVertex.Position;
			CachedBounds += NewVertex.Position;
			NewVertex.Color = FLinearColor(Particle[AttrStride*ENiagaraVectorAttr::Color]);
			NewVertex.ParticleId = static_cast<float>(ParticleIndex) / NumParticles;
			NewVertex.RelativeTime = Particle[AttrStride*ENiagaraVectorAttr::RelativeTime].X;
			NewVertex.Size = FVector2D(1.0f, 1.0f);
			MaxSize = FMath::Max(MaxSize, NewVertex.Size.X);
			MaxSize = FMath::Max(MaxSize, NewVertex.Size.Y);
			NewVertex.Rotation = Particle[AttrStride*ENiagaraVectorAttr::Rotation].X;
			NewVertex.SubImageIndex = 0.f;
		}
		CachedBounds.ExpandBy(MaxSize);
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
