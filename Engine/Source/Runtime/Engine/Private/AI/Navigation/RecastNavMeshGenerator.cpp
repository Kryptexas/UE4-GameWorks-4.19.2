// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#if WITH_RECAST

#if WITH_PHYSX
	#include "../../PhysicsEngine/PhysXSupport.h"
#endif
#include "RecastNavMeshGenerator.h"
#include "PImplRecastNavMesh.h"
#include "SurfaceIterators.h"

// recast includes
#include "Recast.h"
#include "DetourNavMeshBuilder.h"
#include "DetourNavMeshQuery.h"
#include "RecastAlloc.h"
#include "DetourTileCache.h"
#include "DetourTileCacheBuilder.h"
#include "RecastHelpers.h"

#define SEAMLESS_REBUILDING_ENABLED 1

#define GENERATE_SEGMENT_LINKS 1
#define GENERATE_CLUSTER_LINKS 1

/** if set, each tile generator will allocate copy of geometry data, to be used for async processing */
#define CACHE_NAV_GENERATOR_DATA	RECAST_ASYNC_REBUILDING

#define SHOW_NAV_EXPORT_PREVIEW 0

#define TEXT_WEAKOBJ_NAME(obj) (obj.IsValid(false) ? *obj->GetName() : (obj.IsValid(false, true)) ? TEXT("MT-Unreachable") : TEXT("INVALID"))

#define DO_RECAST_STATS 0
#if DO_RECAST_STATS
#define RECAST_STAT SCOPE_CYCLE_COUNTER
#else
#define RECAST_STAT(...) 
#endif

#if !NAVOCTREE_CONTAINS_COLLISION_DATA
#error FRecastNavMeshGenerator requires NavOctree to contain additional data
#endif

/** structure to cache owning RecastNavMesh data so that it doesn't have to be polled
 *	directly from RecastNavMesh while asyncronously generating navmesh */
struct FRecastNavMeshCachedData
{
	typedef TMap<const UClass*, uint8> TAreaClassIDMap;

	struct FConvexNavAreaModifierRecastSortPredicate
	{
		float AreaCosts[RECAST_MAX_AREAS];
		float AreaFixedCosts[RECAST_MAX_AREAS];
		TAreaClassIDMap* AreaClassToIDMap;

		FORCEINLINE void Prepare(FAreaNavModifier* Modifier) const
		{
			const uint8* AreaID = AreaClassToIDMap->Find(Modifier->GetAreaClass());
			if (AreaID != NULL)
			{
				check(*AreaID < RECAST_MAX_AREAS);
				Modifier->Cost = AreaCosts[*AreaID];
				Modifier->FixedCost = AreaFixedCosts[*AreaID];
			}
		}

		FORCEINLINE bool operator()( const FAreaNavModifier& A, const FAreaNavModifier& B ) const 
		{ 
			return A.FixedCost == B.FixedCost ? A.Cost > B.Cost : A.FixedCost > B.FixedCost;
		}
	};

	ARecastNavMesh::FNavPolyFlags FlagsPerArea[RECAST_MAX_AREAS];
	ARecastNavMesh::FNavPolyFlags FlagsPerOffMeshLinkArea[RECAST_MAX_AREAS];
	TAreaClassIDMap AreaClassToIDMap;

	FConvexNavAreaModifierRecastSortPredicate SortPredicate;
	uint32 bSortNavigationAreasByCost : 1;

	FRecastNavMeshCachedData(const ARecastNavMesh* RecastNavMeshActor)
	{
		check(RecastNavMeshActor);

		if (RecastNavMeshActor != NULL)
		{
			// create copies from crucial ARecastNavMesh data
			bSortNavigationAreasByCost = RecastNavMeshActor->bSortNavigationAreasByCost;

			TArray<FSupportedAreaData> Areas;
			RecastNavMeshActor->GetSupportedAreas(Areas);
			FMemory::Memzero(FlagsPerArea, sizeof(ARecastNavMesh::FNavPolyFlags) * RECAST_MAX_AREAS);

			for (int32 i = 0; i < Areas.Num(); i++)
			{
				const UClass* AreaClass = Areas[i].AreaClass;
				const UNavArea* DefArea = AreaClass ? ((UClass*)AreaClass)->GetDefaultObject<UNavArea>() : NULL;
				if (DefArea)
				{
					AreaClassToIDMap.Add(AreaClass, Areas[i].AreaID);
					FlagsPerArea[Areas[i].AreaID] = DefArea->GetAreaFlags();
				}
			}

			FMemory::Memcpy(FlagsPerOffMeshLinkArea, FlagsPerArea, sizeof(FlagsPerArea));
			static const ARecastNavMesh::FNavPolyFlags NavLinkFlag = ARecastNavMesh::GetNavLinkFlag();
			if (NavLinkFlag != 0)
			{
				ARecastNavMesh::FNavPolyFlags* AreaFlag = FlagsPerOffMeshLinkArea;
				for (int32 AreaIndex = 0; AreaIndex < RECAST_MAX_AREAS; ++AreaIndex, ++AreaFlag)
				{
					*AreaFlag |= NavLinkFlag;
				}
			}
		
			SortPredicate.AreaClassToIDMap = &AreaClassToIDMap;
			RecastNavMeshActor->GetDefaultQueryFilter()->GetAllAreaCosts(SortPredicate.AreaCosts, SortPredicate.AreaFixedCosts, RECAST_MAX_AREAS);
		}
	}

	void OnAreaAdded(const UClass* AreaClass, int32 AreaID)
	{
		const UNavArea* DefArea = AreaClass ? ((UClass*)AreaClass)->GetDefaultObject<UNavArea>() : NULL;
		if (DefArea && AreaID >= 0)
		{
			AreaClassToIDMap.Add(AreaClass, AreaID);
			FlagsPerArea[AreaID] = DefArea->GetAreaFlags();

			static const ARecastNavMesh::FNavPolyFlags NavLinkFlag = ARecastNavMesh::GetNavLinkFlag();
			if (NavLinkFlag != 0)
			{
				FlagsPerOffMeshLinkArea[AreaID] = FlagsPerArea[AreaID] | NavLinkFlag;
			}

			SortPredicate.AreaCosts[AreaID] = DefArea->DefaultCost;
			SortPredicate.AreaFixedCosts[AreaID] = DefArea->FixedAreaEnteringCost;
		}		
	}
};


//
class FAsyncNavTileBuildWorker : public FNonAbandonableTask 
{
public:
	/**
	 * Initializes the data and creates the async compression task.
	 */
	FAsyncNavTileBuildWorker(const ANavigationData::FNavDataGeneratorSharedPtr& InNavMeshGenerator, const int32 InTileId, const uint32 InVersion)
		: NavMeshGenerator(InNavMeshGenerator), TileId(InTileId), Version(InVersion)
	{
	}

	~FAsyncNavTileBuildWorker()
	{
	}

	/**
	 * Compresses the texture
	 */
	FORCEINLINE_DEBUGGABLE void DoWork()
	{
//		UE_LOG(LogNavigation, Warning, TEXT("FAsyncNavTileBuildWorker::DoWork %d"), TileId);
		if (NavMeshGenerator.IsValid())
		{
			FRecastNavMeshGenerator* const RecastNavMeshGenerator = static_cast<FRecastNavMeshGenerator*>(NavMeshGenerator.Get());
			SCOPE_CYCLE_COUNTER(STAT_Navigation_BuildTime)
			STAT(double ThisTime = 0);
			{
				SCOPE_SECONDS_COUNTER(ThisTime);
				RecastNavMeshGenerator->GenerateTile(TileId, Version);	
			
				FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
					FSimpleDelegateGraphTask::FDelegate::CreateThreadSafeSP(RecastNavMeshGenerator, &FRecastNavMeshGenerator::UpdateTileGenerationWorkers, TileId)
					, TEXT("Requesting to build next tile if necessary")
					, NULL
					, ENamedThreads::GameThread
					);
			}
			INC_FLOAT_STAT_BY(STAT_Navigation_CumulativeBuildTime,(float)ThisTime*1000);
		}
	}

	/** 
	 * Give the name for external event viewers
	 * @return	the name to display in external event viewers
	 */
	static const TCHAR* Name()
	{
		return TEXT("FAsyncNavTileBuildWorker");
	}

private:
	ANavigationData::FNavDataGeneratorSharedPtr NavMeshGenerator;
	const int32 TileId;
	const uint32 Version;
};

// Consts
static const int32 NAVMESH_TILE_GENERATOR_OWNS_DATA = 0; // @see DT_TILE_FREE_DATA
const float FRecastNavMeshGenerator::DefaultFreshness = 0.5f;

FORCEINLINE bool DoesBoxContainOrOverlapVector(const FBox& BigBox, const FVector& In)
{
	return (In.X >= BigBox.Min.X) && (In.X <= BigBox.Max.X) 
		&& (In.Y >= BigBox.Min.Y) && (In.Y <= BigBox.Max.Y) 
		&& (In.Z >= BigBox.Min.Z) && (In.Z <= BigBox.Max.Z);
}
/** main difference between this and FBox::ContainsBox is that this returns true also when edges overlap */
FORCEINLINE bool DoesBoxContainBox(const FBox& BigBox, const FBox& SmallBox)
{
	return DoesBoxContainOrOverlapVector(BigBox, SmallBox.Min) && DoesBoxContainOrOverlapVector(BigBox, SmallBox.Max);
}

/**
 * Exports geometry to OBJ file. Can be used to verify NavMesh generation in RecastDemo app
 * @param FileName - full name of OBJ file with extension
 * @param GeomVerts - list of vertices
 * @param GeomFaces - list of triangles (3 vert indices for each)
 */
static void ExportGeomToOBJFile(const FString& FileName, const TNavStatArray<float>& GeomCoords, const TNavStatArray<int32>& GeomFaces, const FString& AdditionalData)
{
#if ALLOW_DEBUG_FILES
	SCOPE_CYCLE_COUNTER(STAT_Navigation_TileGeometryExportToObjAsync);

	FArchive* FileAr = IFileManager::Get().CreateDebugFileWriter(*FileName);
	if (FileAr != NULL)
	{
		FStringOutputDevice Ar;
		for (int32 i = 0; i < GeomCoords.Num(); i += 3)
		{
			Ar.Logf(TEXT("v %f %f %f\n"), GeomCoords[i + 0], GeomCoords[i + 1], GeomCoords[i + 2]);
		}

		for (int32 i = 0; i < GeomFaces.Num(); i += 3)
		{
			Ar.Logf(TEXT("f %d %d %d\n"), GeomFaces[i + 0] +1, GeomFaces[i + 1] +1, GeomFaces[i + 2] +1);
		}

		FileAr->Logf(*Ar);
		FileAr->Logf(*AdditionalData);
		FileAr->Close();
	}
#endif
}

//----------------------------------------------------------------------//
// 
// 

struct FRecastGeometryExport : public FNavigableGeometryExport
{
	FRecastGeometryExport(FNavigationRelevantData& InData) : Data(&InData) 
	{
		Data->Bounds = FBox(ForceInit);
	}

	FNavigationRelevantData* Data;
	TNavStatArray<float> VertexBuffer;
	TNavStatArray<int32> IndexBuffer;
	FWalkableSlopeOverride SlopeOverride;

#if WITH_PHYSX
	virtual void ExportPxTriMesh16Bit(class physx::PxTriangleMesh const * const TriMesh, const FTransform& LocalToWorld) OVERRIDE;
	virtual void ExportPxTriMesh32Bit(class physx::PxTriangleMesh const * const TriMesh, const FTransform& LocalToWorld) OVERRIDE;
	virtual void ExportPxConvexMesh(class physx::PxConvexMesh const * const ConvexMesh, const FTransform& LocalToWorld) OVERRIDE;
#endif // WITH_PHYSX
	virtual void ExportCustomMesh(const FVector* InVertices, int32 NumVerts, const int32* InIndices, int32 NumIndices, const FTransform& LocalToWorld) OVERRIDE;
	virtual void AddNavModifiers(const FCompositeNavModifier& Modifiers) OVERRIDE;
};

FRecastVoxelCache::FRecastVoxelCache(const uint8* Memory)
{
	uint8* BytesArr = (uint8*)Memory;
	if (Memory)
	{
		NumTiles = *((int32*)BytesArr);	BytesArr += sizeof(int32);
		Tiles = (FTileInfo*)BytesArr;
	}
	else
	{
		NumTiles = 0;
	}

	FTileInfo* iTile = Tiles;	
	for (int i = 0; i < NumTiles; i++)
	{
		iTile = (FTileInfo*)BytesArr; BytesArr += sizeof(FTileInfo);
		if (iTile->NumSpans)
		{
			iTile->SpanData = (rcSpanCache*)BytesArr; BytesArr += sizeof(rcSpanCache) * iTile->NumSpans;
		}
		else
		{
			iTile->SpanData = 0;
		}

		iTile->NextTile = (FTileInfo*)BytesArr;
	}

	if (NumTiles > 0)
	{
		iTile->NextTile = 0;
	}
	else
	{
		Tiles = 0;
	}
}

FRecastGeometryCache::FRecastGeometryCache(const uint8* Memory)
{
	Header = *((FHeader*)Memory);
	Verts = (float*)(Memory + sizeof(FRecastGeometryCache));
	Indices = (int32*)(Memory + sizeof(FRecastGeometryCache) + (sizeof(float) * Header.NumVerts * 3));
}

