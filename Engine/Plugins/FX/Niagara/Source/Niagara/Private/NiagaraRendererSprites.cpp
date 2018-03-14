// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraRenderer.h"
#include "ParticleResources.h"
#include "NiagaraSpriteVertexFactory.h"
#include "NiagaraDataSet.h"
#include "NiagaraStats.h"

DECLARE_CYCLE_STAT(TEXT("Generate Sprite Vertex Data"), STAT_NiagaraGenSpriteVertexData, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Render Sprites"), STAT_NiagaraRenderSprites, STATGROUP_Niagara);

DECLARE_CYCLE_STAT(TEXT("Genereate GPU Buffers"), STAT_NiagaraGenSpriteGpuBuffers, STATGROUP_Niagara);

struct FNiagaraDynamicDataSprites : public FNiagaraDynamicDataBase
{
	const FNiagaraDataSet *DataSet;
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



NiagaraRendererSprites::NiagaraRendererSprites(ERHIFeatureLevel::Type FeatureLevel, UNiagaraRendererProperties *InProps) 
	: NiagaraRenderer()
	, PositionOffset(INDEX_NONE)
	, VelocityOffset(INDEX_NONE)
	, RotationOffset(INDEX_NONE)
	, SizeOffset(INDEX_NONE)
	, ColorOffset(INDEX_NONE)
	, FacingOffset(INDEX_NONE)
	, AlignmentOffset(INDEX_NONE)
	, SubImageOffset(INDEX_NONE)
	, MaterialParamOffset(INDEX_NONE)
	, CameraOffsetOffset(INDEX_NONE)
	, UVScaleOffset(INDEX_NONE)
	, ParticleRandomOffset(INDEX_NONE)
{
	//check(InProps);
	VertexFactory = new FNiagaraSpriteVertexFactory(NVFT_Sprite, FeatureLevel);
	Properties = Cast<UNiagaraSpriteRendererProperties>(InProps);
	BaseExtents = FVector(0.5f, 0.5f, 0.5f);
}


void NiagaraRendererSprites::ReleaseRenderThreadResources()
{
	VertexFactory->ReleaseResource();
	WorldSpacePrimitiveUniformBuffer.ReleaseResource();
}

void NiagaraRendererSprites::CreateRenderThreadResources()
{
	VertexFactory->SetNumVertsInInstanceBuffer(4);
	VertexFactory->InitResource();
}

void NiagaraRendererSprites::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector, const FNiagaraSceneProxy *SceneProxy) const
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraRender);
	SCOPE_CYCLE_COUNTER(STAT_NiagaraRenderSprites);

	SimpleTimer MeshElementsTimer;

	//check(DynamicDataRender)

	FNiagaraDynamicDataSprites *DynamicDataSprites = static_cast<FNiagaraDynamicDataSprites*>(DynamicDataRender);

	if (!DynamicDataSprites
		|| DynamicDataSprites->DataSet->CurrDataRender().GetNumInstancesAllocated() == 0
		|| DynamicDataSprites->DataSet->CurrDataRender().GetNumInstances() == 0
		|| DynamicDataSprites->DataSet->CurrDataRender().GetGPUBufferFloat() == nullptr
		|| nullptr == Properties
		)
	{
		return;
	}

	int32 NumInstances = DynamicDataSprites->DataSet->CurrDataRender().GetNumInstances();

	const bool bIsWireframe = ViewFamily.EngineShowFlags.Wireframe;
	FMaterialRenderProxy* MaterialRenderProxy = Material->GetRenderProxy(SceneProxy->IsSelected(), SceneProxy->IsHovered());

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
			SceneProxy->GetScene().HasPrecomputedVolumetricLightmap_RenderThread(),
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
			{
				const FSceneView* View = Views[ViewIndex];

				FNiagaraMeshCollectorResourcesSprite& CollectorResources = Collector.AllocateOneFrameResource<FNiagaraMeshCollectorResourcesSprite>();
				FNiagaraSpriteUniformParameters PerViewUniformParameters;// = UniformParameters;
				PerViewUniformParameters.LocalToWorld = bLocalSpace ? SceneProxy->GetLocalToWorld() : FMatrix::Identity;//For now just handle local space like this but maybe in future have a VF variant to avoid the transform entirely?
				PerViewUniformParameters.LocalToWorldInverseTransposed = bLocalSpace ? SceneProxy->GetLocalToWorld().Inverse().GetTransposed() : FMatrix::Identity;
				PerViewUniformParameters.RotationBias = 0.0f;
				PerViewUniformParameters.RotationScale = 1.0f;
				PerViewUniformParameters.TangentSelector = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
				PerViewUniformParameters.InvDeltaSeconds = 30.0f;
				PerViewUniformParameters.NormalsType = 0.0f;
				PerViewUniformParameters.NormalsSphereCenter = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
				PerViewUniformParameters.NormalsCylinderUnitDirection = FVector4(0.0f, 0.0f, 1.0f, 0.0f);
				PerViewUniformParameters.PivotOffset = Properties->PivotInUVSpace * -1.0f; // We do this because we want to slide the coordinates back since 0,0 is the upper left corner.
				PerViewUniformParameters.MacroUVParameters = FVector4(0.0f, 0.0f, 1.0f, 1.0f);
				PerViewUniformParameters.CameraFacingBlend = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
				PerViewUniformParameters.RemoveHMDRoll = Properties->bRemoveHMDRollInVR;
				PerViewUniformParameters.CustomFacingVectorMask = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
				PerViewUniformParameters.SubImageSize = FVector4(Properties->SubImageSize.X, Properties->SubImageSize.Y, 1.0f / Properties->SubImageSize.X, 1.0f / Properties->SubImageSize.Y);
				PerViewUniformParameters.PositionDataOffset = PositionOffset;
				PerViewUniformParameters.VelocityDataOffset = VelocityOffset;
				PerViewUniformParameters.RotationDataOffset = RotationOffset;
				PerViewUniformParameters.SizeDataOffset = SizeOffset;
				PerViewUniformParameters.ColorDataOffset = ColorOffset;
				PerViewUniformParameters.MaterialParamDataOffset = MaterialParamOffset;
				PerViewUniformParameters.SubimageDataOffset = SubImageOffset;
				PerViewUniformParameters.FacingOffset = FacingOffset;
				PerViewUniformParameters.AlignmentOffset = AlignmentOffset;
				PerViewUniformParameters.SubImageBlendMode = Properties->bSubImageBlend;
				PerViewUniformParameters.CameraOffsetOffset = CameraOffsetOffset;
				PerViewUniformParameters.UVScaleOffset = UVScaleOffset;
				PerViewUniformParameters.ParticleRandomOffset = ParticleRandomOffset;

				// Collector.AllocateOneFrameResource uses default ctor, initialize the vertex factory
				ENiagaraSpriteFacingMode FacingMode = Properties->FacingMode;
				ENiagaraSpriteAlignment AlignmentMode = Properties->Alignment;

				if (FacingOffset == -1 && FacingMode == ENiagaraSpriteFacingMode::CustomFacingVector)
				{
					FacingMode = ENiagaraSpriteFacingMode::FaceCamera;
				}

				if (AlignmentOffset == -1 && AlignmentMode == ENiagaraSpriteAlignment::CustomAlignment)
				{
					AlignmentMode = ENiagaraSpriteAlignment::Unaligned;
				}

				if (FacingMode == ENiagaraSpriteFacingMode::FaceCameraDistanceBlend)
				{
					float DistanceBlendMinSq = Properties->MinFacingCameraBlendDistance * Properties->MinFacingCameraBlendDistance;
					float DistanceBlendMaxSq = Properties->MaxFacingCameraBlendDistance * Properties->MaxFacingCameraBlendDistance;
					float InvBlendRange = 1.0f / FMath::Max(DistanceBlendMaxSq - DistanceBlendMinSq, 1.0f);
					float BlendScaledMinDistance = DistanceBlendMinSq * InvBlendRange;

					PerViewUniformParameters.CameraFacingBlend.X = 1.0f;
					PerViewUniformParameters.CameraFacingBlend.Y = InvBlendRange;
					PerViewUniformParameters.CameraFacingBlend.Z = BlendScaledMinDistance;
				}

				if (Properties->Alignment == ENiagaraSpriteAlignment::VelocityAligned)
				{
					// velocity aligned
					PerViewUniformParameters.RotationScale = 0.0f;
					PerViewUniformParameters.TangentSelector = FVector4(0.0f, 1.0f, 0.0f, 0.0f);
				}

				if (Properties->FacingMode == ENiagaraSpriteFacingMode::CustomFacingVector)
				{
					PerViewUniformParameters.CustomFacingVectorMask = Properties->CustomFacingVectorMask;
				}

				CollectorResources.VertexFactory.SetParticleData(DynamicDataSprites->DataSet);
				CollectorResources.VertexFactory.SetAlignmentMode((uint32)AlignmentMode);
				CollectorResources.VertexFactory.SetFacingMode((uint32)FacingMode);

				CollectorResources.VertexFactory.SetParticleFactoryType(NVFT_Sprite);


				CollectorResources.UniformBuffer = FNiagaraSpriteUniformBufferRef::CreateUniformBufferImmediate(PerViewUniformParameters, UniformBuffer_SingleFrame);

				CollectorResources.VertexFactory.SetNumVertsInInstanceBuffer(4);
				CollectorResources.VertexFactory.InitResource();
				CollectorResources.VertexFactory.SetSpriteUniformBuffer(CollectorResources.UniformBuffer);

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
				MeshElement.NumInstances = FMath::Max(0, NumInstances);	//->VertexData.Num();
				MeshElement.MinVertexIndex = 0;
				MeshElement.MaxVertexIndex = 0;// MeshElement.NumInstances * 4 - 1;
				MeshElement.PrimitiveUniformBufferResource = &WorldSpacePrimitiveUniformBuffer;
				MeshElement.IndirectArgsBuffer = DynamicDataSprites->DataSet->GetDataSetIndices().Buffer;
				
				Collector.AddMesh(ViewIndex, MeshBatch);				
			}
		}
	}

	CPUTimeMS += MeshElementsTimer.GetElapsedMilliseconds();
}

