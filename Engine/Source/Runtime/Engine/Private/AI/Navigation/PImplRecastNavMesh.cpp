// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#if WITH_RECAST

// recast includes
#include "PImplRecastNavMesh.h"
#include "RecastHelpers.h"

// recast includes
#include "DetourNavMeshQuery.h"
#include "DetourNavMesh.h"
#include "DetourNode.h"
#include "DetourCommon.h"
#include "RecastAlloc.h"
#if WITH_NAVIGATION_GENERATOR
	#include "RecastNavMeshGenerator.h"
#endif // WITH_NAVIGATION_GENERATOR

//----------------------------------------------------------------------//
// bunch of compile-time checks to assure types used by Recast and our
// mid-layer are the same size
//----------------------------------------------------------------------//
checkAtCompileTime(sizeof(NavNodeRef) == sizeof(dtPolyRef), NavNodeRef_and_dtPolyRef_should_be_same_size);
checkAtCompileTime(RECAST_MAX_AREAS <= DT_MAX_AREAS, Number_of_allowed_areas_cannot_exceed_DT_MAX_AREAS);
checkAtCompileTime(RECAST_STRAIGHTPATH_OFFMESH_CONNECTION == DT_STRAIGHTPATH_OFFMESH_CONNECTION, Path_flags_values_differ);
// @todo ps4 compile issue: FLT_MAX constexpr issue
#if !PLATFORM_PS4
checkAtCompileTime(RECAST_UNWALKABLE_POLY_COST == DT_UNWALKABLE_POLY_COST, Unwalkable_poly_cost_differ);
#endif

/// Helper for accessing navigation query from different threads
#define INITIALIZE_NAVQUERY(NavQueryVariable, NumNodes)	dtNavMeshQuery NavQueryVariable##Private;	\
														dtNavMeshQuery& NavQueryVariable = IsInGameThread() ? SharedNavQuery : NavQueryVariable##Private; \
														NavQueryVariable.init(DetourNavMesh, NumNodes);

static void* DetourMalloc(int Size, dtAllocHint)
{
	void* Result = FMemory::Malloc(uint32(Size));
#if STATS
	const uint32 ActualSize = FMemory::GetAllocSize(Result);
	INC_DWORD_STAT_BY( STAT_NavigationMemory, ActualSize );
#endif // STATS
	return Result;
}

static void* RecastMalloc(int Size, rcAllocHint)
{
	void* Result = FMemory::Malloc(uint32(Size));
#if STATS
	const uint32 ActualSize = FMemory::GetAllocSize(Result);
	INC_DWORD_STAT_BY( STAT_NavigationMemory, ActualSize );
#endif // STATS
	return Result;
}

static void RecastFree( void* Original )
{
#if STATS
	const uint32 Size = FMemory::GetAllocSize(Original);
	DEC_DWORD_STAT_BY( STAT_NavigationMemory, Size );	
#endif // STATS
	FMemory::Free(Original);
}

struct FRecastInitialSetup
{
	FRecastInitialSetup()
	{
		dtAllocSetCustom(DetourMalloc, RecastFree);
		rcAllocSetCustom(RecastMalloc, RecastFree);
	}
};
static FRecastInitialSetup RecastSetup;

/****************************
 * helpers
 ****************************/

static void Unr2RecastVector(FVector const& V, float* R)
{
	// @todo: speed this up with axis swaps instead of a full transform?
	FVector const RecastV = Unreal2RecastPoint(V);
	R[0] = RecastV.X;
	R[1] = RecastV.Y;
	R[2] = RecastV.Z;
}

static void Unr2RecastSizeVector(FVector const& V, float* R)
{
	// @todo: speed this up with axis swaps instead of a full transform?
	FVector const RecastVAbs = Unreal2RecastPoint(V).GetAbs();
	R[0] = RecastVAbs.X;
	R[1] = RecastVAbs.Y;
	R[2] = RecastVAbs.Z;
}

static FVector Recast2UnrVector(float const* R)
{
	return Recast2UnrealPoint(R);
}

ENavigationQueryResult::Type DTStatusToNavQueryResult(dtStatus Status)
{
	// @todo look at possible dtStatus values (in DetourStatus.h), there's more data that can be retrieved from it

	// Partial paths are treated by Recast as Success while we threat as fail
	return dtStatusSucceed(Status) ? (dtStatusDetail(Status, DT_PARTIAL_RESULT) ? ENavigationQueryResult::Fail : ENavigationQueryResult::Success)
		: (dtStatusDetail(Status, DT_INVALID_PARAM) ? ENavigationQueryResult::Error : ENavigationQueryResult::Fail);
}

//----------------------------------------------------------------------//
// FRecastQueryFilter();
//----------------------------------------------------------------------//
void FRecastQueryFilter::Reset()
{
	dtQueryFilter* Filter = static_cast<dtQueryFilter*>(this);
	Filter = new(Filter) dtQueryFilter();
}

void FRecastQueryFilter::SetAreaCost(uint8 AreaType, float Cost)
{
	setAreaCost(AreaType, Cost);
}

void FRecastQueryFilter::SetFixedAreaEnteringCost(uint8 AreaType, float Cost) 
{
#if WITH_FIXED_AREA_ENTERING_COST
	setAreaFixedCost(AreaType, Cost);
#endif // WITH_FIXED_AREA_ENTERING_COST
}

void FRecastQueryFilter::SetAllAreaCosts(const float* CostArray, const int32 Count) 
{
	// @todo could get away with memcopying to m_areaCost, but it's private and would require a little hack
	// need to consider if it's wort a try (not sure there'll be any perf gain)
	if (Count > RECAST_MAX_AREAS)
	{
		UE_LOG(LogNavigation, Warning, TEXT("FRecastQueryFilter: Trying to set cost to more areas than allowed! Discarding redundant values."));
	}

	const int32 ElementsCount = FPlatformMath::Min(Count, RECAST_MAX_AREAS);
	for (int32 i = 0; i < ElementsCount; ++i)
	{
		setAreaCost(i, CostArray[i]);
	}
}

void FRecastQueryFilter::GetAllAreaCosts(float* CostArray, float* FixedCostArray, const int32 Count) const
{
	const float* DetourCosts = getAllAreaCosts();
	const float* DetourFixedCosts = getAllFixedAreaCosts();
	
	FMemory::Memcpy(CostArray, DetourCosts, sizeof(float) * FMath::Min(Count, RECAST_MAX_AREAS));
	FMemory::Memcpy(FixedCostArray, DetourFixedCosts, sizeof(float) * FMath::Min(Count, RECAST_MAX_AREAS));
}

void FRecastQueryFilter::SetBacktrackingEnabled(const bool bBacktracking)
{
	setIsBacktracking(bBacktracking);
}

bool FRecastQueryFilter::IsBacktrackingEnabled() const
{
	return getIsBacktracking();
}

bool FRecastQueryFilter::IsEqual(const INavigationQueryFilterInterface* Other) const
{
	// @NOTE: not type safe, should be changed when another filter type is introduced
	return FMemory::Memcmp(this, Other, sizeof(this)) == 0;
}

//----------------------------------------------------------------------//
// FPImplRecastNavMesh
//----------------------------------------------------------------------//

FPImplRecastNavMesh::FPImplRecastNavMesh(ARecastNavMesh* Owner)
	: NavMeshOwner(Owner)
	, bOwnsNavMeshData(false)
	, DetourNavMesh(NULL)
{
	check(Owner && "Owner must never be NULL");

	INC_DWORD_STAT_BY( STAT_NavigationMemory
		, Owner->HasAnyFlags(RF_ClassDefaultObject) == false ? sizeof(*this) : 0 );
};

FPImplRecastNavMesh::~FPImplRecastNavMesh()
{
	// release navmesh only if we own it
	if (bOwnsNavMeshData && DetourNavMesh != NULL)
	{
		dtFreeNavMesh(DetourNavMesh);
	}
	DetourNavMesh = NULL;

	DEC_DWORD_STAT_BY( STAT_NavigationMemory, sizeof(*this) );
};

/**
 * Serialization.
 * @param Ar - The archive with which to serialize.
 * @returns true if serialization was successful.
 */
void FPImplRecastNavMesh::Serialize( FArchive& Ar )
{
	//@todo: How to handle loading nav meshes saved w/ recast when recast isn't present????

	if (!Ar.IsLoading() && DetourNavMesh == NULL)
	{
		// nothing to write
		return;
	}
	
	// All we really need to do is read/write the data blob for each tile

	if (Ar.IsLoading())
	{
		// allocate the navmesh object
		DetourNavMesh = dtAllocNavMesh();
		if (DetourNavMesh == NULL)
		{
			UE_VLOG(NavMeshOwner, LogNavigation, Error, TEXT("Failed to allocate Recast navmesh"));
		}
		else
		{
			bOwnsNavMeshData = true;
		}
	}

	int32 NumTiles = 0;

	if (Ar.IsSaving())
	{
		dtNavMesh const* ConstNavMesh = DetourNavMesh;
		for (int i = 0; i < ConstNavMesh->getMaxTiles(); ++i)
		{
			const dtMeshTile* Tile = ConstNavMesh->getTile(i);
			if (Tile != NULL && Tile->header != NULL && Tile->dataSize > 0)
			{
				++NumTiles;
			}
		}
	}

	Ar << NumTiles;

	dtNavMeshParams Params = *DetourNavMesh->getParams();
	Ar << Params.orig[0] << Params.orig[1] << Params.orig[2];
	Ar << Params.tileWidth;				///< The width of each tile. (Along the x-axis.)
	Ar << Params.tileHeight;			///< The height of each tile. (Along the z-axis.)
	Ar << Params.maxTiles;				///< The maximum number of tiles the navigation mesh can contain.
	Ar << Params.maxPolys;

	if (Ar.IsLoading())
	{
		// at this point we can tell whether navmesh being loaded is in line
		// ARecastNavMesh's params. If not, just skip it.
		// assumes tiles are rectangular
		const float ActorsTileSize = float(int32(NavMeshOwner->TileSizeUU / NavMeshOwner->CellSize) * NavMeshOwner->CellSize);

		if (ActorsTileSize != Params.tileWidth)
		{
			// just move archive position
			dtFreeNavMesh(DetourNavMesh);
			DetourNavMesh = NULL;

			for (int i = 0; i < NumTiles; ++i)
			{
				dtTileRef TileRef = MAX_uint64;
				int32 TileDataSize = 0;
				Ar << TileRef << TileDataSize;

				if (TileRef == MAX_uint64 || TileDataSize == 0)
				{
					continue;
				}

				unsigned char* TileData = NULL;
				TileDataSize = 0;
				SerializeRecastMeshTile(Ar, TileData, TileDataSize);
				if (TileData != NULL)
				{
					dtMeshHeader* const TileHeader = (dtMeshHeader*)TileData;
					dtFree(TileHeader);
				}
			}
		}
		else
		{
			// regular loading
			dtStatus Status = DetourNavMesh->init(&Params);
			if (dtStatusFailed(Status))
			{
				UE_VLOG(NavMeshOwner, LogNavigation, Error, TEXT("Failed to initialize NavMesh"));
			}

			for (int i = 0; i < NumTiles; ++i)
			{
				dtTileRef TileRef = MAX_uint64;
				int32 TileDataSize = 0;
				Ar << TileRef << TileDataSize;

				if (TileRef == MAX_uint64 || TileDataSize == 0)
				{
					continue;
				}
				
				unsigned char* TileData = NULL;
				TileDataSize = 0;
				SerializeRecastMeshTile(Ar, TileData, TileDataSize);

				if (TileData != NULL)
				{
					dtMeshHeader* const TileHeader = (dtMeshHeader*)TileData;
					DetourNavMesh->addTile(TileData, TileDataSize, DT_TILE_FREE_DATA, TileRef, NULL);
				}
			}
		}
	}
	else if (Ar.IsSaving())
	{
		dtNavMesh const* ConstNavMesh = DetourNavMesh;
		for (int i = 0; i < ConstNavMesh->getMaxTiles(); ++i)
		{
			const dtMeshTile* Tile = ConstNavMesh->getTile(i);
			if (Tile != NULL && Tile->header != NULL && Tile->dataSize > 0)
			{
				dtTileRef TileRef = ConstNavMesh->getTileRef(Tile);
				int32 TileDataSize = Tile->dataSize;
				Ar << TileRef << TileDataSize;

				unsigned char* TileData = Tile->data;
				SerializeRecastMeshTile(Ar, TileData, TileDataSize);
			}
		}
	}
}

