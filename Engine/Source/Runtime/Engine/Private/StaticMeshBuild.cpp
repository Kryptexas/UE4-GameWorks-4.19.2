// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	StaticMeshBuild.cpp: Static mesh building.
=============================================================================*/

#include "EnginePrivate.h"

#if WITH_EDITORONLY_DATA
#include "RawMesh.h"
#include "MeshUtilities.h"
#include "TargetPlatform.h"
#include "GenericOctree.h"
#endif // #if WITH_EDITORONLY_DATA

#define LOCTEXT_NAMESPACE "StaticMeshEditor"

#if WITH_EDITORONLY_DATA
/**
 * Check the render data for the provided mesh and return true if the mesh
 * contains degenerate tangent bases.
 */
static bool HasBadTangents(UStaticMesh* Mesh)
{
	bool bHasBadTangents = false;
	if (Mesh && Mesh->RenderData)
	{
		int32 NumLODs = Mesh->GetNumLODs();
		for (int32 LODIndex = 0; !bHasBadTangents && LODIndex < NumLODs; ++LODIndex)
		{
			FStaticMeshLODResources& LOD = Mesh->RenderData->LODResources[LODIndex];
			int32 NumVerts = LOD.VertexBuffer.GetNumVertices();
			for (int32 VertIndex = 0; !bHasBadTangents && VertIndex < NumVerts; ++VertIndex)
			{
				FPackedNormal& TangentX = LOD.VertexBuffer.VertexTangentX(VertIndex);
				FPackedNormal& TangentZ = LOD.VertexBuffer.VertexTangentZ(VertIndex);
				if (FMath::Abs((int32)TangentX.Vector.X - (int32)TangentZ.Vector.X) <= 1
					&& FMath::Abs((int32)TangentX.Vector.Y - (int32)TangentZ.Vector.Y) <= 1
					&& FMath::Abs((int32)TangentX.Vector.Z - (int32)TangentZ.Vector.Z) <= 1)
				{
					bHasBadTangents = true;
				}
			}
		}
	}
	return bHasBadTangents;
}
#endif // #if WITH_EDITORONLY_DATA

void UStaticMesh::Build(bool bSilent)
{
#if WITH_EDITORONLY_DATA
	if (IsTemplate())
		return;

	if (SourceModels.Num() <= 0)
	{
		UE_LOG(LogStaticMesh,Warning,TEXT("Static mesh has no source models: %s"),*GetPathName());
		return;
	}

	if(!bSilent)
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("Path"), FText::FromString( GetPathName() ) );
		const FText StatusUpdate = FText::Format( LOCTEXT("BeginStaticMeshBuildingTask", "({Path}) Building"), Args );
		GWarn->BeginSlowTask( StatusUpdate, true );	
	}

	// Detach all instances of this static mesh from the scene.
	FStaticMeshComponentRecreateRenderStateContext RecreateRenderStateContext(this,false);

	// Release the static mesh's resources.
	ReleaseResources();

	// Flush the resource release commands to the rendering thread to ensure that the build doesn't occur while a resource is still
	// allocated, and potentially accessing the UStaticMesh.
	ReleaseResourcesFence.Wait();

	// Remember the derived data key of our current render data if any.
	FString ExistingDerivedDataKey = RenderData ? RenderData->DerivedDataKey : TEXT("");

	// Free existing render data and recache.
	CacheDerivedData();

	// Reinitialize the static mesh's resources.
	InitResources();

	// Ensure we have a bodysetup.
	CreateBodySetup();
	check(BodySetup != NULL);

#if WITH_EDITOR
	if( SourceModels.Num() )
	{
		// Rescale simple collision if the user changed the mesh build scale
		BodySetup->RescaleSimpleCollision( SourceModels[0].BuildSettings.BuildScale );
	}

	// Invalidate physics data if this has changed.
	// TODO_STATICMESH: Not necessary any longer?
	BodySetup->InvalidatePhysicsData();
#endif

	// Compare the derived data keys to see if renderable mesh data has actually changed.
	check(RenderData);
	bool bHasRenderDataChanged = RenderData->DerivedDataKey != ExistingDerivedDataKey;

	if (bHasRenderDataChanged)
	{
		// Warn the user if the new mesh has degenerate tangent bases.
		if (HasBadTangents(this))
		{
			FFormatNamedArguments Arguments;
			Arguments.Add( TEXT("Meshname"), FText::FromString(GetName()) );
			const FText WarningMsg = FText::Format( LOCTEXT("MeshHasDegenerateTangents", "{Meshname} has degenerate tangent bases which will result in incorrect shading. Consider enabling Recompute Tangents in the mesh's Build Settings."), Arguments );
			UE_LOG(LogStaticMesh,Warning,TEXT("%s"),*WarningMsg.ToString());
			if (!bSilent)
			{
				FMessageDialog::Open(EAppMsgType::Ok, WarningMsg);
			}
		}

		// Force the static mesh to re-export next time lighting is built
		SetLightingGuid();

		// Find any static mesh components that use this mesh and fixup their override colors if necessary.
		// Also invalidate lighting. *** WARNING components may be reattached here! ***
		for( TObjectIterator<UStaticMeshComponent> It; It; ++It )
		{
			if ( It->StaticMesh == this )
			{
				It->FixupOverrideColorsIfNecessary( true );
				It->InvalidateLightingCache();
			}
		}

	}

	if(!bSilent)
	{
		GWarn->EndSlowTask();
	}
	
#else
	UE_LOG(LogStaticMesh,Fatal,TEXT("UStaticMesh::Build should not be called on non-editor builds."));
#endif
}