namespace RecastGeometryExport {

static UWorld* FindEditorWorld()
{
	if (GEngine)
	{
		const TArray<FWorldContext>& Worlds = GEngine->GetWorldContexts();
		for (int32 i = 0; i < Worlds.Num(); i++)
		{
			if (Worlds[i].WorldType == EWorldType::Editor)
			{
				return Worlds[i].World();
			}
		}
	}

	return NULL;
}

static void StoreCollisionCache(FRecastGeometryExport* GeomExport)
{
	const int32 NumFaces = GeomExport->IndexBuffer.Num() / 3;
	const int32 NumVerts = GeomExport->VertexBuffer.Num() / 3;

	FRecastGeometryCache::FHeader HeaderInfo;
	HeaderInfo.NumFaces = NumFaces;
	HeaderInfo.NumVerts = NumVerts;
	HeaderInfo.SlopeOverride = GeomExport->SlopeOverride;

	// allocate memory
	const int32 HeaderSize = sizeof(FRecastGeometryCache);
	const int32 CoordsSize = sizeof(float) * 3 * NumVerts;
	const int32 IndicesSize = sizeof(int32) * 3 * NumFaces;
	const int32 CacheSize = HeaderSize + CoordsSize + IndicesSize;
	GeomExport->Data->CollisionData.SetNum(CacheSize);

	// store collisions
	uint8* RawMemory = GeomExport->Data->CollisionData.GetTypedData();
	FRecastGeometryCache* CacheMemory = (FRecastGeometryCache*)RawMemory;
	CacheMemory->Header = HeaderInfo;
	CacheMemory->Verts = 0;
	CacheMemory->Indices = 0;

	FMemory::Memcpy(RawMemory + HeaderSize, GeomExport->VertexBuffer.GetTypedData(), CoordsSize);
	FMemory::Memcpy(RawMemory + HeaderSize + CoordsSize, GeomExport->IndexBuffer.GetTypedData(), IndicesSize);
}

#if WITH_PHYSX
/** exports PxConvexMesh as trimesh */
void ExportPxConvexMesh(PxConvexMesh const * const ConvexMesh, const FTransform& LocalToWorld,
						TNavStatArray<float>& VertexBuffer, TNavStatArray<int32>& IndexBuffer,
						FBox& UnrealBounds)
{
	// after FKConvexElem::AddCachedSolidConvexGeom
	if(ConvexMesh == NULL)
	{
		return;
	}

	int32 StartVertOffset = VertexBuffer.Num() / 3;

	// get PhysX data
	const PxVec3* PVertices = ConvexMesh->getVertices();
	const PxU8* PIndexBuffer = ConvexMesh->getIndexBuffer();
	const PxU32 NbPolygons = ConvexMesh->getNbPolygons();

	const bool bFlipWinding = (LocalToWorld.GetDeterminant() < 0.f);
	const int FirstIndex = bFlipWinding ? 1 : 2;
	const int SecondIndex = bFlipWinding ? 2 : 1;

#if SHOW_NAV_EXPORT_PREVIEW
	UWorld* DebugWorld = FindEditorWorld();
#endif // SHOW_NAV_EXPORT_PREVIEW

	for(PxU32 i = 0; i < NbPolygons; ++i)
	{
		PxHullPolygon Data;
		bool bStatus = ConvexMesh->getPolygonData(i, Data);
		check(bStatus);

		const PxU8* indices = PIndexBuffer + Data.mIndexBase;
		
		// add vertices 
		for(PxU32 j = 0; j < Data.mNbVerts; ++j)
		{
			const int32 VertIndex = indices[j];
			const FVector UnrealCoords = LocalToWorld.TransformPosition( P2UVector(PVertices[VertIndex]) );
			UnrealBounds += UnrealCoords;

			VertexBuffer.Add(UnrealCoords.X);
			VertexBuffer.Add(UnrealCoords.Y);
			VertexBuffer.Add(UnrealCoords.Z);
		}

		// Add indices
		const PxU32 nbTris = Data.mNbVerts - 2;
		for(PxU32 j = 0; j < nbTris; ++j)
		{
			IndexBuffer.Add(StartVertOffset + 0 );
			IndexBuffer.Add(StartVertOffset + j + FirstIndex);
			IndexBuffer.Add(StartVertOffset + j + SecondIndex);

#if SHOW_NAV_EXPORT_PREVIEW
			if (DebugWorld)
			{
				FVector V0(VertexBuffer[(StartVertOffset + 0) * 3+0], VertexBuffer[(StartVertOffset + 0) * 3+1], VertexBuffer[(StartVertOffset + 0) * 3+2]);
				FVector V1(VertexBuffer[(StartVertOffset + j + FirstIndex) * 3+0], VertexBuffer[(StartVertOffset + j + FirstIndex) * 3+1], VertexBuffer[(StartVertOffset + j + FirstIndex) * 3+2]);
				FVector V2(VertexBuffer[(StartVertOffset + j + SecondIndex) * 3+0], VertexBuffer[(StartVertOffset + j + SecondIndex) * 3+1], VertexBuffer[(StartVertOffset + j + SecondIndex) * 3+2]);

				DrawDebugLine(DebugWorld, V0, V1, bFlipWinding ? FColor::Red : FColor::Blue, true);
				DrawDebugLine(DebugWorld, V1, V2, bFlipWinding ? FColor::Red : FColor::Blue, true);
				DrawDebugLine(DebugWorld, V2, V0, bFlipWinding ? FColor::Red : FColor::Blue, true);
			}
#endif // SHOW_NAV_EXPORT_PREVIEW
		}

		StartVertOffset += Data.mNbVerts;
	}
}

template<typename TIndicesType> 
FORCEINLINE_DEBUGGABLE void ExportPxTriMesh(PxTriangleMesh const * const TriMesh, const FTransform& LocalToWorld,
											TNavStatArray<float>& VertexBuffer, TNavStatArray<int32>& IndexBuffer,
											FBox& UnrealBounds)
{
	if (TriMesh == NULL)
	{
		return;
	}

	int32 VertOffset = VertexBuffer.Num() / 3;
	const PxVec3* PVerts = TriMesh->getVertices();
	const PxU32 NumTris = TriMesh->getNbTriangles();

	const TIndicesType* Indices = (TIndicesType*)TriMesh->getTriangles();;
		
	VertexBuffer.Reserve(VertexBuffer.Num() + NumTris*3);
	IndexBuffer.Reserve(IndexBuffer.Num() + NumTris*3*2);
	const bool bFlipCullMode = (LocalToWorld.GetDeterminant() < 0.f);
	const int32 IndexOrder[3] = { bFlipCullMode ? 0 : 2, 1, bFlipCullMode ? 2 : 0 };

#if SHOW_NAV_EXPORT_PREVIEW
	UWorld* DebugWorld = FindEditorWorld();
#endif // SHOW_NAV_EXPORT_PREVIEW

	for(PxU32 TriIdx = 0; TriIdx < NumTris; ++TriIdx)
	{
		for (int32 i = 0; i < 3; i++)
		{
			const FVector UnrealCoords = LocalToWorld.TransformPosition(P2UVector(PVerts[Indices[i]]));
			UnrealBounds += UnrealCoords;

			VertexBuffer.Add(UnrealCoords.X);
			VertexBuffer.Add(UnrealCoords.Y);
			VertexBuffer.Add(UnrealCoords.Z);
		}
		Indices += 3;

		IndexBuffer.Add(VertOffset + IndexOrder[0]);
		IndexBuffer.Add(VertOffset + IndexOrder[1]);
		IndexBuffer.Add(VertOffset + IndexOrder[2]);

#if SHOW_NAV_EXPORT_PREVIEW
		if (DebugWorld)
		{
			FVector V0(VertexBuffer[(VertOffset + IndexOrder[0]) * 3+0], VertexBuffer[(VertOffset + IndexOrder[0]) * 3+1], VertexBuffer[(VertOffset + IndexOrder[0]) * 3+2]);
			FVector V1(VertexBuffer[(VertOffset + IndexOrder[1]) * 3+0], VertexBuffer[(VertOffset + IndexOrder[1]) * 3+1], VertexBuffer[(VertOffset + IndexOrder[1]) * 3+2]);
			FVector V2(VertexBuffer[(VertOffset + IndexOrder[2]) * 3+0], VertexBuffer[(VertOffset + IndexOrder[2]) * 3+1], VertexBuffer[(VertOffset + IndexOrder[2]) * 3+2]);

			DrawDebugLine(DebugWorld, V0, V1, bFlipCullMode ? FColor::Red : FColor::Blue, true);
			DrawDebugLine(DebugWorld, V1, V2, bFlipCullMode ? FColor::Red : FColor::Blue, true);
			DrawDebugLine(DebugWorld, V2, V0, bFlipCullMode ? FColor::Red : FColor::Blue, true);
		}
#endif // SHOW_NAV_EXPORT_PREVIEW

		VertOffset += 3;
	}
}
#endif

void ExportCustomMesh(const FVector* InVertices, int32 NumVerts, const int32* InIndices, int32 NumIndices, const FTransform& LocalToWorld,
					  TNavStatArray<float>& VertexBuffer, TNavStatArray<int32>& IndexBuffer, FBox& UnrealBounds)
{
	if (NumVerts <= 0 || NumIndices <= 0)
	{
		return;
	}

	int32 VertOffset = VertexBuffer.Num() / 3;
	VertexBuffer.Reserve(VertexBuffer.Num() + NumVerts*3);
	IndexBuffer.Reserve(IndexBuffer.Num() + NumIndices);

	const bool bFlipCullMode = (LocalToWorld.GetDeterminant() < 0.f);
	const int32 IndexOrder[3] = { bFlipCullMode ? 2 : 0, 1, bFlipCullMode ? 0 : 2 };

#if SHOW_NAV_EXPORT_PREVIEW
	UWorld* DebugWorld = FindEditorWorld();
#endif // SHOW_NAV_EXPORT_PREVIEW

	// Add vertices
	for (int32 i = 0; i < NumVerts; ++i)
	{
		const FVector UnrealCoords = LocalToWorld.TransformPosition(InVertices[i]);
		UnrealBounds += UnrealCoords;

		VertexBuffer.Add(UnrealCoords.X);
		VertexBuffer.Add(UnrealCoords.Y);
		VertexBuffer.Add(UnrealCoords.Z);
	}

	// Add indices
	for (int32 i = 0; i < NumIndices; i += 3)
	{
		IndexBuffer.Add(InIndices[i + IndexOrder[0]] + VertOffset);
		IndexBuffer.Add(InIndices[i + IndexOrder[1]] + VertOffset);
		IndexBuffer.Add(InIndices[i + IndexOrder[2]] + VertOffset);

#if SHOW_NAV_EXPORT_PREVIEW
		if (DebugWorld)
		{
			FVector V0(VertexBuffer[(VertOffset + InIndices[i + IndexOrder[0]]) * 3+0], VertexBuffer[(VertOffset + InIndices[i + IndexOrder[0]]) * 3+1], VertexBuffer[(VertOffset + InIndices[i + IndexOrder[0]]) * 3+2]);
			FVector V1(VertexBuffer[(VertOffset + InIndices[i + IndexOrder[1]]) * 3+0], VertexBuffer[(VertOffset + InIndices[i + IndexOrder[1]]) * 3+1], VertexBuffer[(VertOffset + InIndices[i + IndexOrder[1]]) * 3+2]);
			FVector V2(VertexBuffer[(VertOffset + InIndices[i + IndexOrder[2]]) * 3+0], VertexBuffer[(VertOffset + InIndices[i + IndexOrder[2]]) * 3+1], VertexBuffer[(VertOffset + InIndices[i + IndexOrder[2]]) * 3+2]);

			DrawDebugLine(DebugWorld, V0, V1, bFlipCullMode ? FColor::Red : FColor::Blue, true);
			DrawDebugLine(DebugWorld, V1, V2, bFlipCullMode ? FColor::Red : FColor::Blue, true);
			DrawDebugLine(DebugWorld, V2, V0, bFlipCullMode ? FColor::Red : FColor::Blue, true);
		}
#endif // SHOW_NAV_EXPORT_PREVIEW
	}
}

template<typename OtherAllocator>
FORCEINLINE_DEBUGGABLE void AddFacesToRecast(TArray<FVector, OtherAllocator>& InVerts, TArray<int32, OtherAllocator>& InFaces,
											 TNavStatArray<float>& OutVerts, TNavStatArray<int32>& OutIndices, FBox& UnrealBounds)
{
	// Add indices
	int32 StartVertOffset = OutVerts.Num();
	if (StartVertOffset > 0)
	{
		const int32 FirstIndex = OutIndices.AddUninitialized(InFaces.Num());
		for (int32 Idx=0; Idx < InFaces.Num(); ++Idx)
		{
			OutIndices[FirstIndex + Idx] = InFaces[Idx]+StartVertOffset;
		}
	}
	else
	{
		OutIndices.Append(InFaces);
	}

	// Add vertices
	for (int32 i = 0; i < InVerts.Num(); i++)
	{
		const FVector& RecastCoords = InVerts[i];
		OutVerts.Add(RecastCoords.X);
		OutVerts.Add(RecastCoords.Y);
		OutVerts.Add(RecastCoords.Z);

		UnrealBounds += Recast2UnrealPoint(RecastCoords);
	}
}

FORCEINLINE_DEBUGGABLE void ExportRigidBodyConvexElements(UBodySetup& BodySetup, TNavStatArray<float>& VertexBuffer, TNavStatArray<int32>& IndexBuffer,
														  TNavStatArray<int32>& ShapeBuffer, FBox& UnrealBounds, const FTransform& LocalToWorld)
{
#if WITH_PHYSX
	const int32 ConvexCount = BodySetup.AggGeom.ConvexElems.Num();
	FKConvexElem const * ConvexElem = BodySetup.AggGeom.ConvexElems.GetTypedData();

	for(int32 i=0; i< ConvexCount; ++i, ++ConvexElem)
	{
		// Store index of first vertex in shape buffer
		ShapeBuffer.Add(VertexBuffer.Num() / 3);

		// Get verts/triangles from this hull.
		ExportPxConvexMesh(ConvexElem->ConvexMesh, LocalToWorld, VertexBuffer, IndexBuffer, UnrealBounds);
	}
#endif // WITH_PHYSX
}

FORCEINLINE_DEBUGGABLE void ExportRigidBodyTriMesh(UBodySetup& BodySetup, TNavStatArray<float>& VertexBuffer, TNavStatArray<int32>& IndexBuffer,
												   FBox& UnrealBounds, const FTransform& LocalToWorld)
{
#if WITH_PHYSX
	if (BodySetup.TriMesh != NULL && BodySetup.CollisionTraceFlag == CTF_UseComplexAsSimple)
	{
		if (BodySetup.TriMesh->getTriangleMeshFlags() & PxTriangleMeshFlag::eHAS_16BIT_TRIANGLE_INDICES)
		{
			ExportPxTriMesh<PxU16>(BodySetup.TriMesh, LocalToWorld, VertexBuffer, IndexBuffer, UnrealBounds);
		}
		else
		{
			ExportPxTriMesh<PxU32>(BodySetup.TriMesh, LocalToWorld, VertexBuffer, IndexBuffer, UnrealBounds);
		}
	}
#endif // WITH_PHYSX
}

void ExportRigidBodyBoxElements(UBodySetup& BodySetup, TNavStatArray<float>& VertexBuffer, TNavStatArray<int32>& IndexBuffer,
								TNavStatArray<int32>& ShapeBuffer, FBox& UnrealBounds, const FTransform& LocalToWorld)
{
	for (int32 i = 0; i < BodySetup.AggGeom.BoxElems.Num(); i++)
	{
		const FKBoxElem& BoxInfo = BodySetup.AggGeom.BoxElems[i];
		const FMatrix ElemTM = BoxInfo.GetTransform().ToMatrixWithScale() * LocalToWorld.ToMatrixWithScale();
		const FVector Extent(BoxInfo.X * 0.5f, BoxInfo.Y * 0.5f, BoxInfo.Z * 0.5f);

		const int32 VertBase = VertexBuffer.Num() / 3;
		
		// Store index of first vertex in shape buffer
		ShapeBuffer.Add(VertBase);

		// add box vertices
		FVector UnrealVerts[] = {
			ElemTM.TransformPosition(FVector(-Extent.X,  Extent.Y, -Extent.Z)),
			ElemTM.TransformPosition(FVector( Extent.X,  Extent.Y, -Extent.Z)),
			ElemTM.TransformPosition(FVector( Extent.X, -Extent.Y, -Extent.Z)),
			ElemTM.TransformPosition(FVector(-Extent.X, -Extent.Y, -Extent.Z)),
			ElemTM.TransformPosition(FVector(-Extent.X,  Extent.Y,  Extent.Z)),
			ElemTM.TransformPosition(FVector( Extent.X,  Extent.Y,  Extent.Z)),
			ElemTM.TransformPosition(FVector( Extent.X, -Extent.Y,  Extent.Z)),
			ElemTM.TransformPosition(FVector(-Extent.X, -Extent.Y,  Extent.Z))
		};

		for (int32 iv = 0; iv < ARRAY_COUNT(UnrealVerts); iv++)
		{
			UnrealBounds += UnrealVerts[iv];

			VertexBuffer.Add(UnrealVerts[iv].X);
			VertexBuffer.Add(UnrealVerts[iv].Y);
			VertexBuffer.Add(UnrealVerts[iv].Z);
		}
		
		// bottom
		IndexBuffer.Add(VertBase + 0); IndexBuffer.Add(VertBase + 1); IndexBuffer.Add(VertBase + 2);
		IndexBuffer.Add(VertBase + 0); IndexBuffer.Add(VertBase + 2); IndexBuffer.Add(VertBase + 3);

		// top
		IndexBuffer.Add(VertBase + 4); IndexBuffer.Add(VertBase + 6); IndexBuffer.Add(VertBase + 5);
		IndexBuffer.Add(VertBase + 4); IndexBuffer.Add(VertBase + 7); IndexBuffer.Add(VertBase + 6);

		// front
		IndexBuffer.Add(VertBase + 7); IndexBuffer.Add(VertBase + 2); IndexBuffer.Add(VertBase + 6);
		IndexBuffer.Add(VertBase + 7); IndexBuffer.Add(VertBase + 3); IndexBuffer.Add(VertBase + 2);

		// back
		IndexBuffer.Add(VertBase + 4); IndexBuffer.Add(VertBase + 5); IndexBuffer.Add(VertBase + 1);
		IndexBuffer.Add(VertBase + 4); IndexBuffer.Add(VertBase + 1); IndexBuffer.Add(VertBase + 0);

		// left
		IndexBuffer.Add(VertBase + 7); IndexBuffer.Add(VertBase + 4); IndexBuffer.Add(VertBase + 0);
		IndexBuffer.Add(VertBase + 7); IndexBuffer.Add(VertBase + 0); IndexBuffer.Add(VertBase + 3);

		// right
		IndexBuffer.Add(VertBase + 6); IndexBuffer.Add(VertBase + 1); IndexBuffer.Add(VertBase + 5);
		IndexBuffer.Add(VertBase + 6); IndexBuffer.Add(VertBase + 2); IndexBuffer.Add(VertBase + 1);
	}
}

void ExportRigidBodySphylElements(UBodySetup& BodySetup, TNavStatArray<float>& VertexBuffer, TNavStatArray<int32>& IndexBuffer,
								  TNavStatArray<int32>& ShapeBuffer, FBox& UnrealBounds, const FTransform& LocalToWorld)
{
	TArray<FVector> ArcVerts;

	for (int32 i = 0; i < BodySetup.AggGeom.SphylElems.Num(); i++)
	{
		const FKSphylElem& SphylInfo = BodySetup.AggGeom.SphylElems[i];
		const FMatrix ElemTM = SphylInfo.GetTransform().ToMatrixWithScale() * LocalToWorld.ToMatrixWithScale();

		const int32 VertBase = VertexBuffer.Num() / 3;

		// Store index of first vertex in shape buffer
		ShapeBuffer.Add(VertBase);

		const int32 NumSides = 16;
		const int32 NumRings = (NumSides/2) + 1;

		// The first/last arc are on top of each other.
		const int32 NumVerts = (NumSides+1) * (NumRings+1);

		ArcVerts.Reset();
		ArcVerts.AddZeroed(NumRings+1);
		for (int32 RingIdx=0; RingIdx<NumRings+1; RingIdx++)
		{
			float Angle;
			float ZOffset;
			if (RingIdx <= NumSides/4)
			{
				Angle = ((float)RingIdx/(NumRings-1)) * PI;
				ZOffset = 0.5 * SphylInfo.Length;
			}
			else
			{
				Angle = ((float)(RingIdx-1)/(NumRings-1)) * PI;
				ZOffset = -0.5 * SphylInfo.Length;
			}

			// Note- unit sphere, so position always has mag of one. We can just use it for normal!		
			FVector SpherePos;
			SpherePos.X = 0.0f;
			SpherePos.Y = SphylInfo.Radius * FMath::Sin(Angle);
			SpherePos.Z = SphylInfo.Radius * FMath::Cos(Angle);

			ArcVerts[RingIdx] = SpherePos + FVector(0,0,ZOffset);
		}

		// Then rotate this arc NumSides+1 times.
		for (int32 SideIdx=0; SideIdx<NumSides+1; SideIdx++)
		{
			const FRotator ArcRotator(0, 360.f * ((float)SideIdx/NumSides), 0);
			const FRotationMatrix ArcRot( ArcRotator );
			const FMatrix ArcTM = ArcRot * ElemTM;

			for(int32 VertIdx=0; VertIdx<NumRings+1; VertIdx++)
			{
				const FVector UnrealVert = ArcTM.TransformPosition(ArcVerts[VertIdx]);
				UnrealBounds += UnrealVert;

				VertexBuffer.Add(UnrealVert.X);
				VertexBuffer.Add(UnrealVert.Y);
				VertexBuffer.Add(UnrealVert.Z);
			}
		}

		// Add all of the triangles to the mesh.
		for (int32 SideIdx=0; SideIdx<NumSides; SideIdx++)
		{
			const int32 a0start = VertBase + ((SideIdx+0) * (NumRings+1));
			const int32 a1start = VertBase + ((SideIdx+1) * (NumRings+1));

			for (int32 RingIdx=0; RingIdx<NumRings; RingIdx++)
			{
				IndexBuffer.Add(a0start + RingIdx + 0); IndexBuffer.Add(a1start + RingIdx + 0); IndexBuffer.Add(a0start + RingIdx + 1);
				IndexBuffer.Add(a1start + RingIdx + 0); IndexBuffer.Add(a1start + RingIdx + 1); IndexBuffer.Add(a0start + RingIdx + 1);
			}
		}
	}
}

void ExportRigidBodySphereElements(UBodySetup& BodySetup, TNavStatArray<float>& VertexBuffer, TNavStatArray<int32>& IndexBuffer,
								   TNavStatArray<int32>& ShapeBuffer, FBox& UnrealBounds, const FTransform& LocalToWorld)
{
	TArray<FVector> ArcVerts;

	for (int32 i = 0; i < BodySetup.AggGeom.SphereElems.Num(); i++)
	{
		const FKSphereElem& SphereInfo = BodySetup.AggGeom.SphereElems[i];
		const FMatrix ElemTM = SphereInfo.GetTransform().ToMatrixWithScale() * LocalToWorld.ToMatrixWithScale();

		const int32 VertBase = VertexBuffer.Num() / 3;

		// Store index of first vertex in shape buffer
		ShapeBuffer.Add(VertBase);

		const int32 NumSides = 16;
		const int32 NumRings = (NumSides/2) + 1;

		// The first/last arc are on top of each other.
		const int32 NumVerts = (NumSides+1) * (NumRings+1);

		ArcVerts.Reset();
		ArcVerts.AddZeroed(NumRings+1);
		for (int32 RingIdx=0; RingIdx<NumRings+1; RingIdx++)
		{
			float Angle = ((float)RingIdx/NumRings) * PI;

			// Note- unit sphere, so position always has mag of one. We can just use it for normal!			
			FVector& ArcVert = ArcVerts[RingIdx];
			ArcVert.X = 0.0f;
			ArcVert.Y = SphereInfo.Radius * FMath::Sin(Angle);
			ArcVert.Z = SphereInfo.Radius * FMath::Cos(Angle);
		}

		// Then rotate this arc NumSides+1 times.
		for (int32 SideIdx=0; SideIdx<NumSides+1; SideIdx++)
		{
			const FRotator ArcRotator(0, 360.f * ((float)SideIdx/NumSides), 0);
			const FRotationMatrix ArcRot( ArcRotator );
			const FMatrix ArcTM = ArcRot * ElemTM;

			for(int32 VertIdx=0; VertIdx<NumRings+1; VertIdx++)
			{
				const FVector UnrealVert = ArcTM.TransformPosition(ArcVerts[VertIdx]);
				UnrealBounds += UnrealVert;

				VertexBuffer.Add(UnrealVert.X);
				VertexBuffer.Add(UnrealVert.Y);
				VertexBuffer.Add(UnrealVert.Z);
			}
		}

		// Add all of the triangles to the mesh.
		for (int32 SideIdx=0; SideIdx<NumSides; SideIdx++)
		{
			const int32 a0start = VertBase + ((SideIdx+0) * (NumRings+1));
			const int32 a1start = VertBase + ((SideIdx+1) * (NumRings+1));

			for (int32 RingIdx=0; RingIdx<NumRings; RingIdx++)
			{
				IndexBuffer.Add(a0start + RingIdx + 0); IndexBuffer.Add(a1start + RingIdx + 0); IndexBuffer.Add(a0start + RingIdx + 1);
				IndexBuffer.Add(a1start + RingIdx + 0); IndexBuffer.Add(a1start + RingIdx + 1); IndexBuffer.Add(a0start + RingIdx + 1);
			}
		}
	}
}

FORCEINLINE_DEBUGGABLE void ExportRigidBodySetup(UBodySetup& BodySetup, TNavStatArray<float>& VertexBuffer, TNavStatArray<int32>& IndexBuffer,
												 FBox& UnrealBounds, const FTransform& LocalToWorld)
{
	// Make sure meshes are created before we try and export them
	BodySetup.CreatePhysicsMeshes();

	static TNavStatArray<int32> TemporaryShapeBuffer;
	TemporaryShapeBuffer.Reset();

	ExportRigidBodyTriMesh(BodySetup, VertexBuffer, IndexBuffer, UnrealBounds, LocalToWorld);		
	ExportRigidBodyConvexElements(BodySetup, VertexBuffer, IndexBuffer, TemporaryShapeBuffer, UnrealBounds, LocalToWorld);
	ExportRigidBodyBoxElements(BodySetup, VertexBuffer, IndexBuffer, TemporaryShapeBuffer, UnrealBounds, LocalToWorld);
	ExportRigidBodySphylElements(BodySetup, VertexBuffer, IndexBuffer, TemporaryShapeBuffer, UnrealBounds, LocalToWorld);
	ExportRigidBodySphereElements(BodySetup, VertexBuffer, IndexBuffer, TemporaryShapeBuffer, UnrealBounds, LocalToWorld);
}

FORCEINLINE_DEBUGGABLE void ExportComponent(UActorComponent& Component, FRecastGeometryExport* GeomExport, const FBox* ClipBounds=NULL)
{
#if WITH_PHYSX
	bool bHasData = false;

	UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(&Component);
	if (PrimComp && PrimComp->IsNavigationRelevant())
	{
		if (PrimComp->HasCustomNavigableGeometry() && !PrimComp->DoCustomNavigableGeometryExport(GeomExport)) 
		{
			bHasData = true;
		}

		UBodySetup* BodySetup = PrimComp->GetBodySetup();
		if (BodySetup)
		{
			if (!bHasData)
			{
				ExportRigidBodySetup(*BodySetup, GeomExport->VertexBuffer, GeomExport->IndexBuffer, GeomExport->Data->Bounds, PrimComp->ComponentToWorld);
				bHasData = true;
			}

			GeomExport->SlopeOverride = BodySetup->WalkableSlopeOverride;
		}
	}
#endif // WITH_PHYSX
}

FORCEINLINE_DEBUGGABLE void ExportActor(AActor& Actor, FRecastGeometryExport* GeomExport, const FBox* ClipBounds=NULL)
{
#if WITH_PHYSX
	TArray<UPrimitiveComponent*> Components;
	Actor.GetComponents(Components);

	for (int32 i = 0; i < Components.Num(); ++i)
	{
		UPrimitiveComponent* PrimComp = Components[i];
		if (PrimComp->CanEverAffectNavigation() == true)
		{
			ExportComponent(*PrimComp, GeomExport, ClipBounds);
		}
	}
#endif // WITH_PHYSX
}

FORCEINLINE void TransformVertexSoupToRecast(const TArray<FVector>& VertexSoup, TNavStatArray<FVector>& Verts, TNavStatArray<int32>& Faces)
{
	if (VertexSoup.Num() == 0)
	{
		return;
	}

	check(VertexSoup.Num() % 3 == 0);

	const int32 StaticFacesCount = VertexSoup.Num() / 3;
	int32 VertsCount = Verts.Num();
	const FVector* Vertex = VertexSoup.GetTypedData();

	for (int32 k = 0; k < StaticFacesCount; ++k, Vertex += 3)
	{
		Verts.Add(Unreal2RecastPoint(Vertex[0]));
		Verts.Add(Unreal2RecastPoint(Vertex[1]));
		Verts.Add(Unreal2RecastPoint(Vertex[2]));
		Faces.Add(VertsCount + 2);
		Faces.Add(VertsCount + 1);
		Faces.Add(VertsCount + 0);
			
		VertsCount += 3;
	}
}

FORCEINLINE void CovertCoordDataToRecast(TNavStatArray<float>& Coords)
{
	float* CoordPtr = Coords.GetTypedData();
	const int32 MaxIt = Coords.Num() / 3;
	for (int32 i = 0; i < MaxIt; i++)
	{
		CoordPtr[0] = -CoordPtr[0];

		const float TmpV = -CoordPtr[1];
		CoordPtr[1] = CoordPtr[2];
		CoordPtr[2] = TmpV;

		CoordPtr += 3;
	}
}

void ExportVertexSoup(const TArray<FVector>& VertexSoup, TNavStatArray<float>& VertexBuffer, TNavStatArray<int32>& IndexBuffer, FBox& UnrealBounds)
{
	if (VertexSoup.Num())
	{
		check(VertexSoup.Num() % 3 == 0);
		
		int32 VertBase = VertexBuffer.Num() / 3;
		VertexBuffer.Reserve(VertexSoup.Num() * 3);
		IndexBuffer.Reserve(VertexSoup.Num() / 3);

		const int32 NumVerts = VertexSoup.Num();
		for (int32 i = 0; i < NumVerts; i++)
		{
			const FVector& UnrealCoords = VertexSoup[i];
			UnrealBounds += UnrealCoords;

			const FVector RecastCoords = Unreal2RecastPoint(UnrealCoords);
			VertexBuffer.Add(RecastCoords.X);
			VertexBuffer.Add(RecastCoords.Y);
			VertexBuffer.Add(RecastCoords.Z);
		}

		const int32 NumFaces = VertexSoup.Num() / 3;
		for (int32 i = 0; i < NumFaces; i++)
		{
			IndexBuffer.Add(VertBase + 2);
			IndexBuffer.Add(VertBase + 1);
			IndexBuffer.Add(VertBase + 0);
			VertBase += 3;
		}
	}
}

} // namespace RecastGeometryExport

#if WITH_PHYSX
void FRecastGeometryExport::ExportPxTriMesh16Bit(physx::PxTriangleMesh const * const TriMesh, const FTransform& LocalToWorld)
{
	RecastGeometryExport::ExportPxTriMesh<PxU16>(TriMesh, LocalToWorld, VertexBuffer, IndexBuffer, Data->Bounds);
}

void FRecastGeometryExport::ExportPxTriMesh32Bit(physx::PxTriangleMesh const * const TriMesh, const FTransform& LocalToWorld)
{
	RecastGeometryExport::ExportPxTriMesh<PxU32>(TriMesh, LocalToWorld, VertexBuffer, IndexBuffer, Data->Bounds);
}

void FRecastGeometryExport::ExportPxConvexMesh(physx::PxConvexMesh const * const ConvexMesh, const FTransform& LocalToWorld)
{
	RecastGeometryExport::ExportPxConvexMesh(ConvexMesh, LocalToWorld, VertexBuffer, IndexBuffer, Data->Bounds);
}
#endif // WITH_PHYSX

void FRecastGeometryExport::ExportCustomMesh(const FVector* InVertices, int32 NumVerts, const int32* InIndices, int32 NumIndices, const FTransform& LocalToWorld)
{
	RecastGeometryExport::ExportCustomMesh(InVertices, NumVerts, InIndices, NumIndices, LocalToWorld, VertexBuffer, IndexBuffer, Data->Bounds);
}

void FRecastGeometryExport::AddNavModifiers(const FCompositeNavModifier& Modifiers)
{
	Data->Modifiers.Add(Modifiers);
}

FORCEINLINE void GrowConvexHull(const float ExpandBy, const TArray<FVector>& Verts, TArray<FVector>& OutResult)
{
	if (Verts.Num() < 3)
	{
		return;
	}

	const int32 VertsCount = Verts.Num();
	OutResult = Verts;

	for (int32 i = 0; i < VertsCount; ++i)
	{
		OutResult[i] += (((Verts[i] - Verts[(i+1)%VertsCount]).UnsafeNormal() 
			+ (Verts[i] - Verts[(i-1+VertsCount)%VertsCount]).UnsafeNormal() ) * 0.5f).UnsafeNormal() * ExpandBy;
	}
}

//----------------------------------------------------------------------//

struct FOffMeshData
{
	TArray<dtOffMeshLinkCreateParams> LinkParams;
	const FRecastNavMeshCachedData::TAreaClassIDMap* AreaClassToIDMap;
	const ARecastNavMesh::FNavPolyFlags* FlagsPerArea;


	FOffMeshData() : AreaClassToIDMap(NULL), FlagsPerArea(NULL) {}

	FORCEINLINE void Reserve(const uint32 ElementsCount)
	{
		LinkParams.Reserve(ElementsCount);
	}

