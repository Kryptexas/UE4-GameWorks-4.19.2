// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEffectRenderer.h"
#include "Particles/ParticleResources.h"
#include "NiagaraSpriteVertexFactory.h"
#include "NiagaraDataSet.h"
#include "NiagaraStats.h"

DECLARE_CYCLE_STAT(TEXT("Generate Sprite Vertex Data"), STAT_NiagaraGenSpriteVertexData, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Render Sprites"), STAT_NiagaraRenderSprites, STATGROUP_Niagara);



/**
* Per-particle data sent to the GPU.
*/
struct FNiagaraSpriteVertex
{
	/** The position of the particle. */
	FVector Position;
	/** The relative time of the particle. */
	float RelativeTime;
	/** The previous position of the particle. */
	FVector	OldPosition;
	/** Value that remains constant over the lifetime of a particle. */
	float ParticleId;
	/** The size of the particle. */
	FVector2D Size;
	/** The rotation of the particle. */
	float Rotation;
	/** The sub-image index for the particle. */
	float SubImageIndex;
	/** The color of the particle. */
	FLinearColor Color;
	/* Custom Alignment vector*/
	FVector CustomAlignmentVector;
	/* Custom Facing vector*/
	FVector CustomFacingVector;
};


struct FNiagaraDynamicDataSprites : public FNiagaraDynamicDataBase
{
	TArray<FNiagaraSpriteVertex> VertexData;
	TArray<FParticleVertexDynamicParameter> MaterialParameterVertexData;
	bool bCustomAlignmentAvailable;
};



/* Mesh collector classes */
class FNiagaraMeshCollectorResourcesSprite : public FOneFrameResource
{
public:
	FNiagaraSpriteVertexFactory VertexFactory;
	FNiagaraSpriteUniformBufferRef UniformBuffer;

	virtual ~FNiagaraMeshCollectorResourcesSprite()
	{
		VertexFactory.ReleaseResource();
	}
};



NiagaraEffectRendererSprites::NiagaraEffectRendererSprites(ERHIFeatureLevel::Type FeatureLevel, UNiagaraEffectRendererProperties *InProps) :
	NiagaraEffectRenderer()
{
	//check(InProps);
	VertexFactory = new FNiagaraSpriteVertexFactory(NVFT_Sprite, FeatureLevel);
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

	FNiagaraDynamicDataSprites *DynamicDataSprites = static_cast<FNiagaraDynamicDataSprites*>(DynamicDataRender);
	if (!DynamicDataSprites || DynamicDataSprites->VertexData.Num() == 0 || nullptr == Properties)
	{
		return;
	}

	const bool bIsWireframe = ViewFamily.EngineShowFlags.Wireframe;
	FMaterialRenderProxy* MaterialRenderProxy = Material->GetRenderProxy(SceneProxy->IsSelected(), SceneProxy->IsHovered());

	int32 SizeInBytes = DynamicDataSprites->VertexData.GetTypeSize() * DynamicDataSprites->VertexData.Num();
	FGlobalDynamicVertexBuffer::FAllocation LocalDynamicVertexMaterialParamsAllocation;

	if (DynamicDataSprites->MaterialParameterVertexData.Num() > 0)
	{
		int32 MatParamSizeInBytes = DynamicDataSprites->MaterialParameterVertexData.GetTypeSize() * DynamicDataSprites->MaterialParameterVertexData.Num();
		LocalDynamicVertexMaterialParamsAllocation = FGlobalDynamicVertexBuffer::Get().Allocate(MatParamSizeInBytes);

		if (LocalDynamicVertexMaterialParamsAllocation.IsValid())
		{
			// Copy the extra material vertex data over.
			FMemory::Memcpy(LocalDynamicVertexMaterialParamsAllocation.Buffer, DynamicDataSprites->MaterialParameterVertexData.GetData(), MatParamSizeInBytes);
		}
	}

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


	// Compute the per-view uniform buffers.
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			FGlobalDynamicVertexBuffer::FAllocation LocalDynamicVertexAllocation = FGlobalDynamicVertexBuffer::Get().Allocate(SizeInBytes);
			if (LocalDynamicVertexAllocation.IsValid())
			{
				const FSceneView* View = Views[ViewIndex];
				FMatrix TransMat;

				if (Properties->SortMode == ENiagaraSortMode::SortViewDepth)
				{
					TransMat = View->ViewMatrices.GetViewProjectionMatrix();

					// view depth sorting
					DynamicDataSprites->VertexData.Sort(
						[&TransMat](const FNiagaraSpriteVertex& A, const FNiagaraSpriteVertex& B) {
						float W1 = TransMat.TransformPosition(A.Position).W;
						float W2 = TransMat.TransformPosition(B.Position).W;
						return (W1 > W2);
					});
				}
				else if (Properties->SortMode == ENiagaraSortMode::SortViewDistance)
				{
					FVector ViewLoc = View->ViewLocation;
					// dist sorting
					DynamicDataSprites->VertexData.Sort(
						[&ViewLoc](const FNiagaraSpriteVertex& A, const FNiagaraSpriteVertex& B) {
						float D1 = (ViewLoc - A.Position).SizeSquared();
						float D2 = (ViewLoc - B.Position).SizeSquared();
						return (D1 > D2);
					});
				}

				// Copy the vertex data over.
				FMemory::Memcpy(LocalDynamicVertexAllocation.Buffer, DynamicDataSprites->VertexData.GetData(), SizeInBytes);



				FNiagaraMeshCollectorResourcesSprite& CollectorResources = Collector.AllocateOneFrameResource<FNiagaraMeshCollectorResourcesSprite>();
				FNiagaraSpriteUniformParameters PerViewUniformParameters;// = UniformParameters;
				//PerViewUniformParameters.AxisLockRight = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
				//PerViewUniformParameters.AxisLockUp = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
				PerViewUniformParameters.RotationBias = 0.0f;
				PerViewUniformParameters.RotationScale = 1.0f;
				PerViewUniformParameters.TangentSelector = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
				PerViewUniformParameters.InvDeltaSeconds = 30.0f;
				PerViewUniformParameters.NormalsType = 0.0f;
				PerViewUniformParameters.NormalsSphereCenter = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
				PerViewUniformParameters.NormalsCylinderUnitDirection = FVector4(0.0f, 0.0f, 1.0f, 0.0f);
				PerViewUniformParameters.PivotOffset = FVector2D(-0.5f, -0.5f);
				PerViewUniformParameters.MacroUVParameters = FVector4(0.0f, 0.0f, 1.0f, 1.0f);
				PerViewUniformParameters.CameraFacingBlend = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
				PerViewUniformParameters.RemoveHMDRoll = 0.0f;
				PerViewUniformParameters.CustomFacingVectorMask = Properties->CustomFacingVectorMask;
				PerViewUniformParameters.SubImageSize = FVector4(Properties->SubImageSize.X, Properties->SubImageSize.Y, 1.0f / Properties->SubImageSize.X, 1.0f / Properties->SubImageSize.Y);

				if (Properties->Alignment == ENiagaraSpriteAlignment::VelocityAligned)
				{
					// velocity aligned
					PerViewUniformParameters.RotationScale = 0.0f;
					PerViewUniformParameters.TangentSelector = FVector4(0.0f, 1.0f, 0.0f, 0.0f);
				}

				// Collector.AllocateOneFrameResource uses default ctor, initialize the vertex factory

				// use custom alignment if the data is available and if it's set in the props
				bool bUseCustomAlignment = DynamicDataSprites->bCustomAlignmentAvailable && Properties->Alignment == ENiagaraSpriteAlignment::CustomAlignment;
				bool bUseVectorAlignment = Properties->Alignment != ENiagaraSpriteAlignment::Unaligned;
				
				CollectorResources.VertexFactory.SetCustomAlignment(bUseCustomAlignment);
				CollectorResources.VertexFactory.SetVectorAligned(bUseVectorAlignment);
				CollectorResources.VertexFactory.SetCameraPlaneFacing(Properties->FacingMode == ENiagaraSpriteFacingMode::FaceCameraPlane);

				CollectorResources.VertexFactory.SetFeatureLevel(ViewFamily.GetFeatureLevel());
				CollectorResources.VertexFactory.SetParticleFactoryType(NVFT_Sprite);

				CollectorResources.UniformBuffer = FNiagaraSpriteUniformBufferRef::CreateUniformBufferImmediate(PerViewUniformParameters, UniformBuffer_SingleFrame);

				CollectorResources.VertexFactory.SetNumVertsInInstanceBuffer(4);
				CollectorResources.VertexFactory.InitResource();
				CollectorResources.VertexFactory.SetSpriteUniformBuffer(CollectorResources.UniformBuffer);
				CollectorResources.VertexFactory.SetInstanceBuffer(
					LocalDynamicVertexAllocation.VertexBuffer,
					LocalDynamicVertexAllocation.VertexOffset,
					sizeof(FNiagaraSpriteVertex),
					true
					);

				if (DynamicDataSprites->MaterialParameterVertexData.Num() > 0 && LocalDynamicVertexMaterialParamsAllocation.IsValid())
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
				MeshElement.NumInstances = DynamicDataSprites->VertexData.Num();
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
	return Material && Material->CheckMaterialUsage_Concurrent(MATUSAGE_NiagaraSprites);
}

/** Update render data buffer from attributes */
FNiagaraDynamicDataBase *NiagaraEffectRendererSprites::GenerateVertexData(const FNiagaraSceneProxy* Proxy, const FNiagaraDataSet &Data)
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraGenSpriteVertexData);

	SimpleTimer VertexDataTimer;

	FNiagaraDynamicDataSprites *DynamicData = new FNiagaraDynamicDataSprites;
	TArray<FNiagaraSpriteVertex>& RenderData = DynamicData->VertexData;
	TArray< FParticleVertexDynamicParameter>& RenderMaterialVertexData = DynamicData->MaterialParameterVertexData;

	RenderData.Reset(Data.GetNumInstances());

	//I'm not a great fan of pulling scalar components out to a structured vert buffer like this.
	//TODO: Experiment with a new VF that reads the data directly from the scalar layout.
	FNiagaraDataSetIterator<FVector> PosItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Position")));
	FNiagaraDataSetIterator<FVector> VelItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Velocity")));
	FNiagaraDataSetIterator<FVector> AlignItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Alignment")));
	FNiagaraDataSetIterator<FVector> FacingItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Facing")));
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

	DynamicData->bCustomAlignmentAvailable = AlignItr.IsValid();

	if (MaterialParamItr.IsValid())
	{
		RenderMaterialVertexData.Reset(Data.GetNumInstances());
		RenderMaterialVertexData.AddUninitialized(Data.GetNumInstances());
	}

	FMatrix LocalToWorld = Proxy->GetLocalToWorld();

	uint32 NumSubImages = 1;
	if (Properties)
	{
		NumSubImages = Properties->SubImageSize.X*Properties->SubImageSize.Y;
	}
	float ParticleId = 0.0f, IdInc = 1.0f / Data.GetNumInstances();
	RenderData.AddUninitialized(Data.GetNumInstances());

	for (uint32 ParticleIndex = 0; ParticleIndex < Data.GetNumInstances(); ParticleIndex++)
	{
		FNiagaraSpriteVertex& NewVertex = RenderData[ParticleIndex];
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

		if (AlignItr.IsValid())
		{
			NewVertex.CustomAlignmentVector = *AlignItr;
			AlignItr.Advance();
		}

		if (FacingItr.IsValid())
		{
			NewVertex.CustomFacingVector = *FacingItr;
			FacingItr.Advance();
		}

		PosItr.Advance();
		VelItr.Advance();
		ColItr.Advance();
		AgeItr.Advance();
		RotItr.Advance();
		SizeItr.Advance();
	}

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
		delete static_cast<FNiagaraDynamicDataSprites*>(DynamicDataRender);
		DynamicDataRender = NULL;
	}
	DynamicDataRender = NewDynamicData;
}

int NiagaraEffectRendererSprites::GetDynamicDataSize()
{
	uint32 Size = sizeof(FNiagaraDynamicDataSprites);
	if (DynamicDataRender)
	{
		Size += (static_cast<FNiagaraDynamicDataSprites*>(DynamicDataRender))->VertexData.GetAllocatedSize();
	}

	return Size;
}

bool NiagaraEffectRendererSprites::HasDynamicData()
{
	return DynamicDataRender && static_cast<FNiagaraDynamicDataSprites*>(DynamicDataRender)->VertexData.Num() > 0;
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