/*------------------------------------------------------------------------------
	Remapping of painted vertex colors.
------------------------------------------------------------------------------*/

#if WITH_EDITORONLY_DATA
/** Helper struct for the mesh component vert position octree */
struct FStaticMeshComponentVertPosOctreeSemantics
{
	enum { MaxElementsPerLeaf = 16 };
	enum { MinInclusiveElementsPerNode = 7 };
	enum { MaxNodeDepth = 12 };

	typedef TInlineAllocator<MaxElementsPerLeaf> ElementAllocator;

	/**
	 * Get the bounding box of the provided octree element. In this case, the box
	 * is merely the point specified by the element.
	 *
	 * @param	Element	Octree element to get the bounding box for
	 *
	 * @return	Bounding box of the provided octree element
	 */
	FORCEINLINE static FBoxCenterAndExtent GetBoundingBox( const FPaintedVertex& Element )
	{
		return FBoxCenterAndExtent( Element.Position, FVector::ZeroVector );
	}

	/**
	 * Determine if two octree elements are equal
	 *
	 * @param	A	First octree element to check
	 * @param	B	Second octree element to check
	 *
	 * @return	true if both octree elements are equal, false if they are not
	 */
	FORCEINLINE static bool AreElementsEqual( const FPaintedVertex& A, const FPaintedVertex& B )
	{
		return ( A.Position == B.Position && A.Normal == B.Normal && A.Color == B.Color );
	}

	/** Ignored for this implementation */
	FORCEINLINE static void SetElementId( const FPaintedVertex& Element, FOctreeElementId Id )
	{
	}
};
typedef TOctree<FPaintedVertex, FStaticMeshComponentVertPosOctreeSemantics> TSMCVertPosOctree;

void RemapPaintedVertexColors(
	const TArray<FPaintedVertex>& InPaintedVertices,
	const FColorVertexBuffer& InOverrideColors,
	const FPositionVertexBuffer& NewPositions,
	const FStaticMeshVertexBuffer* OptionalVertexBuffer,
	TArray<FColor>& OutOverrideColors
	)
{

	// Local copy of painted vertices we can scratch on.
	TArray<FPaintedVertex> PaintedVertices(InPaintedVertices);

	// Find the extents formed by the cached vertex positions in order to optimize the octree used later
	FVector MinExtents( PaintedVertices[ 0 ].Position );
	FVector MaxExtents( PaintedVertices[ 0 ].Position );

	for (int32 VertIndex = 0; VertIndex < PaintedVertices.Num(); ++VertIndex)
	{
		FVector& CurVector = PaintedVertices[ VertIndex ].Position;

		MinExtents.X = FMath::Min<float>( MinExtents.X, CurVector.X );
		MinExtents.Y = FMath::Min<float>( MinExtents.Y, CurVector.Y );
		MinExtents.Z = FMath::Min<float>( MinExtents.Z, CurVector.Z );

		MaxExtents.X = FMath::Max<float>( MaxExtents.X, CurVector.X );
		MaxExtents.Y = FMath::Max<float>( MaxExtents.Y, CurVector.Y );
		MaxExtents.Z = FMath::Max<float>( MaxExtents.Z, CurVector.Z );
	}

	// Create an octree which spans the extreme extents of the old and new vertex positions in order to quickly query for the colors
	// of the new vertex positions
	for (int32 VertIndex = 0; VertIndex < (int32)NewPositions.GetNumVertices(); ++VertIndex)
	{
		FVector CurVector = NewPositions.VertexPosition(VertIndex);

		MinExtents.X = FMath::Min<float>( MinExtents.X, CurVector.X );
		MinExtents.Y = FMath::Min<float>( MinExtents.Y, CurVector.Y );
		MinExtents.Z = FMath::Min<float>( MinExtents.Z, CurVector.Z );

		MaxExtents.X = FMath::Max<float>( MaxExtents.X, CurVector.X );
		MaxExtents.Y = FMath::Max<float>( MaxExtents.Y, CurVector.Y );
		MaxExtents.Z = FMath::Max<float>( MaxExtents.Z, CurVector.Z );
	}

	FBox Bounds( MinExtents, MaxExtents );
	TSMCVertPosOctree VertPosOctree( Bounds.GetCenter(), Bounds.GetExtent().GetMax() );

	// Add each old vertex to the octree
	for ( int32 PaintedVertexIndex = 0; PaintedVertexIndex < PaintedVertices.Num(); ++PaintedVertexIndex )
	{
		VertPosOctree.AddElement( PaintedVertices[ PaintedVertexIndex ] );
	}


	// Iterate over each new vertex position, attempting to find the old vertex it is closest to, applying
	// the color of the old vertex to the new position if possible.
	OutOverrideColors.Empty(NewPositions.GetNumVertices());
	const float DistanceOverNormalThreshold = OptionalVertexBuffer ? KINDA_SMALL_NUMBER : 0.0f;
	for ( uint32 NewVertIndex = 0; NewVertIndex < NewPositions.GetNumVertices(); ++NewVertIndex )
	{
		TArray<FPaintedVertex> PointsToConsider;
		TSMCVertPosOctree::TConstIterator<> OctreeIter( VertPosOctree );
		const FVector& CurPosition = NewPositions.VertexPosition( NewVertIndex );
		FVector CurNormal = FVector::ZeroVector;
		if (OptionalVertexBuffer)
		{
			CurNormal = OptionalVertexBuffer->VertexTangentZ( NewVertIndex );
		}

		// Iterate through the octree attempting to find the vertices closest to the current new point
		while ( OctreeIter.HasPendingNodes() )
		{
			const TSMCVertPosOctree::FNode& CurNode = OctreeIter.GetCurrentNode();
			const FOctreeNodeContext& CurContext = OctreeIter.GetCurrentContext();

			// Find the child of the current node, if any, that contains the current new point
			FOctreeChildNodeRef ChildRef = CurContext.GetContainingChild( FBoxCenterAndExtent( CurPosition, FVector::ZeroVector ) );

			if ( !ChildRef.IsNULL() )
			{
				const TSMCVertPosOctree::FNode* ChildNode = CurNode.GetChild( ChildRef );

				// If the specified child node exists and contains any of the old vertices, push it to the iterator for future consideration
				if ( ChildNode && ChildNode->GetInclusiveElementCount() > 0 )
				{
					OctreeIter.PushChild( ChildRef );
				}
				// If the child node doesn't have any of the old vertices in it, it's not worth pursuing any further. In an attempt to find
				// anything to match vs. the new point, add all of the children of the current octree node that have old points in them to the
				// iterator for future consideration.
				else
				{
					FOREACH_OCTREE_CHILD_NODE( ChildRef )
					{
						if( CurNode.HasChild( ChildRef ) && CurNode.GetChild( ChildRef )->GetInclusiveElementCount() > 0 )
						{
							OctreeIter.PushChild( ChildRef );
						}
					}
				}
			}

			// Add all of the elements in the current node to the list of points to consider for closest point calculations
			PointsToConsider.Append( CurNode.GetElements() );
			OctreeIter.Advance();
		}

		// If any points to consider were found, iterate over each and find which one is the closest to the new point 
		if ( PointsToConsider.Num() > 0 )
		{
			FPaintedVertex BestVertex = PointsToConsider[0];
			float BestDistanceSquared = ( BestVertex.Position - CurPosition ).SizeSquared();
			float BestNormalDot = FVector( BestVertex.Normal ) | CurNormal;

			for ( int32 ConsiderationIndex = 1; ConsiderationIndex < PointsToConsider.Num(); ++ConsiderationIndex )
			{
				FPaintedVertex& Vertex = PointsToConsider[ ConsiderationIndex ];
				const float DistSqrd = ( Vertex.Position - CurPosition ).SizeSquared();
				const float NormalDot = FVector( Vertex.Normal ) | CurNormal;
				if ( DistSqrd < BestDistanceSquared - DistanceOverNormalThreshold )
				{
					BestVertex = Vertex;
					BestDistanceSquared = DistSqrd;
					BestNormalDot = NormalDot;
				}
				else if ( OptionalVertexBuffer && DistSqrd < BestDistanceSquared + DistanceOverNormalThreshold && NormalDot > BestNormalDot )
				{
					BestVertex = Vertex;
					BestDistanceSquared = DistSqrd;
					BestNormalDot = NormalDot;
				}
			}

			OutOverrideColors.Add(BestVertex.Color);
		}
	}
}
#endif // #if WITH_EDITORONLY_DATA

