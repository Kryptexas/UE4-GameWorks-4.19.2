// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DynamicMeshBuilder.h: Dynamic mesh builder definitions.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "PackedNormal.h"
#include "HitProxies.h"
#include "RenderUtils.h"
#include "LocalVertexFactory.h"
#include "SceneManagement.h"

class FMaterialRenderProxy;
class FMeshElementCollector;
class FPrimitiveDrawInterface;

/** The vertex type used for dynamic meshes. */
struct FDynamicMeshVertex
{
	FDynamicMeshVertex() {}
	FDynamicMeshVertex( const FVector& InPosition ):
		Position(InPosition),
		TangentX(FVector(1,0,0)),
		TangentZ(FVector(0,0,1)),
		Color(FColor(255,255,255)) 
	{
		// basis determinant default to +1.0
		TangentZ.Vector.W = 255;

		for (int i = 0; i < MAX_STATIC_TEXCOORDS; i++)
		{
			TextureCoordinate[i] = FVector2D::ZeroVector;
		}
	}

	FDynamicMeshVertex(const FVector& InPosition, const FVector2D& InTexCoord, const FColor& InColor) :
		Position(InPosition),
		TangentX(FVector(1, 0, 0)),
		TangentZ(FVector(0, 0, 1)),
		Color(InColor)
	{
		// basis determinant default to +1.0
		TangentZ.Vector.W = 255;

		for (int i = 0; i < MAX_STATIC_TEXCOORDS; i++)
		{
			TextureCoordinate[i] = InTexCoord;
		}
	}

	FDynamicMeshVertex(const FVector& InPosition,const FVector& InTangentX,const FVector& InTangentZ,const FVector2D& InTexCoord, const FColor& InColor):
		Position(InPosition),
		TangentX(InTangentX),
		TangentZ(InTangentZ),
		Color(InColor)
	{
		// basis determinant default to +1.0
		TangentZ.Vector.W = 255;

		for (int i = 0; i < MAX_STATIC_TEXCOORDS; i++)
		{
			TextureCoordinate[i] = InTexCoord;
		}
	}

	FDynamicMeshVertex(const FVector& InPosition, const FVector& LayerTexcoords, const FVector2D& WeightmapTexcoords)
		: Position(InPosition)
		, TangentX(FVector(1, 0, 0))
		, TangentZ(FVector(0, 0, 1))
		, Color(FColor::White)
	{
		// TangentZ.w contains the sign of the tangent basis determinant. Assume +1
		TangentZ.Vector.W = 255;

		TextureCoordinate[0] = FVector2D(LayerTexcoords.X, LayerTexcoords.Y);
		TextureCoordinate[1] = FVector2D(LayerTexcoords.X, LayerTexcoords.Y); // Z not currently set, so use Y
		TextureCoordinate[2] = FVector2D(LayerTexcoords.Y, LayerTexcoords.X); // Z not currently set, so use X
		TextureCoordinate[3] = WeightmapTexcoords;
	};

	void SetTangents( const FVector& InTangentX, const FVector& InTangentY, const FVector& InTangentZ )
	{
		TangentX = InTangentX;
		TangentZ = InTangentZ;
		// store determinant of basis in w component of normal vector
		TangentZ.Vector.W = GetBasisDeterminantSign(InTangentX,InTangentY,InTangentZ) < 0.0f ? 0 : 255;
	}

	FVector GetTangentY() const
	{
		FVector TanX = TangentX;
		FVector TanZ = TangentZ;

		return (TanZ ^ TanX) * ((float)TangentZ.Vector.W / 127.5f - 1.0f);
	};

	FVector Position;
	FVector2D TextureCoordinate[MAX_STATIC_TEXCOORDS];
	FPackedNormal TangentX;
	FPackedNormal TangentZ;
	FColor Color;
};

class FMeshBuilderOneFrameResources : public FOneFrameResource
{
public:
	class FPooledDynamicMeshVertexBuffer* VertexBuffer = nullptr;
	class FPooledDynamicMeshIndexBuffer* IndexBuffer = nullptr;
	class FPooledDynamicMeshVertexFactory* VertexFactory = nullptr;
	class FDynamicMeshPrimitiveUniformBuffer* PrimitiveUniformBuffer = nullptr;
	virtual ENGINE_API ~FMeshBuilderOneFrameResources();

	inline bool IsValidForRendering() 
	{
		return VertexBuffer && IndexBuffer && PrimitiveUniformBuffer && VertexFactory;
	}
};

struct FDynamicMeshDrawOffset
{
	uint32 FirstIndex = 0;
	uint32 MinVertexIndex = 0;
	uint32 MaxVertexIndex = 0;
	uint32 NumPrimitives = 0;
};

struct FDynamicMeshBuilderSettings
{
	bool CastShadow = true;
	bool bDisableBackfaceCulling = false;
	bool bWireframe = false;
	bool bReceivesDecals = true;
	bool bUseSelectionOutline = true;
	bool bCanApplyViewModeOverrides = false;
	bool bUseWireframeSelectionColoring = false;
};