void FPImplRecastNavMesh::SerializeRecastMeshTile(FArchive& Ar, unsigned char*& TileData, int32& TileDataSize)
{
	// The strategy here is to serialize the data blob that is passed into addTile()
	// @see dtCreateNavMeshData() for details on how this data is laid out


	int32 totVertCount;
	int32 totPolyCount;
	int32 maxLinkCount;
	int32 detailMeshCount;
	int32 detailVertCount;
	int32 detailTriCount;
	int32 bvNodeCount;
	int32 offMeshConCount;
	int32 offMeshSegConCount;
	int32 clusterCount;
	int32 clusterConCount;

	if (Ar.IsSaving())
	{
		// fill in data to write
		dtMeshHeader* const H = (dtMeshHeader*)TileData;
		totVertCount = H->vertCount;
		totPolyCount = H->polyCount;
		maxLinkCount = H->maxLinkCount;
		detailMeshCount = H->detailMeshCount;
		detailVertCount = H->detailVertCount;
		detailTriCount = H->detailTriCount;
		bvNodeCount = H->bvNodeCount;
		offMeshConCount = H->offMeshConCount;
		offMeshSegConCount = H->offMeshSegConCount;
		clusterCount = H->clusterCount;
		clusterConCount = H->maxClusterCons;
	}

	Ar << totVertCount << totPolyCount << maxLinkCount;
	Ar << detailMeshCount << detailVertCount << detailTriCount;
	Ar << bvNodeCount << offMeshConCount << offMeshSegConCount;
	Ar << clusterCount << clusterConCount;
	int32 polyClusterCount = detailMeshCount;

	// calc sizes for our data so we know how much to allocate and where to read/write stuff
	// note this may not match the on-disk size or the in-memory size on the machine that generated that data
	const int32 headerSize = dtAlign4(sizeof(dtMeshHeader));
	const int32 vertsSize = dtAlign4(sizeof(float)*3*totVertCount);
	const int32 polysSize = dtAlign4(sizeof(dtPoly)*totPolyCount);
	const int32 linksSize = dtAlign4(sizeof(dtLink)*maxLinkCount);
	const int32 detailMeshesSize = dtAlign4(sizeof(dtPolyDetail)*detailMeshCount);
	const int32 detailVertsSize = dtAlign4(sizeof(float)*3*detailVertCount );
	const int32 detailTrisSize = dtAlign4(sizeof(unsigned char)*4*detailTriCount);
	const int32 bvTreeSize = dtAlign4(sizeof(dtBVNode)*bvNodeCount);
	const int32 offMeshConsSize = dtAlign4(sizeof(dtOffMeshConnection)*offMeshConCount);
	const int32 offMeshSegsSize = dtAlign4(sizeof(dtOffMeshSegmentConnection)*offMeshSegConCount);
	const int32 clustersSize = dtAlign4(sizeof(dtCluster)*clusterCount);
	const int32 polyClustersSize = dtAlign4(sizeof(unsigned short)*polyClusterCount);
	const int32 clusterConSize = dtAlign4(sizeof(unsigned short)*clusterConCount);

	if (Ar.IsLoading())
	{
		check(TileData == NULL);

		// allocate data chunk for this navmesh tile.  this is its final destination.
		TileDataSize = headerSize + vertsSize + polysSize + linksSize +
			detailMeshesSize + detailVertsSize + detailTrisSize +
			bvTreeSize + offMeshConsSize + offMeshSegsSize +
			clustersSize + polyClustersSize + clusterConSize;
		TileData = (unsigned char*)dtAlloc(sizeof(unsigned char)*TileDataSize, DT_ALLOC_PERM);
		if (!TileData)
		{
			UE_LOG(LogNavigation, Error, TEXT("Failed to alloc navmesh tile"));
		}
		FMemory::Memset(TileData, 0, TileDataSize);
	}
	else if (Ar.IsSaving())
	{
		// TileData and TileDataSize should already be set, verify
		check(TileData != NULL);
	}

	if (TileData != NULL)
	{
		// sort out where various data types do/will live
		unsigned char* d = TileData;
		dtMeshHeader* Header = (dtMeshHeader*)d; d += headerSize;
		float* NavVerts = (float*)d; d += vertsSize;
		dtPoly* NavPolys = (dtPoly*)d; d += polysSize;
		d += linksSize;			// @fixme, are links autogenerated on addTile?
		dtPolyDetail* DetailMeshes = (dtPolyDetail*)d; d += detailMeshesSize;
		float* DetailVerts = (float*)d; d += detailVertsSize;
		unsigned char* DetailTris = (unsigned char*)d; d += detailTrisSize;
		dtBVNode* BVTree = (dtBVNode*)d; d += bvTreeSize;
		dtOffMeshConnection* OffMeshCons = (dtOffMeshConnection*)d; d += offMeshConsSize;
		dtOffMeshSegmentConnection* OffMeshSegs = (dtOffMeshSegmentConnection*)d; d += offMeshSegsSize;
		dtCluster* Clusters = (dtCluster*)d; d += clustersSize;
		unsigned short* PolyClusters = (unsigned short*)d; d += polyClustersSize;
		unsigned short* ClusterCons = (unsigned short*)d; d += clusterConSize;

		check(d==(TileData + TileDataSize));

		// now serialize the data in the blob!

		// header
		Ar << Header->magic << Header->version << Header->x << Header->y;
		Ar << Header->layer << Header->userId << Header->polyCount << Header->vertCount;
		Ar << Header->maxLinkCount << Header->detailMeshCount << Header->detailVertCount << Header->detailTriCount;
		Ar << Header->bvNodeCount << Header->offMeshConCount<< Header->offMeshBase;
		Ar << Header->walkableHeight << Header->walkableRadius << Header->walkableClimb;
		Ar << Header->bmin[0] << Header->bmin[1] << Header->bmin[2];
		Ar << Header->bmax[0] << Header->bmax[1] << Header->bmax[2];
		Ar << Header->bvQuantFactor;
		Ar << Header->clusterCount << Header->maxClusterCons;
		Ar << Header->offMeshSegConCount << Header->offMeshSegPolyBase << Header->offMeshSegVertBase;

		// mesh and offmesh connection vertices, just an array of floats (one float triplet per vert)
		{
			float* F = NavVerts;
			for (int32 VertIdx=0; VertIdx < totVertCount; VertIdx++)
			{
				Ar << *F; F++;
				Ar << *F; F++;
				Ar << *F; F++;
			}
		}

		// mesh and off-mesh connection polys
		for (int32 PolyIdx=0; PolyIdx < totPolyCount; ++PolyIdx)
		{
			dtPoly& P = NavPolys[PolyIdx];
			Ar << P.firstLink;

			for (uint32 VertIdx=0; VertIdx < DT_VERTS_PER_POLYGON; ++VertIdx)
			{
				Ar << P.verts[VertIdx];
			}
			for (uint32 NeiIdx=0; NeiIdx < DT_VERTS_PER_POLYGON; ++NeiIdx)
			{
				Ar << P.neis[NeiIdx];
			}
			Ar << P.flags << P.vertCount << P.areaAndtype;
		}

		// serialize detail meshes
		for (int32 MeshIdx=0; MeshIdx < detailMeshCount; ++MeshIdx)
		{
			dtPolyDetail& DM = DetailMeshes[MeshIdx];
			Ar << DM.vertBase << DM.triBase << DM.vertCount << DM.triCount;
		}

		// serialize detail verts (one float triplet per vert)
		{
			float* F = DetailVerts;
			for (int32 VertIdx=0; VertIdx < detailVertCount; ++VertIdx)
			{
				Ar << *F; F++;
				Ar << *F; F++;
				Ar << *F; F++;
			}
		}

		// serialize detail tris (4 one-byte indices per tri)
		{
			unsigned char* V = DetailTris;
			for (int32 TriIdx=0; TriIdx < detailTriCount; ++TriIdx)
			{
				Ar << *V; V++;
				Ar << *V; V++;
				Ar << *V; V++;
				Ar << *V; V++;
			}
		}

		// serialize BV tree
		for (int32 NodeIdx=0; NodeIdx < bvNodeCount; ++NodeIdx)
		{
			dtBVNode& Node = BVTree[NodeIdx];
			Ar << Node.bmin[0] << Node.bmin[1] << Node.bmin[2];
			Ar << Node.bmax[0] << Node.bmax[1] << Node.bmax[2];
			Ar << Node.i;
		}

		// serialize off-mesh connections
		for (int32 ConnIdx=0; ConnIdx < offMeshConCount; ++ConnIdx)
		{
			dtOffMeshConnection& Conn = OffMeshCons[ConnIdx];
			Ar << Conn.pos[0] << Conn.pos[1] << Conn.pos[2] << Conn.pos[3] << Conn.pos[4] << Conn.pos[5];
			Ar << Conn.rad << Conn.poly << Conn.flags << Conn.side << Conn.userId;
		}

		for (int32 SegIdx=0; SegIdx < offMeshSegConCount; ++SegIdx)
		{
			dtOffMeshSegmentConnection& Seg = OffMeshSegs[SegIdx];
			Ar << Seg.startA[0] << Seg.startA[1] << Seg.startA[2];
			Ar << Seg.startB[0] << Seg.startB[1] << Seg.startB[2];
			Ar << Seg.endA[0] << Seg.endA[1] << Seg.endA[2];
			Ar << Seg.endB[0] << Seg.endB[1] << Seg.endB[2];
			Ar << Seg.rad << Seg.firstPoly << Seg.npolys << Seg.flags << Seg.userId;
		}

		// serialize clusters
		for (int32 ClusterIdx = 0; ClusterIdx < clusterCount; ++ClusterIdx)
		{
			dtCluster& Cluster = Clusters[ClusterIdx];
			Ar << Cluster.centerPoly << Cluster.firstLink << Cluster.numLinks;
			Ar << Cluster.center[0] << Cluster.center[1] << Cluster.center[2];
		}

		// serialize poly clusters map
		{
			unsigned short* C = PolyClusters;
			for (int32 PolyClusterIdx = 0; PolyClusterIdx < polyClusterCount; ++PolyClusterIdx)
			{
				Ar << *C; C++;
			}
		}

		// serialize cluster links
		{
			unsigned short* C = ClusterCons;
			for (int32 ClusterConIdx = 0; ClusterConIdx < clusterConCount; ++ClusterConIdx)
			{
				Ar << *C; C++;
			}
		}
	}
}

