// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEffectRenderer.h"
#include "Particles/ParticleResources.h"
#include "ParticleBeamTrailVertexFactory.h"
#include "NiagaraDataSet.h"
#include "NiagaraStats.h"

DECLARE_CYCLE_STAT(TEXT("Generate Sprite Vertex Data"), STAT_NiagaraGenSpriteVertexData, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Generate Ribbon Vertex Data"), STAT_NiagaraGenRibbonVertexData, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Generate Mesh Vertex Data"), STAT_NiagaraGenMeshVertexData, STATGROUP_Niagara);

DECLARE_CYCLE_STAT(TEXT("Render Total"), STAT_NiagaraRender, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Render Sprites"), STAT_NiagaraRenderSprites, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Render Ribbons"), STAT_NiagaraRenderRibbons, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Render Meshes"), STAT_NiagaraRenderMeshes, STATGROUP_Niagara);

struct FNiagaraDynamicDataSprites : public FNiagaraDynamicDataBase
{
	TArray<FParticleSpriteVertex> VertexData;
	TArray<FParticleVertexDynamicParameter> MaterialParameterVertexData;
};

struct FNiagaraDynamicDataRibbon : public FNiagaraDynamicDataBase
{
	TArray<FParticleBeamTrailVertex> VertexData;
	TArray<FParticleBeamTrailVertexDynamicParameter> MaterialParameterVertexData;
};

struct FNiagaraDynamicDataMesh : public FNiagaraDynamicDataBase
{
	TArray<FMeshParticleInstanceVertex> VertexData;
	TArray<FMeshParticleInstanceVertexDynamicParameter> MaterialParameterVertexData;
};


/* Mesh collector classes */
class FNiagaraMeshCollectorResourcesSprite : public FOneFrameResource
{
public:
	FParticleSpriteVertexFactory VertexFactory;
	FParticleSpriteUniformBufferRef UniformBuffer;

	virtual ~FNiagaraMeshCollectorResourcesSprite()
	{
		VertexFactory.ReleaseResource();
	}
};


class FNiagaraMeshCollectorResourcesRibbon : public FOneFrameResource
{
public:
	FParticleBeamTrailVertexFactory VertexFactory;
	FParticleBeamTrailUniformBufferRef UniformBuffer;

	virtual ~FNiagaraMeshCollectorResourcesRibbon()
	{
		VertexFactory.ReleaseResource();
	}
};


class FNiagaraMeshCollectorResourcesMesh : public FOneFrameResource
{
public:
	FMeshParticleVertexFactory VertexFactory;
	FMeshParticleUniformBufferRef UniformBuffer;

	virtual ~FNiagaraMeshCollectorResourcesMesh()
	{
		VertexFactory.ReleaseResource();
	}
};




NiagaraEffectRendererSprites::NiagaraEffectRendererSprites(ERHIFeatureLevel::Type FeatureLevel, UNiagaraEffectRendererProperties *InProps) :
	NiagaraEffectRenderer(),
	DynamicDataRender(NULL)
{
	//check(InProps);
	VertexFactory = new FParticleSpriteVertexFactory(PVFT_Sprite, FeatureLevel);
	Properties = Cast<UNiagaraSpriteRendererProperties>(InProps);
}


void NiagaraEffectRendererSprites::ReleaseRenderThreadResources()
{
	VertexFactory->ReleaseResource();
	WorldSpacePrimitiveUniformBuffer.ReleaseResource();
}

void NiagaraEffectRendererSprites::CreateRenderThreadResources() 
{
	VertexFactory->SetNumVertsInInstanceBuffer(4);
	VertexFactory->InitResource();
}

