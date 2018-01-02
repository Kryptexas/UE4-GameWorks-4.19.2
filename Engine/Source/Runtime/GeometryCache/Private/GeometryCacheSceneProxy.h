// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "Containers/ResourceArray.h"
#include "RenderResource.h"
#include "PrimitiveViewRelevance.h"
#include "PrimitiveSceneProxy.h"
#include "Materials/MaterialInterface.h"
#include "LocalVertexFactory.h"
#include "DynamicMeshBuilder.h"
#include "StaticMeshResources.h"
#include "LogMacros.h"

class FMeshElementCollector;
struct FGeometryCacheMeshData;

DECLARE_STATS_GROUP(TEXT("GeometryCache"), STATGROUP_GeometryCache, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("MeshTime"), STAT_GeometryCacheSceneProxy_GetMeshElements, STATGROUP_GeometryCache );
DECLARE_DWORD_COUNTER_STAT(TEXT("Triangle Count"), STAT_GeometryCacheSceneProxy_TriangleCount, STATGROUP_GeometryCache);
DECLARE_DWORD_COUNTER_STAT(TEXT("Section Count"), STAT_GeometryCacheSceneProxy_MeshBatchCount, STATGROUP_GeometryCache);

GEOMETRYCACHE_API DECLARE_LOG_CATEGORY_EXTERN(LogGeomCache, Log, All);

/** Resource array to pass  */
class GEOMETRYCACHE_API FGeomCacheVertexResourceArray : public FResourceArrayInterface
{
public:
	FGeomCacheVertexResourceArray(void* InData, uint32 InSize);

	virtual const void* GetResourceData() const override;
	virtual uint32 GetResourceDataSize() const override;
	virtual void Discard() override;
	virtual bool IsStatic() const override;
	virtual bool GetAllowCPUAccess() const override;
	virtual void SetAllowCPUAccess(bool bInNeedsCPUAccess) override;

private:
	void* Data;
	uint32 Size;
};

class GEOMETRYCACHE_API FGeomCacheTrackProxy
{
public:
	FGeomCacheTrackProxy(ERHIFeatureLevel::Type InFeatureLevel)
		: VertexFactory(InFeatureLevel, "FGeomCacheTrackProxy")
	{
	}

	/** MeshData storing information used for rendering this Track */
	FGeometryCacheMeshData* MeshData;

	/** Material applied to this Track */
	TArray<UMaterialInterface*> Materials;
	/** Vertex buffer for this Track */
	FStaticMeshVertexBuffers VertexBuffers;
	/** Index buffer for this Track */
	FDynamicMeshIndexBuffer32 IndexBuffer;
	/** Vertex factory for this Track */
	FLocalVertexFactory VertexFactory;
	/** World Matrix for this Track */
	FMatrix WorldMatrix;
};

/** Procedural mesh scene proxy */
class GEOMETRYCACHE_API FGeometryCacheSceneProxy final : public FPrimitiveSceneProxy
{
public:
	SIZE_T GetTypeHash() const override;

	FGeometryCacheSceneProxy(class UGeometryCacheComponent* Component);

	virtual ~FGeometryCacheSceneProxy();

	// Begin FPrimitiveSceneProxy interface.
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

	virtual bool CanBeOccluded() const override;
	virtual uint32 GetMemoryFootprint(void) const;
	uint32 GetAllocatedSize(void) const;
	// End FPrimitiveSceneProxy interface.

	/** Update world matrix for specific section */
	void UpdateSectionWorldMatrix(const int32 SectionIndex, const FMatrix& WorldMatrix);
	/** Update vertex buffer for specific section */
	void UpdateSectionVertexBuffer(const int32 SectionIndex, FGeometryCacheMeshData* MeshData );
	/** Update index buffer for specific section */
	void UpdateSectionIndexBuffer(const int32 SectionIndex, const TArray<uint32>& Indices);

	/** Clears the Sections array*/
	void ClearSections();
	
private:	
	FMaterialRelevance MaterialRelevance;

	/** Array of Track Proxies */
	TArray<FGeomCacheTrackProxy*> Sections;
};