void FPImplRecastNavMesh::SetRecastMesh(dtNavMesh* NavMesh, bool bOwnData)
{
	if (NavMesh == DetourNavMesh)
	{
		return;
	}

	if (DetourNavMesh != NULL && !!bOwnsNavMeshData)
	{
		// if there's already some recast navmesh, and it's owned by this instance then release it
		dtFreeNavMesh(DetourNavMesh);
		DetourNavMesh = NULL;
	}

	bOwnsNavMeshData = bOwnData;
	DetourNavMesh = NavMesh;
}

void FPImplRecastNavMesh::Raycast2D(const FVector& StartLoc, const FVector& EndLoc, const FNavigationQueryFilter& InQueryFilter, ARecastNavMesh::FRaycastResult& RaycastResult) const
{
	if (DetourNavMesh == NULL || NavMeshOwner == NULL)
	{
		return;
	}

	const dtQueryFilter* QueryFilter = ((const FRecastQueryFilter*)(InQueryFilter.GetImplementation()))->GetAsDetourQueryFilter();
	if (QueryFilter == NULL)
	{
		UE_VLOG(NavMeshOwner, LogNavigation, Warning, TEXT("FPImplRecastNavMesh::FindPath failing due to QueryFilter == NULL"));
		return;
	}

	INITIALIZE_NAVQUERY(NavQuery, InQueryFilter.GetMaxSearchNodes());

	const FVector& NavExtent = NavMeshOwner->GetDefaultQueryExtent();
	const float Extent[3] = { NavExtent.X, NavExtent.Z, NavExtent.Y };

	const FVector RecastStart = Unreal2RecastPoint(StartLoc);
	const FVector RecastEnd = Unreal2RecastPoint(EndLoc);

	NavNodeRef StartPolyID = INVALID_NAVNODEREF;
	NavQuery.findNearestPoly(&RecastStart.X, Extent, QueryFilter, &StartPolyID, NULL);
	if (StartPolyID != INVALID_NAVNODEREF)
	{
		const dtStatus RaycastStatus = NavQuery.raycast(StartPolyID, &RecastStart.X, &RecastEnd.X
			, QueryFilter, &RaycastResult.HitTime, &RaycastResult.HitNormal.X
			, RaycastResult.CorridorPolys, &RaycastResult.CorridorPolysCount, RaycastResult.GetMaxCorridorSize());

		if (dtStatusSucceed(RaycastStatus) == false)
		{
			UE_VLOG(NavMeshOwner, LogNavigation, Log, TEXT("FPImplRecastNavMesh::Raycast2D failed"));
		}
	}
}

// @TODONAV
ENavigationQueryResult::Type FPImplRecastNavMesh::FindPath(const FVector& StartLoc, const FVector& EndLoc, FNavMeshPath& Path, const FNavigationQueryFilter& InQueryFilter) const
{
	// temporarily disabling this check due to it causing too much "crashes"
	// @todo but it needs to be back at some point since it realy checks for a buggy setup
	//ensure(DetourNavMesh != NULL || NavMeshOwner->bRebuildAtRuntime == false);

	if (DetourNavMesh == NULL || NavMeshOwner == NULL)
	{
		return ENavigationQueryResult::Error;
	}

	const FRecastQueryFilter* FilterImplementation = (const FRecastQueryFilter*)(InQueryFilter.GetImplementation());
	if (FilterImplementation == NULL)
	{
		UE_VLOG(NavMeshOwner, LogNavigation, Error, TEXT("FPImplRecastNavMesh::FindPath failed due to passed filter having NULL implementation!"));
		return ENavigationQueryResult::Error;
	}

	const dtQueryFilter* QueryFilter = FilterImplementation->GetAsDetourQueryFilter();
	if (QueryFilter == NULL)
	{
		UE_VLOG(NavMeshOwner, LogNavigation, Warning, TEXT("FPImplRecastNavMesh::FindPath failing due to QueryFilter == NULL"));
		return ENavigationQueryResult::Error;
	}

	INITIALIZE_NAVQUERY(NavQuery, InQueryFilter.GetMaxSearchNodes());

	FVector RecastStartPos, RecastEndPos;
	NavNodeRef StartPolyID, EndPolyID;
	const bool bCanSearch = InitPathfinding(StartLoc, EndLoc, NavQuery, QueryFilter, RecastStartPos, StartPolyID, RecastEndPos, EndPolyID);
	if (!bCanSearch)
	{
		return ENavigationQueryResult::Error;
	}

	// initialize output
	Path.PathPoints.Reset();
	Path.PathCorridor.Reset();
	Path.PathCorridorCost.Reset();

	// get path corridor
	static const int32 MAX_PATH_CORRIDOR_POLYS = 128;
	NavNodeRef PathCorridorPolys[MAX_PATH_CORRIDOR_POLYS];
	float PathCorridorCost[MAX_PATH_CORRIDOR_POLYS] = { 0.0f };
	int32 NumPathCorridorPolys;

	const dtStatus FindPathStatus = NavQuery.findPath(StartPolyID, EndPolyID,
		&RecastStartPos.X, &RecastEndPos.X, QueryFilter,
		PathCorridorPolys, &NumPathCorridorPolys, MAX_PATH_CORRIDOR_POLYS, PathCorridorCost, 0);

	PostProcessPath(FindPathStatus, Path, NavQuery, QueryFilter,
		StartPolyID, EndPolyID, StartLoc, EndLoc, RecastStartPos, RecastEndPos,
		PathCorridorPolys, PathCorridorCost, NumPathCorridorPolys);

	return DTStatusToNavQueryResult(FindPathStatus);
}

ENavigationQueryResult::Type FPImplRecastNavMesh::TestPath(const FVector& StartLoc, const FVector& EndLoc, const FNavigationQueryFilter& InQueryFilter) const
{
	const dtQueryFilter* QueryFilter = ((const FRecastQueryFilter*)(InQueryFilter.GetImplementation()))->GetAsDetourQueryFilter();
	if (QueryFilter == NULL)
	{
		UE_VLOG(NavMeshOwner, LogNavigation, Warning, TEXT("FPImplRecastNavMesh::FindPath failing due to QueryFilter == NULL"));
		return ENavigationQueryResult::Error;
	}

	INITIALIZE_NAVQUERY(NavQuery, InQueryFilter.GetMaxSearchNodes());

	FVector RecastStartPos, RecastEndPos;
	NavNodeRef StartPolyID, EndPolyID;
	const bool bCanSearch = InitPathfinding(StartLoc, EndLoc, NavQuery, QueryFilter, RecastStartPos, StartPolyID, RecastEndPos, EndPolyID);
	if (!bCanSearch)
	{
		return ENavigationQueryResult::Error;
	}

	// get path corridor
	static const int32 MAX_PATH_CORRIDOR_POLYS = 128;
	NavNodeRef PathCorridorPolys[MAX_PATH_CORRIDOR_POLYS];
	float PathCorridorCost[MAX_PATH_CORRIDOR_POLYS];
	int32 NumPathCorridorPolys;

	const dtStatus FindPathStatus = NavQuery.findPath(StartPolyID, EndPolyID,
		&RecastStartPos.X, &RecastEndPos.X, QueryFilter,
		PathCorridorPolys, &NumPathCorridorPolys, MAX_PATH_CORRIDOR_POLYS, PathCorridorCost, 0);

	return DTStatusToNavQueryResult(FindPathStatus);
}

ENavigationQueryResult::Type FPImplRecastNavMesh::TestClusterPath(const FVector& StartLoc, const FVector& EndLoc) const
{
	FVector RecastStartPos, RecastEndPos;
	NavNodeRef StartPolyID, EndPolyID;
	const dtQueryFilter* ClusterFilter = ((const FRecastQueryFilter*)NavMeshOwner->GetDefaultQueryFilterImpl())->GetAsDetourQueryFilter();

	INITIALIZE_NAVQUERY(ClusterQuery, RECAST_MAX_SEARCH_NODES);

	const bool bCanSearch = InitPathfinding(StartLoc, EndLoc, ClusterQuery, ClusterFilter, RecastStartPos, StartPolyID, RecastEndPos, EndPolyID);
	if (!bCanSearch)
	{
		return ENavigationQueryResult::Error;
	}

	const dtStatus status = ClusterQuery.testClusterPath(StartPolyID, EndPolyID);
	return DTStatusToNavQueryResult(status);
}

ENavigationQueryResult::Type FPImplRecastNavMesh::FindClusterPath(const FVector& StartLoc, const FVector& EndLoc, FNavMeshPath& Path) const
{
	FVector RecastStartPos, RecastEndPos;
	NavNodeRef StartPolyID, EndPolyID;
	const dtQueryFilter* ClusterFilter = ((const FRecastQueryFilter*)NavMeshOwner->GetDefaultQueryFilterImpl())->GetAsDetourQueryFilter();

	INITIALIZE_NAVQUERY(ClusterQuery, RECAST_MAX_SEARCH_NODES);

	const bool bCanSearch = InitPathfinding(StartLoc, EndLoc, ClusterQuery, ClusterFilter, RecastStartPos, StartPolyID, RecastEndPos, EndPolyID);
	if (!bCanSearch)
	{
		return ENavigationQueryResult::Error;
	}

	// initialize output
	Path.PathPoints.Reset();
	Path.PathCorridor.Reset();
	Path.PathCorridorCost.Reset();

	TArray<dtClusterRef> ClusterPath;

	const ENavigationQueryResult::Type ClusterGraphResult = FindPathOnClusterGraph(StartPolyID, EndPolyID, ClusterQuery, ClusterFilter, ClusterPath);
	if (ClusterGraphResult == ENavigationQueryResult::Error)
	{
		UE_VLOG(NavMeshOwner, LogNavigation, Warning, TEXT("FPImplRecastNavMesh::FindClusterPath unable to search cluster graph"));
		return ENavigationQueryResult::Error;
	}

	const EClusterPath::Type PathType =
		(ClusterGraphResult == ENavigationQueryResult::Fail) ? EClusterPath::Partial : EClusterPath::Complete;

	const ENavigationQueryResult::Type SearchResult = FindPathThroughClusters(StartPolyID, EndPolyID, StartLoc, EndLoc, RecastStartPos, RecastEndPos, ClusterQuery, ClusterFilter, ClusterPath, PathType, Path);
	return SearchResult;
}

ENavigationQueryResult::Type FPImplRecastNavMesh::FindPathThroughClusters(NavNodeRef StartPoly, NavNodeRef EndPoly,
	const FVector& StartLoc, const FVector& EndLoc,
	const FVector& RecastStart, FVector& RecastEnd,
	const dtNavMeshQuery& ClusterQuery, const dtQueryFilter* ClusterFilter,
	const TArray<dtClusterRef>& ClusterPath, const EClusterPath::Type& ClusterPathType,
	FNavMeshPath& Path) const
{
	static const int32 MAX_PATH_CORRIDOR_POLYS = 128;
	NavNodeRef PathCorridorPolys[MAX_PATH_CORRIDOR_POLYS];
	float PathCorridorCost[MAX_PATH_CORRIDOR_POLYS];
	int32 NumPathCorridorPolys;

	dtStatus FindPathStatus = ClusterQuery.findPathThroughClusters(StartPoly, EndPoly, &RecastStart.X, &RecastEnd.X, ClusterFilter,
		ClusterPath.GetTypedData(), ClusterPath.Num(),
		PathCorridorPolys, &NumPathCorridorPolys, MAX_PATH_CORRIDOR_POLYS, PathCorridorCost);

	if (dtStatusSucceed(FindPathStatus))
	{
		if (ClusterPathType == EClusterPath::Partial)
		{
			FindPathStatus |= DT_PARTIAL_RESULT;
		}

		PostProcessPath(FindPathStatus, Path, ClusterQuery, ClusterFilter,
			StartPoly, EndPoly, StartLoc, EndLoc, RecastStart, RecastEnd,
			PathCorridorPolys, PathCorridorCost, NumPathCorridorPolys);
	}

	return DTStatusToNavQueryResult(FindPathStatus);
}