void NiagaraEffectRendererSprites::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector, const FNiagaraSceneProxy *SceneProxy) const
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraRender);
	SCOPE_CYCLE_COUNTER(STAT_NiagaraRenderSprites);

	SimpleTimer MeshElementsTimer;

	//check(DynamicDataRender)
		
	if (!DynamicDataRender || DynamicDataRender->VertexData.Num() == 0)
	{
		return;
	}

	const bool bIsWireframe = ViewFamily.EngineShowFlags.Wireframe;
	FMaterialRenderProxy* MaterialRenderProxy = Material->GetRenderProxy(SceneProxy->IsSelected(), SceneProxy->IsHovered());

	int32 SizeInBytes = DynamicDataRender->VertexData.GetTypeSize() * DynamicDataRender->VertexData.Num();
	FGlobalDynamicVertexBuffer::FAllocation LocalDynamicVertexAllocation = FGlobalDynamicVertexBuffer::Get().Allocate(SizeInBytes);
	FGlobalDynamicVertexBuffer::FAllocation LocalDynamicVertexMaterialParamsAllocation;

	if (DynamicDataRender->MaterialParameterVertexData.Num() > 0)
	{
		int32 MatParamSizeInBytes = DynamicDataRender->MaterialParameterVertexData.GetTypeSize() * DynamicDataRender->MaterialParameterVertexData.Num();
		LocalDynamicVertexMaterialParamsAllocation = FGlobalDynamicVertexBuffer::Get().Allocate(MatParamSizeInBytes);

		if (LocalDynamicVertexMaterialParamsAllocation.IsValid())
		{
			// Copy the extra material vertex data over.
			FMemory::Memcpy(LocalDynamicVertexMaterialParamsAllocation.Buffer, DynamicDataRender->MaterialParameterVertexData.GetData(), MatParamSizeInBytes);
		}
	}

	if (LocalDynamicVertexAllocation.IsValid())
	{
		// Update the primitive uniform buffer if needed.
		if (!WorldSpacePrimitiveUniformBuffer.IsInitialized())
		{
			FPrimitiveUniformShaderParameters PrimitiveUniformShaderParameters = GetPrimitiveUniformShaderParameters(
				FMatrix::Identity,
				SceneProxy->GetActorPosition(),
				SceneProxy->GetBounds(),
				SceneProxy->GetLocalBounds(),
				SceneProxy->ReceivesDecals(),
				false,
				false,
				SceneProxy->UseSingleSampleShadowFromStationaryLights(),
				SceneProxy->UseEditorDepthTest(),
				SceneProxy->GetLightingChannelMask()
				);
			WorldSpacePrimitiveUniformBuffer.SetContents(PrimitiveUniformShaderParameters);
			WorldSpacePrimitiveUniformBuffer.InitResource();
		}

		// Copy the vertex data over.
		FMemory::Memcpy(LocalDynamicVertexAllocation.Buffer, DynamicDataRender->VertexData.GetData(), SizeInBytes);

		// Compute the per-view uniform buffers.
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];

				FNiagaraMeshCollectorResourcesSprite& CollectorResources = Collector.AllocateOneFrameResource<FNiagaraMeshCollectorResourcesSprite>();
				FParticleSpriteUniformParameters PerViewUniformParameters;// = UniformParameters;
				PerViewUniformParameters.AxisLockRight = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
				PerViewUniformParameters.AxisLockUp = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
				PerViewUniformParameters.RotationBias = 0.0f;
				PerViewUniformParameters.RotationScale = 1.0f;
				PerViewUniformParameters.TangentSelector = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
				PerViewUniformParameters.InvDeltaSeconds = 30.0f;
				PerViewUniformParameters.NormalsType = 0.0f;
				PerViewUniformParameters.NormalsSphereCenter = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
				PerViewUniformParameters.NormalsCylinderUnitDirection = FVector4(0.0f, 0.0f, 1.0f, 0.0f);
				PerViewUniformParameters.PivotOffset = FVector2D(-0.5f, -0.5f);
				PerViewUniformParameters.MacroUVParameters = FVector4(0.0f, 0.0f, 1.0f, 1.0f);

				if (Properties)
				{
					PerViewUniformParameters.SubImageSize = FVector4(Properties->SubImageInfo.X, Properties->SubImageInfo.Y, 1.0f / Properties->SubImageInfo.X, 1.0f / Properties->SubImageInfo.Y);

					if (Properties->bBVelocityAligned)
					{
						// velocity aligned
						PerViewUniformParameters.RotationScale = 0.0f;
						PerViewUniformParameters.TangentSelector = FVector4(0.0f, 1.0f, 0.0f, 0.0f);
					}
				}

				// Collector.AllocateOneFrameResource uses default ctor, initialize the vertex factory
				CollectorResources.VertexFactory.SetFeatureLevel(ViewFamily.GetFeatureLevel());
				CollectorResources.VertexFactory.SetParticleFactoryType(PVFT_Sprite);

				CollectorResources.UniformBuffer = FParticleSpriteUniformBufferRef::CreateUniformBufferImmediate(PerViewUniformParameters, UniformBuffer_SingleFrame);

				CollectorResources.VertexFactory.SetNumVertsInInstanceBuffer(4);
				CollectorResources.VertexFactory.InitResource();
				CollectorResources.VertexFactory.SetSpriteUniformBuffer(CollectorResources.UniformBuffer);
				CollectorResources.VertexFactory.SetInstanceBuffer(
					LocalDynamicVertexAllocation.VertexBuffer,
					LocalDynamicVertexAllocation.VertexOffset,
					sizeof(FParticleSpriteVertex),
					true
					);

				if (DynamicDataRender->MaterialParameterVertexData.Num() > 0 && LocalDynamicVertexMaterialParamsAllocation.IsValid())
				{
					CollectorResources.VertexFactory.SetDynamicParameterBuffer(
						LocalDynamicVertexMaterialParamsAllocation.VertexBuffer, 
						LocalDynamicVertexMaterialParamsAllocation.VertexOffset, 
						sizeof(FParticleVertexDynamicParameter),
						true);
				}
				else
				{
					CollectorResources.VertexFactory.SetDynamicParameterBuffer(NULL, 0, 0, true);
				}

				FMeshBatch& MeshBatch = Collector.AllocateMesh();
				MeshBatch.VertexFactory = &CollectorResources.VertexFactory;
				MeshBatch.CastShadow = SceneProxy->CastsDynamicShadow();
				MeshBatch.bUseAsOccluder = false;
				MeshBatch.ReverseCulling = SceneProxy->IsLocalToWorldDeterminantNegative();
				MeshBatch.Type = PT_TriangleList;
				MeshBatch.DepthPriorityGroup = SceneProxy->GetDepthPriorityGroup(View);
				MeshBatch.bCanApplyViewModeOverrides = true;
				MeshBatch.bUseWireframeSelectionColoring = SceneProxy->IsSelected();

				if (bIsWireframe)
				{
					MeshBatch.MaterialRenderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(SceneProxy->IsSelected(), SceneProxy->IsHovered());
				}
				else
				{
					MeshBatch.MaterialRenderProxy = MaterialRenderProxy;
				}

				FMeshBatchElement& MeshElement = MeshBatch.Elements[0];
				MeshElement.IndexBuffer = &GParticleIndexBuffer;
				MeshElement.FirstIndex = 0;
				MeshElement.NumPrimitives = 2;
				MeshElement.NumInstances = DynamicDataRender->VertexData.Num();
				MeshElement.MinVertexIndex = 0;
				MeshElement.MaxVertexIndex = MeshElement.NumInstances * 4 - 1;
				MeshElement.PrimitiveUniformBufferResource = &WorldSpacePrimitiveUniformBuffer;

				Collector.AddMesh(ViewIndex, MeshBatch);
			}
		}
	}

	CPUTimeMS += MeshElementsTimer.GetElapsedMilliseconds();
}

bool NiagaraEffectRendererSprites::SetMaterialUsage()
{
	//Causes deadlock :S Need to look at / rework the setting of materials and render modules.
	return Material && Material->CheckMaterialUsage_Concurrent(MATUSAGE_ParticleSprites);
}