/*------------------------------------------------------------------------------
	Conversion of legacy source data.
------------------------------------------------------------------------------*/

#if WITH_EDITORONLY_DATA

struct FStaticMeshTriangle
{
	FVector		Vertices[3];
	FVector2D	UVs[3][8];
	FColor		Colors[3];
	int32			MaterialIndex;
	int32			FragmentIndex;
	uint32		SmoothingMask;
	int32			NumUVs;

	FVector		TangentX[3]; // Tangent, U-direction
	FVector		TangentY[3]; // Binormal, V-direction
	FVector		TangentZ[3]; // Normal

	uint32		bOverrideTangentBasis;
	uint32		bExplicitNormals;
};

struct FStaticMeshTriangleBulkData : public FUntypedBulkData
{
	virtual int32 GetElementSize() const
	{
		return sizeof(FStaticMeshTriangle);
	}

	virtual void SerializeElement( FArchive& Ar, void* Data, int32 ElementIndex )
	{
		FStaticMeshTriangle& StaticMeshTriangle = *((FStaticMeshTriangle*)Data + ElementIndex);
		Ar << StaticMeshTriangle.Vertices[0];
		Ar << StaticMeshTriangle.Vertices[1];
		Ar << StaticMeshTriangle.Vertices[2];
		for( int32 VertexIndex=0; VertexIndex<3; VertexIndex++ )
		{
			for( int32 UVIndex=0; UVIndex<8; UVIndex++ )
			{
				Ar << StaticMeshTriangle.UVs[VertexIndex][UVIndex];
			}
        }
		Ar << StaticMeshTriangle.Colors[0];
		Ar << StaticMeshTriangle.Colors[1];
		Ar << StaticMeshTriangle.Colors[2];
		Ar << StaticMeshTriangle.MaterialIndex;
		Ar << StaticMeshTriangle.FragmentIndex;
		Ar << StaticMeshTriangle.SmoothingMask;
		Ar << StaticMeshTriangle.NumUVs;
		Ar << StaticMeshTriangle.TangentX[0];
		Ar << StaticMeshTriangle.TangentX[1];
		Ar << StaticMeshTriangle.TangentX[2];
		Ar << StaticMeshTriangle.TangentY[0];
		Ar << StaticMeshTriangle.TangentY[1];
		Ar << StaticMeshTriangle.TangentY[2];
		Ar << StaticMeshTriangle.TangentZ[0];
		Ar << StaticMeshTriangle.TangentZ[1];
		Ar << StaticMeshTriangle.TangentZ[2];
		Ar << StaticMeshTriangle.bOverrideTangentBasis;
		Ar << StaticMeshTriangle.bExplicitNormals;
	}

	virtual bool RequiresSingleElementSerialization( FArchive& Ar )
	{
		return false;
	}
};