ENavigationQueryResult::Type FPImplRecastNavMesh::FindPathOnClusterGraph(NavNodeRef StartPoly, NavNodeRef EndPoly,
	const dtNavMeshQuery& ClusterQuery, const dtQueryFilter* ClusterFilter,
	TArray<NavNodeRef>& ClusterPath) const
{
	ClusterPath.Init(RECAST_MAX_PATH_VERTS);
	int32 PathSize = 0;

	const dtStatus status = ClusterQuery.findPathOnClusterGraph(StartPoly, EndPoly, ClusterFilter, ClusterPath.GetTypedData(), &PathSize, ClusterPath.Num(), 0);
	ClusterPath.SetNum(PathSize);

	return DTStatusToNavQueryResult(status);
}

bool FPImplRecastNavMesh::InitPathfinding(const FVector& UnrealStart, const FVector& UnrealEnd,
	const dtNavMeshQuery& Query, const dtQueryFilter* Filter,
	FVector& RecastStart, dtPolyRef& StartPoly,
	FVector& RecastEnd, dtPolyRef& EndPoly) const
{
	const FVector& NavExtent = NavMeshOwner->GetDefaultQueryExtent();
	const float Extent[3] = { NavExtent.X, NavExtent.Z, NavExtent.Y };

	RecastStart = Unreal2RecastPoint(UnrealStart);
	RecastEnd = Unreal2RecastPoint(UnrealEnd);

	StartPoly = INVALID_NAVNODEREF;
	Query.findNearestPoly(&RecastStart.X, Extent, Filter, &StartPoly, NULL);
	if (StartPoly == INVALID_NAVNODEREF)
	{
		UE_VLOG(NavMeshOwner, LogNavigation, Warning, TEXT("FPImplRecastNavMesh::InitPathfinding start point not on navmesh"));
		UE_VLOG_SEGMENT(NavMeshOwner, UnrealStart, UnrealEnd, FColor::Red, TEXT("Failed path"));
		UE_VLOG_LOCATION(NavMeshOwner, UnrealStart, 15, FColor::Red, TEXT("Start failed"));
		UE_VLOG_BOX(NavMeshOwner, FBox(UnrealStart - NavExtent, UnrealStart + NavExtent), FColor::Red, TEXT_EMPTY);

		return false;
	}

	EndPoly = INVALID_NAVNODEREF;
	Query.findNearestPoly(&RecastEnd.X, Extent, Filter, &EndPoly, NULL);
	if (EndPoly == INVALID_NAVNODEREF)
	{
		UE_VLOG(NavMeshOwner, LogNavigation, Warning, TEXT("FPImplRecastNavMesh::InitPathfinding end point not on navmesh"));
		UE_VLOG_SEGMENT(NavMeshOwner, UnrealEnd, UnrealEnd, FColor::Red, TEXT("Failed path"));
		UE_VLOG_LOCATION(NavMeshOwner, UnrealEnd, 15, FColor::Red, TEXT("End failed"));
		UE_VLOG_BOX(NavMeshOwner, FBox(UnrealEnd - NavExtent, UnrealEnd + NavExtent), FColor::Red, TEXT_EMPTY);

		return false;
	}

	return true;
}

void FPImplRecastNavMesh::PostProcessPath(dtStatus FindPathStatus, FNavMeshPath& Path,
	const dtNavMeshQuery& NavQuery, const dtQueryFilter* Filter,
	NavNodeRef StartPolyID, NavNodeRef EndPolyID,
	const FVector& StartLoc, const FVector& EndLoc,
	const FVector& RecastStartPos, FVector& RecastEndPos,
	NavNodeRef* PathCorridorPolys, float* PathCorridorCost, int32 NumPathCorridorPolys) const
{
	if (dtStatusSucceed(FindPathStatus))
	{
		Path.PathCorridorCost.AddUninitialized(NumPathCorridorPolys);
		if (NumPathCorridorPolys == 1)
		{
			// failsafe cost for single poly corridor
			uint8 AreaID = RECAST_DEFAULT_AREA;
			DetourNavMesh->getPolyArea(StartPolyID, &AreaID);
			
			const float AreaTravelCost = Filter->getAreaCost(AreaID);
			Path.PathCorridorCost[0] = AreaTravelCost * (EndLoc - StartLoc).Size();
		}
		else
		{
			for (int32 i = 0; i < NumPathCorridorPolys; i++)
			{
				Path.PathCorridorCost[i] = PathCorridorCost[i];
			}
		}

		// "string pulling" to get final path

		// temp data to catch output in detour's preferred format
		float RecastPathVerts[RECAST_MAX_PATH_VERTS*3];
		uint8 RecastPathFlags[RECAST_MAX_PATH_VERTS];
		NavNodeRef RecastPolyRefs[RECAST_MAX_PATH_VERTS];

		int32 NumPathVerts = 0;

		Path.PathCorridor.AddUninitialized(NumPathCorridorPolys);
		NavNodeRef* CorridorPoly = PathCorridorPolys;
		NavNodeRef* DestCorridorPoly = Path.PathCorridor.GetTypedData();
		for (int i = 0; i < NumPathCorridorPolys; ++i, ++CorridorPoly, ++DestCorridorPoly)
		{
			*DestCorridorPoly = *CorridorPoly;
		}

		Path.OnPathCorridorUpdated(); 

#if STATS
		if (dtStatusDetail(FindPathStatus, DT_OUT_OF_NODES))
		{
			INC_DWORD_STAT(STAT_Navigation_OutOfNodesPath);
		}

		if (dtStatusDetail(FindPathStatus, DT_PARTIAL_RESULT))
		{
			INC_DWORD_STAT(STAT_Navigation_PartialPath);
		}
#endif

		if (Path.WantsStringPulling())
		{
			// if path is partial (path corridor doesn't contain EndPolyID), find new RecastEndPos on last poly in corridor
			if (dtStatusDetail(FindPathStatus, DT_PARTIAL_RESULT))
			{
				NavNodeRef LastPolyID = PathCorridorPolys[NumPathCorridorPolys - 1];
				float NewEndPoint[3];

				const dtStatus NewEndPointStatus = NavQuery.closestPointOnPoly(LastPolyID, &RecastEndPos.X, NewEndPoint);
				if (dtStatusSucceed(NewEndPointStatus))
				{
					FMemory::Memcpy(&RecastEndPos.X, NewEndPoint, sizeof(NewEndPoint));
				}
			}

			const dtStatus StringPullStatus = NavQuery.findStraightPath(&RecastStartPos.X, &RecastEndPos.X, PathCorridorPolys
				, NumPathCorridorPolys, RecastPathVerts, RecastPathFlags, RecastPolyRefs, &NumPathVerts
				, RECAST_MAX_PATH_VERTS, DT_STRAIGHTPATH_AREA_CROSSINGS);

			if (dtStatusSucceed(StringPullStatus))
			{
				Path.PathPoints.AddZeroed(NumPathVerts);

				// convert to desired format
				FNavPathPoint* CurVert = Path.PathPoints.GetTypedData();
				float* CurRecastVert = RecastPathVerts;
				uint8* CurFlag = RecastPathFlags;
				NavNodeRef* CurPoly = RecastPolyRefs;

				UNavigationSystem* NavSys = NavMeshOwner->GetWorld()->GetNavigationSystem();

				for (int32 VertIdx=0; VertIdx < NumPathVerts; ++VertIdx)
				{
					FNavMeshNodeFlags CurNodeFlags(0);
					CurVert->Location = Recast2UnrVector(CurRecastVert);
					CurRecastVert += 3;

					CurNodeFlags.PathFlags = *CurFlag;
					CurFlag++;

					uint8 AreaID = RECAST_DEFAULT_AREA;
					DetourNavMesh->getPolyArea(*CurPoly, &AreaID);
					CurNodeFlags.Area = AreaID;

					const UClass* AreaClass = NavMeshOwner->GetAreaClass(AreaID);
					const UNavArea* DefArea = AreaClass ? ((UClass*)AreaClass)->GetDefaultObject<UNavArea>() : NULL;
					CurNodeFlags.AreaFlags = DefArea ? DefArea->GetAreaFlags() : 0;

					CurVert->NodeRef = *CurPoly;
					CurPoly++;

					CurVert->Flags = CurNodeFlags.Pack();

					// include smart link data
					// if there will be more "edge types" we change this implementation to something more generic
					if (CurNodeFlags.PathFlags & DT_STRAIGHTPATH_OFFMESH_CONNECTION)
					{
						const dtOffMeshConnection* OffMeshCon = DetourNavMesh->getOffMeshConnectionByRef(CurVert->NodeRef);
						if (OffMeshCon && OffMeshCon->userId)
						{
							CurVert->SmartLink = NavSys->GetSmartLink(OffMeshCon->userId);
						}
					}

					CurVert++;
				}

				// findStraightPath returns 0 for polyId of last point for some reason, even though it knows the poly id.  We will fill that in correctly with the last poly id of the corridor.
				// @TODO shouldn't it be the same as EndPolyID? (nope, it could be partial path)
				Path.PathPoints[NumPathVerts-1].NodeRef = PathCorridorPolys[NumPathCorridorPolys-1];
			}
		}
		else
		{
			// make sure at least beginning and end of path are added
			Path.PathPoints.AddUninitialized(2);
			Path.PathPoints[0].NodeRef = StartPolyID;
			Path.PathPoints[0].Location = StartLoc;
			Path.PathPoints[1].NodeRef = EndPolyID;
			Path.PathPoints[1].Location = EndLoc;
		}

		if (Path.WantsPathCorridor())
		{
			TArray<FNavigationPortalEdge> PathCorridorEdges;
			PathCorridorEdges.Empty(NumPathCorridorPolys - 1);
			for (int32 i = 0; i < NumPathCorridorPolys - 1; ++i)
			{
				unsigned char FromType = 0, ToType = 0;
				float Left[3] = {0.f}, Right[3] = {0.f};

				NavQuery.getPortalPoints(Path.PathCorridor[i], Path.PathCorridor[i+1], Left, Right, FromType, ToType);

				PathCorridorEdges.Add(FNavigationPortalEdge(Recast2UnrVector(Left), Recast2UnrVector(Right), Path.PathCorridor[i+1]));
			}

			Path.SetPathCorridorEdges(PathCorridorEdges);
		}
	}

	if (dtStatusDetail(FindPathStatus, DT_PARTIAL_RESULT))
	{
		Path.SetIsPartial(true);
		// this means path finding algorithm reached the limit of InQueryFilter.GetMaxSearchNodes()
		// nodes in A* node pool. This can mean resulting path is way off.
		Path.SetSearchReachedLimit(dtStatusDetail(FindPathStatus, DT_OUT_OF_NODES));
	}
	Path.MarkReady();
}