/** Update render data buffer from attributes */
FNiagaraDynamicDataBase *NiagaraEffectRendererSprites::GenerateVertexData(const FNiagaraSceneProxy* Proxy, const FNiagaraDataSet &Data)
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraGenSpriteVertexData);

	SimpleTimer VertexDataTimer;
	FVector MaxSize;

	FNiagaraDynamicDataSprites *DynamicData = new FNiagaraDynamicDataSprites;
	TArray<FParticleSpriteVertex>& RenderData = DynamicData->VertexData;
	TArray< FParticleVertexDynamicParameter>& RenderMaterialVertexData = DynamicData->MaterialParameterVertexData;

	RenderData.Reset(Data.GetNumInstances());
	CachedBounds.Init();

	//I'm not a great fan of pulling scalar components out to a structured vert buffer like this.
	//TODO: Experiment with a new VF that reads the data directly from the scalar layout.
	FNiagaraDataSetIterator<FVector> PosItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Position")));
	FNiagaraDataSetIterator<FVector> VelItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Velocity")));
	FNiagaraDataSetIterator<FLinearColor> ColItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(), TEXT("Color")));
	FNiagaraDataSetIterator<float> AgeItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Age")));
	FNiagaraDataSetIterator<float> RotItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Rotation")));
	FNiagaraDataSetIterator<FVector2D> SizeItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec2Def(), TEXT("Size")));
	FNiagaraDataSetIterator<float> SubImageIndexItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("SubImageIndex")));
	FNiagaraDataSetIterator<FVector4> MaterialParamItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec4Def(), TEXT("DynamicMaterialParameter")));

	//Bail if we don't have the required attributes to render this emitter.
	if (!PosItr.IsValid() || !VelItr.IsValid() || !ColItr.IsValid() || !AgeItr.IsValid() || !RotItr.IsValid() || !SizeItr.IsValid())
	{
		return DynamicData;
	}

	if (MaterialParamItr.IsValid())
	{
		RenderMaterialVertexData.Reset(Data.GetNumInstances());
		RenderMaterialVertexData.AddUninitialized(Data.GetNumInstances());
	}

	FMatrix LocalToWorld = Proxy->GetLocalToWorld();

	uint32 NumSubImages = 1;
	if (Properties)
	{
		NumSubImages = Properties->SubImageInfo.X*Properties->SubImageInfo.Y;
	}
	float ParticleId = 0.0f, IdInc = 1.0f / Data.GetNumInstances();
	RenderData.AddUninitialized(Data.GetNumInstances());
	if (bLocalSpace)
	{
		for (uint32 ParticleIndex = 0; ParticleIndex < Data.GetNumInstances(); ParticleIndex++)
		{
			FParticleSpriteVertex& NewVertex = RenderData[ParticleIndex];
			FVector Pos = *PosItr;
			NewVertex.Position = LocalToWorld.TransformPosition(Pos);
			NewVertex.OldPosition = LocalToWorld.TransformPosition(Pos - *VelItr);

			NewVertex.Color = *ColItr;
			NewVertex.ParticleId = ParticleId;
			ParticleId += IdInc;
			NewVertex.RelativeTime = *AgeItr;
			NewVertex.Size = *SizeItr;
			NewVertex.Rotation = *RotItr;
			if (!SubImageIndexItr.IsValid())
			{
				NewVertex.SubImageIndex = 0;
			}
			else
			{
				NewVertex.SubImageIndex = *SubImageIndexItr * NumSubImages;
				SubImageIndexItr.Advance();
			}


			PosItr.Advance();
			VelItr.Advance();
			ColItr.Advance();
			AgeItr.Advance();
			RotItr.Advance();
			SizeItr.Advance();

			MaxSize = MaxSize.ComponentMax(FVector(NewVertex.Size.GetMax()));
			CachedBounds += NewVertex.Position;
		}
	}
	else
	{
		for (uint32 ParticleIndex = 0; ParticleIndex < Data.GetNumInstances(); ParticleIndex++)
		{
			FParticleSpriteVertex& NewVertex = RenderData[ParticleIndex];
			NewVertex.Position = *PosItr;
			NewVertex.OldPosition = NewVertex.Position - *VelItr;

			NewVertex.Color = *ColItr;
			NewVertex.ParticleId = ParticleId;
			ParticleId += IdInc;
			NewVertex.RelativeTime = *AgeItr;
			NewVertex.Size = *SizeItr;
			NewVertex.Rotation = *RotItr; 
			if (!SubImageIndexItr.IsValid())
			{
				NewVertex.SubImageIndex = 0;
			}
			else
			{
				NewVertex.SubImageIndex = *SubImageIndexItr * NumSubImages;
				SubImageIndexItr.Advance();
			}

			PosItr.Advance();
			VelItr.Advance();
			ColItr.Advance();
			AgeItr.Advance();
			RotItr.Advance();
			SizeItr.Advance();
			
			MaxSize = MaxSize.ComponentMax(FVector(NewVertex.Size.GetMax()));
			CachedBounds += NewVertex.Position;
		}
	}
	CachedBounds.ExpandBy(MaxSize);

	if (MaterialParamItr.IsValid())
	{
		for (uint32 ParticleIndex = 0; ParticleIndex < Data.GetNumInstances(); ParticleIndex++)
		{
			RenderMaterialVertexData[ParticleIndex].DynamicValue[0] = (*MaterialParamItr).X;
			RenderMaterialVertexData[ParticleIndex].DynamicValue[1] = (*MaterialParamItr).Y;
			RenderMaterialVertexData[ParticleIndex].DynamicValue[2] = (*MaterialParamItr).Z;
			RenderMaterialVertexData[ParticleIndex].DynamicValue[3] = (*MaterialParamItr).W;
			MaterialParamItr.Advance();
		}
	}
	CPUTimeMS = VertexDataTimer.GetElapsedMilliseconds();

	return DynamicData;
}



void NiagaraEffectRendererSprites::SetDynamicData_RenderThread(FNiagaraDynamicDataBase* NewDynamicData)
{
	check(IsInRenderingThread());

	if (DynamicDataRender)
	{
		delete DynamicDataRender;
		DynamicDataRender = NULL;
	}
	DynamicDataRender = static_cast<FNiagaraDynamicDataSprites*>(NewDynamicData);
}

int NiagaraEffectRendererSprites::GetDynamicDataSize()
{
	uint32 Size = sizeof(FNiagaraDynamicDataSprites);
	if (DynamicDataRender)
	{
		Size += DynamicDataRender->VertexData.GetAllocatedSize();
	}

	return Size;
}

bool NiagaraEffectRendererSprites::HasDynamicData()
{
	return DynamicDataRender && DynamicDataRender->VertexData.Num() > 0;
}

const TArray<FNiagaraVariable>& NiagaraEffectRendererSprites::GetRequiredAttributes()
{
	static TArray<FNiagaraVariable> Attrs;
	
	if (Attrs.Num() == 0)
	{
		FNiagaraVariable PositionStruct = FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), "Position");
		FNiagaraVariable VelStruct = FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), "Velocity");
		FNiagaraVariable ColorStruct = FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(), "Color");
		FNiagaraVariable RotStruct = FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), "Rotation");
		FNiagaraVariable AgeStruct = FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), "Age");
		FNiagaraVariable SizeStruct = FNiagaraVariable(FNiagaraTypeDefinition::GetVec2Def(), "Size");

		Attrs.Add(PositionStruct);
		Attrs.Add(VelStruct);
		Attrs.Add(ColorStruct);
		Attrs.Add(RotStruct);
		Attrs.Add(AgeStruct);
		Attrs.Add(SizeStruct);
	}

	return Attrs;
}



NiagaraEffectRendererRibbon::NiagaraEffectRendererRibbon(ERHIFeatureLevel::Type FeatureLevel, UNiagaraEffectRendererProperties *InProps) :
NiagaraEffectRenderer(),
DynamicDataRender(NULL)
{
	VertexFactory = new FParticleBeamTrailVertexFactory(PVFT_BeamTrail, FeatureLevel);
	Properties = Cast<UNiagaraRibbonRendererProperties>(InProps);
}


void NiagaraEffectRendererRibbon::ReleaseRenderThreadResources()
{
	VertexFactory->ReleaseResource();
	WorldSpacePrimitiveUniformBuffer.ReleaseResource();
}

// FPrimitiveSceneProxy interface.
void NiagaraEffectRendererRibbon::CreateRenderThreadResources()
{
	VertexFactory->InitResource();
}