	void AddLinks(const TArray<FNavigationLink>& Links, const FTransform& LocalToWorld, uint32 AgentMask)
	{
		for (int32 LinkIndex = 0; LinkIndex < Links.Num(); ++LinkIndex)
		{
			const FNavigationLink& Link = Links[LinkIndex];
			if ((Link.SupportedAgentsBits & AgentMask) == 0)
			{
				continue;
			}

			dtOffMeshLinkCreateParams NewInfo;
			FMemory::MemZero(NewInfo);

			if (Link.Direction == ENavLinkDirection::LeftToRight || Link.Direction == ENavLinkDirection::BothWays)
			{
				StoreUnrealPoint(NewInfo.vertsA0, LocalToWorld.TransformPosition(Link.Left));
				StoreUnrealPoint(NewInfo.vertsB0, LocalToWorld.TransformPosition(Link.Right));
			}
			else
			{
				StoreUnrealPoint(NewInfo.vertsA0, LocalToWorld.TransformPosition(Link.Right));
				StoreUnrealPoint(NewInfo.vertsB0, LocalToWorld.TransformPosition(Link.Left));
			}

			NewInfo.type = DT_OFFMESH_CON_POINT | (Link.Direction == ENavLinkDirection::BothWays ? DT_OFFMESH_CON_BIDIR : 0);
			NewInfo.snapRadius = Link.SnapRadius;
			NewInfo.userID = Link.UserId;

			const uint8* AreaID = AreaClassToIDMap->Find(Link.AreaClass ? Link.AreaClass : UNavigationSystem::GetDefaultWalkableArea());
			if (AreaID != NULL)
			{
				NewInfo.area = *AreaID;
				NewInfo.polyFlag = FlagsPerArea[*AreaID];
			}
			else
			{
				UE_LOG(LogNavigation, Warning, TEXT("FRecastTileGenerator: Trying to use undefined area class while defining Off-Mesh links! (%s)"), *GetNameSafe(Link.AreaClass));
			}

			// snap area is currently not supported for regular (point-point) offmesh links

			LinkParams.Add(NewInfo);
		}
	}
	void AddSegmentLinks(const TArray<FNavigationSegmentLink>& Links, const FTransform& LocalToWorld, uint32 AgentMask)
	{
		for (int32 LinkIndex = 0; LinkIndex < Links.Num(); ++LinkIndex)
		{
			const FNavigationSegmentLink& Link = Links[LinkIndex];
			if ((Link.SupportedAgentsBits & AgentMask) == 0)
			{
				continue;
			}

			dtOffMeshLinkCreateParams NewInfo;
			FMemory::MemZero(NewInfo);

			if (Link.Direction == ENavLinkDirection::LeftToRight || Link.Direction == ENavLinkDirection::BothWays)
			{
				StoreUnrealPoint(NewInfo.vertsA0, LocalToWorld.TransformPosition(Link.LeftStart));
				StoreUnrealPoint(NewInfo.vertsA1, LocalToWorld.TransformPosition(Link.LeftEnd));
				StoreUnrealPoint(NewInfo.vertsB0, LocalToWorld.TransformPosition(Link.RightStart));
				StoreUnrealPoint(NewInfo.vertsB1, LocalToWorld.TransformPosition(Link.RightEnd));
			}
			else
			{
				StoreUnrealPoint(NewInfo.vertsA0, LocalToWorld.TransformPosition(Link.RightStart));
				StoreUnrealPoint(NewInfo.vertsA1, LocalToWorld.TransformPosition(Link.RightEnd));
				StoreUnrealPoint(NewInfo.vertsB0, LocalToWorld.TransformPosition(Link.LeftStart));
				StoreUnrealPoint(NewInfo.vertsB1, LocalToWorld.TransformPosition(Link.LeftEnd));
			}

			NewInfo.type = DT_OFFMESH_CON_SEGMENT | (Link.Direction == ENavLinkDirection::BothWays ? DT_OFFMESH_CON_BIDIR : 0);
			NewInfo.snapRadius = Link.SnapRadius;
			NewInfo.userID = Link.UserId;

			const uint8* AreaID = AreaClassToIDMap->Find(Link.AreaClass);
			if (AreaID != NULL)
			{
				NewInfo.area = *AreaID;
				NewInfo.polyFlag = FlagsPerArea[*AreaID];
			}
			else
			{
				UE_LOG(LogNavigation, Warning, TEXT("FRecastTileGenerator: Trying to use undefined area class while defining Off-Mesh links! (%s)"), *GetNameSafe(Link.AreaClass));
			}

			LinkParams.Add(NewInfo);
		}
	}

protected:

	void StoreUnrealPoint(float* dest, const FVector& UnrealPt)
	{
		const FVector RecastPt = Unreal2RecastPoint(UnrealPt);
		dest[0] = RecastPt.X;
		dest[1] = RecastPt.Y;
		dest[2] = RecastPt.Z;
	}
};

//----------------------------------------------------------------------//
// FNavMeshBuildContext
// A navmesh building reporting helper
//----------------------------------------------------------------------//
class FNavMeshBuildContext : public rcContext
{
public:
	FNavMeshBuildContext()
		: rcContext(true)
	{
	}
protected:
	/// Logs a message.
	///  @param[in]		category	The category of the message.
	///  @param[in]		msg			The formatted message.
	///  @param[in]		len			The length of the formatted message.
	virtual void doLog(const rcLogCategory category, const char* Msg, const int32 /*len*/) 
	{
		switch (category) 
		{
		case RC_LOG_ERROR:
			UE_LOG(LogNavigation, Error, TEXT("Recast: %s"), ANSI_TO_TCHAR( Msg ) );
			break;
		case RC_LOG_WARNING:
			UE_LOG(LogNavigation, Log, TEXT("Recast: %s"), ANSI_TO_TCHAR( Msg ) );
			break;
		default:
			UE_LOG(LogNavigation, Verbose, TEXT("Recast: %s"), ANSI_TO_TCHAR( Msg ) );
			break;
		}
	}
};

//----------------------------------------------------------------------//
struct FTileCacheCompressor : public dtTileCacheCompressor
{
	struct FCompressedCacheHeader
	{
		int32 UncompressedSize;
	};

	virtual int32 maxCompressedSize(const int32 bufferSize)
	{
		return FMath::Trunc(bufferSize * 1.1f) + sizeof(FCompressedCacheHeader);
	}

	virtual dtStatus compress(const uint8* buffer, const int32 bufferSize,
		uint8* compressed, const int32 maxCompressedSize, int32* compressedSize)
	{
		const int32 HeaderSize = sizeof(FCompressedCacheHeader);

		FCompressedCacheHeader DataHeader;
		DataHeader.UncompressedSize = bufferSize;
		FMemory::Memcpy((void*)compressed, &DataHeader, HeaderSize);

		uint8* DataPtr = compressed + HeaderSize;		
		int32 DataSize = maxCompressedSize - HeaderSize;

		FCompression::CompressMemory((ECompressionFlags)(COMPRESS_ZLIB | COMPRESS_BiasMemory),
			(void*)DataPtr, DataSize, (const void*)buffer, bufferSize);

		*compressedSize = DataSize + HeaderSize;
		return DT_SUCCESS;
	}

	virtual dtStatus decompress(const uint8* compressed, const int32 compressedSize,
		uint8* buffer, const int32 maxBufferSize, int32* bufferSize)
	{
		const int32 HeaderSize = sizeof(FCompressedCacheHeader);
		
		FCompressedCacheHeader DataHeader;
		FMemory::Memcpy(&DataHeader, (void*)compressed, HeaderSize);

		const uint8* DataPtr = compressed + HeaderSize;		
		const int32 DataSize = compressedSize - HeaderSize;

		FCompression::UncompressMemory((ECompressionFlags)(COMPRESS_ZLIB),
			(void*)buffer, DataHeader.UncompressedSize, (const void*)DataPtr, DataSize);

		*bufferSize = DataHeader.UncompressedSize;
		return DT_SUCCESS;
	}
};

struct FTileCacheAllocator : public dtTileCacheAlloc
{
	virtual void reset()
	{
		 check(0 && "dtTileCacheAlloc.reset() is not supported!");
	}

	virtual void* alloc(const int32 Size)
	{
		return dtAlloc(Size, DT_ALLOC_TEMP);
	}

	virtual void free(void* Data)
	{
		dtFree(Data);
	}
};

//----------------------------------------------------------------------//
// FVoxelCacheRasterizeContext
//----------------------------------------------------------------------//

struct FVoxelCacheRasterizeContext
{
	FVoxelCacheRasterizeContext()
	{
		RasterizeHF = NULL;
	}

	~FVoxelCacheRasterizeContext()
	{
		rcFreeHeightField(RasterizeHF);
		RasterizeHF = 0;
	}

	void Create(int32 FieldSize, float CellSize, float CellHeight)
	{
		if (RasterizeHF == NULL)
		{
			const float DummyBounds[3] = { 0 };

			RasterizeHF = rcAllocHeightfield();
			rcCreateHeightfield(NULL, *RasterizeHF, FieldSize, FieldSize, DummyBounds, DummyBounds, CellSize, CellHeight);
		}
	}

	void Reset()
	{
		rcResetHeightfield(*RasterizeHF);
	}

	void SetupForTile(const float* TileBMin, const float* TileBMax, const float RasterizationPadding)
	{
		Reset();

		rcVcopy(RasterizeHF->bmin, TileBMin);
		rcVcopy(RasterizeHF->bmax, TileBMax);

		RasterizeHF->bmin[0] -= RasterizationPadding;
		RasterizeHF->bmin[2] -= RasterizationPadding;
		RasterizeHF->bmax[0] += RasterizationPadding;
		RasterizeHF->bmax[2] += RasterizationPadding;
	}

	rcHeightfield* RasterizeHF;
};

static FVoxelCacheRasterizeContext VoxelCacheContext;

//----------------------------------------------------------------------//
// FRecastTileDirtyState
//----------------------------------------------------------------------//

FRecastTileDirtyState::FRecastTileDirtyState(const class FRecastTileGenerator* DirtyGenerator)
{
	if (DirtyGenerator)
	{
		DirtyGenerator->GetDirtyState(*this);
	}
}

void FRecastTileDirtyState::Append(const FRecastTileDirtyState& Other)
{
	bRebuildGeometry |= Other.bRebuildGeometry;
	bRebuildLayers |= Other.bRebuildLayers;
	bRebuildAllLayers |= Other.bRebuildAllLayers;

	for (int32 i = 0; i < ARRAY_COUNT(DirtyLayers); i++)
	{
		DirtyLayers[i] |= Other.DirtyLayers[i];
	}

	if (FallbackDirtyLayers.Num() < Other.FallbackDirtyLayers.Num())
	{
		FallbackDirtyLayers.AddZeroed(Other.FallbackDirtyLayers.Num() - FallbackDirtyLayers.Num());
	}

	for (int32 i = 0; i < Other.FallbackDirtyLayers.Num(); i++)
	{
		FallbackDirtyLayers[i] |= Other.FallbackDirtyLayers[i];
	}
}

void FRecastTileDirtyState::Clear()
{
	bRebuildGeometry = false;
	bRebuildLayers = false;
	bRebuildAllLayers = false;
	FMemory::Memzero(DirtyLayers, sizeof(DirtyLayers));

	const int32 NumLayers = FallbackDirtyLayers.Num();
	if (NumLayers > 0)
	{
		FallbackDirtyLayers.Reset();
		FallbackDirtyLayers.AddZeroed(NumLayers);
	}
}

void FRecastTileDirtyState::MarkDirtyLayer(int32 LayerIdx)
{
	const int32 BitfieldSize = 8;
	const int32 MaxBitfieldLayers = BitfieldSize * ARRAY_COUNT(DirtyLayers);

	bRebuildLayers = true;
	if (LayerIdx >= MaxBitfieldLayers)
	{
		// consider increasing size of DirtyLayers if this happens frequently
		const int32 ArrayIdx = (LayerIdx - MaxBitfieldLayers) / BitfieldSize;
		const int32 ShiftIdx = (LayerIdx - MaxBitfieldLayers) % BitfieldSize;

		if (!FallbackDirtyLayers.IsValidIndex(ArrayIdx))
		{
			FallbackDirtyLayers.AddZeroed(ArrayIdx - FallbackDirtyLayers.Num() + 1);
		}
		
		FallbackDirtyLayers[ArrayIdx] = FallbackDirtyLayers[ArrayIdx] | (1 << ShiftIdx);
	}
	else
	{
		const int32 ArrayIdx = LayerIdx / BitfieldSize;
		const int32 ShiftIdx = LayerIdx % BitfieldSize;

		DirtyLayers[ArrayIdx] = DirtyLayers[ArrayIdx] | (1 << ShiftIdx);
	}
}

bool FRecastTileDirtyState::HasDirtyLayer(int32 LayerIdx) const
{
	const int32 BitfieldSize = 8;
	const int32 MaxBitfieldLayers = BitfieldSize * ARRAY_COUNT(DirtyLayers);

	bool bIsDirty = false;
	if (bRebuildLayers)
	{
		if (bRebuildAllLayers)
		{
			bIsDirty = true;
		}
		else if (LayerIdx >= MaxBitfieldLayers)
		{
			const int32 ArrayIdx = (LayerIdx - MaxBitfieldLayers) / BitfieldSize;
			const int32 ShiftIdx = (LayerIdx - MaxBitfieldLayers) % BitfieldSize;

			if (FallbackDirtyLayers.IsValidIndex(ArrayIdx))
			{
				bIsDirty = (FallbackDirtyLayers[ArrayIdx] & (1 << ShiftIdx)) != 0;
			}
		}
		else
		{
			const int32 ArrayIdx = LayerIdx / BitfieldSize;
			const int32 ShiftIdx = LayerIdx % BitfieldSize;

			bIsDirty = (DirtyLayers[ArrayIdx] & (1 << ShiftIdx)) != 0;
		}
	}

	return bIsDirty;
}


//----------------------------------------------------------------------//
// FRecastTileGenerator
//----------------------------------------------------------------------//

TNavStatArray<rcSpanCache> FRecastTileGenerator::StaticGeomSpans;
TNavStatArray<float> FRecastTileGenerator::StaticGeomCoords;
TNavStatArray<int32> FRecastTileGenerator::StaticGeomIndices;
TArray<FAreaNavModifier> FRecastTileGenerator::StaticStaticAreas;
TArray<FAreaNavModifier> FRecastTileGenerator::StaticDynamicAreas;
TArray<FSimpleLinkNavModifier> FRecastTileGenerator::StaticOffmeshLinks;

uint32 GetTileCacheSizeHelper(TArray<FNavMeshTileData>& CompressedTiles)
{
	uint32 TotalMemory = 0;
	for (int32 i = 0; i < CompressedTiles.Num(); i++)
	{
		TotalMemory += CompressedTiles[i].DataSize;
	}

	return TotalMemory;
}

FRecastTileGenerator::FRecastTileGenerator()
	: bInitialized(false), bBeingRebuild(false), bRebuildPending(false), bAsyncBuildInProgress(false), TileX(-1), TileY(-1), TileId(-1), Version(uint32(-1))
{
}

FRecastTileGenerator::~FRecastTileGenerator()
{
	DEC_MEMORY_STAT_BY(STAT_Navigation_TileCacheMemory, GetTileCacheSizeHelper(CompressedLayers));
}

void FRecastTileGenerator::Init(class FRecastNavMeshGenerator* ParentGenerator, const int32 X, const int32 Y, const float TileBmin[3], const float TileBmax[3], const TNavStatArray<FBox>& BoundingBoxes)
{
	if (ParentGenerator == NULL)
	{
		UE_LOG(LogNavigation, Error, TEXT("FRecastTileGenerator: trying to initialize tile generator with empty navmesh generator"))
		return;
	}

	AdditionalCachedData = ParentGenerator->GetAdditionalCachedData();

	if (NavMeshGenerator.HasSameObject(ParentGenerator) == false)
	{
		NavMeshGenerator = ParentGenerator->AsShared();
	}
	TileId = ParentGenerator->GetTileIdAt(X,Y);
	Version = ParentGenerator->GetVersion();

	TileX = X;
	TileY = Y;
	FMemory::Memcpy( BMin, TileBmin, sizeof( BMin ) );
	FMemory::Memcpy( BMax, TileBmax, sizeof( BMin ) );

	// from passed in boxes pick the ones overlapping with tile bounds
	InclusionBounds.Reset();

	bOutsideOfInclusionBounds = false;

	if (BoundingBoxes.Num() > 0)
	{
		bFullyEncapsulatedByInclusionBounds = false;
			
		TileBB = Recast2UnrealBox(BMin, BMax);
		InclusionBounds.Reserve(BoundingBoxes.Num());

		const FBox* Bounds = BoundingBoxes.GetTypedData();	
		for (int32 i = 0; i < BoundingBoxes.Num() && bFullyEncapsulatedByInclusionBounds == false; ++i, ++Bounds)
		{	
			if (Bounds->Intersect( TileBB ))
			{
				InclusionBounds.Add(*Bounds);

				bFullyEncapsulatedByInclusionBounds = DoesBoxContainBox(*Bounds, TileBB);
			}
		}

		bOutsideOfInclusionBounds = (InclusionBounds.Num() == 0);
	}
	else
	{
		bFullyEncapsulatedByInclusionBounds = true;
	}

	FRecastBuildConfig TileConfig = ParentGenerator->GetConfig();
	WalkableClimbVX = TileConfig.walkableClimb;
	WalkableSlopeCos = FMath::Cos(FMath::DegreesToRadians(TileConfig.walkableSlopeAngle));
	RasterizationPadding = TileConfig.borderSize * TileConfig.cs;
	bInitialized = true;
}

void FRecastTileGenerator::InitiateRebuild()
{
	bBeingRebuild = true;
	bRebuildPending = false;
	GeneratingState = DirtyState;
	DirtyState.Clear();
}

void FRecastTileGenerator::AbortRebuild()
{
	bBeingRebuild = false;
	bRebuildPending = false;
	DirtyState.Append(GeneratingState);
	GeneratingState.Clear();
	ClearNavigationData();
}

void FRecastTileGenerator::AbadonGeneration()
{
	bBeingRebuild = false;
	bRebuildPending = false;
	DirtyState.Clear();
	GeneratingState.Clear();
}

void FRecastTileGenerator::FinishRebuild()
{
	bBeingRebuild = false;
	GeneratingState.Clear();
	DirtyState.Append(GeneratingState);
}

void FRecastTileGenerator::StartAsyncBuild()
{
	bAsyncBuildInProgress = true;
}

void FRecastTileGenerator::FinishAsyncBuild()
{
	bAsyncBuildInProgress = false;
}

void FRecastTileGenerator::MarkPendingRebuild()
{
	bRebuildPending = true;
}

void FRecastTileGenerator::ApplyVoxelFilter(struct rcHeightfield* HF, float WalkableRadius)
{
	SCOPE_CYCLE_COUNTER(STAT_Navigation_TileVoxelFilteringAsync);

	if (HF != NULL)
	{
		const int32 Width = HF->width;
		const int32 Height = HF->height;
		const float CellSize = HF->cs;
		const float CellHeight = HF->ch;
		const float BottomX = HF->bmin[0];
		const float BottomZ = HF->bmin[1];
		const float BottomY = HF->bmin[2];
		const int32 SpansCount = Width*Height;
		// we need to expand considered bounding boxes so that
		// it doesn't create "fake cliffs"
		const float ExpandBBBy = WalkableRadius*CellSize;

		const FBox* BBox = InclusionBounds.GetTypedData();
		// optimized common case of single box
		if (InclusionBounds.Num() == 1)
		{
			const FBox BB = BBox->ExpandBy(ExpandBBBy);

			rcSpan** Span = HF->spans;

			for (int32 y = 0; y < Height; ++y)
			{
				for (int32 x = 0; x < Width; ++x)
				{
					const float SpanX = -(BottomX + x * CellSize);
					const float SpanY = -(BottomY + y * CellSize);

					// mark all spans outside of InclusionBounds as unwalkable
					for (rcSpan* s = *Span; s; s = s->next)
					{
						if (s->data.area == RC_WALKABLE_AREA)
						{
							const float SpanMin = CellHeight * s->data.smin + BottomZ;
							const float SpanMax = CellHeight * s->data.smax + BottomZ;

							const FVector SpanMinV(SpanX-CellSize, SpanY-CellSize, SpanMin);
							const FVector SpanMaxV(SpanX, SpanY, SpanMax);

							if (BB.IsInside(SpanMinV) == false && BB.IsInside(SpanMaxV) == false)
							{
								s->data.area = RC_NULL_AREA;
							}
						}
					}
					++Span;
				}
			}
		}
		else
		{
			TArray<FBox> Bounds;
			Bounds.Reserve(InclusionBounds.Num());

			for (int32 i = 0; i < InclusionBounds.Num(); ++i, ++BBox)
			{	
				Bounds.Add(BBox->ExpandBy(ExpandBBBy));
			}
			const int32 BoundsCount = Bounds.Num();

			rcSpan** Span = HF->spans;

			for (int32 y = 0; y < Height; ++y)
			{
				for (int32 x = 0; x < Width; ++x)
				{
					const float SpanX = -(BottomX + x * CellSize);
					const float SpanY = -(BottomY + y * CellSize);

					// mark all spans outside of InclusionBounds as unwalkable
					for (rcSpan* s = *Span; s; s = s->next)
					{
						if (s->data.area == RC_WALKABLE_AREA)
						{
							const float SpanMin = CellHeight * s->data.smin + BottomZ;
							const float SpanMax = CellHeight * s->data.smax + BottomZ;

							const FVector SpanMinV(SpanX-CellSize, SpanY-CellSize, SpanMin);
							const FVector SpanMaxV(SpanX, SpanY, SpanMax);

							bool bIsInsideAnyBB = false;
							const FBox* BB = Bounds.GetTypedData();
							for (int32 BoundIndex = 0; BoundIndex < BoundsCount; ++BoundIndex, ++BB)
							{
								if (BB->IsInside(SpanMinV) || BB->IsInside(SpanMaxV))
								{
									bIsInsideAnyBB = true;
									break;
								}
							}

							if (bIsInsideAnyBB == false)
							{
								s->data.area = RC_NULL_AREA;
							}
						}
					}
					++Span;
				}
			}
		}
	}
}

void FRecastTileGenerator::PrepareVoxelCache(const TNavStatArray<uint8>& RawCollisionCache, TNavStatArray<rcSpanCache>& SpanData)
{
	FRecastGeometryCache CachedCollisions(RawCollisionCache.GetTypedData());
	VoxelCacheContext.SetupForTile(BMin, BMax, RasterizationPadding);

	float SlopeCosPerActor = WalkableSlopeCos;
	CachedCollisions.Header.SlopeOverride.ModifyWalkableFloorZ(SlopeCosPerActor);

	// rasterize triangle soup
	TNavStatArray<uint8> TriAreas;
	TriAreas.AddZeroed(CachedCollisions.Header.NumFaces);

	rcMarkWalkableTrianglesCos(0, SlopeCosPerActor,
		CachedCollisions.Verts, CachedCollisions.Header.NumVerts,
		CachedCollisions.Indices, CachedCollisions.Header.NumFaces,
		TriAreas.GetTypedData());

	rcRasterizeTriangles(0, CachedCollisions.Verts, CachedCollisions.Header.NumVerts,
		CachedCollisions.Indices, TriAreas.GetTypedData(), CachedCollisions.Header.NumFaces,
		*VoxelCacheContext.RasterizeHF, WalkableClimbVX);

	const int32 NumSpans = rcCountSpans(0, *VoxelCacheContext.RasterizeHF);
	if (NumSpans > 0)
	{
		SpanData.AddZeroed(NumSpans);
		rcCacheSpans(0, *VoxelCacheContext.RasterizeHF, SpanData.GetTypedData());
	}
}

bool FRecastTileGenerator::HasVoxelCache(const TNavStatArray<uint8>& RawVoxelCache, rcSpanCache*& CachedVoxels, int32& NumCachedVoxels) const
{
	FRecastVoxelCache VoxelCache(RawVoxelCache.GetTypedData());
	for (FRecastVoxelCache::FTileInfo* iTile = VoxelCache.Tiles; iTile; iTile = iTile->NextTile)
	{
		if (iTile->TileX == TileX && iTile->TileY == TileY)
		{
			CachedVoxels = iTile->SpanData;
			NumCachedVoxels = iTile->NumSpans;
			return true;
		}
	}

	return false;
}