static bool IsDebugNodeModified(const FRecastDebugPathfindingNode& NodeData, const FRecastDebugPathfindingStep& PreviousStep)
{
	const FRecastDebugPathfindingNode* PrevNodeData = PreviousStep.Nodes.Find(NodeData);
	if (PrevNodeData)
	{
		const bool bModified = PrevNodeData->bOpenSet != NodeData.bOpenSet ||
			PrevNodeData->TotalCost != NodeData.TotalCost ||
			PrevNodeData->Cost != NodeData.Cost ||
			PrevNodeData->ParentRef != NodeData.ParentRef ||
			!PrevNodeData->NodePos.Equals(NodeData.NodePos, SMALL_NUMBER);

		return bModified;
	}

	return true;
}

static void StorePathfindingDebugStep(const dtNavMeshQuery& NavQuery, const dtNavMesh* NavMesh, TArray<FRecastDebugPathfindingStep>& Steps)
{
	const int StepIdx = Steps.AddZeroed(1);
	FRecastDebugPathfindingStep& StepInfo = Steps[StepIdx];
	
	dtNode* BestNode = 0;
	float BestNodeCost = 0.0f;
	NavQuery.getCurrentBestResult(BestNode, BestNodeCost);

	const dtNodePool* NodePool = NavQuery.getNodePool();
	for (int32 i = 0; i < NodePool->getNodeCount(); i++)
	{
		const dtNode* Node = NodePool->getNodeAtIdx(i + 1);
		
		FRecastDebugPathfindingNode NodeInfo;
		NodeInfo.PolyRef = Node->id;
		NodeInfo.ParentRef = Node->pidx ? NodePool->getNodeAtIdx(Node->pidx)->id : 0;
		NodeInfo.Cost = Node->cost;
		NodeInfo.TotalCost = Node->total;
		NodeInfo.bOpenSet = !NavQuery.isInClosedList(Node->id);
		NodeInfo.bModified = true;
		NodeInfo.NodePos = Recast2UnrealPoint(&Node->pos[0]);

		const dtPoly* NavPoly = 0;
		const dtMeshTile* NavTile = 0;
		NavMesh->getTileAndPolyByRef(Node->id, &NavTile, &NavPoly);

		NodeInfo.bOffMeshLink = NavPoly ? (NavPoly->getType() != DT_POLYTYPE_GROUND) : false;
		for (int32 iv = 0; iv < NavPoly->vertCount; iv++)
		{
			NodeInfo.Verts.Add(Recast2UnrealPoint(&NavTile->verts[NavPoly->verts[iv] * 3]));
		}

		FSetElementId NodeId = StepInfo.Nodes.Add(NodeInfo);
		if (Node == BestNode)
		{
			StepInfo.BestNode = NodeId;
		}
	}

	if (Steps.Num() > 1)
	{
		FRecastDebugPathfindingStep& PrevStepInfo = Steps[StepIdx - 1];
		for (TSet<FRecastDebugPathfindingNode>::TIterator It(StepInfo.Nodes); It; ++It)
		{
			FRecastDebugPathfindingNode& NodeData = *It;
			NodeData.bModified = IsDebugNodeModified(NodeData, PrevStepInfo);
		}
	}
}

int32 FPImplRecastNavMesh::DebugPathfinding(const FVector& StartLoc, const FVector& EndLoc, const FNavigationQueryFilter& Filter, TArray<FRecastDebugPathfindingStep>& Steps)
{
	int32 NumSteps = 0;

	const dtQueryFilter* QueryFilter = ((const FRecastQueryFilter*)(Filter.GetImplementation()))->GetAsDetourQueryFilter();
	if (QueryFilter == NULL)
	{
		UE_VLOG(NavMeshOwner, LogNavigation, Warning, TEXT("FPImplRecastNavMesh::DebugPathfinding failing due to QueryFilter == NULL"));
		return NumSteps;
	}

	INITIALIZE_NAVQUERY(NavQuery, Filter.GetMaxSearchNodes());

	FVector RecastStartPos, RecastEndPos;
	NavNodeRef StartPolyID, EndPolyID;
	const bool bCanSearch = InitPathfinding(StartLoc, EndLoc, NavQuery, QueryFilter, RecastStartPos, StartPolyID, RecastEndPos, EndPolyID);
	if (!bCanSearch)
	{
		return NumSteps;
	}

	dtStatus status = NavQuery.initSlicedFindPath(StartPolyID, EndPolyID, &RecastStartPos.X, &RecastEndPos.X, QueryFilter);
	while (dtStatusInProgress(status))
	{
		StorePathfindingDebugStep(NavQuery, DetourNavMesh, Steps);
		NumSteps++;

		status = NavQuery.updateSlicedFindPath(1, 0);
	}

	static const int32 MAX_TEMP_POLYS = 16;
	NavNodeRef TempPolys[MAX_TEMP_POLYS];
	int32 NumTempPolys;
	NavQuery.finalizeSlicedFindPath(TempPolys, &NumTempPolys, MAX_TEMP_POLYS);

	return NumSteps;
}

NavNodeRef FPImplRecastNavMesh::GetClusterRefFromPolyRef(const NavNodeRef PolyRef) const
{
	if (DetourNavMesh)
	{
		const dtMeshTile* Tile = DetourNavMesh->getTileByRef(PolyRef);
		uint32 PolyIdx = DetourNavMesh->decodePolyIdPoly(PolyRef);
		if (Tile && Tile->polyClusters && PolyIdx < (uint32)Tile->header->offMeshBase)
		{
			return DetourNavMesh->getClusterRefBase(Tile) | Tile->polyClusters[PolyIdx];
		}
	}

	return 0;
}

FNavLocation FPImplRecastNavMesh::GetRandomPoint(const FNavigationQueryFilter& Filter) const
{
	FNavLocation OutLocation;
	if (DetourNavMesh == NULL)
	{
		return OutLocation;
	}

	INITIALIZE_NAVQUERY(NavQuery, Filter.GetMaxSearchNodes());

	// inits to "pass all"
	const dtQueryFilter* QueryFilter = ((const FRecastQueryFilter*)(Filter.GetImplementation()))->GetAsDetourQueryFilter();
	ensure(QueryFilter);
	if (QueryFilter)
	{
		dtPolyRef Poly;
		float RandPt[3];
		dtStatus Status = NavQuery.findRandomPoint(QueryFilter, FMath::FRand, &Poly, RandPt);
		if (dtStatusSucceed(Status))
		{
			// arrange output
			OutLocation.Location = Recast2UnrVector(RandPt);
			OutLocation.NodeRef = Poly;
		}
	}

	return OutLocation;
}

bool FPImplRecastNavMesh::GetRandomPointInRadius(const FVector& Origin, float Radius, FNavLocation& OutLocation, const FNavigationQueryFilter& Filter) const
{
	if (DetourNavMesh == NULL || Radius <= 0.f)
	{
		return false;
	}

	INITIALIZE_NAVQUERY(NavQuery, Filter.GetMaxSearchNodes());

	// inits to "pass all"
	const dtQueryFilter* QueryFilter = ((const FRecastQueryFilter*)(Filter.GetImplementation()))->GetAsDetourQueryFilter();
	ensure(QueryFilter);
	if (QueryFilter)
	{
		// find starting poly
		// convert start/end pos to Recast coords
		const float Extent[3] = {Radius, Radius, Radius};
		float RecastOrigin[3];
		Unr2RecastVector(Origin, RecastOrigin);
		NavNodeRef OriginPolyID = INVALID_NAVNODEREF;
		NavQuery.findNearestPoly(RecastOrigin, Extent, QueryFilter, &OriginPolyID, NULL);

		dtPolyRef Poly;
		float RandPt[3];
		dtStatus Status = NavQuery.findRandomPointAroundCircle(OriginPolyID, RecastOrigin, Radius
			, QueryFilter, FMath::FRand, &Poly, RandPt);

		if (dtStatusSucceed(Status))
		{
			OutLocation = FNavLocation(Recast2UnrVector(RandPt), Poly);
			return true;
		}
		else
		{
			OutLocation = FNavLocation(Origin, OriginPolyID);
		}
	}

	return false;
}

bool FPImplRecastNavMesh::GetRandomPointInCluster(NavNodeRef ClusterRef, FNavLocation& OutLocation) const
{
	if (DetourNavMesh == NULL || ClusterRef == 0)
	{
		return false;
	}

	INITIALIZE_NAVQUERY(NavQuery, RECAST_MAX_SEARCH_NODES);

	dtPolyRef Poly;
	float RandPt[3];
	dtStatus Status = NavQuery.findRandomPointInCluster(ClusterRef, FMath::FRand, &Poly, RandPt);

	if (dtStatusSucceed(Status))
	{
		OutLocation = FNavLocation(Recast2UnrVector(RandPt), Poly);
		return true;
	}

	return false;
}

bool FPImplRecastNavMesh::ProjectPointToNavMesh(const FVector& Point, FNavLocation& Result, const FVector& Extent, const FNavigationQueryFilter& Filter) const
{
	// sanity check
	if (DetourNavMesh == NULL)
	{
		return false;
	}

	bool bSuccess = false;

	INITIALIZE_NAVQUERY(NavQuery, Filter.GetMaxSearchNodes());

	const dtQueryFilter* QueryFilter = ((const FRecastQueryFilter*)(Filter.GetImplementation()))->GetAsDetourQueryFilter();
	ensure(QueryFilter);
	if (QueryFilter)
	{
		float ClosestPoint[3];

		FVector RcExtent = Unreal2RecastPoint( Extent ).GetAbs();
	
		FVector RcPoint = Unreal2RecastPoint( Point );
		dtPolyRef PolyRef;
		NavQuery.findNearestPoly( &RcPoint.X, &RcExtent.X, QueryFilter, &PolyRef, ClosestPoint );

		if( PolyRef > 0 )
		{
			bSuccess = true;
			Result = FNavLocation(Recast2UnrVector(ClosestPoint), PolyRef);
		}
	}

	return (bSuccess);
}

bool FPImplRecastNavMesh::ProjectPointMulti(const FVector& Point, TArray<FNavLocation>& Result, const FVector& Extent,
											float MinZ, float MaxZ, const FNavigationQueryFilter& Filter) const
{
	// sanity check
	if (DetourNavMesh == NULL)
	{
		return false;
	}

	bool bSuccess = false;

	INITIALIZE_NAVQUERY(NavQuery, Filter.GetMaxSearchNodes());

	const dtQueryFilter* QueryFilter = ((const FRecastQueryFilter*)(Filter.GetImplementation()))->GetAsDetourQueryFilter();
	ensure(QueryFilter);
	if (QueryFilter)
	{
		const FVector AdjustedPoint(Point.X, Point.Y, (MaxZ + MinZ) * 0.5f);
		const FVector AdjustedExtent(Extent.X, Extent.Y, (MaxZ - MinZ) * 0.5f);

		const FVector RcPoint = Unreal2RecastPoint( AdjustedPoint );
		const FVector RcExtent = Unreal2RecastPoint( AdjustedExtent ).GetAbs();

		const int32 MaxHitPolys = 256;
		dtPolyRef HitPolys[MaxHitPolys];
		int32 NumHitPolys = 0;

		dtStatus status = NavQuery.queryPolygons(&RcPoint.X, &RcExtent.X, QueryFilter, HitPolys, &NumHitPolys, MaxHitPolys);
		if (dtStatusSucceed(status))
		{
			for (int32 i = 0; i < NumHitPolys; i++)
			{
				float ClosestPoint[3];
				
				status = NavQuery.closestPointOnPoly(HitPolys[i], &RcPoint.X, ClosestPoint);
				if (dtStatusSucceed(status))
				{
					FNavLocation HitLocation(Recast2UnrealPoint(ClosestPoint), HitPolys[i]);
					if ((HitLocation.Location - AdjustedPoint).SizeSquared2D() < KINDA_SMALL_NUMBER)
					{
						Result.Add(HitLocation);
					}
				}
			}
		}
	}

	return (bSuccess);
}

