// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NavAreas/NavArea.h"
#include "RecastNavMesh.generated.h"

/** Initial checkin. */
#define NAVMESHVER_INITIAL				1
#define NAVMESHVER_TILED_GENERATION		2
#define NAVMESHVER_SEAMLESS_REBUILDING_1 3
#define NAVMESHVER_AREA_CLASSES			4
#define NAVMESHVER_CLUSTER_PATH			5
#define NAVMESHVER_SEGMENT_LINKS		6
#define NAVMESHVER_DYNAMIC_LINKS		7
#define NAVMESHVER_64BIT				9

#define NAVMESHVER_LATEST				NAVMESHVER_64BIT
#define NAVMESHVER_MIN_COMPATIBLE		NAVMESHVER_64BIT

#define RECAST_MAX_SEARCH_NODES		2048
#define RECAST_MAX_PATH_VERTS		64

#define RECAST_MIN_TILE_SIZE		300.f

#define RECAST_MAX_AREAS			64
#define RECAST_DEFAULT_AREA			(RECAST_MAX_AREAS - 1)
#define RECAST_NULL_AREA			0
#define RECAST_STRAIGHTPATH_OFFMESH_CONNECTION 0x04
#define RECAST_UNWALKABLE_POLY_COST	FLT_MAX

UENUM()
namespace ERecastPartitioning
{
	// keep in sync with rcRegionPartitioning enum!
	enum Type
	{
		Monotone,
		Watershed,
		ChunkyMonotone,
	};
}

/** helper to translate FNavPathPoint.Flags */
struct ENGINE_API FNavMeshNodeFlags
{
	/** Extra node information (like "path start", "off-mesh connection") */
	uint8 PathFlags;
	/** Area type after this node */
	uint8 Area;
	/** Area flags for this node */
	uint16 AreaFlags;

	FNavMeshNodeFlags(uint32 Flags) : PathFlags(Flags), Area(Flags >> 8), AreaFlags(Flags >> 16) {}
	uint32 Pack() const { return PathFlags | ((uint32)Area << 8) | ((uint32)AreaFlags << 16); }
};

struct ENGINE_API FNavMeshPath : public FNavigationPath
{
	typedef FNavigationPath Super;

	FNavMeshPath();

	FORCEINLINE void SetWantsStringPulling(const bool bNewWantsStringPulling) { bWantsStringPulling = bNewWantsStringPulling; }
	FORCEINLINE bool WantsStringPulling() const { return bWantsStringPulling; }
	FORCEINLINE bool IsStringPulled() const { return bStrigPulled; }

	FORCEINLINE void SetWantsPathCorridor(const bool bNewWantsPathCorridor) { bWantsPathCorridor = bNewWantsPathCorridor; }
	FORCEINLINE bool WantsPathCorridor() const { return bWantsPathCorridor; }
	
	FORCEINLINE const TArray<FNavigationPortalEdge>* GetPathCorridorEdges() { return bCorridorEdgesGenerated ? &PathCorridorEdges : GeneratePathCorridorEdges(); }
	FORCEINLINE void SetPathCorridorEdges(const TArray<FNavigationPortalEdge>& InPathCorridorEdges) { PathCorridorEdges = InPathCorridorEdges; bCorridorEdgesGenerated = true; }

	FORCEINLINE void OnPathCorridorUpdated() { bCorridorEdgesGenerated = false; }

	virtual void DebugDraw(const ANavigationData* NavData, FColor PathColor, UCanvas* Canvas, bool bPersistent, const uint32 NextPathPointIndex = 0) const OVERRIDE;
	
	bool ContainsWithSameEnd(const FNavMeshPath* Other) const;

	/** A helper function in one of special-case-algorithms. Pops last edge in PathCorridorEdges */
	void PopCorridorEdge();

	void OffsetFromCorners(float Distance);

	/** get cost of path, starting from next poly in corridor */
	virtual float GetCostFromNode(NavNodeRef PathNode) const { return GetCostFromIndex(PathCorridor.Find(PathNode) + 1); }

	/** get cost of path, starting from given point */
	virtual float GetCostFromIndex(int32 PathPointIndex) const
	{
		if (PathPointIndex >= PathCorridorCost.Num() - 1)
		{
			return 0.f;
		}

		float TotalCost = 0.f;
		const float* Cost = PathCorridorCost.GetTypedData();
		for (int32 PolyIndex = PathPointIndex; PolyIndex < PathCorridorCost.Num(); ++PolyIndex, ++Cost)
		{
			TotalCost += *Cost;
		}

		return TotalCost;
	}