void FRecastTileGenerator::AddVoxelCache(TNavStatArray<uint8>& RawVoxelCache, const rcSpanCache* CachedVoxels, const int32 NumCachedVoxels) const
{
	if (RawVoxelCache.Num() == 0)
	{
		RawVoxelCache.AddZeroed(sizeof(int32));
	}

	int32* NumTiles = (int32*)RawVoxelCache.GetTypedData();
	*NumTiles = *NumTiles + 1;

	const int32 NewCacheIdx = RawVoxelCache.Num();
	const int32 HeaderSize = sizeof(FRecastVoxelCache::FTileInfo);
	const int32 VoxelsSize = sizeof(rcSpanCache) * NumCachedVoxels;
	const int32 EntrySize = HeaderSize + VoxelsSize;
	RawVoxelCache.AddZeroed(EntrySize);

	FRecastVoxelCache::FTileInfo* TileInfo = (FRecastVoxelCache::FTileInfo*)(RawVoxelCache.GetTypedData() + NewCacheIdx);
	TileInfo->TileX = TileX;
	TileInfo->TileY = TileY;
	TileInfo->NumSpans = NumCachedVoxels;

	FMemory::Memcpy(RawVoxelCache.GetTypedData() + NewCacheIdx + HeaderSize, CachedVoxels, VoxelsSize);
}

void FRecastTileGenerator::ClearGeometry()
{
#if CACHE_NAV_GENERATOR_DATA
	GeomIndices.Empty();
	GeomCoords.Empty();

	// don't clear static areas here, they are used together with dynamic ones during nav data generation
#else
	StaticGeomIndices.Reset();
	StaticGeomCoords.Reset();
	StaticGeomSpans.Reset();
#endif
}

void FRecastTileGenerator::ClearModifiers()
{
#if CACHE_NAV_GENERATOR_DATA
	StaticAreas.Empty();
	DynamicAreas.Empty();
	OffmeshLinks.Empty();
#else
	StaticStaticAreas.Reset();
	StaticDynamicAreas.Reset();
	StaticOffmeshLinks.Reset();
#endif
}

void FRecastTileGenerator::ClearStaticData()
{
	StaticGeomIndices.Empty();
	StaticGeomCoords.Empty();
	StaticGeomSpans.Empty();
	StaticStaticAreas.Empty();
	StaticDynamicAreas.Empty();
	StaticOffmeshLinks.Empty();
}

void FRecastTileGenerator::AppendModifier(const FCompositeNavModifier& Modifier, bool bStatic)
{
	// append areas to correct array
	if (bStatic)
	{
#if CACHE_NAV_GENERATOR_DATA
		StaticAreas.Append(Modifier.GetAreas());
#else
		StaticStaticAreas.Append(Modifier.GetAreas());
#endif
	}
	else
	{
#if CACHE_NAV_GENERATOR_DATA
		DynamicAreas.Append(Modifier.GetAreas());
#else
		StaticDynamicAreas.Append(Modifier.GetAreas());
#endif
	}

	// append all offmesh links (not included in compress layers)
#if CACHE_NAV_GENERATOR_DATA
	OffmeshLinks.Append(Modifier.GetSimpleLinks());
#else
	StaticOffmeshLinks.Append(Modifier.GetSimpleLinks());
#endif

	// evaluate custom links
	const FCustomLinkNavModifier* LinkModifier = Modifier.GetCustomLinks().GetTypedData();
	for (int32 i = 0; i < Modifier.GetCustomLinks().Num(); i++, LinkModifier++)
	{
		FSimpleLinkNavModifier SimpleLinkCollection(UNavLinkDefinition::GetLinksDefinition(LinkModifier->GetNavLinkClass()), LinkModifier->LocalToWorld);
#if CACHE_NAV_GENERATOR_DATA
		OffmeshLinks.Add(SimpleLinkCollection);
#else
		StaticOffmeshLinks.Add(SimpleLinkCollection);
#endif
	}
}

void FRecastTileGenerator::AppendGeometry(const TNavStatArray<uint8>& RawCollisionCache)
{
	if (RawCollisionCache.Num() == 0)
		return;

	FRecastGeometryCache CollisionCache(RawCollisionCache.GetTypedData());

#if CACHE_NAV_GENERATOR_DATA
	const int32 FirstNewCoord = GeomCoords.Num();
	const int32 FirstNewIndex = GeomIndices.Num();
	const int32 VertBase = FirstNewCoord / 3;

	GeomCoords.AddZeroed(CollisionCache.Header.NumVerts * 3);
	GeomIndices.AddZeroed(CollisionCache.Header.NumFaces * 3);

	FMemory::Memcpy(GeomCoords.GetTypedData() + FirstNewCoord, CollisionCache.Verts, sizeof(float) * CollisionCache.Header.NumVerts * 3);
	int32* DestIndex = GeomIndices.GetTypedData() + FirstNewIndex;
#else
	const int32 FirstNewCoord = StaticGeomCoords.Num();
	const int32 FirstNewIndex = StaticGeomIndices.Num();
	const int32 VertBase = FirstNewCoord / 3;

	StaticGeomCoords.AddZeroed(CollisionCache.Header.NumVerts * 3);
	StaticGeomIndices.AddZeroed(CollisionCache.Header.NumFaces * 3);

	FMemory::Memcpy(StaticGeomCoords.GetTypedData() + FirstNewCoord, CollisionCache.Verts, sizeof(float) * CollisionCache.Header.NumVerts * 3);
	int32* DestIndex = StaticGeomIndices.GetTypedData() + FirstNewIndex;
#endif

	for (int32 i = 0; i < CollisionCache.Header.NumFaces * 3; i++, DestIndex++)
	{
		*DestIndex = CollisionCache.Indices[i] + VertBase;
	}
}

void FRecastTileGenerator::AppendGeometry(const TNavStatArray<FVector>& Verts, const TNavStatArray<int32>& Faces)
{
	if (Faces.Num() == 0 || Verts.Num() == 0)
		return;

#if CACHE_NAV_GENERATOR_DATA
	const int32 FirstNewCoord = GeomCoords.Num();
	const int32 FirstNewIndex = GeomIndices.Num();
	const int32 VertBase = FirstNewCoord / 3;

	GeomCoords.AddZeroed(Verts.Num() * 3);
	GeomIndices.AddZeroed(Faces.Num());

	float* DestCoord = GeomCoords.GetTypedData() + FirstNewCoord;
	int32* DestIndex = GeomIndices.GetTypedData() + FirstNewIndex;
#else
	const int32 FirstNewCoord = StaticGeomCoords.Num();
	const int32 FirstNewIndex = StaticGeomIndices.Num();
	const int32 VertBase = FirstNewCoord / 3;

	StaticGeomCoords.AddZeroed(Verts.Num() * 3);
	StaticGeomIndices.AddZeroed(Faces.Num());

	float* DestCoord = StaticGeomCoords.GetTypedData() + FirstNewCoord;
	int32* DestIndex = StaticGeomIndices.GetTypedData() + FirstNewIndex;
#endif

	for (int32 i = 0; i < Verts.Num(); i++)
	{
		*DestCoord = Verts[i].X; DestCoord++;
		*DestCoord = Verts[i].Y; DestCoord++;
		*DestCoord = Verts[i].Z; DestCoord++;
	}

	for (int32 i = 0; i < Faces.Num(); i++, DestIndex++)
	{
		*DestIndex = Faces[i] + VertBase;
	}
}

void FRecastTileGenerator::AppendVoxels(rcSpanCache* SpanData, int32 NumSpans)
{
	if (NumSpans)
	{
#if !CACHE_NAV_GENERATOR_DATA
		const int32 FirstSpanIdx = StaticGeomSpans.Num();
		StaticGeomSpans.AddZeroed(NumSpans);

		FMemory::Memcpy(StaticGeomSpans.GetTypedData() + FirstSpanIdx, SpanData, NumSpans * sizeof(rcSpanCache));
#endif
	}
}

void FRecastTileGenerator::TriggerAsyncBuild()
{
#if RECAST_ASYNC_REBUILDING
	bool bBuildStarted = false;

	ANavigationData::FNavDataGeneratorSharedPtr NavMeshGeneratorSharedPtr = NavMeshGenerator.Pin();
	if (NavMeshGeneratorSharedPtr.IsValid())
	{
		FAsyncNavTileBuildTask* AsyncBuildTask = new FAsyncNavTileBuildTask(NavMeshGeneratorSharedPtr, TileId, Version);

		// Initiate
		if (AsyncBuildTask != NULL)
		{
			InitiateRebuild();
			bBuildStarted = true;

			AsyncBuildTask->StartBackgroundTask();
		}
	}

	if (!bBuildStarted)
	{
		AbortRebuild();
	}
#endif // RECAST_ASYNC_REBUILDING
}