NavNodeRef FPImplRecastNavMesh::FindNearestPoly(FVector const& Loc, FVector const& Extent, const FNavigationQueryFilter& Filter) const
{
	// sanity check
	if (DetourNavMesh == NULL)
	{
		return INVALID_NAVNODEREF;
	}

	INITIALIZE_NAVQUERY(NavQuery, Filter.GetMaxSearchNodes());

	// inits to "pass all"
	const dtQueryFilter* QueryFilter = ((const FRecastQueryFilter*)(Filter.GetImplementation()))->GetAsDetourQueryFilter();
	ensure(QueryFilter);
	if (QueryFilter)
	{
		float RecastLoc[3];
		Unr2RecastVector(Loc, RecastLoc);
		float RecastExtent[3];
		Unr2RecastSizeVector(Extent, RecastExtent);

		NavNodeRef OutRef;
		dtStatus Status = NavQuery.findNearestPoly(RecastLoc, RecastExtent, QueryFilter, &OutRef, NULL);
		if (dtStatusSucceed(Status))
		{
			return OutRef;
		}
	}

	return INVALID_NAVNODEREF;
}

bool FPImplRecastNavMesh::GetPolysWithinPathingDistance(FVector const& StartLoc, const float PathingDistance, const FNavigationQueryFilter& Filter, TArray<NavNodeRef>& FoundPolys) const
{
	ensure(PathingDistance > 0.0f && "PathingDistance <= 0 doesn't make sense");

	// sanity check
	if (DetourNavMesh == NULL)
	{
		return false;
	}

	INITIALIZE_NAVQUERY(NavQuery, Filter.GetMaxSearchNodes());

	const dtQueryFilter* QueryFilter = ((const FRecastQueryFilter*)(Filter.GetImplementation()))->GetAsDetourQueryFilter();
	ensure(QueryFilter);
	if (QueryFilter == NULL)
	{
		return false;
	}

	// @todo this should be configurable in some kind of FindPathQuery structure
	const FVector& NavExtent = NavMeshOwner->GetDefaultQueryExtent();
	const float Extent[3] = { NavExtent.X, NavExtent.Z, NavExtent.Y };

	float RecastStartPos[3];
	Unr2RecastVector(StartLoc, RecastStartPos);
	// @TODO add failure handling
	NavNodeRef StartPolyID = INVALID_NAVNODEREF;
	NavQuery.findNearestPoly(RecastStartPos, Extent, QueryFilter, &StartPolyID, NULL);
		
	FoundPolys.AddUninitialized(Filter.GetMaxSearchNodes());
	int32 NumPolys;

	dtStatus Status = NavQuery.findPolysInPathDistance(StartPolyID, RecastStartPos
		, PathingDistance, QueryFilter, FoundPolys.GetTypedData(), &NumPolys, Filter.GetMaxSearchNodes());

	FoundPolys.RemoveAt(NumPolys, FoundPolys.Num() - NumPolys);

	return FoundPolys.Num() > 0;
}

bool FPImplRecastNavMesh::GetClustersWithinPathingDistance(FVector const& StartLoc, const float PathingDistance, bool bBackTracking, TArray<NavNodeRef>& FoundPolys) const
{
	ensure(PathingDistance > 0.0f && "PathingDistance <= 0 doesn't make sense");

	// sanity check
	if (DetourNavMesh == NULL)
	{
		return false;
	}

	INITIALIZE_NAVQUERY(NavQuery, RECAST_MAX_SEARCH_NODES);

	const dtQueryFilter* ClusterFilter = ((const FRecastQueryFilter*)NavMeshOwner->GetDefaultQueryFilterImpl())->GetAsDetourQueryFilter();
	ensure(ClusterFilter);
	if (ClusterFilter == NULL)
	{
		return false;
	}

	dtQueryFilter MyFilter(*ClusterFilter);
	MyFilter.setIsBacktracking(bBackTracking);

	// @todo this should be configurable in some kind of FindPathQuery structure
	const FVector& NavExtent = NavMeshOwner->GetDefaultQueryExtent();
	const float Extent[3] = { NavExtent.X, NavExtent.Z, NavExtent.Y };

	float RecastStartPos[3];
	Unr2RecastVector(StartLoc, RecastStartPos);
	// @TODO add failure handling
	NavNodeRef StartPolyID = INVALID_NAVNODEREF;
	NavQuery.findNearestPoly(RecastStartPos, Extent, &MyFilter, &StartPolyID, NULL);

	FoundPolys.AddUninitialized(256);
	int32 NumPolys;

	dtStatus Status = NavQuery.findClustersInPathDistance(StartPolyID, RecastStartPos, PathingDistance, &MyFilter, FoundPolys.GetTypedData(), &NumPolys, 256);
	FoundPolys.RemoveAt(NumPolys, FoundPolys.Num() - NumPolys);

	return FoundPolys.Num() > 0;
}

void FPImplRecastNavMesh::UpdateNavigationLinkArea(int32 UserId, uint8 AreaType, uint16 PolyFlags) const
{
	if (DetourNavMesh)
	{
		DetourNavMesh->updateOffMeshConnectionByUserId(UserId, AreaType, PolyFlags);
	}
}

void FPImplRecastNavMesh::UpdateSegmentLinkArea(int32 UserId, uint8 AreaType, uint16 PolyFlags) const
{
	if (DetourNavMesh)
	{
		DetourNavMesh->updateOffMeshSegmentConnectionByUserId(UserId, AreaType, PolyFlags);
	}
}

bool FPImplRecastNavMesh::GetPolyCenter(NavNodeRef PolyID, FVector& OutCenter) const
{
	if (DetourNavMesh)
	{
		// get poly data from recast
		dtPoly const* Poly;
		dtMeshTile const* Tile;
		dtStatus Status = DetourNavMesh->getTileAndPolyByRef((dtPolyRef)PolyID, &Tile, &Poly);
		if (dtStatusSucceed(Status))
		{
			// average verts
			float Center[3] = {0,0,0};

			for (uint32 VertIdx=0; VertIdx < Poly->vertCount; ++VertIdx)
			{
				const float* V = &Tile->verts[Poly->verts[VertIdx]*3];
				Center[0] += V[0];
				Center[1] += V[1];
				Center[2] += V[2];
			}
			const float InvCount = 1.0f / Poly->vertCount;
			Center[0] *= InvCount;
			Center[1] *= InvCount;
			Center[2] *= InvCount;

			// convert output to UE4 coords
			OutCenter = Recast2UnrVector(Center);

			return true;
		}
	}

	return false;
}

bool FPImplRecastNavMesh::GetPolyVerts(NavNodeRef PolyID, TArray<FVector>& OutVerts) const
{
	if (DetourNavMesh)
	{
		// get poly data from recast
		dtPoly const* Poly;
		dtMeshTile const* Tile;
		dtStatus Status = DetourNavMesh->getTileAndPolyByRef((dtPolyRef)PolyID, &Tile, &Poly);
		if (dtStatusSucceed(Status))
		{
			// flush and pre-size output array
			OutVerts.Empty(Poly->vertCount);

			// convert to UE4 coords and copy verts into output array 
			for (uint32 VertIdx=0; VertIdx < Poly->vertCount; ++VertIdx)
			{
				const float* V = &Tile->verts[Poly->verts[VertIdx]*3];
				OutVerts.Add( Recast2UnrVector(V) );
			}

			return true;
		}
	}

	return false;
}

uint32 FPImplRecastNavMesh::GetPolyAreaID(NavNodeRef PolyID) const
{
	uint32 AreaID = RECAST_NULL_AREA;

	if (DetourNavMesh)
	{
		// get poly data from recast
		dtPoly const* Poly;
		dtMeshTile const* Tile;
		dtStatus Status = DetourNavMesh->getTileAndPolyByRef((dtPolyRef)PolyID, &Tile, &Poly);
		if (dtStatusSucceed(Status))
		{
			AreaID = Poly->getArea();
		}
	}

	return AreaID;
}

bool FPImplRecastNavMesh::GetClusterCenter(NavNodeRef ClusterRef, bool bUseCenterPoly, FVector& OutCenter) const
{
	if (DetourNavMesh == NULL || !ClusterRef)
	{
		return false;
	}

	const dtMeshTile* Tile = DetourNavMesh->getTileByRef(ClusterRef);
	uint32 ClusterIdx = DetourNavMesh->decodeClusterIdCluster(ClusterRef);

	if (Tile && ClusterIdx < (uint32)Tile->header->clusterCount)
	{
		dtCluster& ClusterData = Tile->clusters[ClusterIdx];
		if (bUseCenterPoly)
		{
			return GetPolyCenter(ClusterData.centerPoly, OutCenter);
		}
		
		OutCenter = Recast2UnrealPoint(Tile->clusters[ClusterIdx].center);
		return true;
	}

	return false;
}

bool FPImplRecastNavMesh::GetClusterBounds(NavNodeRef ClusterRef, FBox& OutBounds) const
{
	if (DetourNavMesh == NULL || !ClusterRef)
	{
		return false;
	}

	const dtMeshTile* Tile = DetourNavMesh->getTileByRef(ClusterRef);
	uint32 ClusterIdx = DetourNavMesh->decodeClusterIdCluster(ClusterRef);

	int32 NumPolys = 0;
	if (Tile && ClusterIdx < (uint32)Tile->header->clusterCount)
	{
		for (int32 i = 0; i < Tile->header->offMeshBase; i++)
		{
			if (Tile->polyClusters[i] == ClusterIdx)
			{
				const dtPoly* Poly = &Tile->polys[i];
				for (int32 iVert = 0; iVert < Poly->vertCount; iVert++)
				{
					const float* V = &Tile->verts[Poly->verts[iVert]*3];
					OutBounds += Recast2UnrealPoint(V);
				}

				NumPolys++;
			}
		}
	}

	return NumPolys > 0;
}

void FPImplRecastNavMesh::GetEdgesForPathCorridor(TArray<NavNodeRef>* PathCorridor, TArray<FNavigationPortalEdge>* PathCorridorEdges) const
{
	// sanity check
	if (DetourNavMesh == NULL)
	{
		return;
	}

	INITIALIZE_NAVQUERY(NavQuery, RECAST_MAX_SEARCH_NODES);

	const int32 CorridorLenght = PathCorridor->Num();

	PathCorridorEdges->Empty(CorridorLenght - 1);
	for (int32 i = 0; i < CorridorLenght - 1; ++i)
	{
		unsigned char FromType = 0, ToType = 0;
		float Left[3] = {0.f}, Right[3] = {0.f};
		
		NavQuery.getPortalPoints((*PathCorridor)[i], (*PathCorridor)[i+1], Left, Right, FromType, ToType);

		PathCorridorEdges->Add(FNavigationPortalEdge(Recast2UnrVector(Left), Recast2UnrVector(Right), (*PathCorridor)[i+1]));
	}
}