	FORCEINLINE_DEBUGGABLE float GetTotalPathLength() const
	{
		return bStrigPulled ? GetStringPulledLength(0) : GetPathCorridorLength(0);
	}

	FORCEINLINE int32 GetNodeRefIndex(const NavNodeRef NodeRef) const { return PathCorridor.Find(NodeRef); }

	/** check if path (all polys in corridor) contains given node */
	virtual bool ContainsNode(NavNodeRef NodeRef) const { return PathCorridor.Contains(NodeRef); }

	bool IsPathSegmentANavLink(const int32 PathSegmentStartIndex) const;

protected:
	/** calculates total length of string pulled path. Does not generate string pulled 
	 *	path if it's not already generated (see bWantsStringPulling and bStrigPulled)
	 *	Internal use only */
	float GetStringPulledLength(const int32 StartingPoint) const;

	/** calculates estimated length of path expressed as sequence of navmesh edges.
	 *	It basically sums up distances between every subsequent nav edge pair edge middles.
	 *	Internal use only */
	float GetPathCorridorLength(const int32 StartingEdge) const;

	const TArray<FNavigationPortalEdge>* GeneratePathCorridorEdges();

public:
	
	/** sequence of navigation mesh poly ids representing an obstacle-free navigation corridor */
	TArray<NavNodeRef> PathCorridor;

	/** for every poly in PathCorridor stores traversal cost from previous navpoly */
	TArray<float> PathCorridorCost;

private:
	/** sequence of FVector pairs where each pair represents navmesh portal edge between two polygons navigation corridor.
	 *	Note, that it should always be accessed via GetPathCorridorEdges() since PathCorridorEdges content is generated
	 *	on first access */
	TArray<FNavigationPortalEdge> PathCorridorEdges;

	/** transient variable indicating whether PathCorridorEdges contains up to date information */
	uint32 bCorridorEdgesGenerated : 1;

public:
	/** is this path generated on dynamic navmesh (i.e. one attached to moving surface) */
	uint32 bDynamic : 1;

protected:
	/** does this path contain string pulled path? If true then NumPathVerts > 0 and
	 *	OutPathVerts contains valid data. If false there's only navigation corridor data
	 *	available.*/
	uint32 bStrigPulled : 1;	

	/** If set to true path instance will contain a string pulled version. Otherwise only
	 *	navigation corridor will be available. Defaults to true */
	uint32 bWantsStringPulling : 1;

	/** If set to true path instance will contain path corridor generated as a part 
	 *	pathfinding call (i.e. without the need to generate it with GeneratePathCorridorEdges */
	uint32 bWantsPathCorridor : 1;

public:
	static const FNavPathType Type;
};

#if WITH_RECAST
struct FRecastDebugPathfindingNode
{
	NavNodeRef PolyRef;
	NavNodeRef ParentRef;
	float Cost;
	float TotalCost;
	uint32 bOpenSet : 1;
	uint32 bOffMeshLink : 1;
	uint32 bModified : 1;

	FVector NodePos;
	TArray<FVector> Verts;

	FORCEINLINE bool operator==(const FRecastDebugPathfindingNode& Other) const { return PolyRef == Other.PolyRef; }
	FORCEINLINE friend uint32 GetTypeHash(const FRecastDebugPathfindingNode& Other) { return Other.PolyRef; }
};

struct FRecastDebugPathfindingStep
{
	TSet<FRecastDebugPathfindingNode> Nodes;
	FSetElementId BestNode;
};

struct FRecastDebugGeometry
{
	enum EOffMeshLinkEnd
	{
		OMLE_None = 0x0,
		OMLE_Left = 0x1,
		OMLE_Right = 0x2,
		OMLE_Both = OMLE_Left | OMLE_Right
	};

	struct FOffMeshLink
	{
		FVector Left;
		FVector Right;
		uint8	AreaID;
		uint8	Direction;
		uint8	ValidEnds;
		float	Radius;
		FColor	Color;
	};

	struct FCluster
	{
		TArray<int32> MeshIndices;
	};

	struct FClusterLink
	{
		FVector FromCluster;
		FVector ToCluster;
	};

	struct FOffMeshSegment
	{
		FVector LeftStart, LeftEnd;
		FVector RightStart, RightEnd;
		uint8	AreaID;
		uint8	Direction;
		uint8	ValidEnds;
	};