bool FRecastTileGenerator::GenerateTile()
{
	struct FTileGenerationScopeLock 
	{
		FCriticalSection* SynchObject;
		FRecastTileGenerator* TileGenerator;

		FTileGenerationScopeLock(FCriticalSection* InSynchObject, FRecastTileGenerator* InTileGenerator) 
			: SynchObject(InSynchObject)
			, TileGenerator(InTileGenerator)
		{
			check(SynchObject);
			check(InTileGenerator->bBeingRebuild && "FRecastTileGenerator::TriggerAsyncBuild is the only legit way of triggering navmesh tile generation!");			
			SynchObject->Lock();
		}

		/**
			* Destructor that performs a release on the synchronization object
			*/
		~FTileGenerationScopeLock(void)
		{
			TileGenerator->bBeingRebuild = false;
			check(SynchObject);
			SynchObject->Unlock();
		}
	};

	bool bSuccess = false;
	if (bBeingRebuild == false)
	{
		UE_LOG(LogNavigation, Error, TEXT("NavMeshGeneration: tile (%d,%d) was not marked for rebuilding, abort generation!"), TileX, TileY);
		return bSuccess;
	}

	FTileGenerationScopeLock Lock(&GenerationLock, this);

	if (bInitialized == false)
	{
		UE_LOG(LogNavigation, Error, TEXT("NavMeshGeneration: Trying to generate navmesh tile with uninitialized generator"));
		return bSuccess;
	}

	// make sure it doesn't go away until end of this function
	ANavigationData::FNavDataGeneratorSharedPtr NavMeshGeneratorSharedPtr = NavMeshGenerator.Pin();
	if (NavMeshGeneratorSharedPtr.IsValid() == false)
	{
		return bSuccess;
	}

	FRecastNavMeshGenerator* Generator = (FRecastNavMeshGenerator*)NavMeshGeneratorSharedPtr.Get();
	FNavMeshBuildContext* BuildContext = Generator->GetBuildContext();

	if (!GeneratingState.bRebuildGeometry && !GeneratingState.bRebuildLayers)
	{
		BuildContext->log(RC_LOG_WARNING, "NavMeshGeneration: trying to generate tile while it's not dirty, skipping");
		return bSuccess;
	}

	RECAST_STAT(STAT_Navigation_Async_RecastBuilding);
	const double BuildStartTime = FPlatformTime::Seconds();

	if (GeneratingState.bRebuildGeometry)
	{
		DEC_MEMORY_STAT_BY(STAT_Navigation_TileCacheMemory, GetTileCacheSizeHelper(CompressedLayers));
		CompressedLayers.Reset();

		const bool bLayersReady = GenerateCompressedLayers(BuildContext, Generator);

		INC_MEMORY_STAT_BY(STAT_Navigation_TileCacheMemory, GetTileCacheSizeHelper(CompressedLayers));
		ClearGeometry();

		GeneratingState.bRebuildLayers = bLayersReady;
		if (bLayersReady)
		{
			GeneratingState.bRebuildAllLayers = true;
		}
	}

	if (GeneratingState.bRebuildLayers)
	{
		bSuccess = GenerateNavigationData(BuildContext, Generator);
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	const double CurrentTime = FPlatformTime::Seconds();
	const float TimeTaken = CurrentTime - BuildStartTime;
	UE_LOG(LogNavigation, Display, TEXT("FRecastTileGenerator(%s) tile (%d,%d) took %.3fs"),
		GeneratingState.bRebuildGeometry ? TEXT("full rebuild") : *FString::Printf(TEXT("layer update:%d"), NavigationData.Num()),
		TileX, TileY, TimeTaken);

	LastBuildTimeCost = TimeTaken;
	LastBuildTimeStamp = CurrentTime;
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	// it's possible to have valid generation with empty resulting tile (no navigable geometry in tile)
	return bSuccess;
}

struct FTileRasterizationContext
{
	FTileRasterizationContext() : SolidHF(0), LayerSet(0), CompactHF(0)
	{
	}

	~FTileRasterizationContext()
	{
		rcFreeHeightField(SolidHF);
		rcFreeHeightfieldLayerSet(LayerSet);
		rcFreeCompactHeightfield(CompactHF);
	}

	struct rcHeightfield* SolidHF;
	struct rcHeightfieldLayerSet* LayerSet;
	struct rcCompactHeightfield* CompactHF;
	TArray<FNavMeshTileData> Layers;
};

bool FRecastTileGenerator::GenerateCompressedLayers(class FNavMeshBuildContext* BuildContext, FRecastNavMeshGenerator* Generator)
{
	RECAST_STAT(STAT_Navigation_Async_Recast_Rasterize);
	SCOPE_CYCLE_COUNTER(STAT_Navigation_RecastBuildCompressedLayers);
	
	FRecastBuildConfig TileConfig = Generator->GetConfig();
	TileConfig.width = TileConfig.tileSize + TileConfig.borderSize*2;
	TileConfig.height = TileConfig.tileSize + TileConfig.borderSize*2;

	rcVcopy(TileConfig.bmin, BMin);
	rcVcopy(TileConfig.bmax, BMax);
	const float BBoxPadding = TileConfig.borderSize * TileConfig.cs;
	TileConfig.bmin[0] -= BBoxPadding;
	TileConfig.bmin[2] -= BBoxPadding;
	TileConfig.bmax[0] += BBoxPadding;
	TileConfig.bmax[2] += BBoxPadding;

	BuildContext->log(RC_LOG_PROGRESS, "GenerateCompressedLayers:");
	BuildContext->log(RC_LOG_PROGRESS, " - %d x %d cells", TileConfig.width, TileConfig.height);

#if CACHE_NAV_GENERATOR_DATA
	TNavStatArray<float>* PtrGeomCoords = &GeomCoords;
	TNavStatArray<int32>* PtrGeomIndices = &GeomIndices;
#else
	TNavStatArray<float>* PtrGeomCoords = &StaticGeomCoords;
	TNavStatArray<int32>* PtrGeomIndices = &StaticGeomIndices;
#endif
	TNavStatArray<rcSpanCache>* PtrGeomSpans = &StaticGeomSpans;

	FTileRasterizationContext RasterContext;

	// Allocate voxel heightfield where we rasterize our input data to.
	if (PtrGeomIndices->Num() || PtrGeomSpans->Num())
	{
		RasterContext.SolidHF = rcAllocHeightfield();
		if (RasterContext.SolidHF == NULL)
		{
			BuildContext->log(RC_LOG_ERROR, "GenerateCompressedLayers: Out of memory 'SolidHF'.");
			return false;
		}
		if (!rcCreateHeightfield(BuildContext, *RasterContext.SolidHF, TileConfig.width, TileConfig.height, TileConfig.bmin, TileConfig.bmax, TileConfig.cs, TileConfig.ch))
		{
			BuildContext->log(RC_LOG_ERROR, "GenerateCompressedLayers: Could not create solid heightfield.");
			return false;
		}
	}

	// Rasterize geometry
	if (PtrGeomIndices->Num() && PtrGeomCoords->Num())
	{
		DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Rasterization: without voxel cache"), Stat_RecastRasterNoCache, STATGROUP_Navigation);

		const int32 NumFaces = PtrGeomIndices->Num() / 3;
		const int32 NumVerts = PtrGeomCoords->Num() / 3;

		RECAST_STAT(STAT_Navigation_RasterizeTriangles);
		TNavStatArray<uint8> TriAreas;
		TriAreas.AddZeroed(NumFaces);

		rcMarkWalkableTriangles(BuildContext, TileConfig.walkableSlopeAngle,
			PtrGeomCoords->GetTypedData(), NumVerts, PtrGeomIndices->GetTypedData(), NumFaces,
			TriAreas.GetTypedData());

		rcRasterizeTriangles(BuildContext,
			PtrGeomCoords->GetTypedData(), NumVerts, 
			PtrGeomIndices->GetTypedData(), TriAreas.GetTypedData(), NumFaces,
			*RasterContext.SolidHF, TileConfig.walkableClimb);
	}
	else if (PtrGeomSpans->Num())
	{
		DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Rasterization: apply voxel cache"), Stat_RecastRasterCacheApply, STATGROUP_Navigation);

		rcAddSpans(BuildContext, *RasterContext.SolidHF, TileConfig.walkableClimb, PtrGeomSpans->GetTypedData(), PtrGeomSpans->Num());
	}

	if (!RasterContext.SolidHF || RasterContext.SolidHF->pools == 0)
	{
		BuildContext->log(RC_LOG_WARNING, "GenerateCompressedLayers: empty tile - aborting");
		return true;
	}

	// Reject voxels outside generation boundaries
	if (TileConfig.bPerformVoxelFiltering && !bFullyEncapsulatedByInclusionBounds)
	{
		ApplyVoxelFilter(RasterContext.SolidHF, TileConfig.walkableRadius);
	}

	{
		RECAST_STAT(STAT_Navigation_Async_Recast_Filter);
		// Once all geometry is rasterized, we do initial pass of filtering to
		// remove unwanted overhangs caused by the conservative rasterization
		// as well as filter spans where the character cannot possibly stand.
		rcFilterLowHangingWalkableObstacles(BuildContext, TileConfig.walkableClimb, *RasterContext.SolidHF);
		rcFilterLedgeSpans(BuildContext, TileConfig.walkableHeight, TileConfig.walkableClimb, *RasterContext.SolidHF);
		rcFilterWalkableLowHeightSpans(BuildContext, TileConfig.walkableHeight, *RasterContext.SolidHF);
	}

	{
		RECAST_STAT(STAT_Navigation_Async_Recast_BuildCompact);
		// Compact the heightfield so that it is faster to handle from now on.
		// This will result more cache coherent data as well as the neighbors
		// between walkable cells will be calculated.
		RasterContext.CompactHF = rcAllocCompactHeightfield();
		if (RasterContext.CompactHF == NULL)
		{
			BuildContext->log(RC_LOG_ERROR, "GenerateCompressedLayers: Out of memory 'CompactHF'.");
			return false;
		}
		if (!rcBuildCompactHeightfield(BuildContext, TileConfig.walkableHeight, TileConfig.walkableClimb, *RasterContext.SolidHF, *RasterContext.CompactHF))
		{
			BuildContext->log(RC_LOG_ERROR, "GenerateCompressedLayers: Could not build compact data.");
			return false;
		}
	}

	{
		RECAST_STAT(STAT_Navigation_Async_Recast_Erode);
		// Erode the walkable area by agent radius.
		// do this step only if we're considering non-zero agent radius 
		if (TileConfig.walkableRadius > RECAST_VERY_SMALL_AGENT_RADIUS && !rcErodeWalkableArea(BuildContext, TileConfig.walkableRadius, *RasterContext.CompactHF))
		{
			BuildContext->log(RC_LOG_ERROR, "GenerateCompressedLayers: Could not erode.");
			return false;
		}
	}

	// (Optional) Mark areas.
	MarkStaticAreas(BuildContext, *RasterContext.CompactHF, TileConfig);
	
	// Build layers
	{
		RECAST_STAT(STAT_Navigation_Async_Recast_Layers);
		
		RasterContext.LayerSet = rcAllocHeightfieldLayerSet();
		if (RasterContext.LayerSet == NULL)
		{
			BuildContext->log(RC_LOG_ERROR, "GenerateCompressedLayers: Out of memory 'LayerSet'.");
			return false;
		}

		if (TileConfig.regionPartitioning == RC_REGION_MONOTONE)
		{
			if (!rcBuildHeightfieldLayersMonotone(BuildContext, *RasterContext.CompactHF, TileConfig.borderSize, TileConfig.walkableHeight, *RasterContext.LayerSet))
			{
				BuildContext->log(RC_LOG_ERROR, "GenerateCompressedLayers: Could not build heightfield layers.");
				return 0;
			}
		}
		else if (TileConfig.regionPartitioning == RC_REGION_WATERSHED)
		{
			if (!rcBuildDistanceField(BuildContext, *RasterContext.CompactHF))
			{
				BuildContext->log(RC_LOG_ERROR, "GenerateCompressedLayers: Could not build distance field.");
				return 0;
			}

			if (!rcBuildHeightfieldLayers(BuildContext, *RasterContext.CompactHF, TileConfig.borderSize, TileConfig.walkableHeight, *RasterContext.LayerSet))
			{
				BuildContext->log(RC_LOG_ERROR, "GenerateCompressedLayers: Could not build heightfield layers.");
				return 0;
			}
		}
		else
		{
			if (!rcBuildHeightfieldLayersChunky(BuildContext, *RasterContext.CompactHF, TileConfig.borderSize, TileConfig.walkableHeight, TileConfig.regionChunkSize, *RasterContext.LayerSet))
			{
				BuildContext->log(RC_LOG_ERROR, "GenerateCompressedLayers: Could not build heightfield layers.");
				return 0;
			}
		}

		const int32 NumLayers = RasterContext.LayerSet->nlayers;
		LayerBB.Reset();
		LayerBB.AddZeroed(NumLayers);

		// use this to expand vertically layer's bounds
		// this is needed to allow off-mesh connections that are not quite
		// touching tile layer still connect with it.
		const float StepHeights = TileConfig.AgentMaxClimb;

		FTileCacheCompressor TileCompressor;
		for (int32 i = 0; i < NumLayers; i++)
		{
			const rcHeightfieldLayer* layer = &RasterContext.LayerSet->layers[i];

			// Store header
			dtTileCacheLayerHeader header;
			header.magic = DT_TILECACHE_MAGIC;
			header.version = DT_TILECACHE_VERSION;

			// Tile layer location in the navmesh.
			header.tx = TileX;
			header.ty = TileY;
			header.tlayer = i;
			dtVcopy(header.bmin, layer->bmin);
			dtVcopy(header.bmax, layer->bmax);

			// Tile info.
			header.width = (unsigned short)layer->width;
			header.height = (unsigned short)layer->height;
			header.minx = (unsigned short)layer->minx;
			header.maxx = (unsigned short)layer->maxx;
			header.miny = (unsigned short)layer->miny;
			header.maxy = (unsigned short)layer->maxy;
			header.hmin = (unsigned short)layer->hmin;
			header.hmax = (unsigned short)layer->hmax;

			// Store bounds
			LayerBB[i] = Recast2UnrealBox(header.bmin, header.bmax);
			LayerBB[i].Min.Z -= StepHeights;
			LayerBB[i].Max.Z += StepHeights;

			// Compress tile layer
			uint8* TileData = NULL;
			int32 TileDataSize = 0;
			const dtStatus status = dtBuildTileCacheLayer(&TileCompressor, &header, layer->heights, layer->areas, layer->cons, &TileData, &TileDataSize);
			if (dtStatusFailed(status))
			{
				dtFree(TileData);
				BuildContext->log(RC_LOG_ERROR, "GenerateCompressedLayers: failed to build layer.");
				return false;
			}

			// copy compressed data to new buffer in rasterization context
			// (TileData allocates a lots of space, but only first TileDataSize bytes hold compressed data)

			uint8* CompressedData = (uint8*)dtAlloc(TileDataSize * sizeof(uint8), DT_ALLOC_PERM);
			if (CompressedData == NULL)
			{
				dtFree(TileData);
				BuildContext->log(RC_LOG_ERROR, "GenerateCompressedLayers: Out of memory 'CompressedData'.");
				return false;
			}

			FMemory::Memcpy(CompressedData, TileData, TileDataSize);
			RasterContext.Layers.Add(FNavMeshTileData(CompressedData, TileDataSize, i));
			dtFree(TileData);

			const int32 UncompressedSize = ((sizeof(dtTileCacheLayerHeader)+3) & ~3) + (3 * header.width * header.height);
			const float Inv1kB = 1.0f / 1024.0f;
			BuildContext->log(RC_LOG_PROGRESS, ">> Cache[%d,%d:%d] = %.2fkB (full:%.2fkB rate:%.2f%%)", TileX, TileY, i,
				TileDataSize * Inv1kB, UncompressedSize * Inv1kB, 1.0f * TileDataSize / UncompressedSize);
		}
	}

	// Transfer final data
	CompressedLayers = RasterContext.Layers;
	RasterContext.Layers.Reset();
	return true;
}

void FRecastTileGenerator::MarkStaticAreas(class FNavMeshBuildContext* BuildContext, struct rcCompactHeightfield& CompactHF, const struct FRecastBuildConfig& TileConfig)
{
#if CACHE_NAV_GENERATOR_DATA
	TArray<FAreaNavModifier>* PtrStaticAreas = &StaticAreas;
#else
	TArray<FAreaNavModifier>* PtrStaticAreas = &StaticStaticAreas;
#endif

	const int32 NumAreas = PtrStaticAreas->Num();
	if (NumAreas == 0)
	{
		return;
	}

	FRecastNavMeshCachedData* AdditionalCachedDataPtr = AdditionalCachedData.Get();
	if (AdditionalCachedDataPtr->bSortNavigationAreasByCost && NumAreas > 1)
	{
		FAreaNavModifier* Modifier = PtrStaticAreas->GetTypedData();
		for (int32 ModifierIndex = 0; ModifierIndex < NumAreas; ++ModifierIndex, ++Modifier)
		{
			AdditionalCachedData->SortPredicate.Prepare(Modifier);
		}

		StaticAreas.Sort(AdditionalCachedDataPtr->SortPredicate);
	}

	RECAST_STAT(STAT_Navigation_Async_MarkAreas);
	const float ExpandBy = TileConfig.AgentRadius;

	const FAreaNavModifier* Modifier = PtrStaticAreas->GetTypedData();
	for (int32 ModifierIndex = 0; ModifierIndex < NumAreas; ++ModifierIndex, ++Modifier)
	{
		const uint8* AreaID = AdditionalCachedDataPtr->AreaClassToIDMap.Find(Modifier->GetAreaClass());
		if (AreaID == NULL)
		{
			// happens when area is not supported by agent owning this navmesh
			continue;
		}

		switch (Modifier->GetShapeType())
		{
		case ENavigationShapeType::Cylinder:
			{
				FCylinderNavAreaData CylinderData;
				Modifier->GetCylinder(CylinderData);

				CylinderData.Height += TileConfig.ch;
				CylinderData.Radius += ExpandBy;

				FVector RecastPos = Unreal2RecastPoint(CylinderData.Origin);

				rcMarkCylinderArea(BuildContext, &(RecastPos.X), CylinderData.Radius, CylinderData.Height, *AreaID, CompactHF);
			}
			break;

		case ENavigationShapeType::Box:
			{
				FBoxNavAreaData BoxData;
				Modifier->GetBox(BoxData);

				BoxData.Extent += FVector(ExpandBy, ExpandBy, TileConfig.ch);

				FBox UnrealBox(BoxData.Origin - BoxData.Extent, BoxData.Origin + BoxData.Extent);
				FBox RecastBox = Unreal2RecastBox(UnrealBox);

				rcMarkBoxArea(BuildContext, &(RecastBox.Min.X), &(RecastBox.Max.X), *AreaID, CompactHF);
			}
			break;

		case ENavigationShapeType::Convex:
			{
				FConvexNavAreaData ConvexData;
				Modifier->GetConvex(ConvexData);

				TArray<FVector> ConvexVerts;
				GrowConvexHull(ExpandBy, ConvexData.Points, ConvexVerts);
				ConvexData.MinZ -= TileConfig.ch;
				ConvexData.MaxZ += TileConfig.ch;

				if (ConvexVerts.Num())
				{
					TArray<float> ConvexCoords;
					ConvexCoords.AddZeroed(ConvexVerts.Num() * 3);

					float* ItCoord = ConvexCoords.GetTypedData();
					for (int32 i = 0; i < ConvexVerts.Num(); i++)
					{
						const FVector RecastV = Unreal2RecastPoint(ConvexVerts[i]);
						*ItCoord = RecastV.X; ItCoord++;
						*ItCoord = RecastV.Y; ItCoord++;
						*ItCoord = RecastV.Z; ItCoord++;
					}

					rcMarkConvexPolyArea(BuildContext, ConvexCoords.GetTypedData(), ConvexVerts.Num(),
						ConvexData.MinZ, ConvexData.MaxZ, *AreaID, CompactHF);
				}
			}
			break;
		
		default: break;
		}
	}
}

struct FTileGenerationContext
{
	FTileGenerationContext(struct dtTileCacheAlloc* MyAllocator) :
		Allocator(MyAllocator), Layer(0), DistanceField(0), ContourSet(0), ClusterSet(0), PolyMesh(0), DetailMesh(0)
	{
	}

	~FTileGenerationContext()
	{
		ResetIntermediateData();
	}

	void ResetIntermediateData()
	{
		dtFreeTileCacheLayer(Allocator, Layer);
		Layer = 0;
		dtFreeTileCacheDistanceField(Allocator, DistanceField);
		DistanceField = 0;
		dtFreeTileCacheContourSet(Allocator, ContourSet);
		ContourSet = 0;
		dtFreeTileCacheClusterSet(Allocator, ClusterSet);
		ClusterSet = 0;
		dtFreeTileCachePolyMesh(Allocator, PolyMesh);
		PolyMesh = 0;
		dtFreeTileCachePolyMeshDetail(Allocator, DetailMesh);
		DetailMesh = 0;

		// don't clear NavigationData here!
	}

	struct dtTileCacheAlloc* Allocator;
	struct dtTileCacheLayer* Layer;
	struct dtTileCacheDistanceField* DistanceField;
	struct dtTileCacheContourSet* ContourSet;
	struct dtTileCacheClusterSet* ClusterSet;
	struct dtTileCachePolyMesh* PolyMesh;
	struct dtTileCachePolyMeshDetail* DetailMesh;
	TArray<FNavMeshTileData> NavigationData;
};

bool FRecastTileGenerator::GenerateNavigationData(class FNavMeshBuildContext* BuildContext, FRecastNavMeshGenerator* Generator)
{
	RECAST_STAT(STAT_Navigation_Async_Recast_Generate);
	SCOPE_CYCLE_COUNTER(STAT_Navigation_RecastBuildNavigation);

#if CACHE_NAV_GENERATOR_DATA
	TArray<FSimpleLinkNavModifier>* PtrOffmeshLinks = &OffmeshLinks;
#else
	TArray<FSimpleLinkNavModifier>* PtrOffmeshLinks = &StaticOffmeshLinks;
#endif

	FTileCacheAllocator MyAllocator;
	FTileCacheCompressor TileCompressor;
	
	FTileGenerationContext GenerationContext(&MyAllocator);
	GenerationContext.NavigationData.AddZeroed(CompressedLayers.Num());

	FRecastNavMeshCachedData* AdditionalCachedDataPtr = AdditionalCachedData.Get();
	const FRecastBuildConfig& TileConfig = Generator->GetConfig();
	dtStatus status = DT_SUCCESS;

	int32 NumLayers = 0;
	for (int32 iLayer = 0; iLayer < CompressedLayers.Num(); iLayer++)
	{
		if (!GeneratingState.HasDirtyLayer(iLayer))
		{
			// skip layers not marked for rebuild
			continue;
		}

		FNavMeshTileData& CompressedData = CompressedLayers[iLayer];
		const dtTileCacheLayerHeader* TileHeader = (const dtTileCacheLayerHeader*)CompressedData.GetDataSafe();
		GenerationContext.ResetIntermediateData();

		// Decompress tile layer data. 
		status = dtDecompressTileCacheLayer(&MyAllocator, &TileCompressor, (unsigned char*)CompressedData.GetDataSafe(), CompressedData.DataSize, &GenerationContext.Layer);
		if (dtStatusFailed(status))
		{
			BuildContext->log(RC_LOG_ERROR, "GenerateNavigationData: failed to decompress layer.");
			return false;
		}

		// Rasterize obstacles.
		MarkDynamicAreas(*GenerationContext.Layer, TileConfig);

		{
			RECAST_STAT(STAT_Navigation_Async_Recast_BuildRegions)
			// Build regions
			if (TileConfig.TileCachePartitionType == RC_REGION_MONOTONE)
			{
				status = dtBuildTileCacheRegionsMonotone(&MyAllocator, *GenerationContext.Layer);
			}
			else if (TileConfig.TileCachePartitionType == RC_REGION_WATERSHED)
			{
				GenerationContext.DistanceField = dtAllocTileCacheDistanceField(&MyAllocator);
				if (GenerationContext.DistanceField == NULL)
				{
					BuildContext->log(RC_LOG_ERROR, "GenerateNavigationData: Out of memory 'DistanceField'.");
					return false;
				}

				status = dtBuildTileCacheDistanceField(&MyAllocator, *GenerationContext.Layer, *GenerationContext.DistanceField);
				if (dtStatusFailed(status))
				{
					BuildContext->log(RC_LOG_ERROR, "GenerateNavigationData: Failed to build distance field.");
					return false;
				}

				const int TileBoderSize = 0;
				status = dtBuildTileCacheRegions(&MyAllocator, TileBoderSize, TileConfig.minRegionArea, TileConfig.mergeRegionArea, *GenerationContext.Layer, *GenerationContext.DistanceField);
			}
			else
			{
				status = dtBuildTileCacheRegionsChunky(&MyAllocator, *GenerationContext.Layer, TileConfig.TileCacheChunkSize);
			}

			if (dtStatusFailed(status))
			{
				BuildContext->log(RC_LOG_ERROR, "GenerateNavigationData: Failed to build regions.");
				return false;
			}
		}
	
		{
			RECAST_STAT(STAT_Navigation_Async_Recast_BuildContours);
			// Build contour set
			GenerationContext.ContourSet = dtAllocTileCacheContourSet(&MyAllocator);
			if (GenerationContext.ContourSet == NULL)
			{
				BuildContext->log(RC_LOG_ERROR, "GenerateNavigationData: Out of memory 'ContourSet'.");
				return false;
			}

			GenerationContext.ClusterSet = dtAllocTileCacheClusterSet(&MyAllocator);
			if (GenerationContext.ClusterSet == NULL)
			{
				BuildContext->log(RC_LOG_ERROR, "GenerateNavigationData: Out of memory 'ClusterSet'.");
				return false;
			}

			status = dtBuildTileCacheContours(&MyAllocator, *GenerationContext.Layer,
				TileConfig.walkableClimb, TileConfig.maxSimplificationError, TileConfig.cs, TileConfig.ch,
				*GenerationContext.ContourSet, *GenerationContext.ClusterSet);
			if (dtStatusFailed(status))
			{
				BuildContext->log(RC_LOG_ERROR, "GenerateNavigationData: Failed to generate contour set (0x%08X).", status);
				return false;
			}
		}

		{
			RECAST_STAT(STAT_Navigation_Async_Recast_BuildPolyMesh);
			// Build poly mesh
			GenerationContext.PolyMesh = dtAllocTileCachePolyMesh(&MyAllocator);
			if (GenerationContext.PolyMesh == NULL)
			{
				BuildContext->log(RC_LOG_ERROR, "GenerateNavigationData: Out of memory 'PolyMesh'.");
				return false;
			}

			status = dtBuildTileCachePolyMesh(&MyAllocator, *GenerationContext.ContourSet, *GenerationContext.PolyMesh);
			if (dtStatusFailed(status))
			{
				BuildContext->log(RC_LOG_ERROR, "GenerateNavigationData: Failed to generate poly mesh.");
				return false;
			}
		}

		// Build detail mesh
		if (TileConfig.bGenerateDetailedMesh)
		{
			RECAST_STAT(STAT_Navigation_Async_Recast_BuildPolyDetail);

			// Build detail mesh.
			GenerationContext.DetailMesh = dtAllocTileCachePolyMeshDetail(&MyAllocator);
			if (GenerationContext.DetailMesh == NULL)
			{
				BuildContext->log(RC_LOG_ERROR, "GenerateNavigationData: Out of memory 'DetailMesh'.");
				return false;
			}

			status = dtBuildTileCachePolyMeshDetail(&MyAllocator, TileConfig.cs, TileConfig.ch, TileConfig.detailSampleDist, TileConfig.detailSampleMaxError,
				*GenerationContext.Layer, *GenerationContext.PolyMesh, *GenerationContext.DetailMesh);
			if (dtStatusFailed(status))
			{
				BuildContext->log(RC_LOG_ERROR, "GenerateNavigationData: Failed to generate poly detail mesh.");
				return false;
			}
		}

		unsigned char* NavData = 0;
		int32 NavDataSize = 0;

		if (TileConfig.maxVertsPerPoly <= DT_VERTS_PER_POLYGON &&
			GenerationContext.PolyMesh->npolys > 0 && GenerationContext.PolyMesh->nverts > 0)
		{
			ensure(GenerationContext.PolyMesh->npolys <= TileConfig.MaxPolysPerTile && "Polys per Tile limit exceeded!");
			if (GenerationContext.PolyMesh->nverts >= 0xffff)
			{
				// The vertex indices are ushorts, and cannot point to more than 0xffff vertices.
				BuildContext->log(RC_LOG_ERROR, "Too many vertices per tile %d (max: %d).", GenerationContext.PolyMesh->nverts, 0xffff);
				return false;
			}

			// if we didn't failed already then it's hight time we created data for off-mesh links
			FOffMeshData OffMeshData;
			if (PtrOffmeshLinks->Num() > 0)
			{
				RECAST_STAT(STAT_Navigation_Async_GatherOffMeshData);

				OffMeshData.Reserve(PtrOffmeshLinks->Num());
				OffMeshData.AreaClassToIDMap = &AdditionalCachedDataPtr->AreaClassToIDMap;
				OffMeshData.FlagsPerArea = AdditionalCachedDataPtr->FlagsPerOffMeshLinkArea;
				const uint32 AgentMask = (1 << TileConfig.AgentIndex);

				const FSimpleLinkNavModifier* LinkModifier = PtrOffmeshLinks->GetTypedData();
				for (int32 LinkModifierIndex = 0; LinkModifierIndex < PtrOffmeshLinks->Num(); ++LinkModifierIndex, ++LinkModifier)
				{
					OffMeshData.AddLinks(LinkModifier->Links, LinkModifier->LocalToWorld, AgentMask);
#if GENERATE_SEGMENT_LINKS
					OffMeshData.AddSegmentLinks(LinkModifier->SegmentLinks, LinkModifier->LocalToWorld, AgentMask);
#endif // GENERATE_SEGMENT_LINKS
				}
			}

			// fill flags, or else detour won't be able to find polygons
			// Update poly flags from areas.
			for (int32 i = 0; i < GenerationContext.PolyMesh->npolys; i++)
			{
				GenerationContext.PolyMesh->flags[i] = AdditionalCachedDataPtr->FlagsPerArea[GenerationContext.PolyMesh->areas[i]];
			}

			dtNavMeshCreateParams Params;
			memset(&Params, 0, sizeof(Params));
			Params.verts = GenerationContext.PolyMesh->verts;
			Params.vertCount = GenerationContext.PolyMesh->nverts;
			Params.polys = GenerationContext.PolyMesh->polys;
			Params.polyAreas = GenerationContext.PolyMesh->areas;
			Params.polyFlags = GenerationContext.PolyMesh->flags;
			Params.polyCount = GenerationContext.PolyMesh->npolys;
			Params.nvp = GenerationContext.PolyMesh->nvp;
			if (TileConfig.bGenerateDetailedMesh)
			{
				Params.detailMeshes = GenerationContext.DetailMesh->meshes;
				Params.detailVerts = GenerationContext.DetailMesh->verts;
				Params.detailVertsCount = GenerationContext.DetailMesh->nverts;
				Params.detailTris = GenerationContext.DetailMesh->tris;
				Params.detailTriCount = GenerationContext.DetailMesh->ntris;
			}
			Params.offMeshCons = OffMeshData.LinkParams.GetTypedData();
			Params.offMeshConCount = OffMeshData.LinkParams.Num();
			Params.walkableHeight = TileConfig.AgentHeight;
			Params.walkableRadius = TileConfig.AgentRadius;
			Params.walkableClimb = TileConfig.AgentMaxClimb;
			Params.tileX = TileX;
			Params.tileY = TileY;
			Params.tileLayer = iLayer;
			rcVcopy(Params.bmin, GenerationContext.Layer->header->bmin);
			rcVcopy(Params.bmax, GenerationContext.Layer->header->bmax);
			Params.cs = TileConfig.cs;
			Params.ch = TileConfig.ch;
			Params.buildBvTree = TileConfig.bGenerateBVTree;
#if GENERATE_CLUSTER_LINKS
			Params.clusterCount = GenerationContext.ClusterSet->nclusters;
			Params.clusterCenters = GenerationContext.ClusterSet->center;
			Params.clusterLinkCount = GenerationContext.ClusterSet->nlinks;
			Params.clusterLinks = GenerationContext.ClusterSet->links;
			Params.polyClusters = GenerationContext.PolyMesh->regs;
#endif

			RECAST_STAT(STAT_Navigation_Async_Recast_CreateNavMeshData);

			if (!dtCreateNavMeshData(&Params, &NavData, &NavDataSize))
			{
				BuildContext->log(RC_LOG_ERROR, "Could not build Detour navmesh.");
				return false;
			}
		}

		GenerationContext.NavigationData[NumLayers] = FNavMeshTileData(NavData, NavDataSize, iLayer);
		NumLayers++;

		const float ModkB = 1.0f / 1024.0f;
		BuildContext->log(RC_LOG_PROGRESS, ">> Layer[%d] = Verts(%d) Polys(%d) Memory(%.2fkB) Cache(%.2fkB)",
			iLayer, GenerationContext.PolyMesh->nverts, GenerationContext.PolyMesh->npolys,
			GenerationContext.NavigationData[iLayer].DataSize * ModkB, CompressedLayers[iLayer].DataSize * ModkB);
	}

	// prepare navigation data of actually rebuild layers for transfer
	NavigationData.Reset();
	NavigationData.AddZeroed(NumLayers);
	for (int32 i = 0; i < NumLayers; i++)
	{
		NavigationData[i] = GenerationContext.NavigationData[i];
	}

	return true;
}

void FRecastTileGenerator::MarkDynamicAreas(struct dtTileCacheLayer& Layer, const struct FRecastBuildConfig& TileConfig)
{
#if CACHE_NAV_GENERATOR_DATA
	TArray<FAreaNavModifier>* PtrStaticAreas = &StaticAreas;
	TArray<FAreaNavModifier>* PtrDynamicAreas = &DynamicAreas;
#else
	TArray<FAreaNavModifier>* PtrStaticAreas = &StaticStaticAreas;
	TArray<FAreaNavModifier>* PtrDynamicAreas = &StaticDynamicAreas;
#endif

	if (PtrDynamicAreas->Num() == 0)
	{
		return;
	}

	// combine both area types, to avoid overriding high cost static with low cost dynamic one
	TArray<FAreaNavModifier> CombinedAreas;
	CombinedAreas.Append(*PtrStaticAreas);
	CombinedAreas.Append(*PtrDynamicAreas);

	FRecastNavMeshCachedData* AdditionalCachedDataPtr = AdditionalCachedData.Get();
	if (AdditionalCachedDataPtr->bSortNavigationAreasByCost && CombinedAreas.Num() > 1)
	{
		FAreaNavModifier* Modifier = CombinedAreas.GetTypedData();
		for (int32 ModifierIndex = 0; ModifierIndex < CombinedAreas.Num(); ++ModifierIndex, ++Modifier)
		{
			AdditionalCachedData->SortPredicate.Prepare(Modifier);
		}

		CombinedAreas.Sort(AdditionalCachedDataPtr->SortPredicate);
	}

	RECAST_STAT(STAT_Navigation_Async_MarkAreas);
	const float ExpandBy = TileConfig.AgentRadius;
	const float* LayerRecastOrig = Layer.header->bmin;
	const FBox LayerUnrealBounds = Recast2UnrealBox(Layer.header->bmin, Layer.header->bmax);

	const FAreaNavModifier* Modifier = CombinedAreas.GetTypedData();
	for (int32 ModifierIndex = 0; ModifierIndex < CombinedAreas.Num(); ++ModifierIndex, ++Modifier)
	{
		const uint8* AreaID = AdditionalCachedDataPtr->AreaClassToIDMap.Find(Modifier->GetAreaClass());
		if (AreaID == NULL)
		{
			// happens when area is not supported by agent owning this navmesh
			continue;
		}

		const FBox& ModifierBounds = Modifier->GetBounds();
		if (!LayerUnrealBounds.Intersect(ModifierBounds))
		{
			continue;
		}

		switch (Modifier->GetShapeType())
		{
		case ENavigationShapeType::Cylinder:
			{
				FCylinderNavAreaData CylidnerData;
				Modifier->GetCylinder(CylidnerData);
				
				CylidnerData.Height += TileConfig.ch;
				CylidnerData.Radius += ExpandBy;

				FVector RecastPos = Unreal2RecastPoint(CylidnerData.Origin);

				dtMarkCylinderArea(Layer, LayerRecastOrig, TileConfig.cs, TileConfig.ch,
					&(RecastPos.X), CylidnerData.Radius, CylidnerData.Height, *AreaID);
			}
			break;

		case ENavigationShapeType::Box:
			{
				FBoxNavAreaData BoxData;
				Modifier->GetBox(BoxData);

				BoxData.Extent += FVector(ExpandBy, ExpandBy, TileConfig.ch);

				FVector RecastPos = Unreal2RecastPoint(BoxData.Origin);
				FVector RecastExtent = Unreal2RecastPoint(BoxData.Extent).GetAbs();

				dtMarkBoxArea(Layer, LayerRecastOrig, TileConfig.cs, TileConfig.ch,
					&(RecastPos.X), &(RecastExtent.X), *AreaID);
			}
			break;

		case ENavigationShapeType::Convex:
			{
				FConvexNavAreaData ConvexData;
				Modifier->GetConvex(ConvexData);

				TArray<FVector> ConvexVerts;
				GrowConvexHull(ExpandBy, ConvexData.Points, ConvexVerts);

				ConvexData.MinZ -= TileConfig.ch;
				ConvexData.MaxZ += TileConfig.ch;

				if (ConvexVerts.Num())
				{
					TArray<float> ConvexCoords;
					ConvexCoords.AddZeroed(ConvexVerts.Num() * 3);

					float* ItCoord = ConvexCoords.GetTypedData();
					for (int32 i = 0; i < ConvexVerts.Num(); i++)
					{
						const FVector RecastV = Unreal2RecastPoint(ConvexVerts[i]);
						*ItCoord = RecastV.X; ItCoord++;
						*ItCoord = RecastV.Y; ItCoord++;
						*ItCoord = RecastV.Z; ItCoord++;
					}

					dtMarkConvexArea(Layer, LayerRecastOrig, TileConfig.cs, TileConfig.ch,
						ConvexCoords.GetTypedData(), ConvexVerts.Num(), ConvexData.MinZ, ConvexData.MaxZ, *AreaID);
				}
			}
			break;

		default: break;
		}
	}
}

uint32 FRecastTileGenerator::GetUsedMemCount() const
{
	uint32 TotalMemory = 0;
	TotalMemory += InclusionBounds.GetAllocatedSize();
	TotalMemory += StaticAreas.GetAllocatedSize();
	TotalMemory += DynamicAreas.GetAllocatedSize();
	TotalMemory += OffmeshLinks.GetAllocatedSize();
	TotalMemory += GeomCoords.GetAllocatedSize();
	TotalMemory += GeomIndices.GetAllocatedSize();
	
	const FSimpleLinkNavModifier* SimpleLink = OffmeshLinks.GetTypedData();
	for (int32 Index = 0; Index < OffmeshLinks.Num(); ++Index, ++SimpleLink)
	{
		TotalMemory += SimpleLink->Links.GetAllocatedSize();
	}

	TotalMemory += CompressedLayers.GetAllocatedSize();
	for (int32 i = 0; i < CompressedLayers.Num(); i++)
	{
		TotalMemory += CompressedLayers[i].DataSize;
	}

	TotalMemory += NavigationData.GetAllocatedSize();
	for (int32 i = 0; i < NavigationData.Num(); i++)
	{
		TotalMemory += NavigationData[i].DataSize;
	}

	return TotalMemory;
}

void FRecastTileGenerator::SetDirty(const FNavigationDirtyArea& DirtyArea)
{
	DirtyState.bRebuildGeometry |= DirtyArea.HasFlag(ENavigationDirtyFlag::Geometry);
	if (DirtyState.bRebuildGeometry)
	{
		return;
	}

	if (DirtyArea.HasFlag(ENavigationDirtyFlag::DynamicModifier))
	{
		for (int32 i = 0; i < LayerBB.Num(); i++)
		{
			if (LayerBB[i].Intersect(DirtyArea.Bounds))
			{
				DirtyState.MarkDirtyLayer(i);
			}
		}
	}
}

//----------------------------------------------------------------------//
// FRecastNavMeshGenerator
//----------------------------------------------------------------------//

FRecastNavMeshGenerator::FRecastNavMeshGenerator(class ARecastNavMesh* InDestNavMesh)
	: DetourMesh(NULL), MaxActiveTiles(-1), NumActiveTiles(0), MaxActiveGenerators(64)
	, TilesWidth(-1), TilesHeight(-1)
	, GridWidth(-1), GridHeight(-1)
	, TileSize(-1), RCNavBounds(0), UnrealNavBounds(0)
	, DestNavMesh(InDestNavMesh), OutNavMesh(NULL)
	, bInitialized(false), bBuildFromScratchRequested(false), bRebuildDirtyTilesRequested(false)
	, bAbortAllTileGeneration(false), bOwnsDetourMesh(false), bBuildingLocked(false), Version(0)
{
#if WITH_EDITOR
	bBuildingLocked = UNavigationSystem::GetIsNavigationAutoUpdateEnabled() == false;
#endif // WITH_EDITOR
	BuildContext = new FNavMeshBuildContext();
	INC_DWORD_STAT_BY( STAT_NavigationMemory, sizeof(*this) );

	check(InDestNavMesh);
	check(InDestNavMesh->GetWorld());

//#if SEAMLESS_REBUILDING_ENABLED
//	// register when everything is set up
//	FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
//		FSimpleDelegateGraphTask::FDelegate::CreateSP(this, &FRecastNavMeshGenerator::RegisterForCallbacks)
//		, TEXT("Register for callback")
//		, NULL
//		, ENamedThreads::GameThread
//		);
//#endif // SEAMLESS_REBUILDING_ENABLED
}

FRecastNavMeshGenerator::~FRecastNavMeshGenerator()
{
	// Free only when no ARecastNavMesh action assigned. It there's an actor then he 
	// is responsible 
	SetDetourMesh(NULL);

	if (BuildContext)
	{
		delete BuildContext;
		BuildContext = NULL;
	}

	DEC_DWORD_STAT_BY( STAT_NavigationMemory, sizeof(*this) );
}

void FRecastNavMeshGenerator::Init()
{
	if (GetWorld() == NULL)
	{
		return;	
	}

	FScopeLock lock(&InitLock);

	// use existing navmesh to respect per-mesh tweaked values, or default navmesh to get defaults
	ARecastNavMesh const* const NavGenParams = DestNavMesh.IsValid() ? DestNavMesh.Get() : ARecastNavMesh::StaticClass()->GetDefaultObject<ARecastNavMesh>();
	
	// @todo those variables should be tweakable per navmesh actor
	const float CellSize = NavGenParams ? NavGenParams->CellSize : 19.0;
	const float CellHeight = NavGenParams ? NavGenParams->CellHeight : 10.0;
	const float AgentHeight = NavGenParams ? NavGenParams->AgentHeight : (72.f * 2.0f);
	const float MaxAgentHeight = NavGenParams ? NavGenParams->AgentMaxHeight : 160.f;
	const float AgentMaxSlope = NavGenParams ? NavGenParams->AgentMaxSlope : 55.f;
	const float AgentMaxClimb = NavGenParams ? NavGenParams->AgentMaxStepHeight : 35.f;
	const float AgentRadius = NavGenParams ? NavGenParams->AgentRadius : RECAST_VERY_SMALL_AGENT_RADIUS;

	SetUpGeneration(CellSize, CellHeight, AgentHeight, MaxAgentHeight, AgentMaxSlope, AgentMaxClimb, AgentRadius);

	if (NavGenParams != NULL)
	{
		Config.minRegionArea = (int32)rcSqr(NavGenParams->MinRegionArea / CellSize);
		Config.mergeRegionArea = (int32)rcSqr(NavGenParams->MergeRegionSize / CellSize);
		Config.maxSimplificationError = NavGenParams->MaxSimplificationError;
		Config.bPerformVoxelFiltering = NavGenParams->bPerformVoxelFiltering;

		AdditionalCachedData = MakeShareable(new FRecastNavMeshCachedData(NavGenParams));

		const UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(GetWorld());
		Config.AgentIndex = NavSys ? NavSys->GetSupportedAgentIndex(NavGenParams) : 0;

		Config.tileSize = FMath::Trunc(NavGenParams->TileSizeUU / CellSize);

		Config.regionChunkSize = Config.tileSize / NavGenParams->LayerChunkSplits;
		Config.TileCacheChunkSize = Config.tileSize / NavGenParams->RegionChunkSplits;
		Config.regionPartitioning = NavGenParams->LayerPartitioning;
		Config.TileCachePartitionType = NavGenParams->RegionPartitioning;
	}
	else
	{
		Config.tileSize = 64;
		Config.regionPartitioning = RC_REGION_WATERSHED;
		Config.TileCachePartitionType = RC_REGION_WATERSHED;
	}

	const float* BMin = Config.bmin;
	const float* BMax = Config.bmax;
	rcCalcGridSize(BMin, BMax, CellSize, &GridWidth, &GridHeight);
	const int32 ConstTileSize = Config.tileSize;
	const float tcs = Config.tileSize * Config.cs;
	int32 NewTilesWidth = (GridWidth + ConstTileSize-1) / ConstTileSize;
	int32 NewTilesHeight = (GridHeight + ConstTileSize-1) / ConstTileSize;

	// limit max amount of tiles: config values
	if (NavGenParams)
	{
		if (NewTilesWidth > NavGenParams->MaxTileGridWidth || NewTilesWidth < 0 ||
			NewTilesHeight > NavGenParams->MaxTileGridHeight || NewTilesHeight < 0)
		{
			const int32 OrgTilesWidth = NewTilesWidth;
			const int32 OrgTilesHeight = NewTilesHeight;

			NewTilesWidth = (NewTilesWidth < 0 || NewTilesWidth > NavGenParams->MaxTileGridWidth) ? NavGenParams->MaxTileGridWidth : NewTilesWidth;
			NewTilesHeight = (NewTilesHeight < 0 || NewTilesHeight > NavGenParams->MaxTileGridHeight) ? NavGenParams->MaxTileGridHeight : NewTilesHeight;

			UE_LOG(LogNavigation, Error, TEXT("Navmesh bounds are too large! Limiting requested grid (%d x %d) to: (%d x %d)"),
				OrgTilesWidth, OrgTilesHeight, NewTilesWidth, NewTilesHeight);
		}
	}

	// limit max amount of tiles: 64 bit poly address
	const int32 MaxTileBits = 30;
	const float AvgLayersPerTile = 8.0f;
	const int32 MaxAllowedGridCells = FMath::Trunc((1 << MaxTileBits) / AvgLayersPerTile);
	const int32 NumRequestedCells = NewTilesWidth * NewTilesHeight;
	if (NumRequestedCells < 0 || NumRequestedCells > MaxAllowedGridCells)
	{
		const int32 LimitTiles = FMath::Sqrt(MaxAllowedGridCells);
		const int32 OrgTilesWidth = NewTilesWidth;
		const int32 OrgTilesHeight = NewTilesHeight;

		if (NewTilesHeight < 0 && NewTilesWidth < 0)
		{
			NewTilesWidth = LimitTiles;
			NewTilesHeight = LimitTiles;
		}
		else if (NewTilesHeight > 0 && NewTilesHeight < LimitTiles)
		{
			NewTilesWidth = MaxAllowedGridCells / NewTilesHeight;
		}
		else if (NewTilesWidth > 0 && NewTilesWidth < LimitTiles)
		{
			NewTilesHeight = MaxAllowedGridCells / NewTilesWidth;
		}
		else
		{
			NewTilesWidth = LimitTiles;
			NewTilesHeight = LimitTiles;
		}

		UE_LOG(LogNavigation, Error, TEXT("Navmesh bounds are too large! Limiting requested grid (%d x %d) to: (%d x %d)"),
			OrgTilesWidth, OrgTilesHeight, NewTilesWidth, NewTilesHeight);
	}

	// do this once
	if (bInitialized == false || NewTilesHeight != TilesHeight || NewTilesWidth != TilesWidth) 
	{
		FScopeLock Lock(&TileGenerationLock);
		
		float TileBmin[3];
		float TileBmax[3];

		TilesHeight = NewTilesHeight;
		TilesWidth = NewTilesWidth;

		++Version;
		TileGenerators.Reset();
		TileGenerators.AddUninitialized(TilesWidth * TilesHeight);

		DestNavMesh->ReserveTileSet(TilesWidth, TilesHeight);
		FTileSetItem* TileData = DestNavMesh->GetTileSet();
		for (int32 TileIndex = 0; TileIndex < TilesWidth * TilesHeight; ++TileIndex, ++TileData)
		{
			const int32 X = TileData->X;
			const int32 Y = TileData->Y;

			TileBmin[0] = BMin[0] + X*tcs;
			TileBmin[1] = BMin[1];
			TileBmin[2] = BMin[2] + Y*tcs;

			TileBmax[0] = BMin[0] + (X+1)*tcs;
			TileBmax[1] = BMax[1];
			TileBmax[2] = BMin[2] + (Y+1)*tcs;

			// @todo check if this tile overlaps current navmesh generation boundaries 
			// if not remove it
			FRecastTileGenerator& TileGenerator = TileGenerators[ Y*TilesWidth + X ];
			new(&TileGenerator) FRecastTileGenerator();
			TileGenerator.Init(this, X, Y, TileBmin, TileBmax, InclusionBounds);

			// reinitialize with all data
			new(TileData) FTileSetItem(X, Y, TileGenerator.GetUnrealBB());
		}
	}

	// Max tiles and max polys affect how the tile IDs are calculated.
	// There are (sizeof(dtPolyRef)*8 - DT_MIN_SALT_BITS) bits available for 
	// identifying a tile and a polygon.
	int32 TileBits = MaxTileBits;
	const float MaxLayers = TilesWidth * TilesHeight * AvgLayersPerTile;
	TileBits = FMath::Min(FMath::Trunc(FMath::Log2(FMath::RoundUpToPowerOfTwo(MaxLayers))), MaxTileBits);
	MaxActiveTiles = 1 << TileBits;

	const int32 PolyBits = FMath::Min(30, (int)(sizeof(dtPolyRef)*8 - DT_MIN_SALT_BITS) - TileBits);
	Config.MaxPolysPerTile = 1 << PolyBits;

	/** setup maximum number of active tile generator*/
	const int32 NumberOfWorkerThreads = FTaskGraphInterface::Get().GetNumWorkerThreads();
	const int32 MaxGeneratorsThreads = NumberOfWorkerThreads > 1 ? NumberOfWorkerThreads - 1 : 1;
	ActiveGenerators.AddZeroed(MaxGeneratorsThreads);
	NumActiveTiles = 0;

	// prepare voxel cache if needed
	if (ARecastNavMesh::IsVoxelCacheEnabled())
	{
		VoxelCacheContext.Create(Config.tileSize + Config.borderSize * 2, Config.cs, Config.ch);
	}

	bInitialized = true;
}

bool FRecastNavMeshGenerator::ConstructTiledNavMesh() 
{
	bool bSuccess = false;
	const bool bCreateNewNavMesh = DestNavMesh == NULL || DestNavMesh->GetRecastNavMeshImpl() == NULL
		|| DestNavMesh->GetRecastNavMeshImpl()->GetRecastMesh() == NULL;

	if (bCreateNewNavMesh)
	{
		// create new Detour navmesh instance
		SetDetourMesh(dtAllocNavMesh());

		if ( DetourMesh != NULL )
		{
			dtNavMeshParams TiledMeshParameters;
			FMemory::MemZero(TiledMeshParameters);	
			rcVcopy(TiledMeshParameters.orig, Config.bmin);
			TiledMeshParameters.tileWidth = Config.tileSize * Config.cs;
			TiledMeshParameters.tileHeight = Config.tileSize * Config.cs;
			TiledMeshParameters.maxTiles = MaxActiveTiles;
			TiledMeshParameters.maxPolys = Config.MaxPolysPerTile;

			const dtStatus status = DetourMesh->init(&TiledMeshParameters);

			TransferGeneratedData();

			if ( dtStatusFailed(status) )
			{
				UE_LOG(LogNavigation, Warning, TEXT("ConstructTiledNavMesh: Could not init navmesh.") );
				bSuccess = false;
			}
			else
			{
				bSuccess = true;
			}
		}
		else
		{
			UE_LOG(LogNavigation, Warning, TEXT("ConstructTiledNavMesh: Could not allocate navmesh.") );
			bSuccess = false;
		}
	}
	else
	{
		SetDetourMesh(DestNavMesh->GetRecastNavMeshImpl()->GetRecastMesh(), DO_ForeignData);
		bSuccess = true;
	}
	
	return bSuccess;
}

/** Inclusion geometry should be cached up before calling this. */
void FRecastNavMeshGenerator::SetUpGeneration(float CellSize, float CellHeight, float AgentMinHeight, float AgentMaxHeight, float AgentMaxSlope, float AgentMaxClimb, float AgentRadius)
{
	checkSlow(IsInGameThread() == true);

	const UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(GetWorld());
	if (NavSys == NULL || NavSys->GetWorld() == NULL)
	{
		return;
	}

	Config.Reset();
	Config.cs = CellSize;
	Config.ch = CellHeight;
	Config.walkableSlopeAngle = AgentMaxSlope;
	Config.walkableHeight = (int32)ceilf(AgentMinHeight / CellHeight);
	Config.walkableClimb = (int32)ceilf(AgentMaxClimb / CellHeight);
	const float WalkableRadius = FMath::Ceil(AgentRadius / CellSize);
	Config.walkableRadius = WalkableRadius;
	
	// store original sizes
	Config.AgentHeight = AgentMinHeight;
	Config.AgentMaxClimb = AgentMaxClimb;
	Config.AgentRadius = AgentRadius;

	Config.borderSize = WalkableRadius + 3;
	Config.maxEdgeLen = (int32)(1200.0f / CellSize);
	Config.maxSimplificationError = 1.3f;
	// hardcoded, but can be overridden by RecastNavMesh params later
	Config.minRegionArea = (int32)rcSqr(0);
	Config.mergeRegionArea = (int32)rcSqr(20.f);

	Config.maxVertsPerPoly = (int32)MAX_VERTS_PER_POLY;
	Config.detailSampleDist = 600.0f;
	Config.detailSampleMaxError = 1.0f;
	Config.PolyMaxHeight = (int32)ceilf(AgentMaxHeight / CellHeight);
		
	FBox NavBounds(0);
	
	if (NavSys->ShouldGenerateNavigationEverywhere() == false)
	{
		// Collect bounding geometry
		TArray<const ANavMeshBoundsVolume*> InclusionVolumes;
		InclusionBounds.Empty();

		for( FActorIterator It(NavSys->GetWorld()); It; ++It )
		{
			ANavMeshBoundsVolume const* const V = Cast<ANavMeshBoundsVolume>(*It);
			if (V != NULL)
			{
				InclusionVolumes.Add(V);
			}
		}

		// get aabb in ue3 coords
		for (int32 VolumeIdx = 0; VolumeIdx < InclusionVolumes.Num(); ++VolumeIdx)
		{
			AVolume const* const Vol = InclusionVolumes[VolumeIdx];
			if (Vol != NULL && Vol->BrushComponent.IsValid())
			{
				FBox Bounds = Vol->BrushComponent->Bounds.GetBox();
				if (Bounds.GetSize().IsZero() == true)
				{
					if (Vol->BrushComponent->IsRegistered() == false)
					{
						Vol->BrushComponent->RegisterComponent();
					}
					Vol->BrushComponent->UpdateBounds();
					Bounds = Vol->BrushComponent->Bounds.GetBox();
				}

				if (Bounds.GetSize().IsZero() == false)
				{
					NavBounds += Bounds;
					InclusionBounds.Add(Bounds);
				}
			}
		}
	}

	if (!NavBounds.IsValid)
	{
		// if no bounds give Navigation System a chance to specify bounds
		// will also happen if NavSystem->ShouldGenerateNavigationEverywhere() == true
		NavBounds = NavSys->GetWorldBounds();
	}
	
	// expand bounds a bit to support later inclusion tests
	NavBounds.ExpandBy(CellSize);

	UnrealNavBounds = NavBounds;
	// now move the box to Recast coords
	RCNavBounds = Unreal2RecastBox(NavBounds);
	Config.bmin[0] = RCNavBounds.Min.X;
	Config.bmin[1] = RCNavBounds.Min.Y;
	Config.bmin[2] = RCNavBounds.Min.Z;
	Config.bmax[0] = RCNavBounds.Max.X;
	Config.bmax[1] = RCNavBounds.Max.Y;
	Config.bmax[2] = RCNavBounds.Max.Z;

	// update offset to current one
	if (DestNavMesh.IsValid())
	{
		DestNavMesh->UpdateNavmeshOffset(NavBounds);
	}
}

int32 GetTilesCountHelper(const class dtNavMesh* DetourMesh)
{
	int32 NumTiles = 0;
	if (DetourMesh)
	{
		for (int32 i = 0; i < DetourMesh->getMaxTiles(); i++)
		{
			const dtMeshTile* TileData = DetourMesh->getTile(i);
			if (TileData && TileData->header && TileData->dataSize > 0)
			{
				NumTiles++;
			}
		}
	}

	return NumTiles;
}

void FRecastNavMeshGenerator::SetDetourMesh(class dtNavMesh* NewDetourMesh, FRecastNavMeshGenerator::EDataOwnership OwnsData)
{
	if (NewDetourMesh != DetourMesh && !!bOwnsDetourMesh && DetourMesh != NULL)
	{
		dtFreeNavMesh(DetourMesh);
	}
	bOwnsDetourMesh = NewDetourMesh != NULL && OwnsData == FRecastNavMeshGenerator::DO_OwnsData;
	DetourMesh = NewDetourMesh;

	NumActiveTiles = GetTilesCountHelper(NewDetourMesh);
}

void FRecastNavMeshGenerator::OnAreaAdded(const UClass* AreaClass, int32 AreaID)
{
	if (AdditionalCachedData.IsValid())
	{
		AdditionalCachedData->OnAreaAdded(AreaClass, AreaID);
	}
}

void FRecastNavMeshGenerator::RequestGeneration()
{
	if (ShouldContinueBuilding() == false)
	{
		return;
	}

	const bool bIsInGameThread = IsInGameThread() == true;
	if (AreAnyTilesBeingBuilt() == false && IsBuildingLocked() == false && bIsInGameThread)
	{
		Generate();
	}
	else if (bIsInGameThread == true)
	{
		bBuildFromScratchRequested = true;
	}
	else
	{
		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
			FSimpleDelegateGraphTask::FDelegate::CreateThreadSafeSP(this, &FRecastNavMeshGenerator::RequestGeneration)
			, TEXT("Requesting navmesh rebuild from scratch from RequestGeneration")
			, NULL
			, ENamedThreads::GameThread
		);
	}
}