void NiagaraEffectRendererRibbon::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector, const FNiagaraSceneProxy *SceneProxy) const
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraRender);
	SCOPE_CYCLE_COUNTER(STAT_NiagaraRenderRibbons);

	SimpleTimer MeshElementsTimer;

	if (!DynamicDataRender || DynamicDataRender->VertexData.Num() == 0)
	{
		return;
	}

	const bool bIsWireframe = ViewFamily.EngineShowFlags.Wireframe;
	FMaterialRenderProxy* MaterialRenderProxy = Material->GetRenderProxy(SceneProxy->IsSelected(), SceneProxy->IsHovered());

	int32 SizeInBytes = DynamicDataRender->VertexData.GetTypeSize() * DynamicDataRender->VertexData.Num();
	FGlobalDynamicVertexBuffer::FAllocation LocalDynamicVertexAllocation = FGlobalDynamicVertexBuffer::Get().Allocate(SizeInBytes);
	FGlobalDynamicVertexBuffer::FAllocation LocalDynamicVertexMaterialParamsAllocation;

	if (DynamicDataRender->MaterialParameterVertexData.Num() > 0)
	{
		int32 MatParamSizeInBytes = DynamicDataRender->MaterialParameterVertexData.GetTypeSize() * DynamicDataRender->MaterialParameterVertexData.Num();
		LocalDynamicVertexMaterialParamsAllocation = FGlobalDynamicVertexBuffer::Get().Allocate(MatParamSizeInBytes);

		if (LocalDynamicVertexMaterialParamsAllocation.IsValid())
		{
			// Copy the extra material vertex data over.
			FMemory::Memcpy(LocalDynamicVertexMaterialParamsAllocation.Buffer, DynamicDataRender->MaterialParameterVertexData.GetData(), MatParamSizeInBytes);
		}
	}


	if (LocalDynamicVertexAllocation.IsValid())
	{
		// Update the primitive uniform buffer if needed.
		if (!WorldSpacePrimitiveUniformBuffer.IsInitialized())
		{
			FPrimitiveUniformShaderParameters PrimitiveUniformShaderParameters = GetPrimitiveUniformShaderParameters(
				FMatrix::Identity,
				SceneProxy->GetActorPosition(),
				SceneProxy->GetBounds(),
				SceneProxy->GetLocalBounds(),
				SceneProxy->ReceivesDecals(),
				false,
				false,
				SceneProxy->UseSingleSampleShadowFromStationaryLights(),
				SceneProxy->UseEditorDepthTest(),
				SceneProxy->GetLightingChannelMask()
				);
			WorldSpacePrimitiveUniformBuffer.SetContents(PrimitiveUniformShaderParameters);
			WorldSpacePrimitiveUniformBuffer.InitResource();
		}

		// Copy the vertex data over.
		FMemory::Memcpy(LocalDynamicVertexAllocation.Buffer, DynamicDataRender->VertexData.GetData(), SizeInBytes);

		// Compute the per-view uniform buffers.
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];

				FNiagaraMeshCollectorResourcesRibbon& CollectorResources = Collector.AllocateOneFrameResource<FNiagaraMeshCollectorResourcesRibbon>();
				FParticleBeamTrailUniformParameters PerViewUniformParameters;// = UniformParameters;
				PerViewUniformParameters.CameraUp = View->GetViewUp(); // FVector4(0.0f, 0.0f, 1.0f, 0.0f);
				PerViewUniformParameters.CameraRight = View->GetViewRight();//	FVector4(1.0f, 0.0f, 0.0f, 0.0f);
				PerViewUniformParameters.ScreenAlignment = FVector4(0.0f, 0.0f, 0.0f, 0.0f);

				// Collector.AllocateOneFrameResource uses default ctor, initialize the vertex factory
				CollectorResources.VertexFactory.SetFeatureLevel(ViewFamily.GetFeatureLevel());
				CollectorResources.VertexFactory.SetParticleFactoryType(PVFT_BeamTrail);

				CollectorResources.UniformBuffer = FParticleBeamTrailUniformBufferRef::CreateUniformBufferImmediate(PerViewUniformParameters, UniformBuffer_SingleFrame);

				CollectorResources.VertexFactory.InitResource();
				CollectorResources.VertexFactory.SetBeamTrailUniformBuffer(CollectorResources.UniformBuffer);
				CollectorResources.VertexFactory.SetVertexBuffer(LocalDynamicVertexAllocation.VertexBuffer, LocalDynamicVertexAllocation.VertexOffset, sizeof(FParticleBeamTrailVertex));
				if (DynamicDataRender->MaterialParameterVertexData.Num() > 0 && LocalDynamicVertexMaterialParamsAllocation.IsValid())
				{
					CollectorResources.VertexFactory.SetDynamicParameterBuffer(LocalDynamicVertexMaterialParamsAllocation.VertexBuffer, LocalDynamicVertexMaterialParamsAllocation.VertexOffset, sizeof(FParticleBeamTrailVertexDynamicParameter));
				}
				else
				{
					CollectorResources.VertexFactory.SetDynamicParameterBuffer(NULL, 0, 0);
				}

				FMeshBatch& MeshBatch = Collector.AllocateMesh();
				MeshBatch.VertexFactory = &CollectorResources.VertexFactory;
				MeshBatch.CastShadow = SceneProxy->CastsDynamicShadow();
				MeshBatch.bUseAsOccluder = false;
				MeshBatch.ReverseCulling = SceneProxy->IsLocalToWorldDeterminantNegative();
				MeshBatch.bDisableBackfaceCulling = true;
				MeshBatch.Type = PT_TriangleStrip;
				MeshBatch.DepthPriorityGroup = SceneProxy->GetDepthPriorityGroup(View);
				MeshBatch.bCanApplyViewModeOverrides = true;
				MeshBatch.bUseWireframeSelectionColoring = SceneProxy->IsSelected();

				if (bIsWireframe)
				{
					MeshBatch.MaterialRenderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(SceneProxy->IsSelected(), SceneProxy->IsHovered());
				}
				else
				{
					MeshBatch.MaterialRenderProxy = MaterialRenderProxy;
				}

				FMeshBatchElement& MeshElement = MeshBatch.Elements[0];
				MeshElement.IndexBuffer = &GParticleIndexBuffer;
				MeshElement.FirstIndex = 0;
				MeshElement.NumPrimitives = (DynamicDataRender->VertexData.Num() - 2);
				MeshElement.NumInstances = 1;
				MeshElement.MinVertexIndex = 0;
				MeshElement.MaxVertexIndex = DynamicDataRender->VertexData.Num() - 1;
				MeshElement.PrimitiveUniformBufferResource = &WorldSpacePrimitiveUniformBuffer;

				Collector.AddMesh(ViewIndex, MeshBatch);
			}
		}
	}

	CPUTimeMS += MeshElementsTimer.GetElapsedMilliseconds();
}

void NiagaraEffectRendererRibbon::SetDynamicData_RenderThread(FNiagaraDynamicDataBase* NewDynamicData)
{
	check(IsInRenderingThread());

	if (DynamicDataRender)
	{
		delete DynamicDataRender;
		DynamicDataRender = NULL;
	}
	DynamicDataRender = static_cast<FNiagaraDynamicDataRibbon*>(NewDynamicData);
}

int NiagaraEffectRendererRibbon::GetDynamicDataSize()
{
	uint32 Size = sizeof(FNiagaraDynamicDataRibbon);
	if (DynamicDataRender)
	{
		Size += DynamicDataRender->VertexData.GetAllocatedSize();
	}

	return Size;
}

bool NiagaraEffectRendererRibbon::HasDynamicData()
{
	return DynamicDataRender && DynamicDataRender->VertexData.Num() > 0;
}

const TArray<FNiagaraVariable>& NiagaraEffectRendererRibbon::GetRequiredAttributes()
{
	static TArray<FNiagaraVariable> Attrs;

	if (Attrs.Num() == 0)
	{
		/*
		Attrs.Add(FNiagaraVariableInfo(FName(TEXT("Position")), ENiagaraCompoundType::Vector));
		Attrs.Add(FNiagaraVariableInfo(FName(TEXT("Color")), ENiagaraCompoundType::Vector));
		Attrs.Add(FNiagaraVariableInfo(FName(TEXT("Rotation")), ENiagaraCompoundType::Scalar));
		Attrs.Add(FNiagaraVariableInfo(FName(TEXT("Age")), ENiagaraCompoundType::Scalar));
		*/
	}

	return Attrs;
}

bool NiagaraEffectRendererRibbon::SetMaterialUsage()
{
	return Material && Material->CheckMaterialUsage_Concurrent(MATUSAGE_BeamTrails);
}