	TArray<FVector> MeshVerts;
	TArray<int32> AreaIndices[RECAST_MAX_AREAS];
	TArray<int32> BuiltMeshIndices;
	TArray<FVector> PolyEdges;
	TArray<FVector> NavMeshEdges;
	TArray<FOffMeshLink> OffMeshLinks;
	TArray<FCluster> Clusters;
	TArray<FClusterLink> ClusterLinks;
	TArray<FOffMeshSegment> OffMeshSegments;
	TArray<int32> OffMeshSegmentAreas[RECAST_MAX_AREAS];

	int32 bGatherPolyEdges : 1;
	int32 bGatherNavMeshEdges : 1;

	FRecastDebugGeometry() : bGatherPolyEdges(false), bGatherNavMeshEdges(false)
	{}

	uint32 GetAllocatedSize() const;
};

struct FNavPoly
{
	NavNodeRef Ref;
	FVector Center;
};

namespace ERecastNamedFilter
{
	enum Type 
	{
		FilterOutNavLinks = 0,		// filters out all off-mesh connections
		FilterOutAreas,				// filters out all navigation areas except the default one (RECAST_DEFAULT_AREA)
		FilterOutNavLinksAndAreas,	// combines FilterOutNavLinks and FilterOutAreas

		NamedFiltersCount,
	};
}
#endif //WITH_RECAST


/**
 *	Structure to handle nav mesh tile's raw data persistence and releasing
 */
struct FNavMeshTileData
{
	// helper function so that we release NavData via dtFree not regular delete (for navigation mem stats)
	struct FNavData
	{
		uint8* RawNavData;

		FNavData(uint8* InNavData) : RawNavData(InNavData) {}
		~FNavData();
	};

	// layer index
	int32 LayerIndex;
	// size of allocated data
	int32 DataSize;
	// actual tile data
	TSharedPtr<FNavData, ESPMode::ThreadSafe> NavData;

	FNavMeshTileData() : LayerIndex(0), DataSize(0) { }

	explicit FNavMeshTileData(uint8* RawData, int32 RawDataSize, int32 LayerIdx = 0)
		: LayerIndex(LayerIdx), DataSize(RawDataSize)
	{
		NavData = MakeShareable(new FNavData(RawData));
	}

	FORCEINLINE uint8* GetData()
	{
		check(NavData.IsValid());
		return NavData->RawNavData;
	}

	FORCEINLINE const uint8* GetData() const
	{
		check(NavData.IsValid());
		return NavData->RawNavData;
	}

	FORCEINLINE const uint8* GetDataSafe() const
	{
		return NavData.IsValid() ? NavData->RawNavData : NULL;
	}
	
	FORCEINLINE bool operator==(const uint8* RawData) const
	{
		return GetData() == RawData;
	}

	FORCEINLINE bool IsValid() const { return NavData.IsValid() && GetData() != NULL && DataSize > 0; }
	FORCEINLINE void Release() { NavData.Reset(); DataSize = 0; LayerIndex = 0; }
};

struct FRecastTickHelper : FTickableGameObject
{
	class ARecastNavMesh* Owner;

	FRecastTickHelper() : Owner(NULL) {}
	virtual void Tick(float DeltaTime);
	virtual bool IsTickable() const { return (Owner != NULL); }
	virtual bool IsTickableInEditor() const { return true; }
	virtual TStatId GetStatId() const ;
};

struct FTileSetItem
{
	/** tile coords on grid */
	int16 X, Y;
	/** 2D location of tile center */
	FVector2D TileCenter;
	/** squared 2D distance to closest generation seed (e.g. player) */
	float Dist2DSq;
	/** sort order */
	int32 SortOrder;
	/** does it have compressed geometry stored in generator? */
	uint8 bHasCompressedGeometry : 1;
	/** does it have generated navmesh? */
 	uint8 bHasNavmesh : 1;

	FTileSetItem(int32 InitialX = 0, int32 InitialY = 0, int32 InitialOrder = 0)
		: X(InitialX), Y(InitialY), SortOrder(InitialOrder), bHasCompressedGeometry(false), bHasNavmesh(false)
	{
	}

	FTileSetItem(int16 TileX, int16 TileY, const FBox& TileBounds)
		: X(TileX), Y(TileY), TileCenter(TileBounds.GetCenter()), SortOrder(0), bHasCompressedGeometry(false), bHasNavmesh(false)
	{
	}
};

UCLASS(config=Engine, hidecategories=(Transform,"Utilities|Orientation",Actor,Layers,Replication))
class ENGINE_API ARecastNavMesh : public ANavigationData
{
	GENERATED_UCLASS_BODY()

