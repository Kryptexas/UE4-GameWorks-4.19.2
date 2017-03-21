// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEffectRendererMeshes.h"
#include "Particles/ParticleResources.h"
#include "NiagaraMeshVertexFactory.h"
#include "NiagaraDataSet.h"
#include "NiagaraStats.h"
#include "Async/ParallelFor.h"

DECLARE_CYCLE_STAT(TEXT("Generate Mesh Vertex Data"), STAT_NiagaraGenMeshVertexData, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Render Meshes"), STAT_NiagaraRenderMeshes, STATGROUP_Niagara);



extern int32 GbNiagaraParallelEffectRenderers;




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


NiagaraEffectRendererMeshes::NiagaraEffectRendererMeshes(ERHIFeatureLevel::Type FeatureLevel, UNiagaraEffectRendererProperties *InProps) :
	NiagaraEffectRenderer()
{
	//check(InProps);
	VertexFactory = ConstructNiagaraMeshVertexFactory(NVFT_Mesh, FeatureLevel, sizeof(FNiagaraMeshInstanceVertex), 0);
	Properties = Cast<UNiagaraMeshRendererProperties>(InProps);


	if (Properties && Properties->ParticleMesh)
	{
		const FStaticMeshLODResources& LODModel = Properties->ParticleMesh->RenderData->LODResources[0];
		for (int32 SectionIndex = 0; SectionIndex < LODModel.Sections.Num(); SectionIndex++)
		{
			//FMaterialRenderProxy* MaterialProxy = MeshMaterials[SectionIndex]->GetRenderProxy(bSelected);
			const FStaticMeshSection& Section = LODModel.Sections[SectionIndex];
			UMaterialInterface* ParticleMeshMaterial = Properties->ParticleMesh->GetMaterial(Section.MaterialIndex);
			if (ParticleMeshMaterial)
			{
				ParticleMeshMaterial->CheckMaterialUsage_Concurrent(MATUSAGE_MeshParticles);
			}
		}
	}

}

void NiagaraEffectRendererMeshes::SetupVertexFactory(FNiagaraMeshVertexFactory *InVertexFactory, const FStaticMeshLODResources& LODResources) const
{
	FNiagaraMeshVertexFactory::FDataType Data;

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
		STRUCT_OFFSET(FNiagaraMeshInstanceVertex, Color),
		0,
		VET_Float4,
		true
		);

	// Particle transform matrix
	for (int32 MatrixRow = 0; MatrixRow < 3; MatrixRow++)
	{
		Data.TransformComponent[MatrixRow] = FVertexStreamComponent(
			NULL,
			STRUCT_OFFSET(FNiagaraMeshInstanceVertex, Transform) + sizeof(FVector4)* MatrixRow,
			0,
			VET_Float4,
			true
			);
	}

	Data.VelocityComponent = FVertexStreamComponent(
		NULL,
		STRUCT_OFFSET(FNiagaraMeshInstanceVertex, Velocity),
		0,
		VET_Float4,
		true
		);

	// SubUVs.
	Data.SubUVs = FVertexStreamComponent(
		NULL,
		STRUCT_OFFSET(FNiagaraMeshInstanceVertex, SubUVParams),
		0,
		VET_Short4,
		true
		);

	// Pack SubUV Lerp and the particle's relative time
	Data.SubUVLerpAndRelTime = FVertexStreamComponent(
		NULL,
		STRUCT_OFFSET(FNiagaraMeshInstanceVertex, SubUVLerp),
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

	FNiagaraDynamicDataMesh *DynamicDataMesh = (static_cast<FNiagaraDynamicDataMesh*>(DynamicDataRender));
	if (!DynamicDataMesh || DynamicDataMesh->VertexData.Num() == 0)
	{
		return;
	}

	int32 SizeInBytes = DynamicDataMesh->VertexData.GetTypeSize() * DynamicDataMesh->VertexData.Num();
	FGlobalDynamicVertexBuffer::FAllocation LocalDynamicVertexAllocation = FGlobalDynamicVertexBuffer::Get().Allocate(SizeInBytes);
	FGlobalDynamicVertexBuffer::FAllocation LocalDynamicVertexMaterialParamsAllocation;

	if (DynamicDataMesh->MaterialParameterVertexData.Num() > 0)
	{
		int32 MatParamSizeInBytes = DynamicDataMesh->MaterialParameterVertexData.GetTypeSize() * DynamicDataMesh->MaterialParameterVertexData.Num();
		LocalDynamicVertexMaterialParamsAllocation = FGlobalDynamicVertexBuffer::Get().Allocate(MatParamSizeInBytes);

		if (LocalDynamicVertexMaterialParamsAllocation.IsValid())
		{
			// Copy the vertex data over.
			FMemory::Memcpy(LocalDynamicVertexMaterialParamsAllocation.Buffer, DynamicDataMesh->MaterialParameterVertexData.GetData(), MatParamSizeInBytes);
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
		FMemory::Memcpy(LocalDynamicVertexAllocation.Buffer, DynamicDataMesh->VertexData.GetData(), SizeInBytes);

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
				PerViewUniformParameters.PrevTransformAvailable = false;
				PerViewUniformParameters.DeltaSeconds = ViewFamily.DeltaWorldTime;
				/*
				if (Properties)
				{
				PerViewUniformParameters.SubImageSize = FVector4(Properties->SubImageInfo.X, Properties->SubImageInfo.Y, 1.0f / Properties->SubImageInfo.X, 1.0f / Properties->SubImageInfo.Y);
				}
				*/

				// Collector.AllocateOneFrameResource uses default ctor, initialize the vertex factory
				CollectorResources.VertexFactory.SetFeatureLevel(ViewFamily.GetFeatureLevel());
				CollectorResources.VertexFactory.SetParticleFactoryType(NVFT_Mesh);
				CollectorResources.UniformBuffer = FNiagaraMeshUniformBufferRef::CreateUniformBufferImmediate(PerViewUniformParameters, UniformBuffer_SingleFrame);
				CollectorResources.VertexFactory.SetStrides(sizeof(FNiagaraMeshInstanceVertex), 0);

				CollectorResources.VertexFactory.InitResource();
				CollectorResources.VertexFactory.SetUniformBuffer(CollectorResources.UniformBuffer);
				CollectorResources.VertexFactory.SetInstanceBuffer(
					LocalDynamicVertexAllocation.VertexBuffer,
					LocalDynamicVertexAllocation.VertexOffset,
					sizeof(FNiagaraMeshInstanceVertex)
					);

				if (LocalDynamicVertexMaterialParamsAllocation.IsValid() && DynamicDataMesh->MaterialParameterVertexData.Num() > 0)
				{
					CollectorResources.VertexFactory.SetDynamicParameterBuffer(LocalDynamicVertexMaterialParamsAllocation.VertexBuffer, LocalDynamicVertexMaterialParamsAllocation.VertexOffset,
						sizeof(FNiagaraMeshInstanceVertexDynamicParameter));
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
					if (!ParticleMeshMaterial)
					{
						continue;
					}
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
					BatchElement.NumInstances = DynamicDataMesh->VertexData.Num();

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
					FNiagaraMeshVertexFactory::FBatchParametersCPU& BatchParameters = Collector.AllocateOneFrameResource<FNiagaraMeshVertexFactory::FBatchParametersCPU>();
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
	FNiagaraDynamicDataMesh *DynamicData = new FNiagaraDynamicDataMesh;
	TArray<FNiagaraMeshInstanceVertex>& RenderData = DynamicData->VertexData;
	TArray< FNiagaraMeshInstanceVertexDynamicParameter>& RenderMaterialVertexData = DynamicData->MaterialParameterVertexData;

	RenderData.Reset(Data.GetNumInstances());

	FNiagaraDataSetIterator<FVector> PosItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Position")));
	FNiagaraDataSetIterator<FVector> VelItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Velocity")));
	FNiagaraDataSetIterator<FLinearColor> ColItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(), TEXT("Color")));
	FNiagaraDataSetIterator<float> AgeItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Age")));
	FNiagaraDataSetIterator<FVector4> XFormItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec4Def(), TEXT("Transform")));
	FNiagaraDataSetIterator<FVector> ScaleItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Scale")));
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
	FVector MatrixScale = LocalToWorld.ExtractScaling();

	UStaticMesh *Mesh = nullptr;
	if (Properties)
	{
		Mesh = Properties->ParticleMesh;
	}
	RenderData.AddUninitialized(Data.GetNumInstances());


	// setup number of threads, assuming 3000 particles per thread max
	uint32 NumThreads = 1;
	uint32 ParticlesPerThread = Data.GetNumInstances();
	if (GbNiagaraParallelEffectRenderers)
	{
		ParticlesPerThread = FMath::Min(3000U, Data.GetNumInstances());
		NumThreads = (Data.GetNumInstances() + ParticlesPerThread - 1) / ParticlesPerThread;
	}

	// generate vertex buffer
	ParallelFor(NumThreads,
		[&](int32 ThreadIdx)
	{
		uint32 StartIndex = ThreadIdx * ParticlesPerThread;
		uint32 EndIndex = FMath::Min(StartIndex + ParticlesPerThread, Data.GetNumInstances());

		for (uint32 ParticleIndex = StartIndex; ParticleIndex < EndIndex; ParticleIndex++)
		{
			FVector4 Pos = PosItr.GetAt(ParticleIndex);
			FLinearColor Col = ColItr.GetAt(ParticleIndex);
			float Age = AgeItr.GetAt(ParticleIndex);

			FNiagaraMeshInstanceVertex& NewVertex = RenderData[ParticleIndex];
			NewVertex.Color = Col;
			NewVertex.RelativeTime = Age;

			FRotationMatrix RotMat = FRotationMatrix(FRotator());
			FVector Scale = FVector(1.0f, 1.0f, 1.0f);
			if (XFormItr.IsValid())
			{
				FVector4 XForm = XFormItr.GetAt(ParticleIndex);
				FRotator Rotator = XForm.ToOrientationRotator();
				RotMat = Rotator;

				FRotator Rot(0.0f, 0.0f, XForm.W);
				FRotationMatrix AxisRot(Rot);
				AxisRot *= RotMat;
				RotMat = AxisRot;
				Scale = FVector(XForm.W, XForm.W, XForm.W);
			}

			if (ScaleItr.IsValid())
			{
				Scale = ScaleItr.GetAt(ParticleIndex);
			}
			else if (SizeItr.IsValid())
			{
				FVector2D Size = SizeItr.GetAt(ParticleIndex);
				Scale = FVector(Size.X, Size.X, Size.X);
			}

			if (bLocalSpace)
			{
				Scale *= MatrixScale;
				RotMat *= LocalToWorld;
				Pos = LocalToWorld.TransformPosition(Pos);
			}

			NewVertex.Transform[0].X = RotMat.GetColumn(0).X*Scale.X;  NewVertex.Transform[0].Y = RotMat.GetColumn(0).Y*Scale.Y;   NewVertex.Transform[0].Z = RotMat.GetColumn(0).Z*Scale.Z;
			NewVertex.Transform[1].X = RotMat.GetColumn(1).X*Scale.X;  NewVertex.Transform[1].Y = RotMat.GetColumn(1).Y*Scale.Y;   NewVertex.Transform[1].Z = RotMat.GetColumn(1).Z*Scale.Z;
			NewVertex.Transform[2].X = RotMat.GetColumn(2).X*Scale.X;  NewVertex.Transform[2].Y = RotMat.GetColumn(2).Y*Scale.Y;   NewVertex.Transform[2].Z = RotMat.GetColumn(2).Z*Scale.Z;
			NewVertex.Transform[0].W = Pos.X;
			NewVertex.Transform[1].W = Pos.Y;
			NewVertex.Transform[2].W = Pos.Z;
			FVector Vel = VelItr.GetAt(ParticleIndex);
			FVector Direction;
			float Speed;
			Vel.ToDirectionAndLength(Direction, Speed);

			// Pack direction and speed.
			NewVertex.Velocity = FVector4(Direction, Speed);
		}
	}
	);

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
		delete static_cast<FNiagaraDynamicDataMesh*>(DynamicDataRender);
		DynamicDataRender = NULL;
	}
	DynamicDataRender = NewDynamicData;
}

int NiagaraEffectRendererMeshes::GetDynamicDataSize()
{
	uint32 Size = sizeof(FNiagaraDynamicDataMesh);
	if (DynamicDataRender)
	{
		Size += (static_cast<FNiagaraDynamicDataMesh*>(DynamicDataRender))->VertexData.GetAllocatedSize();
	}

	return Size;
}

bool NiagaraEffectRendererMeshes::HasDynamicData()
{
	return DynamicDataRender && (static_cast<FNiagaraDynamicDataMesh*>(DynamicDataRender))->VertexData.Num() > 0;
}


const TArray<FNiagaraVariable>& NiagaraEffectRendererMeshes::GetRequiredAttributes()
{
	static TArray<FNiagaraVariable> Attrs;

	if (Attrs.Num() == 0)
	{
		FNiagaraVariable PositionStruct = FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), "Position");
		Attrs.Add(PositionStruct);
	}

	return Attrs;
}