bool FPImplRecastNavMesh::FilterPolys(TArray<NavNodeRef>& PolyRefs, const class FRecastQueryFilter* Filter) const
{
	if (Filter == NULL || DetourNavMesh == NULL)
	{
		return false;
	}

	for (int32 PolyIndex = PolyRefs.Num() - 1; PolyIndex >= 0; --PolyIndex)
	{
		dtPolyRef TestRef = PolyRefs[PolyIndex];

		// get poly data from recast
		dtPoly const* Poly = NULL;
		dtMeshTile const* Tile = NULL;
		const dtStatus Status = DetourNavMesh->getTileAndPolyByRef(TestRef, &Tile, &Poly);

		if (dtStatusSucceed(Status))
		{
			const bool bPassedFilter = Filter->passFilter(TestRef, Tile, Poly);
			const bool bWalkableArea = Filter->getAreaCost(Poly->getArea()) > 0.0f;
			if (bPassedFilter && bWalkableArea)
			{
				continue;
			}
		}
		
		PolyRefs.RemoveAt(PolyIndex, 1);
	}

	return true;
}

bool FPImplRecastNavMesh::GetPolysInTile(int32 TileIndex, TArray<FNavPoly>& Polys) const
{
	if (DetourNavMesh == NULL || TileIndex < 0 || TileIndex >= DetourNavMesh->getMaxTiles())
	{
		return false;
	}

	const dtMeshTile* Tile = ((const dtNavMesh*)DetourNavMesh)->getTile(TileIndex);
	const int32 MaxPolys = Tile ? Tile->header->offMeshBase : 0;
	if (MaxPolys > 0)
	{
		// only ground type polys
		int32 BaseIdx = Polys.Num();
		Polys.AddZeroed(MaxPolys);

		dtPoly* Poly = Tile->polys;
		for (int32 i = 0; i < MaxPolys; i++, Poly++)
		{
			FVector PolyCenter(0);
			for (int k = 0; k < Poly->vertCount; ++k)
			{
				PolyCenter += Recast2UnrealPoint(&Tile->verts[Poly->verts[k]*3]);
			}
			PolyCenter /= Poly->vertCount;

			FNavPoly& OutPoly = Polys[BaseIdx + i];
			OutPoly.Ref = DetourNavMesh->encodePolyId(Tile->salt, TileIndex, i);
			OutPoly.Center = PolyCenter;
		}
	}

	return (MaxPolys > 0);
}

/** Internal. Calculates squared 2d distance of given point PT to segment P-Q. Values given in Recast coordinates */
static FORCEINLINE float PointDistToSegment2DSquared(const float* PT, const float* P, const float* Q)
{
	float pqx = Q[0] - P[0];
	float pqz = Q[2] - P[2];
	float dx = PT[0] - P[0];
	float dz = PT[2] - P[2];
	float d = pqx*pqx + pqz*pqz;
	float t = pqx*dx + pqz*dz;
	if (d != 0) t /= d;
	dx = P[0] + t*pqx - PT[0];
	dz = P[2] + t*pqz - PT[2];
	return dx*dx + dz*dz;
}

void FPImplRecastNavMesh::GetDebugTileBounds(FBox& OuterBox, int32& NumTilesX, int32& NumTilesY) const
{
 	if (DetourNavMesh)
 	{
 		dtNavMeshParams const* Params = DetourNavMesh->getParams();
		NumTilesX = FMath::TruncFloat( FMath::Sqrt((float)Params->maxTiles) );
		NumTilesY = NumTilesX;

 		FVector const Mn = Recast2UnrVector(Params->orig);

		float RecastMax[3];
		RecastMax[0] = Params->orig[0] + (Params->tileWidth * NumTilesX);
		RecastMax[1] = Params->orig[1];
		RecastMax[2] = Params->orig[2] + (Params->tileHeight * NumTilesY);
		FVector const Mx = Recast2UnrVector(RecastMax);

		OuterBox = FBox(Mx, Mn);
	}
	else
	{
		NumTilesX = 0;
		NumTilesY = 0;
		OuterBox = FBox();
	}
}

/** 
 * Traverses given tile's edges and detects the ones that are either poly (i.e. not triangle, but whole navmesh polygon) 
 * or navmesh edge. Returns a pair of verts for each edge found.
 */
void FPImplRecastNavMesh::GetDebugPolyEdges(const dtMeshTile* Tile, bool bInternalEdges, bool bNavMeshEdges, TArray<FVector>& InternalEdgeVerts, TArray<FVector>& NavMeshEdgeVerts) const
{
	static const float thr = FMath::Square(0.01f);

	ensure(bInternalEdges || bNavMeshEdges);
	const bool bExportAllEdges = bInternalEdges && !bNavMeshEdges;
	
	for (int i = 0; i < Tile->header->polyCount; ++i)
	{
		const dtPoly* Poly = &Tile->polys[i];

		if (Poly->getType() != DT_POLYTYPE_GROUND)
		{
			continue;
		}

		const dtPolyDetail* pd = &Tile->detailMeshes[i];		
		for (int j = 0, nj = (int)Poly->vertCount; j < nj; ++j)
		{
			bool bIsExternal = !bExportAllEdges && (Poly->neis[j] == 0 || Poly->neis[j] & DT_EXT_LINK);
			bool bIsConnected = !bIsExternal;

			if (Poly->getArea() == RECAST_NULL_AREA)
			{
				if (Poly->neis[j] && !(Poly->neis[j] & DT_EXT_LINK) &&
					Poly->neis[j] <= Tile->header->offMeshBase &&
					Tile->polys[Poly->neis[j] - 1].getArea() != RECAST_NULL_AREA)
				{
					bIsExternal = true;
					bIsConnected = false;
				}
				else if (Poly->neis[j] == 0)
				{
					bIsExternal = true;
					bIsConnected = false;
				}
			}
			else if (bIsExternal)
			{
				unsigned int k = Poly->firstLink;
				while (k != DT_NULL_LINK)
				{
					const dtLink& link = DetourNavMesh->getLink(Tile, k);
					k = link.next;

					if (link.edge == j)
					{
						bIsConnected = true;
						break;
					}
				}
			}

			TArray<FVector>* EdgeVerts = bInternalEdges && bIsConnected ? &InternalEdgeVerts 
				: (bNavMeshEdges && bIsExternal && !bIsConnected ? &NavMeshEdgeVerts : NULL);
			if (EdgeVerts == NULL)
			{
				continue;
			}

			const float* V0 = &Tile->verts[Poly->verts[j]*3];
			const float* V1 = &Tile->verts[Poly->verts[(j+1) % nj]*3];

			// Draw detail mesh edges which align with the actual poly edge.
			// This is really slow.
			for (int32 k = 0; k < pd->triCount; ++k)
			{
				const unsigned char* t = &(Tile->detailTris[(pd->triBase+k)*4]);
				const float* tv[3];

				for (int32 m = 0; m < 3; ++m)
				{
					if (t[m] < Poly->vertCount)
					{
						tv[m] = &Tile->verts[Poly->verts[t[m]]*3];
					}
					else
					{
						tv[m] = &Tile->detailVerts[(pd->vertBase+(t[m] - Poly->vertCount))*3];
					}
				}
				for (int m = 0, n = 2; m < 3; n=m++)
				{
					if (((t[3] >> (n*2)) & 0x3) == 0)
					{
						continue;	// Skip inner detail edges.
					}
					
					if (PointDistToSegment2DSquared(tv[n],V0,V1) < thr && PointDistToSegment2DSquared(tv[m],V0,V1) < thr)
					{
						int32 const AddIdx = (*EdgeVerts).AddZeroed(2);
						(*EdgeVerts)[AddIdx] = Recast2UnrVector(tv[n]);
						(*EdgeVerts)[AddIdx+1] = Recast2UnrVector(tv[m]);
					}
				}
			}
		}
	}
}

uint8 GetValidEnds(const dtNavMesh* NavMesh, const dtMeshTile* Tile, const dtPoly* Poly)
{
	if ((Poly->getType() & DT_POLYTYPE_GROUND) != 0)
	{
		return false;
	}

	uint8 ValidEnds = FRecastDebugGeometry::OMLE_None;

	unsigned int k = Poly->firstLink;
	while (k != DT_NULL_LINK)
	{
		const dtLink& link = NavMesh->getLink(Tile, k);
		k = link.next;

		if (link.edge == 0)
		{
			ValidEnds |= FRecastDebugGeometry::OMLE_Left;
		}
		if (link.edge == 1)
		{
			ValidEnds |= FRecastDebugGeometry::OMLE_Right;
		}
	}

	return ValidEnds;
}

/** 
 * @param PolyEdges			[out] Array of worldspace vertex locations for tile edges.  Edges are pairwise verts, i.e. [0,1], [2,3], etc
 * @param NavMeshEdges		[out] Array of worldspace vertex locations for the edge of the navmesh.  Edges are pairwise verts, i.e. [0,1], [2,3], etc
 * 
 * @todo PolyEdges and NavMeshEdges could probably be Index arrays into MeshVerts and be generated in the master loop instead of separate traversals. 
 */