	typedef uint16 FNavPolyFlags;

	virtual void Serialize( FArchive& Ar ) OVERRIDE;

	/** should we draw edges of every navmesh's triangle */
	UPROPERTY(EditAnywhere, Category=Display)
	uint32 bDrawTriangleEdges:1;

	/** should we draw edges of every poly (i.e. not only border-edges)  */
	UPROPERTY(EditAnywhere, Category=Display)
	uint32 bDrawPolyEdges:1;

	/** should we draw border-edges */
	UPROPERTY(EditAnywhere, Category=Display)
	uint32 bDrawNavMeshEdges:1;

	/** should we draw the tile boundaries */
	UPROPERTY(EditAnywhere, Category=Display)
	uint32 bDrawTileBounds:1;
	
	/** Draw input geometry passed to the navmesh generator.  Recommend disabling other geometry rendering via viewport showflags in editor. */
	UPROPERTY(EditAnywhere, Category=Display)
	uint32 bDrawPathCollidingGeometry:1;

	UPROPERTY(EditAnywhere, Category=Display)
	uint32 bDrawTileLabels:1;

	UPROPERTY(EditAnywhere, Category=Display)
	uint32 bDrawPolygonLabels:1;

	UPROPERTY(EditAnywhere, Category=Display)
	uint32 bDrawDefaultPolygonCost:1;

	UPROPERTY(EditAnywhere, Category=Display)
	uint32 bDrawLabelsOnPathNodes:1;

	UPROPERTY(EditAnywhere, Category=Display)
	uint32 bDrawNavLinks:1;

	UPROPERTY(EditAnywhere, Category=Display)
	uint32 bDrawFailedNavLinks:1;
	
	UPROPERTY(EditAnywhere, Category=Display)
	uint32 bDrawClusters:1;

	UPROPERTY(config)
	uint32 bDistinctlyDrawTilesBeingBuilt:1;

	/** vertical offset added to navmesh's debug representation for better readability */
	UPROPERTY(EditAnywhere, Category=Display, config)
	float DrawOffset;
	
	//----------------------------------------------------------------------//
	// NavMesh generation parameters
	//----------------------------------------------------------------------//

	/** size of single tile, expressed in uu */
	UPROPERTY(EditAnywhere, Category=Generation, config, meta=(ClampMin = "300.0"))
	float TileSizeUU;

	/** horizontal size of voxelization cell */
	UPROPERTY(EditAnywhere, Category=Generation, config, meta=(ClampMin = "1.0"))
	float CellSize;

	/** vertical size of voxelization cell */
	UPROPERTY(EditAnywhere, Category=Generation, config, meta=(ClampMin = "1.0"))
	float CellHeight;

	/** Radius of smallest agent to traverse this navmesh */
	UPROPERTY(EditAnywhere, Category=Generation, config, meta=(ClampMin = "0.0"))
	float AgentRadius;

	UPROPERTY(EditAnywhere, Category=Generation, config, meta=(ClampMin = "0.0"))
	float AgentHeight;

	/** Size of the tallest agent that will path with this navmesh. */
	UPROPERTY(EditAnywhere, Category=Generation, config, meta=(ClampMin = "0.0"))
	float AgentMaxHeight;

	/* The maximum slope (angle) that the agent can move on. */ 
	UPROPERTY(EditAnywhere, Category=Generation, config, meta=(ClampMin = "0.0", ClampMax = "89.0", UIMin = "0.0", UIMax = "89.0" ))
	float AgentMaxSlope;

	UPROPERTY(EditAnywhere, Category=Generation, config, meta=(ClampMin = "0.0"))
	float AgentMaxStepHeight;

	/* The minimum dimension of area. Areas smaller than this will be discarded */
	UPROPERTY(EditAnywhere, Category=Generation, config, meta=(ClampMin = "0.0"))
	float MinRegionArea;

	/* The size limit of regions to be merged with bigger regions */
	UPROPERTY(EditAnywhere, Category=Generation, config, meta=(ClampMin = "0.0"))
	float MergeRegionSize;

	/** How much navigable shapes can get simplified - the higher the value the more freedom */
	UPROPERTY(EditAnywhere, Category=Generation, config)
	float MaxSimplificationError;

	/** navmesh draw distance in game (always visible in editor) */
	UPROPERTY(config)
	float DefaultDrawDistance;

	/** specifes default limit to A* nodes used when performing navigation queries. 
	 *	Can be overridden by passing custom FNavigationQueryFilter */
	UPROPERTY(config)
	float DefaultMaxSearchNodes;