void FRecastNavMeshGenerator::RequestDirtyTilesRebuild()
{
	if (bRebuildDirtyTilesRequested == true)
	{
		return;
	}

	bRebuildDirtyTilesRequested = true;

		// kick off a task to rebuild influenced tiles			
	FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
		FSimpleDelegateGraphTask::FDelegate::CreateThreadSafeSP(this, &FRecastNavMeshGenerator::RegenerateDirtyTiles)
		, TEXT("NavMesh Tiles Regeneration"), NULL, ENamedThreads::GameThread
	);
}

bool FRecastNavMeshGenerator::Generate()
{
	bool const bGenerated = GenerateTiledNavMesh();

	// we rebuilt, update to latest
	DestNavMesh->UpdateNavVersion();

	return bGenerated;
}

void FRecastNavMeshGenerator::RebuildDirtyAreas(const TArray<FNavigationDirtyArea>& InDirtyAreas)
{
	const FNavigationDirtyArea* DirtyArea = InDirtyAreas.GetTypedData();
	bool bOverlaps = false;
	for (int32 DirtyIndex = 0; DirtyIndex < InDirtyAreas.Num(); ++DirtyIndex, ++DirtyArea)
	{
		if (DirtyArea->Bounds.Intersect(UnrealNavBounds))
		{
			bOverlaps = true;
			break;
		}
	}

	if (bOverlaps)
	{
		FScopeLock Lock(&NavMeshDirtyLock);
		DirtyAreas.Append(InDirtyAreas);

		if (AreAnyTilesBeingBuilt() == false && IsBuildingLocked() == false)
		{
			RequestDirtyTilesRebuild();
		}
	}
}

bool FRecastNavMeshGenerator::GenerateTiledNavMesh() 
{
	// it's possible for someone to request rebuild when there are tiles already rebuilding. 
	// if number of tiles is to be changed then this rebuild needs to be postponed
	if (AreAnyTilesBeingBuilt() == true || IsBuildingLocked() == true)
	{
		bBuildFromScratchRequested = true;

		return false;
	}

	bBuildFromScratchRequested = false;
	bRebuildDirtyTilesRequested = false;

	Init();

	// Gather path colliding geometry
	const double BuildStartTime = FPlatformTime::Seconds();

	if (DetourMesh != NULL)
	{
		// @todo temporarily here to make every subsequent navmesh rebuilds work. Should go away 
		// in next commit
		//TileGenerators.Reset();
		if (DestNavMesh.IsValid())
		{
			// Make owner destroy current navmesh. Again, this is a temporary bit
			DestNavMesh->GetRecastNavMeshImpl()->SetRecastMesh(NULL);
		}
		else
		{
			dtFreeNavMesh(DetourMesh);
		}

		SetDetourMesh(NULL);
	}

	if ( (DetourMesh == NULL && ConstructTiledNavMesh() == false) )
	{
		return false;	
	}

	const float* BMin = Config.bmin;
	const float* BMax = Config.bmax;
	int32 NewGridWidth = 0;
	int32 NewGridHeight = 0;
	rcCalcGridSize(BMin, BMax, Config.cs, &NewGridWidth, &NewGridHeight);
	const int32 ts = Config.tileSize;
	const int32 tw = (NewGridWidth + ts-1) / ts;
	const int32 th = (NewGridHeight + ts-1) / ts;
	if (tw * th <= 0)
	{
		return false;
	}
	else if (TilesWidth != tw || TilesHeight != th || GridWidth != NewGridWidth || GridHeight != NewGridHeight)
	{
		// @todo there's also a low probability number of tiles is the same but other values are different
		// need to detect that (just in case)
		
		TilesWidth = tw;
		TilesHeight = th;
		GridWidth = NewGridWidth;
		GridHeight = NewGridHeight;

		const float tcs = Config.tileSize * Config.cs;

		// Start the build process.
		float TileBmin[3];
		float TileBmax[3];

		TileGenerationLock.Lock();
		// do fresh start
		++Version;
		TileGenerators.Reset();
		TileGenerators.AddUninitialized(th*tw);
		TileGenerationLock.Unlock();

		DestNavMesh->ReserveTileSet(TilesWidth, TilesHeight);
		FTileSetItem* TileData = DestNavMesh->GetTileSet();
		for (int32 TileIndex = 0; TileIndex < TilesWidth * TilesHeight; ++TileIndex, ++TileData)
		{
			const int32 X = TileData->X;
			const int32 Y = TileData->Y;

			TileBmin[0] = BMin[0] + X*tcs;
			TileBmin[1] = BMin[1];
			TileBmin[2] = BMin[2] + Y*tcs;

			TileBmax[0] = BMin[0] + (X+1)*tcs;
			TileBmax[1] = BMax[1];
			TileBmax[2] = BMin[2] + (Y+1)*tcs;

			// @todo check if this tile overlaps current navmesh generation boundaries 
			// if not remove it
			FRecastTileGenerator& TileGenerator = TileGenerators[ Y*TilesWidth + X ];
			new(&TileGenerator) FRecastTileGenerator();
			TileGenerator.Init(this, X, Y, TileBmin, TileBmax, InclusionBounds);

			// reinitialize with all data
			new(TileData) FTileSetItem(X, Y, TileGenerator.GetUnrealBB());
		}
	}

	RebuildAll();

	UE_LOG(LogNavigation, Log, TEXT("RecastNavMeshGenerator: prepare tiles for generation took %.5fs")
		, FPlatformTime::Seconds() - BuildStartTime);

	return true;
}