struct FFragmentRange
{
	int32 BaseIndex;
	int32 NumPrimitives;

	friend FArchive& operator<<(FArchive& Ar,FFragmentRange& FragmentRange)
	{
		Ar << FragmentRange.BaseIndex << FragmentRange.NumPrimitives;
		return Ar;
	}
};

class FLegacyStaticMeshElement
{
public:
	class UMaterialInterface*		Material;
	FString					Name;			
	bool					EnableCollision,
							OldEnableCollision,
							bEnableShadowCasting;
	uint32					FirstIndex,
							NumTriangles,
							MinVertexIndex,
							MaxVertexIndex;
	int32 MaterialIndex;
	TArray<FFragmentRange> Fragments;

	/** Constructor. */
	FLegacyStaticMeshElement():
		Material(NULL),
		EnableCollision(false),
		OldEnableCollision(false),
		bEnableShadowCasting(true),
		FirstIndex(0),
		NumTriangles(0),
		MinVertexIndex(0),
		MaxVertexIndex(0),
		MaterialIndex(0)
	{}

	friend FArchive& operator<<(FArchive& Ar,FLegacyStaticMeshElement& E)
	{
		Ar	<< E.Material
			<< E.EnableCollision
			<< E.OldEnableCollision
			<< E.bEnableShadowCasting
			<< E.FirstIndex
			<< E.NumTriangles
			<< E.MinVertexIndex
			<< E.MaxVertexIndex
			<< E.MaterialIndex;

		Ar << E.Fragments;
		return Ar;
	}
};

struct FLegacyStaticMeshLODInfo
{
	TArray<FLegacyStaticMeshElement> Elements;

	friend FArchive& operator<<(FArchive& Ar,FLegacyStaticMeshLODInfo& LODInfo)
    {
		// Nothing to actually serialize.
		return Ar;
    }
};

struct FLegacyStaticMeshRenderData
{
	FStaticMeshVertexBuffer VertexBuffer;
	FPositionVertexBuffer PositionVertexBuffer;
	FColorVertexBuffer ColorVertexBuffer;
	uint32 NumVertices;
	FRawStaticIndexBuffer					IndexBuffer;
	FRawStaticIndexBuffer					ShadowIndexBuffer;
	FRawIndexBuffer							WireframeIndexBuffer;
	TArray<FLegacyStaticMeshElement>		Elements;
	FStaticMeshTriangleBulkData				RawTriangles;
	FRawStaticIndexBuffer					AdjacencyIndexBuffer;

	void Serialize(FArchive& Ar, UObject* Owner, int32 Idx)
    {
		const uint8 AdjacencyDataStripFlag = 1;
		FStripDataFlags StripFlags( Ar, Ar.IsCooking() && !Ar.CookingTarget()->SupportsFeature(ETargetPlatformFeatures::Tessellation) ? AdjacencyDataStripFlag : 0 );

		UStaticMesh* StaticMesh = Cast<UStaticMesh>( Owner );
		bool bNeedsCPUAccess = true;

		if( !StripFlags.IsEditorDataStripped() )
        {
			RawTriangles.Serialize( Ar, Owner );
        }

		Ar << Elements;

		if( !StripFlags.IsDataStrippedForServer() )
        {
			PositionVertexBuffer.Serialize( Ar, bNeedsCPUAccess );
			VertexBuffer.Serialize( Ar, bNeedsCPUAccess );
			ColorVertexBuffer.Serialize( Ar, bNeedsCPUAccess );
			Ar << NumVertices;
			IndexBuffer.Serialize( Ar, bNeedsCPUAccess );
			if (Ar.UE4Ver() >= VER_UE4_SHADOW_ONLY_INDEX_BUFFERS)
            {		
				ShadowIndexBuffer.Serialize(Ar, bNeedsCPUAccess);
            }
			if( !StripFlags.IsEditorDataStripped() )
            {
				Ar << WireframeIndexBuffer;
            }
			if ( !StripFlags.IsClassDataStripped( AdjacencyDataStripFlag ) )
            { 					
				AdjacencyIndexBuffer.Serialize( Ar, bNeedsCPUAccess );
            }
        }

		if (Ar.IsLoading())
        {
			if (PositionVertexBuffer.GetNumVertices() != NumVertices)
            {
				PositionVertexBuffer.RemoveLegacyShadowVolumeVertices(NumVertices);
			}
			if (VertexBuffer.GetNumVertices() != NumVertices)
			{
				VertexBuffer.RemoveLegacyShadowVolumeVertices(NumVertices);
			}
			if (VertexBuffer.GetNumVertices() != NumVertices)
            {				
				ColorVertexBuffer.RemoveLegacyShadowVolumeVertices(NumVertices);
			}
        }
    }										
};

/**
 * Constructs a raw mesh from legacy raw triangles.
 */
