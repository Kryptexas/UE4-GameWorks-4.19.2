// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraRendererMeshes.h"
#include "ParticleResources.h"
#include "NiagaraMeshVertexFactory.h"
#include "NiagaraDataSet.h"
#include "NiagaraStats.h"
#include "Async/ParallelFor.h"
#include "Engine/StaticMesh.h"

DECLARE_CYCLE_STAT(TEXT("Generate Mesh Vertex Data"), STAT_NiagaraGenMeshVertexData, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Render Meshes"), STAT_NiagaraRenderMeshes, STATGROUP_Niagara);



extern int32 GbNiagaraParallelEmitterRenderers;




class FNiagaraMeshCollectorResourcesMesh : public FOneFrameResource
{
public:
	FNiagaraMeshVertexFactory VertexFactory;
	FNiagaraMeshUniformBufferRef UniformBuffer;

	virtual ~FNiagaraMeshCollectorResourcesMesh()
	{
		VertexFactory.ReleaseResource();
	}
};


NiagaraRendererMeshes::NiagaraRendererMeshes(ERHIFeatureLevel::Type FeatureLevel, UNiagaraRendererProperties *InProps) :
	NiagaraRenderer()
{
	//check(InProps);
	VertexFactory = ConstructNiagaraMeshVertexFactory(NVFT_Mesh, FeatureLevel, sizeof(FNiagaraMeshInstanceVertex), 0);
	Properties = Cast<UNiagaraMeshRendererProperties>(InProps);


	if (Properties && Properties->ParticleMesh)
	{
		if (Properties->bOverrideMaterials && Properties->OverrideMaterials.Num() != 0)
		{
			for (UMaterialInterface* Interface : Properties->OverrideMaterials)
			{
				if (Interface)
				{
					Interface->CheckMaterialUsage_Concurrent(MATUSAGE_NiagaraMeshParticles);
				}
			}
		}

		const FStaticMeshLODResources& LODModel = Properties->ParticleMesh->RenderData->LODResources[0];
		for (int32 SectionIndex = 0; SectionIndex < LODModel.Sections.Num(); SectionIndex++)
		{
			//FMaterialRenderProxy* MaterialProxy = MeshMaterials[SectionIndex]->GetRenderProxy(bSelected);
			const FStaticMeshSection& Section = LODModel.Sections[SectionIndex];
			UMaterialInterface* ParticleMeshMaterial = Properties->ParticleMesh->GetMaterial(Section.MaterialIndex);
			if (ParticleMeshMaterial && Properties->bOverrideMaterials == false)
			{
				if (ParticleMeshMaterial)
				{
					ParticleMeshMaterial->CheckMaterialUsage_Concurrent(MATUSAGE_NiagaraMeshParticles);
				}
			}
		}

		BaseExtents = Properties->ParticleMesh->GetBounds().BoxExtent;
	}

}

void NiagaraRendererMeshes::SetupVertexFactory(FNiagaraMeshVertexFactory *InVertexFactory, const FStaticMeshLODResources& LODResources) const
{
	FNiagaraMeshVertexFactory::FDataType Data;

	LODResources.VertexBuffers.PositionVertexBuffer.BindPositionVertexBuffer(InVertexFactory, Data);
	LODResources.VertexBuffers.StaticMeshVertexBuffer.BindTangentVertexBuffer(InVertexFactory, Data);
	LODResources.VertexBuffers.StaticMeshVertexBuffer.BindTexCoordVertexBuffer(InVertexFactory, Data, MAX_TEXCOORDS);
	LODResources.VertexBuffers.ColorVertexBuffer.BindColorVertexBuffer(InVertexFactory, Data);

	// Initialize instanced data. Vertex buffer and stride are set before render.
	// Particle color
	Data.ParticleColorComponent = FVertexStreamComponent(
		NULL,
		STRUCT_OFFSET(FNiagaraMeshInstanceVertex, Color),
		0,
		VET_Float4,
		EVertexStreamUsage::Instancing
		);

	// Particle transform matrix
	for (int32 MatrixRow = 0; MatrixRow < 3; MatrixRow++)
	{
		Data.TransformComponent[MatrixRow] = FVertexStreamComponent(
			NULL,
			STRUCT_OFFSET(FNiagaraMeshInstanceVertex, Transform) + sizeof(FVector4)* MatrixRow,
			0,
			VET_Float4,
			EVertexStreamUsage::Instancing
			);
	}

	Data.VelocityComponent = FVertexStreamComponent(
		NULL,
		STRUCT_OFFSET(FNiagaraMeshInstanceVertex, Velocity),
		0,
		VET_Float4,
		EVertexStreamUsage::Instancing
		);
	// SubUVs.
	Data.SubUVs = FVertexStreamComponent(
		NULL,
		STRUCT_OFFSET(FNiagaraMeshInstanceVertex, SubUVParams),
		0,
		VET_Short4,
		EVertexStreamUsage::Instancing
		);

	// Pack SubUV Lerp and the particle's relative time
	Data.SubUVLerpAndRelTime = FVertexStreamComponent(
		NULL,
		STRUCT_OFFSET(FNiagaraMeshInstanceVertex, SubUVLerp),
		0,
		VET_Float2,
		EVertexStreamUsage::Instancing
		);

	Data.bInitialized = true;
	InVertexFactory->SetData(Data);
}



void NiagaraRendererMeshes::ReleaseRenderThreadResources()
{
	VertexFactory->ReleaseResource();
	WorldSpacePrimitiveUniformBuffer.ReleaseResource();
}

void NiagaraRendererMeshes::CreateRenderThreadResources()
{
	VertexFactory->InitResource();
}


void NiagaraRendererMeshes::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector, const FNiagaraSceneProxy *SceneProxy) const
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraRender);
	SCOPE_CYCLE_COUNTER(STAT_NiagaraRenderMeshes);

	SimpleTimer MeshElementsTimer;

	FNiagaraDynamicDataMesh *DynamicDataMesh = (static_cast<FNiagaraDynamicDataMesh*>(DynamicDataRender));
	//if (!DynamicDataMesh || !DynamicDataMesh->DataSet || DynamicDataMesh->DataSet->GetNumInstances()==0)
	if (!DynamicDataMesh 
		|| DynamicDataMesh->DataSet->CurrDataRender().GetNumInstancesAllocated() == 0
		|| DynamicDataMesh->DataSet->CurrDataRender().GetNumInstances() == 0
		|| DynamicDataMesh->DataSet->CurrDataRender().GetGPUBufferFloat() == nullptr
		|| nullptr == Properties)
	{
		return;
	}

	//check(DynamicDataMesh->DataSet->PrevDataRender().GetNumInstances() > 0);
	//check(DynamicDataMesh->DataSet->PrevDataRender().GetNumInstancesAllocated() > 0);
	//check(DynamicDataMesh->DataSet->PrevData().GetGPUBufferFloat()->NumBytes > 0 || DynamicDataMesh->DataSet->PrevData().GetGPUBufferInt()->NumBytes > 0)
	
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
				false,
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
				const FSceneView* View = Views[ViewIndex];
				const FStaticMeshLODResources& LODModel = Properties->ParticleMesh->RenderData->LODResources[0];

				FNiagaraMeshCollectorResourcesMesh& CollectorResources = Collector.AllocateOneFrameResource<FNiagaraMeshCollectorResourcesMesh>();
				SetupVertexFactory(&CollectorResources.VertexFactory, LODModel);
				FNiagaraMeshUniformParameters PerViewUniformParameters;// = UniformParameters;
				PerViewUniformParameters.LocalToWorld = bLocalSpace ? SceneProxy->GetLocalToWorld() : FMatrix::Identity;//For now just handle local space like this but maybe in future have a VF variant to avoid the transform entirely?
				PerViewUniformParameters.LocalToWorldInverseTransposed = bLocalSpace ? SceneProxy->GetLocalToWorld().Inverse().GetTransposed() : FMatrix::Identity;
				PerViewUniformParameters.PrevTransformAvailable = false;
				PerViewUniformParameters.DeltaSeconds = ViewFamily.DeltaWorldTime;
				PerViewUniformParameters.PositionDataOffset = DynamicDataMesh->PositionDataOffset;
				PerViewUniformParameters.VelocityDataOffset = DynamicDataMesh->VelocityDataOffset;
				PerViewUniformParameters.ColorDataOffset = DynamicDataMesh->ColorDataOffset;
				PerViewUniformParameters.TransformDataOffset = DynamicDataMesh->TransformDataOffset;
				PerViewUniformParameters.ScaleDataOffset = DynamicDataMesh->ScaleDataOffset;
				PerViewUniformParameters.SizeDataOffset = DynamicDataMesh->SizeDataOffset;
				PerViewUniformParameters.MaterialParamDataOffset = DynamicDataMesh->MaterialParamDataOffset;
				/*
				if (Properties)
				{
				PerViewUniformParameters.SubImageSize = FVector4(Properties->SubImageInfo.X, Properties->SubImageInfo.Y, 1.0f / Properties->SubImageInfo.X, 1.0f / Properties->SubImageInfo.Y);
				}
				*/
				CollectorResources.VertexFactory.SetParticleData(DynamicDataMesh->DataSet);

				// Collector.AllocateOneFrameResource uses default ctor, initialize the vertex factory
				CollectorResources.VertexFactory.SetParticleFactoryType(NVFT_Mesh);
				CollectorResources.VertexFactory.SetMeshFacingMode((uint32)Properties->FacingMode);
				CollectorResources.UniformBuffer = FNiagaraMeshUniformBufferRef::CreateUniformBufferImmediate(PerViewUniformParameters, UniformBuffer_SingleFrame);
				CollectorResources.VertexFactory.SetStrides(0, 0);

				CollectorResources.VertexFactory.InitResource();
				CollectorResources.VertexFactory.SetUniformBuffer(CollectorResources.UniformBuffer);
			
				const bool bIsWireframe = AllowDebugViewmodes() && View->Family->EngineShowFlags.Wireframe;

				for (int32 SectionIndex = 0; SectionIndex < LODModel.Sections.Num(); SectionIndex++)
				{
					const FStaticMeshSection& Section = LODModel.Sections[SectionIndex];
					UMaterialInterface* ParticleMeshMaterial = Properties->ParticleMesh->GetMaterial(Section.MaterialIndex);
					if (!ParticleMeshMaterial)
					{
						continue;
					}
					FMaterialRenderProxy* MaterialProxy = nullptr;
					
					if (Properties->bOverrideMaterials && Properties->OverrideMaterials.Num() > Section.MaterialIndex && 
						Properties->OverrideMaterials[Section.MaterialIndex] != nullptr)
					{
						MaterialProxy = Properties->OverrideMaterials[Section.MaterialIndex]->GetRenderProxy(false, false);
					}

					if (MaterialProxy == nullptr)
					{
						MaterialProxy = ParticleMeshMaterial->GetRenderProxy(false, false);
					}

					if ((Section.NumTriangles == 0) || (MaterialProxy == NULL))
					{
						//@todo. This should never occur, but it does occasionally.
						continue;
					}


					FMeshBatch& Mesh = Collector.AllocateMesh();
					Mesh.VertexFactory = &CollectorResources.VertexFactory;
					Mesh.LCI = NULL;
					Mesh.ReverseCulling = SceneProxy->IsLocalToWorldDeterminantNegative();
					Mesh.CastShadow = SceneProxy->CastsDynamicShadow();
					Mesh.DepthPriorityGroup = (ESceneDepthPriorityGroup)SceneProxy->GetDepthPriorityGroup(View);

					FMeshBatchElement& BatchElement = Mesh.Elements[0];
					BatchElement.PrimitiveUniformBufferResource = &WorldSpacePrimitiveUniformBuffer;
					BatchElement.FirstIndex = 0;
					BatchElement.MinVertexIndex = 0;
					BatchElement.MaxVertexIndex = 0;
					BatchElement.NumInstances = DynamicDataMesh->DataSet->CurrDataRender().GetNumInstances();
					BatchElement.IndirectArgsBuffer = DynamicDataMesh->DataSet->GetDataSetIndices().Buffer;

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

					Mesh.bCanApplyViewModeOverrides = true;
					Mesh.bUseWireframeSelectionColoring = SceneProxy->IsSelected();
					Collector.AddMesh(ViewIndex, Mesh);
				}
			}
		}
	}

	CPUTimeMS += MeshElementsTimer.GetElapsedMilliseconds();
}