	/** partitioning method for creating navmesh polys */
	UPROPERTY(EditAnywhere, Category=Generation, config, AdvancedDisplay)
	TEnumAsByte<ERecastPartitioning::Type> RegionPartitioning;

	/** partitioning method for creating tile layers */
	UPROPERTY(EditAnywhere, Category=Generation, config, AdvancedDisplay)
	TEnumAsByte<ERecastPartitioning::Type> LayerPartitioning;

	/** number of chunk splits (along single axis) used for region's partitioning: ChunkyMonotone */
	UPROPERTY(EditAnywhere, Category=Generation, config, AdvancedDisplay)
	int32 RegionChunkSplits;

	/** number of chunk splits (along single axis) used for layer's partitioning: ChunkyMonotone */
	UPROPERTY(EditAnywhere, Category=Generation, config, AdvancedDisplay)
	int32 LayerChunkSplits;

	/** Limit for tile grid size: width */
	UPROPERTY(config)
	int32 MaxTileGridWidth;

	/** Limit for tile grid size: height */
	UPROPERTY(config)
	int32 MaxTileGridHeight;

	/** Controls whether Navigation Areas will be sorted by cost before application 
	 *	to navmesh during navmesh generation. This is relevant then there are
	 *	areas overlapping and we want to have area cost express area relevancy
	 *	as well. Setting it to true will result in having area sorted by cost,
	 *	but it will also increase navmesh generation cost a bit */
	UPROPERTY(EditAnywhere, Category=Generation, config)
	uint32 bSortNavigationAreasByCost:1;

	/** controls whether voxel filterring will be applied (via FRecastTileGenerator::ApplyVoxelFilter). 
	 *	Results in generated navemesh better fitting navigation bounds, but hits (a bit) generation performance */
	UPROPERTY(EditAnywhere, Category=Generation, config)
	uint32 bPerformVoxelFiltering:1;

	/** TODO: switch to disable new code from OffsetFromCorners if necessary - remove it later */
	UPROPERTY(config)
	uint32 bUseBetterOffsetsFromCorners : 1;

private:
	/** Cache rasterized voxels instead of just collision vertices/indices in navigation octree */
	UPROPERTY(config)
	uint32 bUseVoxelCache : 1;

	/** internal data to store time for next sort for our set with tiles */
	float NextTimeToSortTiles;

	/** indicates how often we will sort navigation tiles to mach players position */
	UPROPERTY(config)
	float TileSetUpdateInterval;

	/** contains last available dtPoly's flag bit set (8th bit at the moment of writing) */
	static FNavPolyFlags NavLinkFlag;

	/** Squared draw distance */
	static float DrawDistanceSq;

public:

	struct FRaycastResult
	{
		enum 
		{
			MAX_PATH_CORRIDOR_POLYS = 128
		};

		NavNodeRef CorridorPolys[MAX_PATH_CORRIDOR_POLYS];
		float CorridorCost[MAX_PATH_CORRIDOR_POLYS];
		int32 CorridorPolysCount;
		float HitTime;
		FVector HitNormal;

		FRaycastResult()
			: CorridorPolysCount(0)
			, HitTime(FLT_MAX)
			, HitNormal(0.f)
		{
			FMemory::MemZero(CorridorPolys);
			FMemory::MemZero(CorridorCost);
		}

		FORCEINLINE int32 GetMaxCorridorSize() const { return MAX_PATH_CORRIDOR_POLYS; }
		FORCEINLINE bool HasHit() const { return HitTime != FLT_MAX; }
	};

	//----------------------------------------------------------------------//
	// Recast runtime params
	//----------------------------------------------------------------------//
	/** Euclidean distance heuristic scale used while pathfinding */
	UPROPERTY(EditAnywhere, Category=Pathfinding, config, meta=(ClampMin = "0.1"))
	float HeuristicScale;

	FORCEINLINE static void SetDrawDistance(float NewDistance) { DrawDistanceSq = NewDistance * NewDistance; }
	FORCEINLINE static float GetDrawDistanceSq() { return DrawDistanceSq; }

	//////////////////////////////////////////////////////////////////////////

#if WITH_RECAST
public:
	//----------------------------------------------------------------------//
	// Life cycle & serialization
	//----------------------------------------------------------------------//
	/** scans the world and creates appropriate RecastNavMesh instances */
	static ANavigationData* CreateNavigationInstances(UNavigationSystem* NavSys);

	/** Dtor */
	virtual ~ARecastNavMesh();