FNiagaraDynamicDataBase *NiagaraEffectRendererRibbon::GenerateVertexData(const FNiagaraSceneProxy* Proxy, const FNiagaraDataSet &Data)
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraGenRibbonVertexData);

	SimpleTimer VertexDataTimer;

	FNiagaraDynamicDataRibbon *DynamicData = new FNiagaraDynamicDataRibbon;
	TArray<FParticleBeamTrailVertex>& RenderData = DynamicData->VertexData;

	RenderData.Reset(Data.GetNumInstances() * 2);
	CachedBounds.Init();

	// build a sorted list by age, so we always get particles in order 
	// regardless of them being moved around due to dieing and spawning
	TArray<int32> SortedIndices;
	for (uint32 Idx = 0; Idx < Data.GetNumInstances(); Idx++)
	{
		SortedIndices.Add(Idx);
	}

        //TODO: FIX UP RIBBONS
// 	const FVector4 *PosPtr = Data.GetVariableData(FNiagaraVariableInfo(FName(TEXT("Position")), ENiagaraCompoundType::Vector));
// 	const FVector4 *ColorPtr = Data.GetVariableData(FNiagaraVariableInfo(FName(TEXT("Color")), ENiagaraCompoundType::Vector));
// 	const FVector4 *RotPtr = Data.GetVariableData(FNiagaraVariableInfo(FName(TEXT("Rotation")), ENiagaraCompoundType::Vector));
// 	const FVector4 *AgePtr = Data.GetVariableData(FNiagaraVariableInfo(FName(TEXT("Age")), ENiagaraCompoundType::Vector));

	const FVector4 *PosPtr = NULL;// Data.GetVariableData(FNiagaraVariableInfo(FName(TEXT("Position")), ENiagaraCompoundType::Vector));
	const FVector4 *ColorPtr = NULL;//Data.GetVariableData(FNiagaraVariableInfo(FName(TEXT("Color")), ENiagaraCompoundType::Vector));
	const FVector4 *RotPtr = NULL;//Data.GetVariableData(FNiagaraVariableInfo(FName(TEXT("Rotation")), ENiagaraCompoundType::Vector));
	const FVector4 *AgePtr = NULL;//Data.GetVariableData(FNiagaraVariableInfo(FName(TEXT("Age")), ENiagaraCompoundType::Vector));

	//Bail if we don't have the required attributes to render this emitter.
	if (!PosPtr || !ColorPtr || !AgePtr || !RotPtr)
	{
		return DynamicData;
	}

	SortedIndices.Sort(
		[&AgePtr](const int32& A, const int32& B) {
		return AgePtr[A].X < AgePtr[B].X;
	}
	);

	// TODO : deal with the dynamic vertex material parameter should the user have specified it as an output...

	FVector2D UVs[4] = { FVector2D(0.0f, 0.0f), FVector2D(1.0f, 0.0f), FVector2D(1.0f, 1.0f), FVector2D(0.0f, 1.0f) };

	FVector PrevPos, PrevPos2, PrevDir(0.0f, 0.0f, 0.1f);
	for (int32 i = 0; i < SortedIndices.Num() - 1; i++)
	{
		uint32 Index1 = SortedIndices[i];
		uint32 Index2 = SortedIndices[i + 1];

		const FVector ParticlePos = PosPtr[Index1];
		FVector ParticleDir = PosPtr[Index2] - ParticlePos;
		if (ParticleDir.SizeSquared() <= FMath::Square(SMALL_NUMBER))
		{
			ParticleDir = PrevDir*0.1f;
		}
		FVector NormDir = ParticleDir.GetSafeNormal();

		FVector ParticleRight = FVector::CrossProduct(NormDir, FVector(0.0f, 0.0f, 1.0f));
		ParticleRight *= RotPtr[Index1].Y;
		FVector ParticleRightRot = ParticleRight.RotateAngleAxis(RotPtr[Index1].X, NormDir);

		if (i == 0)
		{
			AddRibbonVert(RenderData, ParticlePos + ParticleRightRot, Data, UVs[0], ColorPtr[Index1], AgePtr[Index1], RotPtr[i]);
			AddRibbonVert(RenderData, ParticlePos - ParticleRightRot, Data, UVs[1], ColorPtr[Index1], AgePtr[Index1], RotPtr[i]);
		}
		else
		{
			AddRibbonVert(RenderData, PrevPos2, Data, UVs[0], ColorPtr[Index1], AgePtr[Index1], RotPtr[i]);
			AddRibbonVert(RenderData, PrevPos, Data, UVs[1], ColorPtr[Index1], AgePtr[Index1], RotPtr[i]);
		}

		ParticleRightRot = ParticleRight.RotateAngleAxis(RotPtr[Index2].X, NormDir);
		AddRibbonVert(RenderData, ParticlePos - ParticleRightRot + ParticleDir, Data, UVs[2], ColorPtr[Index2], AgePtr[Index2], RotPtr[i]);
		AddRibbonVert(RenderData, ParticlePos + ParticleRightRot + ParticleDir, Data, UVs[3], ColorPtr[Index2], AgePtr[Index2], RotPtr[i]);
		PrevPos = ParticlePos - ParticleRightRot + ParticleDir;
		PrevPos2 = ParticlePos + ParticleRightRot + ParticleDir;
		PrevDir = ParticleDir;

		CachedBounds += ParticlePos + ParticleRightRot;
		CachedBounds += ParticlePos - ParticleRightRot;
	}

	CPUTimeMS = VertexDataTimer.GetElapsedMilliseconds();

	return DynamicData;
}











NiagaraEffectRendererMeshes::NiagaraEffectRendererMeshes(ERHIFeatureLevel::Type FeatureLevel, UNiagaraEffectRendererProperties *InProps) :
NiagaraEffectRenderer(),
DynamicDataRender(NULL)
{
	//check(InProps);
	VertexFactory = ConstructMeshParticleVertexFactory(PVFT_Mesh, FeatureLevel, sizeof(FMeshParticleInstanceVertex), 0);
	Properties = Cast<UNiagaraMeshRendererProperties>(InProps);


	if (Properties && Properties->ParticleMesh)
	{
		const FStaticMeshLODResources& LODModel = Properties->ParticleMesh->RenderData->LODResources[0];
		for (int32 SectionIndex = 0; SectionIndex < LODModel.Sections.Num(); SectionIndex++)
		{
			//FMaterialRenderProxy* MaterialProxy = MeshMaterials[SectionIndex]->GetRenderProxy(bSelected);
			const FStaticMeshSection& Section = LODModel.Sections[SectionIndex];
			UMaterialInterface* ParticleMeshMaterial = Properties->ParticleMesh->GetMaterial(Section.MaterialIndex);
			ParticleMeshMaterial->CheckMaterialUsage_Concurrent(MATUSAGE_MeshParticles);
		}
	}

}

