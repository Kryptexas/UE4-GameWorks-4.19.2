// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "GeometryCacheSceneProxy.h"
#include "MaterialShared.h"
#include "SceneManagement.h"
#include "EngineGlobals.h"
#include "Materials/Material.h"
#include "Engine/Engine.h"
#include "GeometryCacheComponent.h"
#include "GeometryCacheMeshData.h"

DEFINE_LOG_CATEGORY(LogGeomCache);

FGeometryCacheSceneProxy::FGeometryCacheSceneProxy(UGeometryCacheComponent* Component) : FPrimitiveSceneProxy(Component)
, MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
{
	// Copy each section
	const int32 NumSections = Component->TrackSections.Num();
	Sections.AddZeroed(NumSections);
	for (int SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		FTrackRenderData& SrcSection = Component->TrackSections[SectionIdx];
		
		if (SrcSection.MeshData->Indices.Num() > 0)
		{
			FGeomCacheTrackProxy* NewSection = new FGeomCacheTrackProxy(GetScene().GetFeatureLevel());

			NewSection->WorldMatrix = SrcSection.WorldMatrix;
			FGeometryCacheMeshData* MeshData = NewSection->MeshData = SrcSection.MeshData;

			// Copy data from vertex buffer
			const int32 NumVerts = MeshData->Vertices.Num();

			// Copy index buffer
			NewSection->IndexBuffer.Indices = SrcSection.MeshData->Indices;

			NewSection->VertexBuffers.InitFromDynamicVertex(&NewSection->VertexFactory, MeshData->Vertices);

			// Enqueue initialization of render resource
			BeginInitResource(&NewSection->IndexBuffer);

			// Grab materials
			for (FGeometryCacheMeshBatchInfo& BatchInfo : MeshData->BatchesInfo)
			{
				UMaterialInterface* Material = Component->GetMaterial(BatchInfo.MaterialIndex);
				if (Material == nullptr)
				{
					Material = UMaterial::GetDefaultMaterial(MD_Surface);
				}

				NewSection->Materials.Push(Material);
			}			

			// Save ref to new section
			Sections[SectionIdx] = NewSection;
		}
	}
}

FGeometryCacheSceneProxy::~FGeometryCacheSceneProxy()
{
	for (FGeomCacheTrackProxy* Section : Sections)
	{
		if (Section != nullptr)
		{
			Section->VertexBuffers.PositionVertexBuffer.ReleaseResource();
			Section->VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
			Section->VertexBuffers.ColorVertexBuffer.ReleaseResource();
			Section->IndexBuffer.ReleaseResource();
			Section->VertexFactory.ReleaseResource();
			delete Section;
		}
	}
	Sections.Empty();
}

SIZE_T FGeometryCacheSceneProxy::GetTypeHash() const
{
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}

void FGeometryCacheSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	SCOPE_CYCLE_COUNTER(STAT_GeometryCacheSceneProxy_GetMeshElements);

	// Set up wireframe material (if needed)
	const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

	FColoredMaterialRenderProxy* WireframeMaterialInstance = nullptr;
	if (bWireframe)
	{
		WireframeMaterialInstance = new FColoredMaterialRenderProxy(
			GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy(IsSelected()) : nullptr,
			FLinearColor(0, 0.5f, 1.f)
			);

		Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);
	}
	
	// Iterate over sections	
	for (const FGeomCacheTrackProxy* TrackProxy : Sections )
	{
		// Render out stored TrackProxy's
		if (TrackProxy != nullptr)
		{
			INC_DWORD_STAT_BY(STAT_GeometryCacheSceneProxy_MeshBatchCount, TrackProxy->MeshData->BatchesInfo.Num());

			int32 BatchIndex = 0;
			for (FGeometryCacheMeshBatchInfo& BatchInfo : TrackProxy->MeshData->BatchesInfo)
			{
				FMaterialRenderProxy* MaterialProxy = bWireframe ? WireframeMaterialInstance : TrackProxy->Materials[BatchIndex]->GetRenderProxy(IsSelected());

				for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
				{
					if (VisibilityMap & (1 << ViewIndex))
					{
						const FSceneView* View = Views[ViewIndex];
						// Draw the mesh.
						FMeshBatch& Mesh = Collector.AllocateMesh();
						FMeshBatchElement& BatchElement = Mesh.Elements[0];
						BatchElement.IndexBuffer = &TrackProxy->IndexBuffer;
						Mesh.bWireframe = bWireframe;
						Mesh.VertexFactory = &TrackProxy->VertexFactory;
						Mesh.MaterialRenderProxy = MaterialProxy;
						BatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(TrackProxy->WorldMatrix * GetLocalToWorld(), GetBounds(), GetLocalBounds(), true, UseEditorDepthTest());
						BatchElement.FirstIndex = BatchInfo.StartIndex;
						BatchElement.NumPrimitives = BatchInfo.NumTriangles;
						BatchElement.MinVertexIndex = 0;
						BatchElement.MaxVertexIndex = TrackProxy->VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;
						Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
						Mesh.Type = PT_TriangleList;
						Mesh.DepthPriorityGroup = SDPG_World;
						Mesh.bCanApplyViewModeOverrides = false;
						Collector.AddMesh(ViewIndex, Mesh);

						INC_DWORD_STAT_BY(STAT_GeometryCacheSceneProxy_TriangleCount, BatchElement.NumPrimitives);
					}
				}

				++BatchIndex;
			}			
		}
		
	}

	// Draw bounds
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			// Render bounds
			RenderBounds(Collector.GetPDI(ViewIndex), ViewFamily.EngineShowFlags, GetBounds(), IsSelected());
		}
	}