static bool BuildRawMeshFromRawTriangles(
	FRawMesh& OutRawMesh,
	FMeshBuildSettings& OutBuildSettings,
	struct FStaticMeshTriangle* RawTriangles,
	int32 NumTriangles,
	const TCHAR* MeshName
	)
{
	int32 NumWedges = NumTriangles * 3;
	OutRawMesh.Empty();
	OutRawMesh.FaceMaterialIndices.Empty(NumTriangles);
	OutRawMesh.FaceSmoothingMasks.Empty(NumWedges);
	OutRawMesh.VertexPositions.Empty(NumWedges);
	OutRawMesh.WedgeIndices.Empty(NumWedges);

	int32 NumTexCoords = 0;
	bool bHaveNormals = false;
	bool bHaveTangents = false;
	bool bHaveColors = false;
	bool bCorruptFlags = false;
	bool bCorruptNormals = false;
	bool bCorruptTangents = false;
	bool bCorruptPositions = false;

	for (int32 TriIndex = 0; TriIndex < NumTriangles; ++TriIndex)
    {
		FStaticMeshTriangle* Tri = RawTriangles + TriIndex;
		OutRawMesh.FaceMaterialIndices.Add(Tri->MaterialIndex);
		OutRawMesh.FaceSmoothingMasks.Add(Tri->SmoothingMask);

		bCorruptFlags |= (RawTriangles[0].bExplicitNormals & 0xFFFFFFFE) != 0
			|| (RawTriangles[0].bOverrideTangentBasis & 0xFFFFFFFE) != 0;

		for (int32 CornerIndex = 0; CornerIndex < 3; ++CornerIndex)
        {
			FVector Position = Tri->Vertices[CornerIndex];
			OutRawMesh.WedgeIndices.Add(OutRawMesh.VertexPositions.Add(Position));
			NumTexCoords = FMath::Max(NumTexCoords, Tri->NumUVs);
			bHaveNormals |= Tri->TangentZ[CornerIndex] != FVector::ZeroVector;
			bHaveTangents |= (Tri->TangentX[CornerIndex] != FVector::ZeroVector
				&& Tri->TangentY[CornerIndex] != FVector::ZeroVector);
			bHaveColors |= Tri->Colors[CornerIndex] != FColor::White;

			bCorruptPositions |= Position.ContainsNaN();
			bCorruptTangents |= Tri->TangentX[CornerIndex].ContainsNaN()
				|| Tri->TangentY[CornerIndex].ContainsNaN();
			bCorruptNormals |= Tri->TangentZ[CornerIndex].ContainsNaN();
        }
    }

	bool bTooManyTexCoords = NumTexCoords > 8;
	NumTexCoords = FMath::Min(NumTexCoords, 8); // FStaticMeshTriangle has at most 8 texture coordinates.
	NumTexCoords = FMath::Min<int32>(NumTexCoords, MAX_MESH_TEXTURE_COORDS);

	for (int32 TexCoordIndex = 0; TexCoordIndex < FMath::Max(1, NumTexCoords); ++TexCoordIndex)
	{
		OutRawMesh.WedgeTexCoords[TexCoordIndex].AddZeroed(NumTriangles * 3);
	}

	for (int32 TriIndex = 0; TriIndex < NumTriangles; ++TriIndex)
	{
		FStaticMeshTriangle* Tri = RawTriangles + TriIndex;
		for (int32 CornerIndex = 0; CornerIndex < 3; ++CornerIndex)
		{
			for (int32 TexCoordIndex = 0; TexCoordIndex < NumTexCoords; ++TexCoordIndex)
			{
				FVector2D UV = TexCoordIndex < Tri->NumUVs ? Tri->UVs[CornerIndex][TexCoordIndex] : FVector2D(0.0f,0.0f);
				OutRawMesh.WedgeTexCoords[TexCoordIndex][TriIndex * 3 + CornerIndex] = UV;
			}
		}
	}

	if (bHaveNormals && !bCorruptNormals)
	{
		OutRawMesh.WedgeTangentZ.AddZeroed(NumTriangles * 3);
		for (int32 TriIndex = 0; TriIndex < NumTriangles; ++TriIndex)
		{
			if (RawTriangles[TriIndex].bExplicitNormals || RawTriangles[TriIndex].bOverrideTangentBasis)
			{
				for (int32 CornerIndex = 0; CornerIndex < 3; ++CornerIndex)
				{
					OutRawMesh.WedgeTangentZ[TriIndex * 3 + CornerIndex] =
						RawTriangles[TriIndex].TangentZ[CornerIndex];
				}
			}
		}
	}
	if (bHaveTangents && !bCorruptTangents)
	{
		OutRawMesh.WedgeTangentX.AddZeroed(NumTriangles * 3);
		OutRawMesh.WedgeTangentY.AddZeroed(NumTriangles * 3);
		for (int32 TriIndex = 0; TriIndex < NumTriangles; ++TriIndex)
	{
			if (RawTriangles[TriIndex].bOverrideTangentBasis)
			{
				for (int32 CornerIndex = 0; CornerIndex < 3; ++CornerIndex)
				{
					OutRawMesh.WedgeTangentX[TriIndex * 3 + CornerIndex] =
						RawTriangles[TriIndex].TangentX[CornerIndex];
					OutRawMesh.WedgeTangentY[TriIndex * 3 + CornerIndex] =
						RawTriangles[TriIndex].TangentY[CornerIndex];
				}
			}
		}
	}

	if (bHaveColors)
	{
		OutRawMesh.WedgeColors.AddUninitialized(NumTriangles * 3);
		for (int32 TriIndex = 0; TriIndex < NumTriangles; ++TriIndex)
		{
			for (int32 CornerIndex = 0; CornerIndex < 3; ++CornerIndex)
			{
				OutRawMesh.WedgeColors[TriIndex * 3 + CornerIndex] =
					RawTriangles[TriIndex].Colors[CornerIndex];
			}
		}
	}

	if (bCorruptPositions || bCorruptFlags || bCorruptNormals || bCorruptTangents || bTooManyTexCoords)
	{
		UE_LOG(LogStaticMesh,Verbose,TEXT("Legacy raw triangles CORRUPT (%s%s%s%s%s) for %s"),
			bCorruptPositions ? TEXT("NaN positions,") : TEXT(""),
			bCorruptFlags ? TEXT("flags,") : TEXT(""),
			bCorruptNormals ? TEXT("NaN normals,") : TEXT(""),
			bCorruptTangents ? TEXT("NaN tangents,") : TEXT(""),
			bTooManyTexCoords ? TEXT(">8 texcoords") : TEXT(""),
			MeshName
			);
	}

	if (bCorruptPositions)
	{
		OutRawMesh = FRawMesh();
	}

	OutBuildSettings.bRecomputeTangents = !bHaveTangents || bCorruptTangents;
	OutBuildSettings.bRecomputeNormals = !bHaveNormals || bCorruptNormals;

	return !(bCorruptPositions || bCorruptFlags || bTooManyTexCoords) && OutRawMesh.IsValid();
}