void NiagaraEffectRendererMeshes::SetupVertexFactory(FMeshParticleVertexFactory *InVertexFactory, const FStaticMeshLODResources& LODResources) const
{
	FMeshParticleVertexFactory::FDataType Data;

	Data.PositionComponent = FVertexStreamComponent(
		&LODResources.PositionVertexBuffer,
		STRUCT_OFFSET(FPositionVertex, Position),
		LODResources.PositionVertexBuffer.GetStride(),
		VET_Float3
		);

	uint32 TangentXOffset = 0;
	uint32 TangetnZOffset = 0;
	uint32 UVsBaseOffset = 0;

	SELECT_STATIC_MESH_VERTEX_TYPE(
		LODResources.VertexBuffer.GetUseHighPrecisionTangentBasis(),
		LODResources.VertexBuffer.GetUseFullPrecisionUVs(),
		LODResources.VertexBuffer.GetNumTexCoords(),
		{
			TangentXOffset = STRUCT_OFFSET(VertexType, TangentX);
			TangetnZOffset = STRUCT_OFFSET(VertexType, TangentZ);
			UVsBaseOffset = STRUCT_OFFSET(VertexType, UVs);
		});

	Data.TangentBasisComponents[0] = FVertexStreamComponent(
		&LODResources.VertexBuffer,
		TangentXOffset,
		LODResources.VertexBuffer.GetStride(),
		LODResources.VertexBuffer.GetUseHighPrecisionTangentBasis() ?
			TStaticMeshVertexTangentTypeSelector<EStaticMeshVertexTangentBasisType::HighPrecision>::VertexElementType : 
			TStaticMeshVertexTangentTypeSelector<EStaticMeshVertexTangentBasisType::Default>::VertexElementType
		);

	Data.TangentBasisComponents[1] = FVertexStreamComponent(
		&LODResources.VertexBuffer,
		TangetnZOffset,
		LODResources.VertexBuffer.GetStride(),
		LODResources.VertexBuffer.GetUseHighPrecisionTangentBasis() ?
			TStaticMeshVertexTangentTypeSelector<EStaticMeshVertexTangentBasisType::HighPrecision>::VertexElementType : 
			TStaticMeshVertexTangentTypeSelector<EStaticMeshVertexTangentBasisType::Default>::VertexElementType
		);

	Data.TextureCoordinates.Empty();

	uint32 UVSizeInBytes = LODResources.VertexBuffer.GetUseFullPrecisionUVs() ?
		sizeof(TStaticMeshVertexUVsTypeSelector<EStaticMeshVertexUVType::HighPrecision>::UVsTypeT) : sizeof(TStaticMeshVertexUVsTypeSelector<EStaticMeshVertexUVType::Default>::UVsTypeT);

	EVertexElementType UVVertexElementType = LODResources.VertexBuffer.GetUseFullPrecisionUVs() ?
		VET_Float2 : VET_Half2;

	uint32 NumTexCoords = FMath::Min<uint32>(LODResources.VertexBuffer.GetNumTexCoords(), MAX_TEXCOORDS);
	for (uint32 UVIndex = 0; UVIndex < NumTexCoords; UVIndex++)
	{
		Data.TextureCoordinates.Add(FVertexStreamComponent(
			&LODResources.VertexBuffer,
			UVsBaseOffset + UVSizeInBytes * UVIndex,
			LODResources.VertexBuffer.GetStride(),
			UVVertexElementType
			));
	}

	if (LODResources.ColorVertexBuffer.GetNumVertices() > 0)
	{
		Data.VertexColorComponent = FVertexStreamComponent(
			&LODResources.ColorVertexBuffer,
			0,
			LODResources.ColorVertexBuffer.GetStride(),
			VET_Color
			);
	}


	// Initialize instanced data. Vertex buffer and stride are set before render.
	// Particle color
	Data.ParticleColorComponent = FVertexStreamComponent(
		NULL,
		STRUCT_OFFSET(FMeshParticleInstanceVertex, Color),
		0,
		VET_Float4,
		true
		);

	// Particle transform matrix
	for (int32 MatrixRow = 0; MatrixRow < 3; MatrixRow++)
	{
		Data.TransformComponent[MatrixRow] = FVertexStreamComponent(
			NULL,
			STRUCT_OFFSET(FMeshParticleInstanceVertex, Transform) + sizeof(FVector4)* MatrixRow,
			0,
			VET_Float4,
			true
			);
	}

	Data.VelocityComponent = FVertexStreamComponent(
		NULL,
		STRUCT_OFFSET(FMeshParticleInstanceVertex, Velocity),
		0,
		VET_Float4,
		true
		);

	// SubUVs.
	Data.SubUVs = FVertexStreamComponent(
		NULL,
		STRUCT_OFFSET(FMeshParticleInstanceVertex, SubUVParams),
		0,
		VET_Short4,
		true
		);

	// Pack SubUV Lerp and the particle's relative time
	Data.SubUVLerpAndRelTime = FVertexStreamComponent(
		NULL,
		STRUCT_OFFSET(FMeshParticleInstanceVertex, SubUVLerp),
		0,
		VET_Float2,
		true
		);

	Data.bInitialized = true;
	InVertexFactory->SetData(Data);
}



void NiagaraEffectRendererMeshes::ReleaseRenderThreadResources()
{
	VertexFactory->ReleaseResource();
	WorldSpacePrimitiveUniformBuffer.ReleaseResource();
}

void NiagaraEffectRendererMeshes::CreateRenderThreadResources()
{
	VertexFactory->InitResource();
}