void FRecastNavMeshGenerator::GenerateTile(const int32 TileId, const uint32 TileGeneratorVersion)
{
	SCOPE_CYCLE_COUNTER(STAT_Navigation_TileBuildAsync);

	if (bAbortAllTileGeneration  == true)
	{
		// the whole generator is shutting down, skip all building
		UE_LOG(LogNavigation, Log, TEXT("FRecastNavMeshGenerator::GenerateTile abondoning tile (%d,%d) rebuild due to whole navmesh generator shutting down."));
		return;
	}

	if (TileGeneratorVersion < Version)
	{
		UE_LOG(LogNavigation, Log, TEXT("FRecastNavMeshGenerator::GenerateTile abondoning tile (%d,%d) rebuild due to version mismatch."));
		return;
	}
	// all needed data we have already cached locally, we just need to know the owner
	// is not dead or about to be destroyed. No functions will be called on it.
	if (DestNavMesh.IsValid(/*bEvenIfPendingKill = */false, /*bThreadsafeTest = */true) == false)
	{
		UE_LOG(LogNavigation, Warning, TEXT("FRecastNavMeshGenerator::GenerateTile failed to trigger actual tile building due to DestNavMesh Not Being Valid. Aborting rebuild for this tile."));
		return;
	}

	if (TileId < TileGenerators.Num())
	{
		FRecastTileGenerator& TileGenerator = TileGenerators[TileId];
		TileGenerator.StartAsyncBuild();

		UE_LOG(LogNavigation, Log, TEXT("%s> Generating Tile %d,%d"),
			TEXT_WEAKOBJ_NAME(DestNavMesh), TileGenerator.GetTileX(), TileGenerator.GetTileY());

		// tile generation can result in empty tile - we want to add it anyway 
		// since it can mean there's no navigable geometry in regenerated tile
		const bool bSuccess = TileGenerator.GenerateTile();

		// cache recast navmesh actor instance if it's valid
		ARecastNavMesh* CachedRecastNavMeshInstance = DestNavMesh.Get();
		bool bAddTileSuccessful = false;

		if (bSuccess && ShouldContinueBuilding() && UnrealNavBounds.IsValid)
		{
			// add newly generated set of layers
			bAddTileSuccessful = AddTile(&TileGenerator, CachedRecastNavMeshInstance);

			if (bAddTileSuccessful)
			{
				// store navmesh data in our UE4 object
				bAddTileSuccessful = TransferGeneratedData();
				TileGenerator.FinishRebuild();
			}
		} 
		
		if (bAddTileSuccessful == false)
		{
			// abort current generation request
			TileGenerator.AbortRebuild();

			// mark dirty area
			MarkAbortedGenerator(&TileGenerator);

			// request rebuild only if owner navmesh _really_ exists
			if (DestNavMesh.IsValid(/*bEvenIfPendingKill = */false, /*bThreadsafeTest = */true))
			{
				RequestDirtyTilesRebuild();
			}		
		}

		UE_LOG(LogNavigation, Log, TEXT("%s> Done Generating Tile %d,%d"),
			TEXT_WEAKOBJ_NAME(DestNavMesh), TileGenerator.GetTileX(), TileGenerator.GetTileY());

		TileGenerator.FinishAsyncBuild();
		UpdateBuilding();
		DestNavMesh->RequestDrawingUpdate();
	}
	else
	{
		UE_LOG(LogNavigation, Warning, TEXT("FRecastNavMeshGenerator::GenerateTile failed to trigger actual tile building due to Tile Index Being Out Off Array Bounds"));
	}
}

bool FRecastNavMeshGenerator::IsBuildingLocked() const 
{ 
	if (bBuildingLocked)
	{
		return true;
	}

#if WITH_NAVIGATION_GENERATOR
	UWorld* World = GetWorld();
	if (World && World->GetNavigationSystem() && World->GetNavigationSystem()->IsNavigationBuildingLocked())
	{
		return true;
	}
#endif // WITH_NAVIGATION_GENERATOR

	return false;
}

void FRecastNavMeshGenerator::OnNavigationBuildingLocked()
{
	bBuildingLocked = true;
}

void FRecastNavMeshGenerator::OnNavigationBuildingUnlocked(bool bForce)
{
	if (bBuildingLocked == true || bForce == true)
	{
		bBuildingLocked = false;
		bBuildFromScratchRequested |= bForce;
		UpdateBuilding();
	}
}

void FRecastNavMeshGenerator::TiggerGeneration()
{
	RequestGeneration();
}

void FRecastNavMeshGenerator::UpdateBuilding()
{
	if (ShouldContinueBuilding() == false)
	{
		return;
	}
	else if (bBuildFromScratchRequested == true)
	{
		bBuildFromScratchRequested = false;

		{
			FScopeLock Lock(&NavMeshDirtyLock);
			DirtyAreas.Reset();
			DirtyGenerators.Empty(MaxActiveGenerators);
		}

		// need to send it to main thread - uses some gamethread-only iterators
		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
			FSimpleDelegateGraphTask::FDelegate::CreateThreadSafeSP(this, &FRecastNavMeshGenerator::TiggerGeneration)
			, TEXT("Requesting navmesh regen from UpdateBuilding")
			, NULL
			, ENamedThreads::GameThread
		);
	}
	else if(!UnrealNavBounds.IsValid)
	{
		return;
	}
	else if(bRebuildDirtyTilesRequested == false && AreAnyTilesBeingBuilt() == false)
	{
		RequestDirtyTilesRebuild();
	}
}

void FRecastNavMeshGenerator::RemoveTileLayers(const int32 TileX, const int32 TileY, TArray<FNavMeshGenerationResult>& AsyncResults)
{
	SCOPE_CYCLE_COUNTER(STAT_Navigation_AddTile);
	const int32 NumLayers = DetourMesh ? DetourMesh->getTileCountAt(TileX, TileY) : 0;
	if (NumLayers <= 0)
	{
		return;
	}

	TArray<dtMeshTile*> Tiles;
	Tiles.AddZeroed(NumLayers);
	DetourMesh->getTilesAt(TileX, TileY, (const dtMeshTile**)Tiles.GetTypedData(), NumLayers);
	
	for (int32 i = 0; i < NumLayers; i++)
	{
		const int32 LayerIndex = Tiles[i]->header->layer;
		const dtTileRef TileRef = DetourMesh->getTileRef(Tiles[i]);

		NumActiveTiles--;
		UE_LOG(LogNavigation, Log, TEXT("%s> Tile (%d,%d:%d), removing TileRef: 0x%X (active:%d)"),
			TEXT_WEAKOBJ_NAME(DestNavMesh), TileX, TileY, LayerIndex, TileRef, NumActiveTiles);

		uint8* RawNavData = NULL;
		DetourMesh->removeTile(TileRef, &RawNavData, NULL);

		FNavMeshGenerationResult ResultInfo;
		ResultInfo.OldRawNavData = RawNavData;
		ResultInfo.OldTileRef = TileRef;
		ResultInfo.TileIndex = DetourMesh->decodePolyIdTile(TileRef);
		AsyncResults.Add(ResultInfo);
	}
}

bool FRecastNavMeshGenerator::AddTile(FRecastTileGenerator* TileGenerator, ARecastNavMesh* CachedRecastNavMeshInstance)
{
	bool bOperationSuccessful = false;

	SCOPE_CYCLE_COUNTER(STAT_Navigation_AddTile);

	SCOPE_CYCLE_COUNTER(STAT_Navigation_BuildTime)
	STAT(double ThisTime = 0);
	{
		SCOPE_SECONDS_COUNTER(ThisTime);
		TArray<FNavMeshTileData> TileLayers;
		TileGenerator->TransferNavigationData(TileLayers);

		TArray<FNavMeshGenerationResult> AsyncResults;
		const int32 TileX = TileGenerator->GetTileX();
		const int32 TileY = TileGenerator->GetTileY();

		{
			FScopeLock Lock(&TileAddingLock);

			// remove all layers if geometry has changed
			if (TileGenerator->IsRebuildingGeometry())
			{
				RemoveTileLayers(TileX, TileY, AsyncResults);
			}

			FTileSetItem* MyTileData = DestNavMesh.IsValid() ? DestNavMesh->GetTileSetItemAt(TileX, TileY) : NULL;
			
			if (MyTileData != NULL)
			{
				bOperationSuccessful = true;

				MyTileData->bHasCompressedGeometry = true;

				bool bHasNavmesh = true;
				for (int32 i = 0; i < TileLayers.Num(); i++)
				{
					const int32 LayerIndex = TileLayers[i].LayerIndex;
					const dtTileRef OldTileRef = DetourMesh->getTileRefAt(TileX, TileY, LayerIndex);

					uint8* OldRawNavData = NULL;
					if (OldTileRef)
					{
						NumActiveTiles--;
						UE_LOG(LogNavigation, Log, TEXT("%s> Tile (%d,%d:%d), removing TileRef: 0x%X (active:%d)"),
							TEXT_WEAKOBJ_NAME(DestNavMesh), TileX, TileY, LayerIndex, OldTileRef, NumActiveTiles);

						// this call will fill OldRawNavData with address of previously added data
						// if OldRawNavData is NULL it means either this tile was empty, or the memory
						// allocated has been owned by navmesh itself (i.e. has been serialized-in along
						// with the level)
						DetourMesh->removeTile(OldTileRef, &OldRawNavData, NULL);
					}

					if (TileLayers[i].IsValid()) 
					{
						bool bRejectNavmesh = false;
						dtTileRef ResultTileRef = 0;

						// let navmesh know it's tile generator who owns the data
						dtStatus status = DetourMesh->addTile(TileLayers[i].GetData(), TileLayers[i].DataSize, NAVMESH_TILE_GENERATOR_OWNS_DATA, 0, &ResultTileRef);

						if (dtStatusFailed(status))
						{
							if (dtStatusDetail(status, DT_OUT_OF_MEMORY))
							{
								UE_LOG(LogNavigation, Error, TEXT("%s> Tile (%d,%d:%d), tile limit reached!! (%d)"),
									TEXT_WEAKOBJ_NAME(DestNavMesh), TileX, TileY, LayerIndex, DetourMesh->getMaxTiles());
							}

							// release data here so that NavData is not stored in %
							bRejectNavmesh = true;
						}
						else
						{
							NumActiveTiles++;
							UE_LOG(LogNavigation, Log, TEXT("%s> Tile (%d,%d:%d), added TileRef: 0x%X (active:%d)"),
								TEXT_WEAKOBJ_NAME(DestNavMesh), TileX, TileY, LayerIndex, ResultTileRef, NumActiveTiles);
						}

						if (bRejectNavmesh)
						{
							// release data here so that NavData is not stored in 
							TileLayers[i].Release();
							bHasNavmesh = false;
						}
					}

					// store information that needs to be processed in game thread 
					FNavMeshGenerationResult ResultInfo;
					ResultInfo.OldRawNavData = OldRawNavData;
					ResultInfo.NewNavData = TileLayers[i];
					ResultInfo.OldTileRef = OldTileRef;
					ResultInfo.TileIndex = DetourMesh->decodePolyIdTile(OldTileRef);
					AsyncResults.Add(ResultInfo);
				}

				MyTileData->bHasNavmesh = bHasNavmesh;
			}
		}

		if (bOperationSuccessful)
		{
			// send off to game thread to not use critical section for modifying AsyncGenerationResultContainer
			FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
				FSimpleDelegateGraphTask::FDelegate::CreateThreadSafeSP(this, &FRecastNavMeshGenerator::StoreAsyncResults, AsyncResults)
				, TEXT("Storing async nav generetion result")
				, NULL
				, ENamedThreads::GameThread
				);
		}
	}
	INC_FLOAT_STAT_BY(STAT_Navigation_CumulativeBuildTime,(float)ThisTime*1000);

	return bOperationSuccessful;
}

void FRecastNavMeshGenerator::StoreAsyncResults(TArray<FNavMeshGenerationResult> AsyncResults)
{
	// needs to be done on GameThread since we're not guarding access to AsyncGenerationResultContainer
	// with any critical section
	ensure(IsInGameThread());

	AsyncGenerationResultContainer.Append(AsyncResults);
}

void FRecastNavMeshGenerator::GetAsyncResultsCopy(TNavStatArray<FNavMeshGenerationResult>& Dest, bool bClearSource)
{
	// needs to be done on GameThread since we're not guarding access to AsyncGenerationResultContainer
	// with any critical section
	ensure(IsInGameThread());

	Dest = AsyncGenerationResultContainer;

	if (bClearSource)
	{
		AsyncGenerationResultContainer.Reset();
	}
}

bool FRecastNavMeshGenerator::HasDirtyTiles() const
{
	return (DirtyGenerators.Num() > 0);
}

void FRecastNavMeshGenerator::RebuildAll()
{
	if (bInitialized == false)
	{
		RequestGeneration();
	}
	else
	{
		// if rebuilding all no point in keeping "old" invalidated areas
		FNavigationDirtyArea BigArea(Recast2UnrealBox(Config.bmin, Config.bmax), ENavigationDirtyFlag::All);
		FScopeLock Lock(&NavMeshDirtyLock);

		DirtyAreas.Empty();
		DirtyAreas.Add(BigArea);
		RequestDirtyTilesRebuild();
	}
}

void FRecastNavMeshGenerator::MarkAbortedGenerator(const FRecastTileGenerator* TileGenerator)
{
	FNavigationDirtyArea DirtyArea(TileGenerator->GetUnrealBB().ExpandBy(-1.0f),
		TileGenerator->HasDirtyGeometry() ? ENavigationDirtyFlag::All : ENavigationDirtyFlag::DynamicModifier);

	FScopeLock Lock(&NavMeshDirtyLock);
	DirtyAreas.Add(DirtyArea);
}

void FRecastNavMeshGenerator::MarkDirtyGenerators()
{
	if (DirtyAreas.Num() == 0)
	{
		return;
	}

	SCOPE_CYCLE_COUNTER(STAT_Navigation_BuildTime)

	// create a copy of dirty areas array
	TNavStatArray<FNavigationDirtyArea> DirtyAreasCopy;
	{
		FScopeLock Lock(&NavMeshDirtyLock);
		DirtyAreasCopy = DirtyAreas;
		DirtyAreas.Reset();
	}

	FScopeLock Lock(&TileGenerationLock);
	const float InvTileCellSize = 1.0f / (Config.tileSize * Config.cs);

	TSet<int32> DirtyIndices;

	// find all tiles that need regeneration:
	const FNavigationDirtyArea* DirtyArea = DirtyAreasCopy.GetTypedData();
	for (int32 i = 0; i < DirtyAreasCopy.Num(); ++i, ++DirtyArea)
	{
		const FBox RCBB = Unreal2RecastBox(GrowBoundingBox(DirtyArea->Bounds));
		const int32 XMin = FMath::Max(FMath::Trunc((RCBB.Min.X - RCNavBounds.Min.X) * InvTileCellSize), 0);
		const int32 YMin = FMath::Max(FMath::Trunc((RCBB.Min.Z - RCNavBounds.Min.Z) * InvTileCellSize), 0);
		const int32 XMax = FMath::Min(FMath::Trunc((RCBB.Max.X - RCNavBounds.Min.X) * InvTileCellSize), TilesWidth-1);
		const int32 YMax = FMath::Min(FMath::Trunc((RCBB.Max.Z - RCNavBounds.Min.Z) * InvTileCellSize), TilesHeight-1);

		for (int32 y = YMin; y <= YMax; ++y)
		{
			for (int32 x = XMin; x <= XMax; ++x)
			{
				// grab that generator and mark it as dirty
				// @todo make thread safe
				const int32 GenIdx = x + (y * TilesWidth);
				TileGenerators[GenIdx].SetDirty(*DirtyArea);
				DirtyIndices.Add(GenIdx);
			}
		}
	}

	// store current dirty state associated with just added areas.
	//
	// Generators that already received octree data and are waiting for free
	// worker thread will clear dirty state after finishing rebuild.
	// This lost information will prevent them from running again, when
	// building queue is finally emptied and StartDirtyGenerators is called.

	FRecastTileDirtyState DirtyState;
	for (TSet<int32>::TIterator It(DirtyIndices); It; ++It)
	{
		const int32 GenIdx = *It;
		TileGenerators[GenIdx].GetDirtyState(DirtyState);
		DirtyGenerators.Add(GenIdx, DirtyState);
	}
}

struct FCompareNavMeshTiles
{
	const FTileSetItem* TileSet;

	FCompareNavMeshTiles(const FTileSetItem* InTileSet) : TileSet(InTileSet) {}

	FORCEINLINE bool operator()(const int32& A, const int32& B) const
	{
		const FTileSetItem* TileA = &TileSet[A];
		const FTileSetItem* TileB = &TileSet[B];

		// lesser goes first
		return TileA->SortOrder < TileB->SortOrder;
	}
};

void FRecastNavMeshGenerator::StartDirtyGenerators()
{
	if (DirtyGenerators.Num() == 0 || GeneratorsQueue.Num() > 0)
	{
		return;
	}

	const UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(GetWorld());
	const FNavigationOctree* NavOctree = NavSys ? NavSys->GetNavOctree() : NULL;
	if (NavOctree == NULL)
	{
		UE_LOG(LogNavigation, Error, TEXT("Failed to rebuild dirty navmesh tiles due to %s being NULL"),
			NavSys == NULL ? TEXT("NavigationSystem") : TEXT("NavOctree"));

		return;
	}

	UE_LOG(LogNavigation, Log, TEXT("%s> StartDirtyGenerators"), TEXT_WEAKOBJ_NAME(DestNavMesh));

	STAT(double ThisTime = 0);
	{
		SCOPE_SECONDS_COUNTER(ThisTime);
		SCOPE_CYCLE_COUNTER(STAT_Navigation_BuildTime)

			const float TileCellSize = (Config.tileSize * Config.cs);
		const float* BMin = Config.bmin;
		const float* BMax = Config.bmax;

		FCompareNavMeshTiles SortFunc(DestNavMesh->GetTileSet());

		FScopeLock Lock(&TileGenerationLock);
		DirtyGenerators.KeySort(SortFunc);

		FRecastTileDirtyState DirtyState;
		for (TMap<int32,FRecastTileDirtyState>::TIterator It(DirtyGenerators); It; ++It)
		{
			const int32 GenIdx = It.Key();
			DirtyState = It.Value();

			FRecastTileGenerator* TileGenerator = &TileGenerators[GenIdx];

			UE_LOG(LogNavigation, Log, TEXT("%s> Tile %d,%d dirty (%s)"), TEXT_WEAKOBJ_NAME(DestNavMesh),
				TileGenerator->GetTileX(), TileGenerator->GetTileY(),
				DirtyState.bRebuildGeometry ? TEXT("geometry") : TEXT("layers"));

			// if tile's regenerating at the moment put it into delayed tile generators container
			if (TileGenerator->IsBeingRebuild() == true)
			{
				UE_LOG(LogNavigation, Log, TEXT("%s> Tile %d,%d is currently being built - postpone"),
					TEXT_WEAKOBJ_NAME(DestNavMesh), TileGenerator->GetTileX(), TileGenerator->GetTileY());

				continue;
			}

			// initialize full rebuild when geometry has changed
			TileGenerator->ClearModifiers();
			TileGenerator->SetDirtyState(DirtyState);

			if (TileGenerator->HasDirtyGeometry())
			{
				const int32 X = TileGenerator->GetTileX();
				const int32 Y = TileGenerator->GetTileY();

				const float TileBMin[] = { BMin[0] + ((X + 0) * TileCellSize), BMin[1], BMin[2] + ((Y + 0) * TileCellSize) };
				const float TileBMax[] = { BMin[0] + ((X + 1) * TileCellSize), BMax[1], BMin[2] + ((Y + 1) * TileCellSize) };

				TileGenerator->ClearGeometry();
				TileGenerator->Init(this, X, Y, TileBMin, TileBMax, InclusionBounds);

				// Init call does some checks whether there's a point in starting generation at all
				if (!TileGenerator->ShouldBeBuilt())
				{
					TileGenerator->AbadonGeneration();
				}
			}

			It.RemoveCurrent();

			// skip if dirty flags were removed
			if (!TileGenerator->HasDirtyGeometry() && !TileGenerator->HasDirtyLayers())
			{
				if (TileGenerator->IsPendingRebuild())
				{
					GeneratorsQueue.RemoveSingle(TileGenerator);
				}

				continue;
			}

			FillGeneratorData(TileGenerator, NavOctree);

#if RECAST_ASYNC_REBUILDING
			// async build: add to queue, up to MaxActiveGenerators at the same time
			if (!TileGenerator->IsPendingRebuild())
			{
				TileGenerator->MarkPendingRebuild();
				GeneratorsQueue.Add(TileGenerator);
			}

			if (GeneratorsQueue.Num() >= MaxActiveGenerators)
			{
				break;
			}
#else
			// sync build: generate immediately
			TileGenerator->InitiateRebuild();
			GenerateTile(TileGenerator->GetId(), TileGenerator->GetVersion());
#endif
		}

#if !RECAST_ASYNC_REBUILDING
		// sync building: clean up static allocations
		FRecastTileGenerator::ClearStaticData();
#endif

		// start processing generator queue
		UpdateTileGenerationWorkers(INDEX_NONE);
	}

	INC_FLOAT_STAT_BY(STAT_Navigation_CumulativeBuildTime,(float)ThisTime*1000);

#if STATS
	UE_LOG(LogNavigation, Log, TEXT("FRecastNavMeshGenerator::StartDirtyGenerators time: %.4fms"), ThisTime*1000);
#endif
}

void FRecastNavMeshGenerator::FillGeneratorData(FRecastTileGenerator* TileGenerator, const FNavigationOctree* NavOctree)
{
	const bool bUseVoxelCache = ARecastNavMesh::IsVoxelCacheEnabled();
	const FNavAgentProperties NavAgentProps = DestNavMesh->NavDataConfig;

	for(FNavigationOctree::TConstElementBoxIterator<FNavigationOctree::DefaultStackAllocator> It(*NavOctree, GrowBoundingBox(TileGenerator->GetUnrealBB()));
		It.HasPendingElements();
		It.Advance())
	{
		const FNavigationOctreeElement& Element = It.GetCurrentElement();
		const bool bShouldUse = Element.ShouldUseGeometry(&DestNavMesh->NavDataConfig);
		if (bShouldUse)
		{
			const bool bExportGeometry = TileGenerator->HasDirtyGeometry() && Element.Data.HasGeometry();
			if (bExportGeometry)
			{
				if (bUseVoxelCache)
				{
					TNavStatArray<rcSpanCache> SpanData;
					rcSpanCache* CachedVoxels = 0;
					int32 NumCachedVoxels = 0;

					DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Rasterization: prepare voxel cache"), Stat_RecastRasterCachePrep, STATGROUP_Navigation);

					if (!TileGenerator->HasVoxelCache(Element.Data.VoxelData, CachedVoxels, NumCachedVoxels))
					{
						// rasterize
						TileGenerator->PrepareVoxelCache(Element.Data.CollisionData, SpanData);
						CachedVoxels = SpanData.GetTypedData();
						NumCachedVoxels = SpanData.Num();

						// encode
						const int32 PrevElementMemory = Element.Data.GetAllocatedSize();
						FNavigationRelevantData* ModData = (FNavigationRelevantData*)&Element.Data;
						TileGenerator->AddVoxelCache(ModData->VoxelData, CachedVoxels, NumCachedVoxels);

						const int32 NewElementMemory = Element.Data.GetAllocatedSize();
						const int32 ElementMemoryDelta = NewElementMemory - PrevElementMemory;
						INC_MEMORY_STAT_BY(STAT_Navigation_CollisionTreeMemory, ElementMemoryDelta);
					}

					TileGenerator->AppendVoxels(CachedVoxels, NumCachedVoxels);
				}
				else
				{
					TileGenerator->AppendGeometry(Element.Data.CollisionData);
				}
			}

			const FCompositeNavModifier ModifierInstance = Element.GetModifierForAgent(&NavAgentProps);
			TileGenerator->AppendModifier(ModifierInstance, bExportGeometry);
		}
	}
}