void FPImplRecastNavMesh::GetDebugGeometry(FRecastDebugGeometry& OutGeometry, int32 TileIndex) const
{
	if (DetourNavMesh)
	{
		if (TileIndex >= DetourNavMesh->getMaxTiles())
		{
			return;
		}
				
		check(NavMeshOwner);
		dtNavMesh const* ConstNavMesh = DetourNavMesh;
		
		// presize our tarrays for efficiency
		const int32 NumTiles = TileIndex == INDEX_NONE ? DetourNavMesh->getMaxTiles() : TileIndex + 1;
		const int32 StartingTile = TileIndex == INDEX_NONE ? 0 : TileIndex;

		int32 NumVertsToReserve = 0;
		int32 NumIndicesToReserve = 0;
		for (int32 TileIdx=StartingTile; TileIdx < NumTiles; ++TileIdx)
		{
			dtMeshTile const* const Tile = ConstNavMesh->getTile(TileIdx);
			dtMeshHeader const* const Header = Tile->header;

			if (Header != NULL)
			{
				NumVertsToReserve += Header->vertCount + Header->detailVertCount;

				for (int32 PolyIdx=0; PolyIdx < Header->polyCount; ++PolyIdx)
				{
					dtPolyDetail const* const DetailPoly = &Tile->detailMeshes[PolyIdx];
					NumIndicesToReserve += (DetailPoly->triCount * 3);
				}
			}
		}
		int32 NumClusters = 0;
		for (int32 TileIdx=StartingTile; TileIdx < NumTiles; ++TileIdx)
		{
			dtMeshTile const* const Tile = ConstNavMesh->getTile(TileIdx);
			dtMeshHeader const* const Header = Tile->header;

			if (Header != NULL)
			{
				NumClusters = FMath::Max(Header->clusterCount, NumClusters);
			}
		}

		OutGeometry.MeshVerts.Reserve(NumVertsToReserve);
		OutGeometry.AreaIndices[0].Reserve(NumIndicesToReserve);
		OutGeometry.BuiltMeshIndices.Reserve(NumIndicesToReserve);
		OutGeometry.Clusters.AddZeroed(NumClusters);

#if WITH_NAVIGATION_GENERATOR
		const FRecastNavMeshGenerator* Generator = NavMeshOwner ? (const FRecastNavMeshGenerator*)(NavMeshOwner->GetGenerator()) : NULL;
#endif

		// spin through all polys in all tiles and draw them
		// @see drawMeshTile() in recast code for reference
		uint32 AllTilesVertBase = 0;
		for (int32 TileIdx=StartingTile; TileIdx < NumTiles; ++TileIdx)
		{
			dtMeshTile const* const Tile = ConstNavMesh->getTile(TileIdx);
			dtMeshHeader const* const Header = Tile->header;

			if (Header == NULL)
			{
				continue;
			}

#if WITH_NAVIGATION_GENERATOR
			const bool bIsBeingBuilt = Generator != NULL && !!NavMeshOwner->bDistinctlyDrawTilesBeingBuilt 
				&& Generator->IsTileFresh(Header->x, Header->y);
#else
			const bool bIsBeingBuilt = false;
#endif
			
			// add all the poly verts
			float* F = Tile->verts;
			for (int32 VertIdx=0; VertIdx < Header->vertCount; ++VertIdx)
			{
				FVector const VertPos = Recast2UnrVector(F);
				OutGeometry.MeshVerts.Add(VertPos);
				F += 3;
			}
			int32 const DetailVertIndexBase = Header->vertCount;
			// add the detail verts
			F = Tile->detailVerts;
			for (int32 DetailVertIdx=0; DetailVertIdx < Header->detailVertCount; ++DetailVertIdx)
			{
				FVector const VertPos = Recast2UnrVector(F);
				OutGeometry.MeshVerts.Add(VertPos);
				F += 3;
			}

			// add all the indices
			for (int32 PolyIdx=0; PolyIdx < Header->polyCount; ++PolyIdx)
			{
				dtPoly const* const Poly = &Tile->polys[PolyIdx];
				
				if (Poly->getType() == DT_POLYTYPE_GROUND)
				{
					dtPolyDetail const* const DetailPoly = &Tile->detailMeshes[PolyIdx];

					TArray<int32>* Indices = bIsBeingBuilt ? &OutGeometry.BuiltMeshIndices : &OutGeometry.AreaIndices[Poly->getArea()];

					// one triangle at a time
					for (int32 TriIdx=0; TriIdx < DetailPoly->triCount; ++TriIdx)
					{
						int32 DetailTriIdx = (DetailPoly->triBase + TriIdx) * 4;
						const unsigned char* DetailTri = &Tile->detailTris[DetailTriIdx];

						// calc indices into the vert buffer we just populated
						int32 TriVertIndices[3];
						for (int32 TriVertIdx=0; TriVertIdx<3; ++TriVertIdx)
						{
							if (DetailTri[TriVertIdx] < Poly->vertCount)
							{
								TriVertIndices[TriVertIdx] = AllTilesVertBase + Poly->verts[ DetailTri[TriVertIdx] ];
							}
							else
							{
								TriVertIndices[TriVertIdx] = AllTilesVertBase + DetailVertIndexBase + (DetailPoly->vertBase + DetailTri[TriVertIdx] - Poly->vertCount);
							}
						}

						Indices->Add(TriVertIndices[0]);
						Indices->Add(TriVertIndices[1]);
 						Indices->Add(TriVertIndices[2]);

						if (Tile->polyClusters && OutGeometry.Clusters.IsValidIndex(Tile->polyClusters[PolyIdx]))
						{
							TArray<int32>& ClusterIndices = OutGeometry.Clusters[Tile->polyClusters[PolyIdx]].MeshIndices;
							ClusterIndices.Add(TriVertIndices[0]);
							ClusterIndices.Add(TriVertIndices[1]);
							ClusterIndices.Add(TriVertIndices[2]);
						}
 					}
				}
 			}

			for (int32 i = 0; i < Header->offMeshConCount; ++i)
			{
				const dtOffMeshConnection* OffMeshConnection = &Tile->offMeshCons[i];

				if (OffMeshConnection != NULL)
				{
					dtPoly const* const LinkPoly = &Tile->polys[OffMeshConnection->poly];
					const float* va = &Tile->verts[LinkPoly->verts[0]*3]; //OffMeshConnection->pos;
					const float* vb = &Tile->verts[LinkPoly->verts[1]*3]; //OffMeshConnection->pos[3];

					const FRecastDebugGeometry::FOffMeshLink Link = {
						Recast2UnrVector(va)
						, Recast2UnrVector(vb)
						, LinkPoly->getArea()
						, (uint8)OffMeshConnection->getBiDirectional()
						, GetValidEnds(DetourNavMesh, Tile, LinkPoly)
						, OffMeshConnection->rad
					};

					OutGeometry.OffMeshLinks.Add(Link);
				}
			}

			for (int32 i = 0; i < Header->offMeshSegConCount; ++i)
			{
				const dtOffMeshSegmentConnection* OffMeshSeg = &Tile->offMeshSeg[i];
				if (OffMeshSeg != NULL)
				{
					const int32 polyBase = Header->offMeshSegPolyBase + OffMeshSeg->firstPoly;
					for (int32 j = 0; j < OffMeshSeg->npolys; j++)
					{
						dtPoly const* const LinkPoly = &Tile->polys[polyBase + j];

						FRecastDebugGeometry::FOffMeshSegment Link;
						Link.LeftStart  = Recast2UnrealPoint(&Tile->verts[LinkPoly->verts[0]*3]);
						Link.LeftEnd    = Recast2UnrealPoint(&Tile->verts[LinkPoly->verts[1]*3]);
						Link.RightStart = Recast2UnrealPoint(&Tile->verts[LinkPoly->verts[2]*3]);
						Link.RightEnd   = Recast2UnrealPoint(&Tile->verts[LinkPoly->verts[3]*3]);
						Link.AreaID		= LinkPoly->getArea();
						Link.Direction	= (uint8)OffMeshSeg->getBiDirectional();
						Link.ValidEnds  = GetValidEnds(DetourNavMesh, Tile, LinkPoly);
						
						const int LinkIdx = OutGeometry.OffMeshSegments.Add(Link);
						OutGeometry.OffMeshSegmentAreas[Link.AreaID].Add(LinkIdx);
					}
				}
			}
 
			for (int32 i = 0; i < Header->clusterCount; i++)
			{
				const dtCluster& c0 = Tile->clusters[i];
				uint32 iLink = c0.firstLink;
				while (iLink != DT_NULL_LINK)
				{
					const dtClusterLink& link = DetourNavMesh->getClusterLink(Tile, iLink);
					iLink = link.next;

					dtMeshTile const* const OtherTile = ConstNavMesh->getTileByRef(link.ref);
					if (OtherTile)
					{
						int32 linkedIdx = ConstNavMesh->decodeClusterIdCluster(link.ref);
						const dtCluster& c1 = OtherTile->clusters[linkedIdx];

						FRecastDebugGeometry::FClusterLink LinkGeom;
						LinkGeom.FromCluster = Recast2UnrVector(c0.center);
						LinkGeom.ToCluster = Recast2UnrVector(c1.center);

						if (linkedIdx > i || TileIdx > (int32)ConstNavMesh->decodeClusterIdTile(link.ref))
						{
							FVector UpDir(0,0,1.0f);
							FVector LinkDir = (LinkGeom.ToCluster - LinkGeom.FromCluster).SafeNormal();
							FVector SideDir = FVector::CrossProduct(LinkDir, UpDir);
							LinkGeom.FromCluster += SideDir * 40.0f;
							LinkGeom.ToCluster += SideDir * 40.0f;
						}

						OutGeometry.ClusterLinks.Add(LinkGeom);
					}
				}
			}

 			// Get tile edges and navmesh edges
			if (OutGeometry.bGatherPolyEdges || OutGeometry.bGatherNavMeshEdges)
			{
				GetDebugPolyEdges(Tile, !!OutGeometry.bGatherPolyEdges, !!OutGeometry.bGatherNavMeshEdges
					, OutGeometry.PolyEdges, OutGeometry.NavMeshEdges);
			}
 
 			AllTilesVertBase += Header->vertCount + Header->detailVertCount;
		}
 	}
}

FBox FPImplRecastNavMesh::GetNavMeshBounds() const
{
	FBox Bbox(0);

	// @todo, calc once and cache it
	if (DetourNavMesh)
	{
		// workaround for privacy issue in the recast API
		dtNavMesh const* const ConstRecastNavMesh = DetourNavMesh;

		// spin through all the tiles and accumulate the bounds
		for (int32 TileIdx=0; TileIdx < DetourNavMesh->getMaxTiles(); ++TileIdx)
		{
			dtMeshTile const* const Tile = ConstRecastNavMesh->getTile(TileIdx);
			if (Tile)
			{
				dtMeshHeader const* const Header = Tile->header;
				if (Header)
				{
					const FBox NodeBox = Recast2UnrealBox(Header->bmin, Header->bmax);
					Bbox += NodeBox;
				}
			}
		}
	}

	return Bbox;
}

FBox FPImplRecastNavMesh::GetNavMeshTileBounds(int32 TileIndex) const
{
	FBox Bbox(0);

	if (DetourNavMesh && TileIndex >= 0 && TileIndex < DetourNavMesh->getMaxTiles())
	{
		// workaround for privacy issue in the recast API
		dtNavMesh const* const ConstRecastNavMesh = DetourNavMesh;

		dtMeshTile const* const Tile = ConstRecastNavMesh->getTile(TileIndex);
		if (Tile)
		{
			dtMeshHeader const* const Header = Tile->header;
			if (Header)
			{
				Bbox = Recast2UnrealBox(Header->bmin, Header->bmax);
			}
		}
	}

	return Bbox;
}

/** Retrieves XY coordinates of tile specified by index */
void FPImplRecastNavMesh::GetNavMeshTileXY(int32 TileIndex, int32& OutX, int32& OutY, int32& OutLayer) const
{
	if (DetourNavMesh && TileIndex >= 0 && TileIndex < DetourNavMesh->getMaxTiles())
	{
		// workaround for privacy issue in the recast API
		dtNavMesh const* const ConstRecastNavMesh = DetourNavMesh;

		dtMeshTile const* const Tile = ConstRecastNavMesh->getTile(TileIndex);
		if (Tile)
		{
			dtMeshHeader const* const Header = Tile->header;
			if (Header)
			{
				OutX = Header->x;
				OutY = Header->y;
				OutLayer = Header->layer;
			}
		}
	}
}

float FPImplRecastNavMesh::GetTotalDataSize() const
{
	float TotalBytes = sizeof(*this);

	if (DetourNavMesh)
	{
		// iterate all tiles and sum up their DataSize
		dtNavMesh const* ConstNavMesh = DetourNavMesh;
		for (int i = 0; i < ConstNavMesh->getMaxTiles(); ++i)
		{
			const dtMeshTile* Tile = ConstNavMesh->getTile(i);
			if (Tile != NULL && Tile->header != NULL)
			{
				TotalBytes += Tile->dataSize;
			}
		}
	}

	return TotalBytes / 1024;
}

void FPImplRecastNavMesh::ApplyWorldOffset(const FVector& InOffset, bool bWorldShift)
{
	if (DetourNavMesh != NULL)
	{
		// transform offset to Recast space
		const FVector OffsetRC = Unreal2RecastPoint(InOffset);
		// apply offset
		DetourNavMesh->applyWorldOffset(&OffsetRC.X);
	}

}

uint16 FPImplRecastNavMesh::GetFilterForbiddenFlags(const FRecastQueryFilter* Filter)
{
	return ((const dtQueryFilter*)Filter)->getExcludeFlags();
}

void FPImplRecastNavMesh::SetFilterForbiddenFlags(FRecastQueryFilter* Filter, uint16 ForbiddenFlags)
{
	((dtQueryFilter*)Filter)->setExcludeFlags(ForbiddenFlags);
	// include-exclude don't need to be symmetrical, filter will check both conditions
}

#endif // WITH_RECAST