/**
 * A utility used to construct dynamically generated meshes, and render them to a FPrimitiveDrawInterface.
 * Note: This is meant to be easy to use, not fast.  It moves the data around more than necessary, and requires dynamically allocating RHI
 * resources.  Exercise caution.
 */
class FDynamicMeshBuilder
{
public:

	/** Initialization constructor. */
	ENGINE_API FDynamicMeshBuilder(ERHIFeatureLevel::Type InFeatureLevel, uint32 InNumTexCoords = 1, uint32 InLightmapCoordinateIndex = 0, bool InUse16bitTexCoord = false);

	/** Destructor. */
	ENGINE_API ~FDynamicMeshBuilder();

	/** Adds a vertex to the mesh. */
	ENGINE_API int32 AddVertex(
		const FVector& InPosition,
		const FVector2D& InTextureCoordinate,
		const FVector& InTangentX,
		const FVector& InTangentY,
		const FVector& InTangentZ,
		const FColor& InColor
		);

	/** Adds a vertex to the mesh. */
	ENGINE_API int32 AddVertex(const FDynamicMeshVertex &InVertex);

	/** Adds a triangle to the mesh. */
	ENGINE_API void AddTriangle(int32 V0,int32 V1,int32 V2);

	/** Adds many vertices to the mesh. */
	ENGINE_API int32 AddVertices(const TArray<FDynamicMeshVertex> &InVertices);

	/** Add many indices to the mesh. */
	ENGINE_API void AddTriangles(const TArray<uint32> &InIndices);

	/** Adds a mesh of what's been built so far to the collector. */
	ENGINE_API void GetMesh(const FMatrix& LocalToWorld, const FMaterialRenderProxy* MaterialRenderProxy, uint8 DepthPriorityGroup, bool bDisableBackfaceCulling, bool bReceivesDecals, int32 ViewIndex, FMeshElementCollector& Collector);
	ENGINE_API void GetMesh(const FMatrix& LocalToWorld, const FMaterialRenderProxy* MaterialRenderProxy, uint8 DepthPriorityGroup, bool bDisableBackfaceCulling, bool bReceivesDecals, bool bUseSelectionOutline, int32 ViewIndex, 
							FMeshElementCollector& Collector, HHitProxy* HitProxy);
	ENGINE_API void GetMesh(const FMatrix& LocalToWorld, const FMaterialRenderProxy* MaterialRenderProxy, uint8 DepthPriorityGroup, bool bDisableBackfaceCulling, bool bReceivesDecals, bool bUseSelectionOutline, int32 ViewIndex, 
							FMeshElementCollector& Collector, const FHitProxyId HitProxyId = FHitProxyId());
	ENGINE_API void GetMesh(const FMatrix& LocalToWorld, const FMaterialRenderProxy* MaterialRenderProxy, uint8 DepthPriorityGroup, const FDynamicMeshBuilderSettings& Settings, FDynamicMeshDrawOffset const * const DrawOffset, int32 ViewIndex,
		FMeshElementCollector& Collector, const FHitProxyId HitProxyId = FHitProxyId());

	ENGINE_API void GetMeshElement(const FMatrix& LocalToWorld, const FMaterialRenderProxy* MaterialRenderProxy, uint8 DepthPriorityGroup, bool bDisableBackfaceCulling, bool bReceivesDecals, int32 ViewIndex, FMeshBuilderOneFrameResources& OneFrameResource, FMeshBatch& Mesh);

	/**
	 * Draws the mesh to the given primitive draw interface.
	 * @param PDI - The primitive draw interface to draw the mesh on.
	 * @param LocalToWorld - The local to world transform to apply to the vertices of the mesh.
	 * @param FMaterialRenderProxy - The material instance to render on the mesh.
	 * @param DepthPriorityGroup - The depth priority group to render the mesh in.
	 * @param HitProxyId - Hit proxy to use for this mesh.  Use INDEX_NONE for no hit proxy.
	 */
	ENGINE_API void Draw(FPrimitiveDrawInterface* PDI,const FMatrix& LocalToWorld,const FMaterialRenderProxy* MaterialRenderProxy,uint8 DepthPriorityGroup,bool bDisableBackfaceCulling=false, bool bReceivesDecals=true, const FHitProxyId HitProxyId = FHitProxyId());

private:
	class FMeshBuilderOneFrameResources* OneFrameResources = nullptr;
	class FPooledDynamicMeshIndexBuffer* IndexBuffer = nullptr;
	class FPooledDynamicMeshVertexBuffer* VertexBuffer = nullptr;
	ERHIFeatureLevel::Type FeatureLevel;
};

/** Index Buffer */
class ENGINE_API FDynamicMeshIndexBuffer32 : public FIndexBuffer
{
public:
	TArray<uint32> Indices;

	virtual void InitRHI() override;
	void UpdateRHI();
};

class ENGINE_API FDynamicMeshIndexBuffer16 : public FIndexBuffer
{
public:
	TArray<uint16> Indices;

	virtual void InitRHI() override;
	void UpdateRHI();
};