	// Begin UObject Interface
	virtual void PostInitProperties() OVERRIDE;
	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) OVERRIDE;
#if WITH_EDITOR
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	// End UObject Interface

	/** Handles storing and releasing raw navigation tile data */
	void UpdateNavTileData(uint8* OldTileData, FNavMeshTileData NewTileData);

#if WITH_EDITOR
	/** RecastNavMesh instances are dynamically spawned and should not be coppied */
	virtual bool ShouldExport() OVERRIDE { return false; }
#endif

	/** called by TickHelper, works in game AND in editor */
	void TickMe(float DeltaTime);

	virtual void CleanUp() OVERRIDE;

	// Begin ANavigationData Interface
	virtual FNavLocation GetRandomPoint(TSharedPtr<const FNavigationQueryFilter> Filter = NULL) const OVERRIDE;
	virtual bool GetRandomPointInRadius(const FVector& Origin, float Radius, FNavLocation& OutResult, TSharedPtr<const FNavigationQueryFilter> Filter = NULL) const OVERRIDE;

	virtual bool ProjectPoint(const FVector& Point, FNavLocation& OutLocation, const FVector& Extent, TSharedPtr<const FNavigationQueryFilter> Filter = NULL) const OVERRIDE;
	
	virtual ENavigationQueryResult::Type CalcPathCost(const FVector& PathStart, const FVector& PathEnd, float& OutPathCost, TSharedPtr<const FNavigationQueryFilter> Filter = NULL) const OVERRIDE;
	virtual ENavigationQueryResult::Type CalcPathLength(const FVector& PathStart, const FVector& PathEnd, float& OutPathLength, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL) const OVERRIDE;
	virtual ENavigationQueryResult::Type CalcPathLengthAndCost(const FVector& PathStart, const FVector& PathEnd, float& OutPathLength, float& OutPathCost, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL) const OVERRIDE;

	virtual UPrimitiveComponent* ConstructRenderingComponent() OVERRIDE;
	/** Returns bounding box for the navmesh. */
	virtual FBox GetBounds() const OVERRIDE { return GetNavMeshBounds(); }
	/** Called on world origin changes **/
	virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) OVERRIDE;

protected:
#if WITH_NAVIGATION_GENERATOR
	virtual class FNavDataGenerator* ConstructGenerator(const FNavAgentProperties& AgentProps) OVERRIDE;
	virtual void SortNavigationTiles();
#endif // WITH_NAVIGATION_GENERATOR
	// End ANavigationData Interface

	/** Serialization helper. */
	void SerializeRecastNavMesh(FArchive& Ar, class FPImplRecastNavMesh*& NavMesh);

public:
	/** Returns bounding box for the whole navmesh. */
	FBox GetNavMeshBounds() const;

	/** Returns bounding box for a given navmesh tile. */
	FBox GetNavMeshTileBounds(int32 TileIndex) const;

	/** Retrieves XY coordinates of tile specified by index */
	void GetNavMeshTileXY(int32 TileIndex, int32& OutX, int32& OutY, int32& Layer) const;

	/** Retrieves number of tiles in this navmesh */
	int32 GetNavMeshTilesCount() const;

	void GetEdgesForPathCorridor(TArray<NavNodeRef>* PathCorridor, TArray<struct FNavigationPortalEdge>* PathCorridorEdges) const;

	// @todo docuement
	//void SetRecastNavMesh(class FPImplRecastNavMesh* RecastMesh);

	void UpdateDrawing();

	/** Creates a task to be executed on GameThread calling UpdateDrawing */
	void RequestDrawingUpdate();

	virtual void SetConfig(const FNavDataConfig& Src) OVERRIDE;
protected:
	virtual void FillConfig(FNavDataConfig& Dest) OVERRIDE;

	FORCEINLINE const FNavigationQueryFilter& GetRightFilterRef(TSharedPtr<const FNavigationQueryFilter> Filter) const 
	{
		return *(Filter.IsValid() ? Filter.Get() : GetDefaultQueryFilter().Get());
	}

public:

	static bool IsVoxelCacheEnabled();

	//----------------------------------------------------------------------//
	// Debug                                                                
	//----------------------------------------------------------------------//
	/** Debug rendering support. */
	void GetDebugGeometry(FRecastDebugGeometry& OutGeometry, int32 TileIndex = INDEX_NONE) const;
	// @todo docuement
	void GetDebugTileBounds(FBox& OuterBox, int32& NumTilesX, int32& NumTilesY) const;

	// @todo docuement
	void DrawDebugPathCorridor(NavNodeRef const* PathPolys, int32 NumPathPolys, bool bPersistent=true) const;