bool NiagaraRendererSprites::SetMaterialUsage()
{
	//Causes deadlock :S Need to look at / rework the setting of materials and render modules.
	return Material && Material->CheckMaterialUsage_Concurrent(MATUSAGE_NiagaraSprites);
}

/** Update render data buffer from attributes */
FNiagaraDynamicDataBase *NiagaraRendererSprites::GenerateVertexData(const FNiagaraSceneProxy* Proxy, FNiagaraDataSet &Data, const ENiagaraSimTarget Target)
{
	FNiagaraDynamicDataSprites *DynamicData = nullptr;

	if (Data.CurrData().GetNumInstances() > 0 && (Target != ENiagaraSimTarget::CPUSim || (Target == ENiagaraSimTarget::CPUSim && Data.CurrData().GetSizeBytes() != 0)))
	{
		SimpleTimer VertexDataTimer;

		SCOPE_CYCLE_COUNTER(STAT_NiagaraGenSpriteVertexData);

		if (PositionOffset == INDEX_NONE)
		{
			const FNiagaraVariableLayoutInfo* PositionLayout = Data.GetVariableLayout(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Position")));
			const FNiagaraVariableLayoutInfo* VelocityLayout = Data.GetVariableLayout(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Velocity")));
			const FNiagaraVariableLayoutInfo* RotationLayout = Data.GetVariableLayout(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("SpriteRotation")));
			const FNiagaraVariableLayoutInfo* SizeLayout = Data.GetVariableLayout(FNiagaraVariable(FNiagaraTypeDefinition::GetVec2Def(), TEXT("SpriteSize")));
			const FNiagaraVariableLayoutInfo* ColorLayout = Data.GetVariableLayout(FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(), TEXT("Color")));
			PositionOffset = PositionLayout->FloatComponentStart;
			VelocityOffset = VelocityLayout->FloatComponentStart;
			RotationOffset = RotationLayout->FloatComponentStart;
			SizeOffset = SizeLayout->FloatComponentStart;
			ColorOffset = ColorLayout->FloatComponentStart;

			// optional attributes; we pass -1 as the offset so the VF can branch
			int32 IntDummy;
			Data.GetVariableComponentOffsets(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("SpriteFacing")), FacingOffset, IntDummy);
			Data.GetVariableComponentOffsets(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("SpriteAlignment")), AlignmentOffset, IntDummy);
			Data.GetVariableComponentOffsets(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("SubImageIndex")), SubImageOffset, IntDummy);
			Data.GetVariableComponentOffsets(FNiagaraVariable(FNiagaraTypeDefinition::GetVec4Def(), TEXT("DynamicMaterialParameter")), MaterialParamOffset, IntDummy);
			Data.GetVariableComponentOffsets(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("CameraOffset")), CameraOffsetOffset, IntDummy);
			Data.GetVariableComponentOffsets(FNiagaraVariable(FNiagaraTypeDefinition::GetVec2Def(), TEXT("UVScale")), UVScaleOffset, IntDummy);
			Data.GetVariableComponentOffsets(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("MaterialRandom")), ParticleRandomOffset, IntDummy);
		}
		
		if (!bEnabled || PositionOffset == INDEX_NONE || VelocityOffset == INDEX_NONE || RotationOffset == INDEX_NONE || SizeOffset == INDEX_NONE || ColorOffset == INDEX_NONE)
		{
			return nullptr;
		}
		
		DynamicData = new FNiagaraDynamicDataSprites;
		// if we're CPU simulating, need to init the GPU buffers for the vertex factory
			//This is a killer for perf. Assume its all the GPU buffer allocation.
			//Need to allocate big shared buffer and allocate ranges out of those.
		SCOPE_CYCLE_COUNTER(STAT_NiagaraGenSpriteGpuBuffers);
		if (Target == ENiagaraSimTarget::CPUSim)
		{
			Data.InitGPUFromCPU();
		}
		else
		{
			check(Data.GetDataSetIndices().Buffer != nullptr);
			Data.InitGPUSimSRVs();
		}

		DynamicData->DataSet = &Data;
		CPUTimeMS = VertexDataTimer.GetElapsedMilliseconds();
	}

	return DynamicData;  // for VF that can fetch from particle data directly
}