/**
 * Constructs a raw mesh from legacy render data.
 */
static void BuildRawMeshFromRenderData(
	FRawMesh& OutRawMesh,
	struct FMeshBuildSettings& OutBuildSettings,
	FLegacyStaticMeshRenderData& RenderData,
	const TCHAR* MeshName
	)
{
	FStaticMeshTriangle* RawTriangles = (FStaticMeshTriangle*)RenderData.RawTriangles.Lock(LOCK_READ_ONLY);
	bool bBuiltFromRawTriangles = BuildRawMeshFromRawTriangles(
		OutRawMesh,
		OutBuildSettings,
		RawTriangles,
		RenderData.RawTriangles.GetElementCount(),
		MeshName
		);
	RenderData.RawTriangles.Unlock();

	if (bBuiltFromRawTriangles)
	{
		return;
	}

	OutRawMesh.Empty();
	
	FIndexArrayView Indices = RenderData.IndexBuffer.GetArrayView();
	int32 NumVertices = RenderData.PositionVertexBuffer.GetNumVertices();
	int32 NumTriangles = Indices.Num() / 3;
	int32 NumWedges = NumTriangles * 3;

	// Copy vertex positions.
	{
		OutRawMesh.VertexPositions.AddUninitialized(NumVertices);
		for (int32 i = 0; i < NumVertices; ++i)
		{
			OutRawMesh.VertexPositions[i] = RenderData.PositionVertexBuffer.VertexPosition(i);
		}
	}

	// Copy per-wedge texture coordinates.
	for (uint32 TexCoordIndex = 0; TexCoordIndex < RenderData.VertexBuffer.GetNumTexCoords(); ++TexCoordIndex)
	{
		OutRawMesh.WedgeTexCoords[TexCoordIndex].AddUninitialized(NumWedges);
		for (int32 i = 0; i < NumWedges; ++i)
		{
			uint32 VertIndex = Indices[i];
			OutRawMesh.WedgeTexCoords[TexCoordIndex][i] = RenderData.VertexBuffer.GetVertexUV(VertIndex, TexCoordIndex);
		}
	}

	// Copy per-wedge colors if they exist.
	if (RenderData.ColorVertexBuffer.GetNumVertices() > 0)
	{
		OutRawMesh.WedgeColors.AddUninitialized(NumWedges);
		for (int32 i = 0; i < NumWedges; ++i)
		{
			uint32 VertIndex = Indices[i];
			OutRawMesh.WedgeColors[i] = RenderData.ColorVertexBuffer.VertexColor(VertIndex);
		}
	}

	// Copy per-wedge tangents.
	{
		OutRawMesh.WedgeTangentX.AddUninitialized(NumWedges);
		OutRawMesh.WedgeTangentY.AddUninitialized(NumWedges);
		OutRawMesh.WedgeTangentZ.AddUninitialized(NumWedges);
		for (int32 i = 0; i < NumWedges; ++i)
		{
			uint32 VertIndex = Indices[i];
			OutRawMesh.WedgeTangentX[i] = RenderData.VertexBuffer.VertexTangentX(VertIndex);
			OutRawMesh.WedgeTangentY[i] = RenderData.VertexBuffer.VertexTangentY(VertIndex);
			OutRawMesh.WedgeTangentZ[i] = RenderData.VertexBuffer.VertexTangentZ(VertIndex);
		}
	}

	// Copy per-face information.
	{
		OutRawMesh.FaceMaterialIndices.AddZeroed(NumTriangles);
		OutRawMesh.FaceSmoothingMasks.AddZeroed(NumTriangles);
		OutRawMesh.WedgeIndices.AddZeroed(NumWedges);
		for (int32 SectionIndex = 0; SectionIndex < RenderData.Elements.Num(); ++SectionIndex)
		{
			const FLegacyStaticMeshElement& Section = RenderData.Elements[SectionIndex];
			int32 FirstFace = Section.FirstIndex / 3;
			int32 LastFace = FirstFace + Section.NumTriangles;
			for (int32 i = FirstFace; i < LastFace; ++i)
			{
				OutRawMesh.FaceMaterialIndices[i] = Section.MaterialIndex;
				// Smoothing group information has been lost but is already baked in to the tangent basis.
				for (int32 j = 0; j < 3; ++j)
				{
					OutRawMesh.WedgeIndices[i * 3 + j] = Indices[i * 3 + j];
				}
			}
		}
	}

	check(OutRawMesh.IsValid());

	OutBuildSettings.bRecomputeNormals = false;
	OutBuildSettings.bRecomputeTangents = false;
	OutBuildSettings.bRemoveDegenerates = false;
	OutBuildSettings.bUseFullPrecisionUVs = RenderData.VertexBuffer.GetUseFullPrecisionUVs();
}