void NiagaraEffectRendererMeshes::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector, const FNiagaraSceneProxy *SceneProxy) const
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraRender);
	SCOPE_CYCLE_COUNTER(STAT_NiagaraRenderMeshes);

	SimpleTimer MeshElementsTimer;

	//check(DynamicDataRender)

	if (!DynamicDataRender || DynamicDataRender->VertexData.Num() == 0)
	{
		return;
	}

	int32 SizeInBytes = DynamicDataRender->VertexData.GetTypeSize() * DynamicDataRender->VertexData.Num();
	FGlobalDynamicVertexBuffer::FAllocation LocalDynamicVertexAllocation = FGlobalDynamicVertexBuffer::Get().Allocate(SizeInBytes);
	FGlobalDynamicVertexBuffer::FAllocation LocalDynamicVertexMaterialParamsAllocation;

	if (DynamicDataRender->MaterialParameterVertexData.Num() > 0)
	{
		int32 MatParamSizeInBytes = DynamicDataRender->MaterialParameterVertexData.GetTypeSize() * DynamicDataRender->MaterialParameterVertexData.Num();
		LocalDynamicVertexMaterialParamsAllocation = FGlobalDynamicVertexBuffer::Get().Allocate(MatParamSizeInBytes);

		if (LocalDynamicVertexMaterialParamsAllocation.IsValid())
		{
			// Copy the vertex data over.
			FMemory::Memcpy(LocalDynamicVertexMaterialParamsAllocation.Buffer, DynamicDataRender->MaterialParameterVertexData.GetData(), MatParamSizeInBytes);
		}
	}

	if (LocalDynamicVertexAllocation.IsValid())
	{
		// Update the primitive uniform buffer if needed.
		if (!WorldSpacePrimitiveUniformBuffer.IsInitialized())
		{
			FPrimitiveUniformShaderParameters PrimitiveUniformShaderParameters = GetPrimitiveUniformShaderParameters(
				FMatrix::Identity,
				SceneProxy->GetActorPosition(),
				SceneProxy->GetBounds(),
				SceneProxy->GetLocalBounds(),
				SceneProxy->ReceivesDecals(),
				false,
				false,
				false,
				SceneProxy->UseEditorDepthTest(),
				SceneProxy->GetLightingChannelMask()
				);
			WorldSpacePrimitiveUniformBuffer.SetContents(PrimitiveUniformShaderParameters);
			WorldSpacePrimitiveUniformBuffer.InitResource();
		}

		// Copy the vertex data over.
		FMemory::Memcpy(LocalDynamicVertexAllocation.Buffer, DynamicDataRender->VertexData.GetData(), SizeInBytes);

		// Compute the per-view uniform buffers.
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];
				const FStaticMeshLODResources& LODModel = Properties->ParticleMesh->RenderData->LODResources[0];

				FNiagaraMeshCollectorResourcesMesh& CollectorResources = Collector.AllocateOneFrameResource<FNiagaraMeshCollectorResourcesMesh>();
				SetupVertexFactory(&CollectorResources.VertexFactory, LODModel);
				FMeshParticleUniformParameters PerViewUniformParameters;// = UniformParameters;
				/*
				if (Properties)
				{
					PerViewUniformParameters.SubImageSize = FVector4(Properties->SubImageInfo.X, Properties->SubImageInfo.Y, 1.0f / Properties->SubImageInfo.X, 1.0f / Properties->SubImageInfo.Y);
				}
				*/

				// Collector.AllocateOneFrameResource uses default ctor, initialize the vertex factory
				CollectorResources.VertexFactory.SetFeatureLevel(ViewFamily.GetFeatureLevel());
				CollectorResources.VertexFactory.SetParticleFactoryType(PVFT_Mesh);
				CollectorResources.UniformBuffer = FMeshParticleUniformBufferRef::CreateUniformBufferImmediate(PerViewUniformParameters, UniformBuffer_SingleFrame);
				CollectorResources.VertexFactory.SetStrides(sizeof(FMeshParticleInstanceVertex), 0);

				CollectorResources.VertexFactory.InitResource();
				CollectorResources.VertexFactory.SetUniformBuffer(CollectorResources.UniformBuffer);
				CollectorResources.VertexFactory.SetInstanceBuffer(
					LocalDynamicVertexAllocation.VertexBuffer,
					LocalDynamicVertexAllocation.VertexOffset,
					sizeof(FMeshParticleInstanceVertex)
					);

				if (LocalDynamicVertexMaterialParamsAllocation.IsValid() && DynamicDataRender->MaterialParameterVertexData.Num() > 0)
				{
					CollectorResources.VertexFactory.SetDynamicParameterBuffer(LocalDynamicVertexMaterialParamsAllocation.VertexBuffer, LocalDynamicVertexMaterialParamsAllocation.VertexOffset,
						sizeof(FMeshParticleInstanceVertexDynamicParameter));
				}
				else
				{
					CollectorResources.VertexFactory.SetDynamicParameterBuffer(NULL, 0, 0);
				}

				const bool bIsWireframe = AllowDebugViewmodes() && View->Family->EngineShowFlags.Wireframe;

				for (int32 SectionIndex = 0; SectionIndex < LODModel.Sections.Num(); SectionIndex++)
				{
					const FStaticMeshSection& Section = LODModel.Sections[SectionIndex];
					UMaterialInterface* ParticleMeshMaterial = Properties->ParticleMesh->GetMaterial(Section.MaterialIndex);
					FMaterialRenderProxy* MaterialProxy = ParticleMeshMaterial->GetRenderProxy(false, false);

					if ((Section.NumTriangles == 0) || (MaterialProxy == NULL))
					{
						//@todo. This should never occur, but it does occasionally.
						continue;
					}

					FMeshBatch& Mesh = Collector.AllocateMesh();
					Mesh.VertexFactory = &CollectorResources.VertexFactory;
					Mesh.DynamicVertexData = NULL;
					Mesh.LCI = NULL;
					Mesh.UseDynamicData = false;
					Mesh.ReverseCulling = SceneProxy->IsLocalToWorldDeterminantNegative();
					Mesh.CastShadow = SceneProxy->CastsDynamicShadow();
					Mesh.DepthPriorityGroup = (ESceneDepthPriorityGroup)SceneProxy->GetDepthPriorityGroup(View);

					FMeshBatchElement& BatchElement = Mesh.Elements[0];
					BatchElement.PrimitiveUniformBufferResource = &WorldSpacePrimitiveUniformBuffer;
					BatchElement.FirstIndex = Section.FirstIndex;
					BatchElement.MinVertexIndex = Section.MinVertexIndex;
					BatchElement.MaxVertexIndex = Section.MaxVertexIndex;
					BatchElement.NumInstances = DynamicDataRender->VertexData.Num();

					if (bIsWireframe)
					{
						if (LODModel.WireframeIndexBuffer.IsInitialized())
						{
							Mesh.Type = PT_LineList;
							Mesh.MaterialRenderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(SceneProxy->IsSelected(), SceneProxy->IsHovered());
							BatchElement.FirstIndex = 0;
							BatchElement.IndexBuffer = &LODModel.WireframeIndexBuffer;
							BatchElement.NumPrimitives = LODModel.WireframeIndexBuffer.GetNumIndices() / 2;

						}
						else
						{
							Mesh.Type = PT_TriangleList;
							Mesh.MaterialRenderProxy = MaterialProxy;
							Mesh.bWireframe = true;
							BatchElement.FirstIndex = 0;
							BatchElement.IndexBuffer = &LODModel.IndexBuffer;
							BatchElement.NumPrimitives = LODModel.IndexBuffer.GetNumIndices() / 3;
						}
					}
					else
					{
						Mesh.Type = PT_TriangleList;
						Mesh.MaterialRenderProxy = MaterialProxy;
						BatchElement.IndexBuffer = &LODModel.IndexBuffer;
						BatchElement.FirstIndex = Section.FirstIndex;
						BatchElement.NumPrimitives = Section.NumTriangles;
					}

					/*  TODO: make this work
					if (!bInstanced)
					{
						FMeshParticleVertexFactory::FBatchParametersCPU& BatchParameters = Collector.AllocateOneFrameResource<FMeshParticleVertexFactory::FBatchParametersCPU>();
						BatchParameters.InstanceBuffer = InstanceVerticesCPU->InstanceDataAllocationsCPU.GetData();
						BatchParameters.DynamicParameterBuffer = InstanceVerticesCPU->DynamicParameterDataAllocationsCPU.GetData();
						BatchElement.UserData = &BatchParameters;
						BatchElement.bUserDataIsColorVertexBuffer = false;
						BatchElement.UserIndex = 0;

						Mesh.Elements.Reserve(ParticleCount);
						for (int32 ParticleIndex = 1; ParticleIndex < ParticleCount; ++ParticleIndex)
						{
							FMeshBatchElement* NextElement = new(Mesh.Elements) FMeshBatchElement();
							*NextElement = Mesh.Elements[0];
							NextElement->UserIndex = ParticleIndex;
						}
					}*/

					Mesh.bCanApplyViewModeOverrides = true;
					Mesh.bUseWireframeSelectionColoring = SceneProxy->IsSelected();

					Collector.AddMesh(ViewIndex, Mesh);
				}
			}
		}
	}

	CPUTimeMS += MeshElementsTimer.GetElapsedMilliseconds();
}



bool NiagaraEffectRendererMeshes::SetMaterialUsage()
{
	//Causes deadlock :S Need to look at / rework the setting of materials and render modules.
	return Material && Material->CheckMaterialUsage_Concurrent(MATUSAGE_MeshParticles);
}