bool NiagaraRendererMeshes::SetMaterialUsage()
{
	//Causes deadlock :S Need to look at / rework the setting of materials and render modules.
	return Material && Material->CheckMaterialUsage_Concurrent(MATUSAGE_NiagaraMeshParticles);
}



/** Update render data buffer from attributes */
FNiagaraDynamicDataBase *NiagaraRendererMeshes::GenerateVertexData(const FNiagaraSceneProxy* Proxy, FNiagaraDataSet &Data, const ENiagaraSimTarget Target)
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraGenMeshVertexData);

	if (!Properties || Properties->ParticleMesh == nullptr || !bEnabled || Data.CurrData().GetNumInstances() == 0)
	{
		return nullptr;
	}

	SimpleTimer VertexDataTimer;
//	TArray<FNiagaraMeshInstanceVertex>& RenderData = DynamicData->VertexData;
	//TArray< FNiagaraMeshInstanceVertexDynamicParameter>& RenderMaterialVertexData = DynamicData->MaterialParameterVertexData;


	const FNiagaraVariableLayoutInfo* PositionLayout = Data.GetVariableLayout(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Position")));
	const FNiagaraVariableLayoutInfo* VelocityLayout = Data.GetVariableLayout(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Velocity")));
	const FNiagaraVariableLayoutInfo* ColorLayout = Data.GetVariableLayout(FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(), TEXT("Color")));
	const FNiagaraVariableLayoutInfo* XformLayout = Data.GetVariableLayout(FNiagaraVariable(FNiagaraTypeDefinition::GetVec4Def(), TEXT("MeshOrientation")));
	const FNiagaraVariableLayoutInfo* ScaleLayout = Data.GetVariableLayout(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Scale")));
	const FNiagaraVariableLayoutInfo* SizeLayout = Data.GetVariableLayout(FNiagaraVariable(FNiagaraTypeDefinition::GetVec2Def(), TEXT("SpriteSize")));
	const FNiagaraVariableLayoutInfo* MatParamLayout = Data.GetVariableLayout(FNiagaraVariable(FNiagaraTypeDefinition::GetVec4Def(), TEXT("DynamicMaterialParameter")));

	//Bail if we don't have the required attributes to render this emitter.
	if (!bEnabled || !PositionLayout /*|| !XformLayout*/ || !VelocityLayout)
	{
		return nullptr;
	}


	UStaticMesh *Mesh = nullptr;
	if (Properties)
	{
		Mesh = Properties->ParticleMesh;
	}

	// required attributes
	FNiagaraDynamicDataMesh *DynamicData = new FNiagaraDynamicDataMesh;
	DynamicData->PositionDataOffset = PositionLayout->FloatComponentStart;
	DynamicData->VelocityDataOffset = VelocityLayout->FloatComponentStart;

	// optional attributes
	int32 IntDummy;
	Data.GetVariableComponentOffsets(FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(), TEXT("Color")), DynamicData->ColorDataOffset, IntDummy);
	Data.GetVariableComponentOffsets(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Scale")), DynamicData->ScaleDataOffset, IntDummy);
	Data.GetVariableComponentOffsets(FNiagaraVariable(FNiagaraTypeDefinition::GetVec2Def(), TEXT("SpriteSize")), DynamicData->SizeDataOffset, IntDummy);
	Data.GetVariableComponentOffsets(FNiagaraVariable(FNiagaraTypeDefinition::GetVec4Def(), TEXT("DynamicMaterialParameter")), DynamicData->MaterialParamDataOffset, IntDummy);
	Data.GetVariableComponentOffsets(FNiagaraVariable(FNiagaraTypeDefinition::GetVec4Def(), TEXT("MeshOrientation")), DynamicData->TransformDataOffset, IntDummy);

	// if we're CPU simulating, need to init the GPU buffers for the vertex factory
	if (Target == ENiagaraSimTarget::CPUSim)
	{
		//Data.ValidateBufferIndices();
		Data.InitGPUFromCPU();
	}
	else
	{
		check(Data.GetDataSetIndices().Buffer != nullptr);
		Data.InitGPUSimSRVs();
	}

	DynamicData->DataSet = &Data;
	CPUTimeMS = VertexDataTimer.GetElapsedMilliseconds();
	return DynamicData;  
}



void NiagaraRendererMeshes::SetDynamicData_RenderThread(FNiagaraDynamicDataBase* NewDynamicData)
{
	check(IsInRenderingThread());

	if (DynamicDataRender)
	{
		delete static_cast<FNiagaraDynamicDataMesh*>(DynamicDataRender);
		DynamicDataRender = NULL;
	}
	DynamicDataRender = NewDynamicData;
}

int NiagaraRendererMeshes::GetDynamicDataSize()
{
	uint32 Size = sizeof(FNiagaraDynamicDataMesh);
	if (DynamicDataRender && static_cast<FNiagaraDynamicDataMesh*>(DynamicDataRender)->DataSet)
	{
		//Size += (static_cast<FNiagaraDynamicDataMesh*>(DynamicDataRender))->DataSet->PrevDataRender().GetNumInstances() * sizeof(float);
	}

	return Size;
}

bool NiagaraRendererMeshes::HasDynamicData()
{
	return DynamicDataRender != nullptr;
}

#if WITH_EDITORONLY_DATA

const TArray<FNiagaraVariable>& NiagaraRendererMeshes::GetRequiredAttributes()
{
	return Properties->GetRequiredAttributes();
}

const TArray<FNiagaraVariable>& NiagaraRendererMeshes::GetOptionalAttributes()
{
	return Properties->GetOptionalAttributes();
}

#endif