void UStaticMesh::SerializeLegacySouceData(FArchive& Ar, const FBoxSphereBounds& LegacyBounds)
{
	int32 InternalVersion = 0;
	bool bHaveSourceData = false;
	TArray<FStaticMeshOptimizationSettings> OptimizationSettings;
	bool bHasBeenSimplified = false;

	Ar << InternalVersion;
	Ar << bHaveSourceData;
	if (bHaveSourceData)
	{
		FStaticMeshSourceModel& SrcModel = *new(SourceModels) FStaticMeshSourceModel();

		// Serialize in the old source render data.
		FLegacyStaticMeshRenderData TempRenderData;
		TempRenderData.Serialize(Ar, this, INDEX_NONE);

		// Convert it to a raw mesh.
		FRawMesh RawMesh;
		BuildRawMeshFromRenderData(RawMesh, SrcModel.BuildSettings, TempRenderData, *GetPathName());

		// Store the raw mesh as our source model.
		SrcModel.RawMeshBulkData->SaveRawMesh(RawMesh);
	}
	Ar << OptimizationSettings;
	Ar << bHasBeenSimplified;

	// Convert any old optimization settings to the source model's reduction settings.
	for (int32 i = 0; i < OptimizationSettings.Num(); ++i)
	{
		if (!SourceModels.IsValidIndex(i))
		{
			new(SourceModels) FStaticMeshSourceModel();
		}
		FStaticMeshSourceModel& SrcModel = SourceModels[i];

		FMeshReductionSettings& NewSettings = SrcModel.ReductionSettings;
		switch (OptimizationSettings[i].ReductionMethod)
		{
		case OT_NumOfTriangles:
			NewSettings.PercentTriangles = FMath::Clamp(OptimizationSettings[i].NumOfTrianglesPercentage / 100.0f, 0.0f, 1.0f);
			NewSettings.MaxDeviation = 0.0f;
			break;
		case OT_MaxDeviation:
			NewSettings.PercentTriangles = 1.0f;
			NewSettings.MaxDeviation = OptimizationSettings[i].MaxDeviationPercentage * LegacyBounds.SphereRadius;
			break;
		default:
			NewSettings.PercentTriangles = 1.0f;
			NewSettings.MaxDeviation = 0.0f;
			break;
		}
		NewSettings.WeldingThreshold = OptimizationSettings[i].WeldingThreshold;
		NewSettings.HardAngleThreshold = OptimizationSettings[i].NormalsThreshold;
		NewSettings.SilhouetteImportance = (EMeshFeatureImportance::Type)OptimizationSettings[i].SilhouetteImportance;
		NewSettings.TextureImportance = (EMeshFeatureImportance::Type)OptimizationSettings[i].TextureImportance;
		NewSettings.ShadingImportance = (EMeshFeatureImportance::Type)OptimizationSettings[i].ShadingImportance;
	}

	// Serialize in any legacy LOD models.
	TIndirectArray<FLegacyStaticMeshRenderData> LegacyLODModels;
	LegacyLODModels.Serialize(Ar, this);

	TArray<FLegacyStaticMeshLODInfo> LegacyLODInfo;
	Ar << LegacyLODInfo;

	for (int32 LODIndex = 0; LODIndex < LegacyLODModels.Num(); ++LODIndex)
	{
		if (!SourceModels.IsValidIndex(LODIndex))
		{
			new(SourceModels) FStaticMeshSourceModel();
		}
		FStaticMeshSourceModel& SrcModel = SourceModels[LODIndex];

		// Really we want to use LOD0 only if the mesh has /not/ been simplified.
		// Unfortunately some data seems to be corrupted in source meshes but not LOD0, so just use LOD0.
		//if (SrcModel.RawMeshBulkData->IsEmpty() && (!bHasBeenSimplified || LODIndex == 0))
		{
			// Store the raw mesh for this LOD.
			FRawMesh RawMesh;
			BuildRawMeshFromRenderData(RawMesh, SrcModel.BuildSettings, LegacyLODModels[LODIndex], *GetPathName());
			SrcModel.RawMeshBulkData->SaveRawMesh(RawMesh);
		}

		// And make sure to clear reduction settings for LOD0.
		if (LODIndex == 0)
		{
			FMeshReductionSettings DefaultSettings;
			SrcModel.ReductionSettings = DefaultSettings;
		}

		// Always use a hash as the guid for legacy models.
		SrcModel.RawMeshBulkData->UseHashAsGuid(this);

		// Setup the material map for this LOD model.
		if (LODIndex == 0)
		{
			for (int32 SectionIndex = 0; SectionIndex < LegacyLODModels[LODIndex].Elements.Num(); ++SectionIndex)
			{
				FMeshSectionInfo Info;
				Info.MaterialIndex = Materials.Add(LegacyLODModels[LODIndex].Elements[SectionIndex].Material);
				Info.bEnableCollision = LegacyLODModels[LODIndex].Elements[SectionIndex].EnableCollision;
				Info.bCastShadow = LegacyLODModels[LODIndex].Elements[SectionIndex].bEnableShadowCasting;
				SectionInfoMap.Set(LODIndex, SectionIndex, Info);
			}
		}
		else
		{
			for (int32 SectionIndex = 0; SectionIndex < LegacyLODModels[LODIndex].Elements.Num(); ++SectionIndex)
			{
				FMeshSectionInfo Info;
				Info.MaterialIndex = Materials.AddUnique(LegacyLODModels[LODIndex].Elements[SectionIndex].Material);
				Info.bEnableCollision = LegacyLODModels[LODIndex].Elements[SectionIndex].EnableCollision;
				Info.bCastShadow = LegacyLODModels[LODIndex].Elements[SectionIndex].bEnableShadowCasting;
				SectionInfoMap.Set(LODIndex, SectionIndex, Info);
			}
		}
	}
}