void FRecastNavMeshGenerator::RegenerateDirtyTiles()
{
	bRebuildDirtyTilesRequested = false;

	if (ShouldContinueBuilding() == false || !UnrealNavBounds.IsValid || DestNavMesh.IsValid() == false)
	{
		return;
	}

	MarkDirtyGenerators();
	StartDirtyGenerators();
}

void FRecastNavMeshGenerator::UpdateTileGenerationWorkers(int32 TileId)
{
#if RECAST_ASYNC_REBUILDING
	bool bRequestRenderingDirty = false;
	FRecastTileGenerator** CurrentGenerator = ActiveGenerators.GetTypedData();
	int32 QueueIndex = 0;

	for (int32 i = 0; i < ActiveGenerators.Num(); ++i, ++CurrentGenerator)
	{
		if (*CurrentGenerator == NULL || (*CurrentGenerator != NULL && (*CurrentGenerator)->GetId() == TileId))
		{
			*CurrentGenerator = NULL;

			while (QueueIndex < GeneratorsQueue.Num())
			{
				FRecastTileGenerator* GeneratorCandidate = GeneratorsQueue[QueueIndex++];
				// generators in GeneratorsQueue should have FRecastTileGenerator::bRebuildPending
				// @todo this is a safety feature, should not be needed once generators queuing is done less dodgy
				if (GeneratorCandidate != NULL && GeneratorCandidate->IsPendingRebuild())
				{
					*CurrentGenerator = GeneratorCandidate;
					(*CurrentGenerator)->TriggerAsyncBuild();
					bRequestRenderingDirty = true;
					break;
				}
			}
		}
	}

	if (QueueIndex > 0)
	{
		GeneratorsQueue.RemoveAt(0, QueueIndex, /*bAllowShrinking=*/false);
	}

	if (bRequestRenderingDirty)
	{
		// let navmesh drawing mark tiles being built
		DestNavMesh->RequestDrawingUpdate();
	}
#else
	DestNavMesh->RequestDrawingUpdate();
#endif // RECAST_ASYNC_REBUILDING

	// prepare next batch of dirty generators in next tick
	if (GeneratorsQueue.Num() == 0 && DirtyGenerators.Num() > 0)
	{
		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
			FSimpleDelegateGraphTask::FDelegate::CreateRaw(this, &FRecastNavMeshGenerator::StartDirtyGenerators), 
			TEXT("Prepare navmesh tile generators"), NULL, ENamedThreads::GameThread);
	}
}

void FRecastNavMeshGenerator::ExportActorGeometry(AActor& Actor, FNavigationRelevantData& Data)
{
	FRecastGeometryExport GeomExport(Data);
	RecastGeometryExport::ExportActor(Actor, &GeomExport);
	RecastGeometryExport::CovertCoordDataToRecast(GeomExport.VertexBuffer);
	RecastGeometryExport::StoreCollisionCache(&GeomExport);
}

void FRecastNavMeshGenerator::ExportComponentGeometry(UActorComponent& Component, FNavigationRelevantData& Data)
{
	FRecastGeometryExport GeomExport(Data);
	RecastGeometryExport::ExportComponent(Component, &GeomExport);
	RecastGeometryExport::CovertCoordDataToRecast(GeomExport.VertexBuffer);
	RecastGeometryExport::StoreCollisionCache(&GeomExport);
}

void FRecastNavMeshGenerator::ExportVertexSoupGeometry(const TArray<FVector>& Verts, FNavigationRelevantData& Data)
{
	FRecastGeometryExport GeomExport(Data);
	RecastGeometryExport::ExportVertexSoup(Verts, GeomExport.VertexBuffer, GeomExport.IndexBuffer, GeomExport.Data->Bounds);
	RecastGeometryExport::StoreCollisionCache(&GeomExport);
}

void FRecastNavMeshGenerator::ExportRigidBodyGeometry(UBodySetup& BodySetup, TNavStatArray<FVector>& OutVertexBuffer, TNavStatArray<int32>& OutIndexBuffer, const FTransform& LocalToWorld)
{
	TNavStatArray<float> VertCoords;
	FBox TempBounds;

	RecastGeometryExport::ExportRigidBodySetup(BodySetup, VertCoords, OutIndexBuffer, TempBounds, LocalToWorld);

	OutVertexBuffer.Reserve(OutVertexBuffer.Num() + (VertCoords.Num() / 3));
	for (int32 i = 0; i < VertCoords.Num(); i += 3)
	{
		OutVertexBuffer.Add(FVector(VertCoords[i + 0], VertCoords[i + 1], VertCoords[i + 2]));
	}
}

void FRecastNavMeshGenerator::ExportRigidBodyGeometry(UBodySetup& BodySetup, TNavStatArray<FVector>& OutTriMeshVertexBuffer, TNavStatArray<int32>& OutTriMeshIndexBuffer
	, TNavStatArray<FVector>& OutConvexVertexBuffer, TNavStatArray<int32>& OutConvexIndexBuffer, TNavStatArray<int32>& OutShapeBuffer
	, const FTransform& LocalToWorld)
{
	BodySetup.CreatePhysicsMeshes();

	TNavStatArray<float> VertCoords;
	FBox TempBounds;

	VertCoords.Reset();
	RecastGeometryExport::ExportRigidBodyTriMesh(BodySetup, VertCoords, OutTriMeshIndexBuffer, TempBounds, LocalToWorld);

	OutTriMeshVertexBuffer.Reserve(OutTriMeshVertexBuffer.Num() + (VertCoords.Num() / 3));
	for (int32 i = 0; i < VertCoords.Num(); i += 3)
	{
		OutTriMeshVertexBuffer.Add(FVector(VertCoords[i + 0], VertCoords[i + 1], VertCoords[i + 2]));
	}

	VertCoords.Reset();
	RecastGeometryExport::ExportRigidBodyConvexElements(BodySetup, VertCoords, OutConvexIndexBuffer, OutShapeBuffer, TempBounds, LocalToWorld);
	RecastGeometryExport::ExportRigidBodyBoxElements(BodySetup, VertCoords, OutConvexIndexBuffer, OutShapeBuffer, TempBounds, LocalToWorld);
	RecastGeometryExport::ExportRigidBodySphylElements(BodySetup, VertCoords, OutConvexIndexBuffer, OutShapeBuffer, TempBounds, LocalToWorld);
	RecastGeometryExport::ExportRigidBodySphereElements(BodySetup, VertCoords, OutConvexIndexBuffer, OutShapeBuffer, TempBounds, LocalToWorld);
	
	OutConvexVertexBuffer.Reserve(OutConvexVertexBuffer.Num() + (VertCoords.Num() / 3));
	for (int32 i = 0; i < VertCoords.Num(); i += 3)
	{
		OutConvexVertexBuffer.Add(FVector(VertCoords[i + 0], VertCoords[i + 1], VertCoords[i + 2]));
	}
}

bool FRecastNavMeshGenerator::TransferGeneratedData()
{
	if (DestNavMesh.IsValid() && DestNavMesh->GetRecastNavMeshImpl() != NULL)
	{
		// make sure FPImplRecastNavMesh knows data is not his - generator own that data at the moment
		DestNavMesh->GetRecastNavMeshImpl()->SetRecastMesh(DetourMesh, /*bOwnData=*/false);

		// this should be done synchronously
		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
			FSimpleDelegateGraphTask::FDelegate::CreateUObject(DestNavMesh.Get(), &ARecastNavMesh::UpdateNavMeshDrawing)
			, TEXT("Requesting navmesh redraw")
			, NULL
			, ENamedThreads::GameThread
			);

		return true;
	}
	else
	{
		return false;
	}
}

void FRecastNavMeshGenerator::RefreshParentReference()
{
#if WITH_EDITOR
	FScopeLock Lock(&GatheringDataLock);

	// we need to make sure there's any navmesh at all to rebuild 
	if (DetourMesh == NULL)
	{
		// first try to get it from owner-ARecastNavMesh
		FPImplRecastNavMesh* RecastMeshWrapper = DestNavMesh->GetRecastNavMeshImpl();
		if (RecastMeshWrapper != NULL)
		{
			SetDetourMesh(RecastMeshWrapper->GetRecastMesh(), FRecastNavMeshGenerator::DO_ForeignData);
		}

		// if it's still empty then create one
		if (DetourMesh == NULL && ConstructTiledNavMesh() == false)
		{
			UE_LOG(LogNavigation, Error, TEXT("Failed to find and generate Recast navmesh to rebuild"));
		}
	}
#endif	//#if WITH_EDITOR
}

void FRecastNavMeshGenerator::OnNavigationBoundsUpdated(AVolume* Volume)
{
	if (Cast<ANavMeshBoundsVolume>(Volume) != NULL && Volume->BrushComponent.IsValid())
	{
		RequestGeneration();
	}
}

void FRecastNavMeshGenerator::OnNavigationDataDestroyed(class ANavigationData* NavData)
{
	if (NavData == DestNavMesh.Get())
	{
		SetDetourMesh(NULL);
		DestNavMesh = NULL;
		GeneratorsQueue.Reset();
		ActiveGenerators.Reset();
		bAbortAllTileGeneration = true;
	}
}

bool FRecastNavMeshGenerator::IsBuildInProgress(bool bCheckDirtyToo) const
{
	bool bRetValue = AreAnyTilesBeingBuilt(bCheckDirtyToo);
	if (bCheckDirtyToo)
	{
		bRetValue = bRetValue || DirtyAreas.Num() > 0 || DirtyGenerators.Num() > 0;
	}

	return bRetValue;
}

bool FRecastNavMeshGenerator::AreAnyTilesBeingBuilt(bool bCheckDirtyToo) const
{
#if RECAST_ASYNC_REBUILDING	
	bool bRet = false;
	const FRecastTileGenerator* TileGenerator = TileGenerators.GetTypedData();

	for (int32 i = 0; i < TileGenerators.Num(); ++i, ++TileGenerator)
	{
		if ((bCheckDirtyToo && TileGenerator->IsDirty()) || TileGenerator->IsBeingRebuild())
		{
			bRet = true;
			break;
		}
	}

	return bRet;
#else
	// no need to check if anything is building since all tiles are build "instantly" 
	// (blocking the one-and-only thread)
	return false;
#endif // RECAST_ASYNC_REBUILDING
}

bool FRecastNavMeshGenerator::IsAsyncBuildInProgress() const
{
#if RECAST_ASYNC_REBUILDING	
	const FRecastTileGenerator* TileGenerator = TileGenerators.GetTypedData();
	for (int32 i = 0; i < TileGenerators.Num(); ++i, ++TileGenerator)
	{
		if (TileGenerator->IsAsyncBuildInProgress())
		{
			UE_LOG(LogNavigation, Log, TEXT("Waiting for async build of tile (%d,%d)"), TileGenerator->GetTileX(), TileGenerator->GetTileY());
			return true;
		}
	}
#endif

	return false;
}

bool FRecastNavMeshGenerator::IsTileFresh(int32 X, int32 Y, float FreshnessTime) const
{
	const int32 TileIndex = GetTileIdAt(X, Y);
	if (TileGenerators.IsValidIndex(TileIndex) == false)
	{
		return false;
	}
	const FRecastTileGenerator* TileGenerator = &TileGenerators[TileIndex];

	return TileGenerator != NULL && (TileGenerator->IsDirty() || TileGenerator->IsBeingRebuild() 
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		|| (FPlatformTime::Seconds() - TileGenerator->GetLastBuildTimeStamp() < FreshnessTime)
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		);
}

void FRecastNavMeshGenerator::OnWorldInitDone(bool bAllowedToRebuild)
{
	Init();

	if ((DetourMesh != NULL || ConstructTiledNavMesh() == true) && bAllowedToRebuild)
	{
		RebuildAll();
	}
}

uint32 FRecastNavMeshGenerator::LogMemUsed() const 
{
	UE_LOG(LogNavigation, Display, TEXT("    FRecastNavMeshGenerator: self %d"), sizeof(FRecastNavMeshGenerator));
	
	uint32 GeneratorsMem = 0;
	for (int32 i = 0; i < TileGenerators.Num(); ++i)
	{
		GeneratorsMem += TileGenerators[i].GetUsedMemCount();
	}

	GeneratorsMem += TileGenerators.GetAllocatedSize();
	UE_LOG(LogNavigation, Display, TEXT("    FRecastNavMeshGenerator: Total Generator\'s size %u, count %d"), GeneratorsMem, TileGenerators.Num());

	return GeneratorsMem + sizeof(FRecastNavMeshGenerator);
}

#if WITH_EDITOR
void FRecastNavMeshGenerator::ExportNavigationData(const FString& FileName) const
{
	const UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(GetWorld());
	const FNavigationOctree* NavOctree = NavSys ? NavSys->GetNavOctree() : NULL;
	if (NavOctree == NULL)
	{
		UE_LOG(LogNavigation, Error, TEXT("Failed to export navigation data due to %s being NULL"), NavSys == NULL ? TEXT("NavigationSystem") : TEXT("NavOctree"));
		return;
	}

	FString CurrentTimeStr = FDateTime::Now().ToString();
	for (int32 Index = 0; Index < NavSys->NavDataSet.Num(); ++Index)
	{
		// feed data from octtree and mark for rebuild				
		TNavStatArray<float> CoordBuffer;
		TNavStatArray<int32> IndexBuffer;
		const ARecastNavMesh* NavData = Cast<const ARecastNavMesh>(NavSys->NavDataSet[Index]);
		if (NavData)
		{
			struct FAreaExportData
			{
				FConvexNavAreaData Convex;
				uint8 AreaId;
			};
			TArray<FAreaExportData> AreaExport;

			for(FNavigationOctree::TConstElementBoxIterator<FNavigationOctree::DefaultStackAllocator> It(*NavOctree, UnrealNavBounds);
				It.HasPendingElements();
				It.Advance())
			{
				const FNavigationOctreeElement& Element = It.GetCurrentElement();
				const bool bExportGeometry = Element.Data.HasGeometry() && Element.ShouldUseGeometry(&DestNavMesh->NavDataConfig);

				if (bExportGeometry && Element.Data.CollisionData.Num())
				{
					FRecastGeometryCache CachedGeometry(Element.Data.CollisionData.GetTypedData());
					IndexBuffer.Reserve( IndexBuffer.Num() + (CachedGeometry.Header.NumFaces * 3 ));
					CoordBuffer.Reserve( CoordBuffer.Num() + (CachedGeometry.Header.NumVerts * 3 ));
					for (int32 i = 0; i < CachedGeometry.Header.NumFaces * 3; i++)
					{
						IndexBuffer.Add(CachedGeometry.Indices[i] + CoordBuffer.Num() / 3);
					}
					for (int32 i = 0; i < CachedGeometry.Header.NumVerts * 3; i++)
					{
						CoordBuffer.Add(CachedGeometry.Verts[i]);
					}
				}
				else
				{
					const TArray<FAreaNavModifier>& AreaMods = Element.Data.Modifiers.GetAreas();
					for (int32 i = 0; i < AreaMods.Num(); i++)
					{
						FAreaExportData ExportInfo;
						ExportInfo.AreaId = NavData->GetAreaID(AreaMods[i].GetAreaClass());

						if (AreaMods[i].GetShapeType() == ENavigationShapeType::Convex)
						{
							AreaMods[i].GetConvex(ExportInfo.Convex);

							TArray<FVector> ConvexVerts;
							GrowConvexHull(NavData->AgentRadius, ExportInfo.Convex.Points, ConvexVerts);
							ExportInfo.Convex.MinZ -= NavData->CellHeight;
							ExportInfo.Convex.MaxZ += NavData->CellHeight;
							ExportInfo.Convex.Points = ConvexVerts;

							AreaExport.Add(ExportInfo);
						}
					}
				}
			}

			UWorld* NavigationWorld = GetWorld();
			for (int32 LevelIndex = 0; LevelIndex < NavigationWorld->GetNumLevels(); ++LevelIndex) 
			{
				const ULevel* const Level =  NavigationWorld->GetLevel(LevelIndex);
				if (Level == NULL)
				{
					continue;
				}

				const TArray<FVector>* LevelGeom = Level->GetStaticNavigableGeometry();
				if (LevelGeom != NULL && LevelGeom->Num() > 0)
				{
					TNavStatArray<FVector> Verts;
					TNavStatArray<int32> Faces;
					// For every ULevel in World take its pre-generated static geometry vertex soup
					RecastGeometryExport::TransformVertexSoupToRecast(*LevelGeom, Verts, Faces);

					IndexBuffer.Reserve( IndexBuffer.Num() + Faces.Num() );
					CoordBuffer.Reserve( CoordBuffer.Num() + Verts.Num() * 3);
					for (int32 i = 0; i < Faces.Num(); i++)
					{
						IndexBuffer.Add(Faces[i] + CoordBuffer.Num() / 3);
					}
					for (int32 i = 0; i < Verts.Num(); i++)
					{
						CoordBuffer.Add(Verts[i].X);
						CoordBuffer.Add(Verts[i].Y);
						CoordBuffer.Add(Verts[i].Z);
					}
				}
			}

			FString AreaExportStr;
			for (int32 i = 0; i < AreaExport.Num(); i++)
			{
				const FAreaExportData& ExportInfo = AreaExport[i];
				AreaExportStr += FString::Printf(TEXT("\nAE %d %d %f %f\n"),
					ExportInfo.AreaId, ExportInfo.Convex.Points.Num(), ExportInfo.Convex.MinZ, ExportInfo.Convex.MaxZ);

				for (int32 iv = 0; iv < ExportInfo.Convex.Points.Num(); iv++)
				{
					FVector Pt = Unreal2RecastPoint(ExportInfo.Convex.Points[iv]);
					AreaExportStr += FString::Printf(TEXT("Av %f %f %f\n"), Pt.X, Pt.Y, Pt.Z);
				}
			}

			FString AdditionalData;
			if (AreaExport.Num())
			{
				AdditionalData += "# Area export\n";
				AdditionalData += AreaExportStr;
				AdditionalData += "\n";
			}

			AdditionalData += "# RecastDemo specific data\n";

	#if 0
			// use this bounds to have accurate navigation data bounds
			const FVector Center = Unreal2RecastPoint(NavData->GetBounds().GetCenter());
			FVector Extent = FVector(NavData->GetBounds().GetExtent());
			Extent = FVector(Extent.X, Extent.Z, Extent.Y);
	#else
			// this bounds match navigation bounds from level
			const FVector Center = RCNavBounds.GetCenter();
			const FVector Extent = RCNavBounds.GetExtent();
	#endif
			const FBox Box = FBox::BuildAABB(Center, Extent);
			AdditionalData += FString::Printf(
				TEXT("rd_bbox %7.7f %7.7f %7.7f %7.7f %7.7f %7.7f\n"), 
				Box.Min.X, Box.Min.Y, Box.Min.Z, 
				Box.Max.X, Box.Max.Y, Box.Max.Z
			);

#if WITH_NAVIGATION_GENERATOR
			const FRecastNavMeshGenerator* CurrentGen = static_cast<const FRecastNavMeshGenerator*>(NavData->GetGenerator());
			check(CurrentGen);
			AdditionalData += FString::Printf(TEXT("# AgentHeight\n"));
			AdditionalData += FString::Printf(TEXT("rd_agh %5.5f\n"), CurrentGen->Config.AgentHeight);
			AdditionalData += FString::Printf(TEXT("# AgentRadius\n"));
			AdditionalData += FString::Printf(TEXT("rd_agr %5.5f\n"), CurrentGen->Config.AgentRadius);

			AdditionalData += FString::Printf(TEXT("# Cell Size\n"));
			AdditionalData += FString::Printf(TEXT("rd_cs %5.5f\n"), CurrentGen->Config.cs);
			AdditionalData += FString::Printf(TEXT("# Cell Height\n"));
			AdditionalData += FString::Printf(TEXT("rd_ch %5.5f\n"), CurrentGen->Config.ch);

			AdditionalData += FString::Printf(TEXT("# Agent max climb\n"));
			AdditionalData += FString::Printf(TEXT("rd_amc %d\n"), (int)CurrentGen->Config.AgentMaxClimb);
			AdditionalData += FString::Printf(TEXT("# Agent max slope\n"));
			AdditionalData += FString::Printf(TEXT("rd_ams %5.5f\n"), CurrentGen->Config.walkableSlopeAngle);

			AdditionalData += FString::Printf(TEXT("# Region min size\n"));
			AdditionalData += FString::Printf(TEXT("rd_rmis %d\n"), (uint32)FMath::Sqrt(CurrentGen->Config.minRegionArea));
			AdditionalData += FString::Printf(TEXT("# Region merge size\n"));
			AdditionalData += FString::Printf(TEXT("rd_rmas %d\n"), (uint32)FMath::Sqrt(CurrentGen->Config.mergeRegionArea));

			AdditionalData += FString::Printf(TEXT("# Max edge len\n"));
			AdditionalData += FString::Printf(TEXT("rd_mel %d\n"), CurrentGen->Config.maxEdgeLen);

			AdditionalData += FString::Printf(TEXT("# Perform Voxel Filtering\n"));
			AdditionalData += FString::Printf(TEXT("rd_pvf %d\n"), CurrentGen->Config.bPerformVoxelFiltering);
			AdditionalData += FString::Printf(TEXT("# Generate Detailed Mesh\n"));
			AdditionalData += FString::Printf(TEXT("rd_gdm %d\n"), CurrentGen->Config.bGenerateDetailedMesh);
			AdditionalData += FString::Printf(TEXT("# MaxPolysPerTile\n"));
			AdditionalData += FString::Printf(TEXT("rd_mppt %d\n"), CurrentGen->Config.MaxPolysPerTile);
			AdditionalData += FString::Printf(TEXT("# maxVertsPerPoly\n"));
			AdditionalData += FString::Printf(TEXT("rd_mvpp %d\n"), CurrentGen->Config.maxVertsPerPoly);
			AdditionalData += FString::Printf(TEXT("# Tile size\n"));
			AdditionalData += FString::Printf(TEXT("rd_ts %d\n"), CurrentGen->Config.tileSize);

			AdditionalData += FString::Printf(TEXT("\n"));
#endif // WITH_NAVIGATION_GENERATOR
			const FString FilePathName = FileName + FString::Printf(TEXT("_NavDataSet%d_%s.obj"), Index, *CurrentTimeStr) ;
			ExportGeomToOBJFile(FilePathName, CoordBuffer, IndexBuffer, AdditionalData);
		}
	}
}
#endif

static class FNavigationGeomExec : private FSelfRegisteringExec
{
public:
	/** Console commands, see embeded usage statement **/
	virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar ) OVERRIDE
	{
#if ALLOW_DEBUG_FILES && WITH_EDITOR
		bool bCorrectCmd = FParse::Command(&Cmd, TEXT("ExportNavigation"));
		if (bCorrectCmd && !InWorld)
		{
			UE_LOG(LogNavigation, Error, TEXT("Failed to export navigation data due to missing UWorld"));
		}
		else if (InWorld && bCorrectCmd)
		{
			if (InWorld->GetNavigationSystem())
			{
				if (const ANavigationData* NavData = InWorld->GetNavigationSystem()->GetMainNavData())
				{
#if WITH_NAVIGATION_GENERATOR
					if (const FNavDataGenerator* Generator = NavData->GetGenerator())
					{
						const FString Name = NavData->GetName();
						Generator->ExportNavigationData( FString::Printf( TEXT("%s/%s"), *FPaths::GameSavedDir(), *Name ));
						return true;
					}
					else
#endif // WITH_NAVIGATION_GENERATOR
					{
						UE_LOG(LogNavigation, Error, TEXT("Failed to export navigation data due to missing generator"));
					}
				}
				else
				{
					UE_LOG(LogNavigation, Error, TEXT("Failed to export navigation data due to navigation data"));
				}
			}
			else
			{
				UE_LOG(LogNavigation, Error, TEXT("Failed to export navigation data due to missing navigation system"));
			}
		}
#endif // ALLOW_DEBUG_FILES && WITH_EDITOR
		return false;
	}
} NavigationGeomExec;

#endif // WITH_RECAST