#if !UE_BUILD_SHIPPING
	virtual uint32 LogMemUsed() const OVERRIDE;
#endif // !UE_BUILD_SHIPPING

	void UpdateNavMeshDrawing();

	//----------------------------------------------------------------------//
	// Utilities
	//----------------------------------------------------------------------//
	virtual void OnNavAreaAdded(const UClass* NavAreaClass, int32 AgentIndex) OVERRIDE;
	virtual int32 GetNewAreaID(const UClass* AreaClass) const OVERRIDE;
	virtual int32 GetMaxSupportedAreas() const OVERRIDE { return RECAST_MAX_AREAS; }
	
	/** Get forbidden area flags from default query filter */
	uint16 GetDefaultForbiddenFlags() const;
	/** Change forbidden area flags in default query filter */
	void SetDefaultForbiddenFlags(uint16 ForbiddenAreaFlags);

	//----------------------------------------------------------------------//
	// Smart links
	//----------------------------------------------------------------------//

	virtual void UpdateSmartLink(class USmartNavLinkComponent* LinkComp) OVERRIDE;

	/** update area class and poly flags for all offmesh links with given UserId */
	void UpdateNavigationLinkArea(int32 UserId, TSubclassOf<class UNavArea> AreaClass) const;

	/** update area class and poly flags for all offmesh segment links with given UserId */
	void UpdateSegmentLinkArea(int32 UserId, TSubclassOf<class UNavArea> AreaClass) const;

	//----------------------------------------------------------------------//
	// Batch processing (important with async rebuilding)
	//----------------------------------------------------------------------//

	/** Starts batch processing and locks access to navmesh from other threads */
	void BeginBatchQuery() const;

	/** Finishes batch processing and release locks */
	void FinishBatchQuery() const;

	//----------------------------------------------------------------------//
	// Querying                                                                
	//----------------------------------------------------------------------//

	FColor GetAreaIDColor(uint8 AreaID) const;

	/** Returns nearest navmesh polygon to Loc, or INVALID_NAVMESHREF if Loc is not on the navmesh. */
	NavNodeRef FindNearestPoly(FVector const& Loc, FVector const& Extent, TSharedPtr<const FNavigationQueryFilter> Filter = NULL) const;

	/** Retrieves center of the specified polygon. Returns false on error. */
	bool GetPolyCenter(NavNodeRef PolyID, FVector& OutCenter) const;

	/** Retrieves the vertices for the specified polygon. Returns false on error. */
	bool GetPolyVerts(NavNodeRef PolyID, TArray<FVector>& OutVerts) const;

	/** Retrieves area ID for the specified polygon. */
	uint32 GetPolyAreaID(NavNodeRef PolyID) const;

	/** Retrieves center of cluster (either middle of center poly, or calculated from all vertices). Returns false on error. */
	bool GetClusterCenter(NavNodeRef ClusterRef, bool bUseCenterPoly, FVector& OutCenter) const;

	/** Retrieves bounds of cluster. Returns false on error. */
	bool GetClusterBounds(NavNodeRef ClusterRef, FBox& OutBounds) const;

	/** Get random point in given cluster */
	bool GetRandomPointInCluster(NavNodeRef ClusterRef, FNavLocation& OutLocation) const;

	/** Get cluster ref containing given poly ref */
	NavNodeRef GetClusterRef(NavNodeRef PolyRef) const;

	/** Retrieves all polys within given pathing distance from StartLocation.
	 *	@NOTE query is not using string-pulled path distance (for performance reasons),
	 *		it measured distance between middles of portal edges, do you might want to 
	 *		add an extra margin to PathingDistance */
	bool GetPolysWithinPathingDistance(FVector const& StartLoc, const float PathingDistance, TArray<NavNodeRef>& FoundPolys, TSharedPtr<const FNavigationQueryFilter> Filter = NULL) const;

	/** Retrieves all clusters within given pathing distance from StartLocation. */
	bool GetClustersWithinPathingDistance(FVector const& StartLoc, const float PathingDistance, TArray<NavNodeRef>& FoundClusters, bool bBackTracking = false) const;

	/** Filters nav polys in PolyRefs with Filter */
	bool FilterPolys(TArray<NavNodeRef>& PolyRefs, const class FRecastQueryFilter* Filter) const;

	/** Get all polys from tile */
	bool GetPolysInTile(int32 TileIndex, TArray<FNavPoly>& Polys) const;

	/** Projects point on navmesh, returning all hits along vertical line defined by min-max Z params */
	bool ProjectPointMulti(const FVector& Point, TArray<FNavLocation>& OutLocations, const FVector& Extent,
		float MinZ, float MaxZ, TSharedPtr<const FNavigationQueryFilter> Filter = NULL) const;

	// @todo docuement
	static FPathFindingResult FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query);
	static FPathFindingResult FindHierarchicalPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query);
	static bool TestPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query);
	static bool TestHierarchicalPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query);
	static bool NavMeshRaycast(const ANavigationData* Self, const FVector& RayStart, const FVector& RayEnd, FVector& HitLocation, TSharedPtr<const FNavigationQueryFilter> QueryFilter);
	
	/** @return true is specified segment is fully on navmesh (respecting the optional filter) */
	bool IsSegmentOnNavmesh(const FVector& SegmentStart, const FVector& SegmentEnd, TSharedPtr<const FNavigationQueryFilter> Filter = NULL) const;

	/** Runs A* pathfinding on navmesh and collect data for every step */
	int32 DebugPathfinding(const FPathFindingQuery& Query, TArray<FRecastDebugPathfindingStep>& Steps);

	/** Checks whether this instance of navmesh supports given Agent type */
	bool IsGeneratedFor(const FNavAgentProperties& AgentProps) const;

	static const class FRecastQueryFilter* GetNamedFilter(ERecastNamedFilter::Type FilterType);
	FORCEINLINE static FNavPolyFlags GetNavLinkFlag() { return NavLinkFlag; }
	
	/** initialize tiles set with given size */
	void ReserveTileSet(int32 NewGridWidth, int32 NewGridHeight);

	/** get information about given tile index */
	FTileSetItem* GetTileSetItemAt(int32 TileX, int32 TileY);
	
	const FTileSetItem* GetTileSet() const { return TileSet.GetTypedData(); }
	FTileSetItem* GetTileSet() { return TileSet.GetTypedData(); }
	
	virtual bool NeedsRebuild() OVERRIDE;

	/** update offset for navmesh in recast library, to current one */
	/** @param Offset - current offset */
	void UpdateNavmeshOffset( const FBox& NavBounds );