#endif
}

FPrimitiveViewRelevance FGeometryCacheSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View);
	Result.bShadowRelevance = IsShadowCast(View);
	Result.bDynamicRelevance = true;
	MaterialRelevance.SetPrimitiveViewRelevance(Result);
	return Result;
}

bool FGeometryCacheSceneProxy::CanBeOccluded() const
{
	return !MaterialRelevance.bDisableDepthTest;
}

uint32 FGeometryCacheSceneProxy::GetMemoryFootprint(void) const
{
	return(sizeof(*this) + GetAllocatedSize());
}

uint32 FGeometryCacheSceneProxy::GetAllocatedSize(void) const
{
	return(FPrimitiveSceneProxy::GetAllocatedSize());
}

void FGeometryCacheSceneProxy::UpdateSectionWorldMatrix(const int32 SectionIndex, const FMatrix& WorldMatrix)
{
	check(SectionIndex < Sections.Num() && "Section Index out of range");
	Sections[SectionIndex]->WorldMatrix = WorldMatrix;
}


void FGeometryCacheSceneProxy::UpdateSectionVertexBuffer(const int32 SectionIndex, FGeometryCacheMeshData* MeshData)
{
	check(SectionIndex < Sections.Num() && "Section Index out of range");
	check(IsInRenderingThread());

	Sections[SectionIndex]->MeshData = MeshData;

	const bool bRecreate = Sections[SectionIndex]->VertexBuffers.PositionVertexBuffer.GetNumVertices() != Sections[SectionIndex]->MeshData->Vertices.Num();

	if (!bRecreate)
	{
		UE_LOG(LogGeomCache, Warning, TEXT("Recreation of Geomcaches unsupported use SOA Position- Texture- Tangent- Color Buffer seperately and Lock Unlock (but those will be about as bad as re-creation) ideally have a dedicated updatepath for big VB in the RHI"));
	}

	//always recreate
	Sections[SectionIndex]->VertexBuffers.InitFromDynamicVertex(&Sections[SectionIndex]->VertexFactory, MeshData->Vertices);
}

void FGeometryCacheSceneProxy::UpdateSectionIndexBuffer(const int32 SectionIndex, const TArray<uint32>& Indices)
{
	check(SectionIndex < Sections.Num() && "Section Index out of range");
	check(IsInRenderingThread());

	const bool bRecreate = Sections[SectionIndex]->IndexBuffer.Indices.Num() != Indices.Num();
	
	Sections[SectionIndex]->IndexBuffer.Indices.Empty(Indices.Num());
	Sections[SectionIndex]->IndexBuffer.Indices.Append(Indices);
	if (bRecreate)
	{
		Sections[SectionIndex]->IndexBuffer.InitRHI();
	}
	else
	{
		Sections[SectionIndex]->IndexBuffer.UpdateRHI();
	}	
}

void FGeometryCacheSceneProxy::ClearSections()
{
	Sections.Empty();
}

FGeomCacheVertexResourceArray::FGeomCacheVertexResourceArray(void* InData, uint32 InSize) : Data(InData)
, Size(InSize)
{

}

const void* FGeomCacheVertexResourceArray::GetResourceData() const
{
	return Data;
}

uint32 FGeomCacheVertexResourceArray::GetResourceDataSize() const
{
	return Size;
}

void FGeomCacheVertexResourceArray::Discard()
{

}

bool FGeomCacheVertexResourceArray::IsStatic() const
{
	return false;
}

bool FGeomCacheVertexResourceArray::GetAllowCPUAccess() const
{
	return false;
}

void FGeomCacheVertexResourceArray::SetAllowCPUAccess(bool bInNeedsCPUAccess)
{

}