void NiagaraRendererSprites::SetDynamicData_RenderThread(FNiagaraDynamicDataBase* NewDynamicData)
{
	check(IsInRenderingThread());

	if (DynamicDataRender)
	{
		delete static_cast<FNiagaraDynamicDataSprites*>(DynamicDataRender);
		DynamicDataRender = NULL;
	}
	DynamicDataRender = NewDynamicData;
}

int NiagaraRendererSprites::GetDynamicDataSize()
{
	uint32 Size = sizeof(FNiagaraDynamicDataSprites);
	return Size;
}

bool NiagaraRendererSprites::HasDynamicData()
{
//	return DynamicDataRender && static_cast<FNiagaraDynamicDataSprites*>(DynamicDataRender)->VertexData.Num() > 0;
	return DynamicDataRender!=nullptr;// && static_cast<FNiagaraDynamicDataSprites*>(DynamicDataRender)->NumInstances > 0;
}

#if WITH_EDITORONLY_DATA

const TArray<FNiagaraVariable>& NiagaraRendererSprites::GetRequiredAttributes()
{
	return Properties->GetRequiredAttributes();
}

const TArray<FNiagaraVariable>& NiagaraRendererSprites::GetOptionalAttributes()
{
	return Properties->GetOptionalAttributes();
}

#endif