protected:
	// @todo docuement
	UPrimitiveComponent* ConstructRenderingComponentImpl();

	virtual void DestroyGenerator() OVERRIDE;

	/** Spawns an ARecastNavMesh instance, and configures it if AgentProps != NULL */
	static ARecastNavMesh* SpawnInstance(UNavigationSystem* NavSys, const FNavDataConfig* AgentProps = NULL);

private:
	friend class FRecastNavMeshGenerator;
	// retrieves RecastNavMeshImpl
	FPImplRecastNavMesh* GetRecastNavMeshImpl() { return RecastNavMeshImpl; }
	// destroys FPImplRecastNavMesh instance if it has been created 
	void DestroyRecastPImpl();
	// @todo docuement
	void UpdateNavVersion();

protected:
	TArray<FTileSetItem> TileSet;

private:
	int32 GridWidth;
	int32 GridHeight;

	/** NavMesh versioning. */
	uint32 NavMeshVersion;
	FRecastTickHelper TickHelper;
	
	/** 
	 * This is a pimpl-style arrangement used to tightly hide the Recast internals from the rest of the engine.
	 * Using this class should *not* require the inclusion of the private RecastNavMesh.h
	 *	@NOTE: if we switch over to C++11 this should be unique_ptr
	 */
	class FPImplRecastNavMesh* RecastNavMeshImpl;

	/** array containing tile references of all navmesh tiles recently rebuild. Handled and cleared in Tick function */
	TArray<uint32> FreshTiles;

	/** Holds shared pointers to navmesh tile data currently used
	 *	@TODO will no longer be needed if we switch over to support only async pathfinding */	
	TNavStatArray<FNavMeshTileData> TileNavDataContainer;
		
#if RECAST_ASYNC_REBUILDING
	mutable FCriticalSection* NavDataReadWriteLock;
	
	/** this critical section will be used befor navmesh generator creation */
	FCriticalSection NavDataReadWriteLockDummy;

	/** batch query counter */
	mutable int32 BatchQueryCounter;

#endif // RECAST_ASYNC_REBUILDING

public:
	static FNavigationTypeCreator Creator;
private:
	static const FRecastQueryFilter* NamedFilters[ERecastNamedFilter::NamedFiltersCount];

#endif // WITH_RECAST

};