/** Update render data buffer from attributes */
FNiagaraDynamicDataBase *NiagaraEffectRendererMeshes::GenerateVertexData(const FNiagaraSceneProxy* Proxy, const FNiagaraDataSet &Data)
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraGenMeshVertexData);

	if (!Properties || Properties->ParticleMesh == nullptr)
	{
		return nullptr;
	}


	SimpleTimer VertexDataTimer;
	FVector MaxSize;

	FNiagaraDynamicDataMesh *DynamicData = new FNiagaraDynamicDataMesh;
	TArray<FMeshParticleInstanceVertex>& RenderData = DynamicData->VertexData;
	TArray< FMeshParticleInstanceVertexDynamicParameter>& RenderMaterialVertexData = DynamicData->MaterialParameterVertexData;

	RenderData.Reset(Data.GetNumInstances());
	CachedBounds.Init();


	FNiagaraDataSetIterator<FVector> PosItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Position")));
	FNiagaraDataSetIterator<FVector> VelItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Velocity")));
	FNiagaraDataSetIterator<FLinearColor> ColItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(), TEXT("Color")));
	FNiagaraDataSetIterator<float> AgeItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Age")));
	FNiagaraDataSetIterator<FVector4> XFormItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec4Def(), TEXT("Transform")));
	FNiagaraDataSetIterator<FVector2D> SizeItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec2Def(), TEXT("Size")));
	FNiagaraDataSetIterator<FVector4> MaterialParamItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec4Def(), TEXT("DynamicMaterialParameter")));

	//Bail if we don't have the required attributes to render this emitter.
	if (!PosItr.IsValid() || !ColItr.IsValid() || !AgeItr.IsValid() || !VelItr.IsValid())
	{
		return DynamicData;
	}

	if (MaterialParamItr.IsValid())
	{
		RenderMaterialVertexData.Reset(Data.GetNumInstances());
		RenderMaterialVertexData.AddUninitialized(Data.GetNumInstances());
	}

	FMatrix LocalToWorld = Proxy->GetLocalToWorld();

 	UStaticMesh *Mesh = nullptr;
 	if (Properties)
 	{
 		Mesh = Properties->ParticleMesh;
 	}
 	float ParticleId = 0.0f, IdInc = 1.0f / Data.GetNumInstances();
 	RenderData.AddUninitialized(Data.GetNumInstances());
 	for (uint32 ParticleIndex = 0; ParticleIndex < Data.GetNumInstances(); ParticleIndex++)
 	{
		FVector4 Pos = *PosItr;
		FLinearColor Col = *ColItr;
		float Age = *AgeItr;

		FMeshParticleInstanceVertex& NewVertex = RenderData[ParticleIndex];
		NewVertex.Color = Col;
 		NewVertex.Color.A = Col.A;
 		//NewVertex.ParticleId = ParticleId;
 		ParticleId += IdInc;
 		NewVertex.RelativeTime = Age;
 
 		FRotationMatrix RotMat(FRotator(0.0f, 0.0f, 0.0f));
 		FVector Scale = FVector(1.0f,1.0f,1.0f);
 		if (XFormItr.IsValid())
 		{
			FVector4 XForm = *XFormItr;
 			FVector Euler(XForm);
 			FRotator Rotator = FRotator::MakeFromEuler(Euler);
			RotMat = Rotator;
			Scale = FVector(XForm.W, XForm.W, XForm.W);
		}
		else if(SizeItr.IsValid())
		{
			FVector2D Size = *SizeItr;
			Scale = FVector(Size.X, Size.X, Size.X);
		}

		if (bLocalSpace)
		{
			Scale *= LocalToWorld.ExtractScaling();
			RotMat *= LocalToWorld;
			Pos = LocalToWorld.TransformPosition(Pos);
		}
 
 		NewVertex.Transform[0].X = RotMat.GetColumn(0).X*Scale.X;  NewVertex.Transform[0].Y = RotMat.GetColumn(0).Y*Scale.Y;   NewVertex.Transform[0].Z = RotMat.GetColumn(0).Z*Scale.Z;
 		NewVertex.Transform[1].X = RotMat.GetColumn(1).X*Scale.X;  NewVertex.Transform[1].Y = RotMat.GetColumn(1).Y*Scale.Y;   NewVertex.Transform[1].Z = RotMat.GetColumn(1).Z*Scale.Z;
 		NewVertex.Transform[2].X = RotMat.GetColumn(2).X*Scale.X;  NewVertex.Transform[2].Y = RotMat.GetColumn(2).Y*Scale.Y;   NewVertex.Transform[2].Z = RotMat.GetColumn(2).Z*Scale.Z;
 		NewVertex.Transform[0].W = Pos.X;
 		NewVertex.Transform[1].W = Pos.Y;
 		NewVertex.Transform[2].W = Pos.Z;
		FVector4 Vel = *VelItr;
		NewVertex.Velocity = Vel;
 
 		MaxSize = MaxSize.ComponentMax(Mesh->GetBoundingBox().GetExtent());
 		CachedBounds += Pos;

		PosItr.Advance();
		VelItr.Advance();
		AgeItr.Advance();
		if (XFormItr.IsValid())
		{
			XFormItr.Advance();
		}
		if (SizeItr.IsValid())
		{
			SizeItr.Advance();
		}
 	}
	CachedBounds.ExpandBy(MaxSize);
	if (MaterialParamItr.IsValid())
	{
		for (uint32 ParticleIndex = 0; ParticleIndex < Data.GetNumInstances(); ParticleIndex++)
		{
			RenderMaterialVertexData[ParticleIndex].DynamicValue[0] = (*MaterialParamItr).X;
			RenderMaterialVertexData[ParticleIndex].DynamicValue[1] = (*MaterialParamItr).Y;
			RenderMaterialVertexData[ParticleIndex].DynamicValue[2] = (*MaterialParamItr).Z;
			RenderMaterialVertexData[ParticleIndex].DynamicValue[3] = (*MaterialParamItr).W;
			MaterialParamItr.Advance();
		}
	}
	CPUTimeMS = VertexDataTimer.GetElapsedMilliseconds();


	return DynamicData;
}



void NiagaraEffectRendererMeshes::SetDynamicData_RenderThread(FNiagaraDynamicDataBase* NewDynamicData)
{
	check(IsInRenderingThread());

	if (DynamicDataRender)
	{
		delete DynamicDataRender;
		DynamicDataRender = NULL;
	}
	DynamicDataRender = static_cast<FNiagaraDynamicDataMesh*>(NewDynamicData);
}

int NiagaraEffectRendererMeshes::GetDynamicDataSize()
{
	uint32 Size = sizeof(FNiagaraDynamicDataMesh);
	if (DynamicDataRender)
	{
		Size += DynamicDataRender->VertexData.GetAllocatedSize();
	}

	return Size;
}

bool NiagaraEffectRendererMeshes::HasDynamicData()
{
	return DynamicDataRender && DynamicDataRender->VertexData.Num() > 0;
}


const TArray<FNiagaraVariable>& NiagaraEffectRendererMeshes::GetRequiredAttributes()
{
	static TArray<FNiagaraVariable> Attrs;

	if (Attrs.Num() == 0)
	{
		/*
		Attrs.Add(FNiagaraVariableInfo(FName(TEXT("Position")), ENiagaraCompoundType::Vector));
		Attrs.Add(FNiagaraVariableInfo(FName(TEXT("Color")), ENiagaraCompoundType::Vector));
		//Attrs.Add(FNiagaraVariableInfo(FName(TEXT("Rotation")), ENiagaraCompoundType::Vector));
		Attrs.Add(FNiagaraVariableInfo(FName(TEXT("Age")), ENiagaraCompoundType::Scalar));
		*/
	}

	return Attrs;
}