void UStaticMesh::FixupZeroTriangleSections()
{
	if (RenderData->MaterialIndexToImportIndex.Num() > 0 && RenderData->LODResources.Num())
	{
		TArray<int32> MaterialMap;
		FMeshSectionInfoMap NewSectionInfoMap;

		// Iterate over all sections of all LODs and identify all material indices that need to be remapped.
		for (int32 LODIndex = 0; LODIndex < RenderData->LODResources.Num(); ++ LODIndex)
		{
			FStaticMeshLODResources& LOD = RenderData->LODResources[LODIndex];
			int32 NumSections = LOD.Sections.Num();

			for (int32 SectionIndex = 0; SectionIndex < NumSections; ++SectionIndex)
			{
				FMeshSectionInfo DefaultSectionInfo(SectionIndex);
				if (RenderData->MaterialIndexToImportIndex.IsValidIndex(SectionIndex))
				{
					int32 ImportIndex = RenderData->MaterialIndexToImportIndex[SectionIndex];
					FMeshSectionInfo SectionInfo = SectionInfoMap.Get(LODIndex, ImportIndex);
					int32 OriginalMaterialIndex = SectionInfo.MaterialIndex;

					// If import index == material index, remap it.
					if (SectionInfo.MaterialIndex == ImportIndex)
					{
						SectionInfo.MaterialIndex = SectionIndex;
					}

					// Update the material mapping table.
					while (SectionInfo.MaterialIndex >= MaterialMap.Num())
					{
						MaterialMap.Add(INDEX_NONE);
					}
					if (SectionInfo.MaterialIndex >= 0)
					{
						MaterialMap[SectionInfo.MaterialIndex] = OriginalMaterialIndex;
					}

					// Update the new section info map if needed.
					if (SectionInfo != DefaultSectionInfo)
					{
						NewSectionInfoMap.Set(LODIndex, SectionIndex, SectionInfo);
					}
				}
			}
		}

		// Compact the materials array.
		for (int32 i = RenderData->LODResources[0].Sections.Num(); i < MaterialMap.Num(); ++i)
		{
			if (MaterialMap[i] == INDEX_NONE)
			{
				int32 NextValidIndex = i+1;
				for (; NextValidIndex < MaterialMap.Num(); ++NextValidIndex)
				{
					if (MaterialMap[NextValidIndex] != INDEX_NONE)
					{
						break;
					}
				}
				if (MaterialMap.IsValidIndex(NextValidIndex))
				{
					MaterialMap[i] = MaterialMap[NextValidIndex];
					for (TMap<uint32,FMeshSectionInfo>::TIterator It(NewSectionInfoMap.Map); It; ++It)
					{
						FMeshSectionInfo& SectionInfo = It.Value();
						if (SectionInfo.MaterialIndex == NextValidIndex)
						{
							SectionInfo.MaterialIndex = i;
						}
					}
				}
				MaterialMap.RemoveAt(i, NextValidIndex - i);
			}
		}

		SectionInfoMap.Clear();
		SectionInfoMap.CopyFrom(NewSectionInfoMap);

		// Check if we need to remap materials.
		bool bRemapMaterials = false;
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialMap.Num(); ++MaterialIndex)
		{
			if (MaterialMap[MaterialIndex] != MaterialIndex)
			{
				bRemapMaterials = true;
				break;
			}
		}

		// Remap the materials array if needed.
		if (bRemapMaterials)
		{
			TArray<UMaterialInterface*> OldMaterials;
			Exchange(Materials,OldMaterials);
			Materials.Empty(MaterialMap.Num());
			for (int32 MaterialIndex = 0; MaterialIndex < MaterialMap.Num(); ++MaterialIndex)
			{
				UMaterialInterface* Material = NULL;
				int32 OldMaterialIndex = MaterialMap[MaterialIndex];
				if (OldMaterials.IsValidIndex(OldMaterialIndex))
				{
					Material = OldMaterials[OldMaterialIndex];
				}
				Materials.Add(Material);
			}
		}
	}
	else
	{
		int32 FoundMaxMaterialIndex = -1;
		TSet<int32> DiscoveredMaterialIndices;
		
		// Find the maximum material index that is used by the mesh
		// Also keep track of which materials are actually used in the array
		for(int32 LODIndex = 0; LODIndex < RenderData->LODResources.Num(); ++LODIndex)
		{
			if (RenderData->LODResources.IsValidIndex(LODIndex))
			{
				FStaticMeshLODResources& LOD = RenderData->LODResources[LODIndex];
				int32 NumSections = LOD.Sections.Num();
				for (int32 SectionIndex = 0; SectionIndex < NumSections; ++SectionIndex)
				{
					FMeshSectionInfo Info = SectionInfoMap.Get(LODIndex, SectionIndex);
					if(Info.MaterialIndex > FoundMaxMaterialIndex)
					{
						FoundMaxMaterialIndex = Info.MaterialIndex;
					}

					DiscoveredMaterialIndices.Add(Info.MaterialIndex);
				}
			}
		}

		// NULL references to materials in indices that are not used by any LOD.
		// This is to fix up an import bug which caused more materials to be added to this array than needed.
		for ( int32 MaterialIdx = 0; MaterialIdx < Materials.Num(); ++MaterialIdx )
		{
			if ( !DiscoveredMaterialIndices.Contains(MaterialIdx) )
			{
				// Materials that are not used by any LOD resource should not be in this array.
				Materials[MaterialIdx] = NULL;
			}
		}

		// Remove entries at the end of the materials array.
		if (Materials.Num() > (FoundMaxMaterialIndex + 1))
		{
			Materials.RemoveAt(FoundMaxMaterialIndex+1, Materials.Num() - FoundMaxMaterialIndex - 1);
		}
	}
}

#endif // #if WITH_EDITORONLY_DATA

#undef LOCTEXT_NAMESPACE
