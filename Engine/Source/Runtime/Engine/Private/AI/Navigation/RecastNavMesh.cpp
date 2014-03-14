// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "RecastHelpers.h"

#if WITH_EDITOR
#include "ObjectEditorUtils.h"
#endif

#if WITH_RECAST
#include "DetourAlloc.h"
#endif 

#if WITH_EDITOR
#include "UnrealEd.h"
#endif

#if RECAST_ASYNC_REBUILDING

#define SECTION_LOCK_TILES			FConditionalScopeLock ReadWriteLock(NavDataReadWriteLock, BatchQueryCounter <= 0);
#define SECTION_LOCK_TILES_FOR(x)	FConditionalScopeLock ReadWriteLock(x->NavDataReadWriteLock, x->BatchQueryCounter <= 0);

#else

#define SECTION_LOCK_TILES
#define SECTION_LOCK_TILES_FOR(x)

#endif // RECAST_ASYNC_REBUILDING

namespace
{
	FORCEINLINE FVector2D TileIndexToXY(int32 GridWidth, int32 Index)
	{
		return FVector2D(Index % GridWidth, Index / GridWidth);
	}

	FORCEINLINE int32 TileXYToIndex(int32 GridWidth, int32 X, int32 Y)
	{
		return Y * GridWidth + X;
	}
}

FNavMeshTileData::FNavData::~FNavData()
{
#if WITH_RECAST
	dtFree(RawNavData);
#else
	delete RawNavData;
#endif
}

// Similar to FScopeLock, but locks only when condition if set
class FConditionalScopeLock
{
public:
	FConditionalScopeLock(FCriticalSection* InSyncObject, bool bShouldLock) 
		: SyncObject(InSyncObject), bLock(bShouldLock)
	{
		if (bLock)
		{
			check(SyncObject);
			SyncObject->Lock();
		}
	}

	~FConditionalScopeLock()
	{
		if (bLock)
		{
			check(SyncObject);
			SyncObject->Unlock();
		}
	}

private:
	FConditionalScopeLock();
	FConditionalScopeLock(FConditionalScopeLock* InScopeLock);
	FConditionalScopeLock& operator= (FConditionalScopeLock& InScopeLock) { return *this; }

	FCriticalSection* SyncObject;
	bool bLock;
};

float ARecastNavMesh::DrawDistanceSq = 0.0f;
#if !WITH_RECAST

ARecastNavMesh::ARecastNavMesh(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
}

void ARecastNavMesh::Serialize( FArchive& Ar )
{
	Super::Serialize(Ar);

	uint32 NavMeshVersion;
	Ar << NavMeshVersion;

	//@todo: How to handle loading nav meshes saved w/ recast when recast isn't present????

	// when writing, write a zero here for now.  will come back and fill it in later.
	uint32 RecastNavMeshSizeBytes = 0;
	int32 RecastNavMeshSizePos = Ar.Tell();
	Ar << RecastNavMeshSizeBytes;

	if (Ar.IsLoading())
	{
		// incompatible, just skip over this data.  navmesh needs rebuilt.
		Ar.Seek( RecastNavMeshSizePos + RecastNavMeshSizeBytes );

		// Mark self for delete
		CleanUpAndMarkPendingKill();
	}
}

#else // WITH_RECAST

#include "PImplRecastNavMesh.h"
#include "RecastNavMeshGenerator.h"
#include "DetourNavMeshQuery.h"

#define DO_NAVMESH_DEBUG_DRAWING_PER_TILE 0

static const FColor NavMeshRenderColor_RecastMesh(72,255,64);

//----------------------------------------------------------------------//
// FRecastDebugGeometry
//----------------------------------------------------------------------//
uint32 FRecastDebugGeometry::GetAllocatedSize() const
{
	uint32 Size = sizeof(*this) + MeshVerts.GetAllocatedSize()
		+ BuiltMeshIndices.GetAllocatedSize()
		+ PolyEdges.GetAllocatedSize()
		+ NavMeshEdges.GetAllocatedSize()
		+ OffMeshLinks.GetAllocatedSize()
		+ OffMeshSegments.GetAllocatedSize()
		+ Clusters.GetAllocatedSize()
		+ ClusterLinks.GetAllocatedSize();

	for (int i = 0; i < RECAST_MAX_AREAS; ++i)
	{
		Size += AreaIndices[i].GetAllocatedSize();
	}

	for (int i = 0; i < Clusters.Num(); ++i)
	{
		Size += Clusters[i].MeshIndices.GetAllocatedSize();
	}

	return Size;
}
//----------------------------------------------------------------------//
// ARecastNavMesh
//----------------------------------------------------------------------//

void FRecastTickHelper::Tick(float DeltaTime)
{
	Owner->TickMe(DeltaTime);
}

TStatId FRecastTickHelper::GetStatId() const 
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FRecastTickHelper, STATGROUP_Tickables);
}


namespace ERecastNamedFilter
{
	FRecastQueryFilter FilterOutNavLinksImpl;
	FRecastQueryFilter FilterOutAreasImpl;
	FRecastQueryFilter FilterOutNavLinksAndAreasImpl;
}

const FRecastQueryFilter* ARecastNavMesh::NamedFilters[] = {
	&ERecastNamedFilter::FilterOutNavLinksImpl
	, &ERecastNamedFilter::FilterOutAreasImpl
	, &ERecastNamedFilter::FilterOutNavLinksAndAreasImpl
};

namespace
{
struct FRecastNamedFiltersCreator
{
	ARecastNavMesh::FNavPolyFlags NavLinkFlag;

	FRecastNamedFiltersCreator()
	{
		// setting up the last bit available in dtPoly::flags
		NavLinkFlag = ARecastNavMesh::FNavPolyFlags(1 << (sizeof(((dtPoly*)0)->flags) * 8 - 1));

		ERecastNamedFilter::FilterOutNavLinksImpl.setExcludeFlags(NavLinkFlag);
		ERecastNamedFilter::FilterOutNavLinksAndAreasImpl.setExcludeFlags(NavLinkFlag);

		for (int32 AreaID = 0; AreaID < RECAST_MAX_AREAS; ++AreaID)
		{
			ERecastNamedFilter::FilterOutAreasImpl.setAreaCost(AreaID, RECAST_UNWALKABLE_POLY_COST);
			ERecastNamedFilter::FilterOutNavLinksAndAreasImpl.setAreaCost(AreaID, RECAST_UNWALKABLE_POLY_COST);
		}

		ERecastNamedFilter::FilterOutAreasImpl.setAreaCost(RECAST_DEFAULT_AREA, 1.f);
		ERecastNamedFilter::FilterOutNavLinksAndAreasImpl.setAreaCost(RECAST_DEFAULT_AREA, 1.f);
	}
};
}
static const FRecastNamedFiltersCreator RecastNamedFiltersCreator;

ARecastNavMesh::FNavPolyFlags ARecastNavMesh::NavLinkFlag = RecastNamedFiltersCreator.NavLinkFlag;

FNavigationTypeCreator ARecastNavMesh::Creator(FCreateNavigationDataInstanceDelegate::CreateStatic(&ARecastNavMesh::CreateNavigationInstances));

ARecastNavMesh::ARecastNavMesh(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, bDrawNavMeshEdges(true)
	, bDrawNavLinks(true)
	, bDistinctlyDrawTilesBeingBuilt(true)
	, DrawOffset(10.f)
	, MaxSimplificationError(1.3f)	// from RecastDemo
	, DefaultMaxSearchNodes(RECAST_MAX_SEARCH_NODES)
	, bPerformVoxelFiltering(true)	
	, NextTimeToSortTiles(0.f)
	, TileSetUpdateInterval(1.0f)
	, GridWidth(-1)
	, GridHeight(-1)
	, NavMeshVersion(NAVMESHVER_LATEST)	
	, RecastNavMeshImpl(NULL)
{
	HeuristicScale = 0.999f;
	RegionPartitioning = ERecastPartitioning::Watershed;
	LayerPartitioning = ERecastPartitioning::Watershed;
	RegionChunkSplits = 2;
	LayerChunkSplits = 2;

#if RECAST_ASYNC_REBUILDING
	BatchQueryCounter = 0;
#endif // RECAST_ASYNC_REBUILDING


	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		INC_DWORD_STAT_BY( STAT_NavigationMemory, sizeof(*this) );

		FindPathImplementation = FindPath;
		FindHierarchicalPathImplementation = FindHierarchicalPath;

		TestPathImplementation = TestPath;
		TestHierarchicalPathImplementation = TestHierarchicalPath;

		RaycastImplementation = NavMeshRaycast;

		RecastNavMeshImpl = new FPImplRecastNavMesh(this);
		DefaultQueryFilter->SetFilterType<FRecastQueryFilter>();
		TickHelper.Owner = this;
	}
}

ARecastNavMesh::~ARecastNavMesh()
{
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		for (int32 i = 0; i < TileNavDataContainer.Num(); i++)
		{
			DEC_MEMORY_STAT_BY(STAT_Navigation_NavDataMemory, TileNavDataContainer[i].DataSize);
		}

		DEC_DWORD_STAT_BY( STAT_NavigationMemory, sizeof(*this) );
		DestroyRecastPImpl();
	}
}

void ARecastNavMesh::DestroyRecastPImpl()
{
	if (RecastNavMeshImpl != NULL)
	{
		delete RecastNavMeshImpl;
		RecastNavMeshImpl = NULL;
	}
}

void ARecastNavMesh::TickMe(float DeltaTime)
{
#if WITH_NAVIGATION_GENERATOR
	// sanity check
	if (RecastNavMeshImpl == NULL)
	{
		return;
	}

	if (NavDataGenerator.IsValid() && ((FRecastNavMeshGenerator*)NavDataGenerator.Get())->HasResultsPending() == true)
	{
		TNavStatArray<FNavMeshGenerationResult> AsyncResults;
		((FRecastNavMeshGenerator*)NavDataGenerator.Get())->GetAsyncResultsCopy(AsyncResults, /*bClearSource=*/true);

		FNavMeshGenerationResult* Result = AsyncResults.GetTypedData();
		for (int32 ResultIndex = 0; ResultIndex < AsyncResults.Num(); ++ResultIndex, ++Result)
		{
			UpdateNavTileData(Result->OldRawNavData, Result->NewNavData);
			FreshTiles.Add(Result->TileIndex);
		}
	}

	const int32 FreshTilesCount = FreshTiles.Num();
	if (FreshTilesCount > 0)
	{
		const TArray<uint32> FreshTilesCopy(FreshTiles);
		FreshTiles.Reset();
		const int32 PathsCount = ActivePaths.Num();

		FNavPathWeakPtr* WeakPathPtr = (ActivePaths.GetTypedData() + PathsCount - 1);
		for (int32 PathIndex = PathsCount - 1; PathIndex >= 0; --PathIndex, --WeakPathPtr)
		{
			FNavPathSharedPtr SharedPath = WeakPathPtr->Pin();
			if (WeakPathPtr->IsValid() == false)
			{
				ActivePaths.RemoveAtSwap(PathIndex, 1, /*bAllowShrinking=*/false);
			}
			else 
			{
				// iterate through all tile refs in FreshTilesCopy and 
				const FNavMeshPath* Path = (const FNavMeshPath*)(SharedPath.Get());
				if (Path->IsReady() == false)
				{
					// path not filled yet anyway
					continue;
				}

				const int32 PathLenght = Path->PathCorridor.Num();
				const NavNodeRef* PathPoly = Path->PathCorridor.GetTypedData();
				for (int32 NodeIndex = 0; NodeIndex < PathLenght; ++NodeIndex, ++PathPoly)
				{
					const uint32 NodeTileIdx = RecastNavMeshImpl->GetTileIndexFromPolyRef(*PathPoly);
					if (FreshTilesCopy.Contains(NodeTileIdx))
					{
						SharedPath->Invalidate();
						ActivePaths.RemoveAtSwap(PathIndex, 1, /*bAllowShrinking=*/false);
						break;
					}
				}
			}
		}
	}

	NextTimeToSortTiles += DeltaTime;
	if (NextTimeToSortTiles >= TileSetUpdateInterval)
	{
		SortNavigationTiles();
		NextTimeToSortTiles = 0;
	}
#endif // WITH_NAVIGATION_GENERATOR
}

#if WITH_NAVIGATION_GENERATOR

void ARecastNavMesh::SortNavigationTiles()
{
	struct FTileSetItemsSortPredicate
	{
		TArray<FTileSetItem>& TileSet;

		FTileSetItemsSortPredicate(TArray<FTileSetItem>& InTileSet)
			: TileSet(InTileSet)
		{}

		bool operator()(int32 A, int32 B) const 
		{
			// lesser distance = better
			return TileSet[A].Dist2DSq < TileSet[B].Dist2DSq;
		}
	};

	TArray<FVector2D> SeedLocations;

	UWorld* CurWorld = GetWorld();
	for (FConstPlayerControllerIterator PlayerIt = CurWorld->GetPlayerControllerIterator(); PlayerIt; ++PlayerIt)
	{
		if (*PlayerIt && (*PlayerIt)->GetPawn() != NULL)
		{
			const FVector2D SeedLoc((*PlayerIt)->GetPawn()->GetActorLocation());
			SeedLocations.Add(SeedLoc);
		}
	}

	const int32 NumSeeds = SeedLocations.Num();
	if (NumSeeds <= 0)
	{
		return;
	}
	
	// changes to TileSet must be thread safe, because tile workers are accessing it
	{
		SECTION_LOCK_TILES;

		TArray<int32> SortedIndices;
		SortedIndices.AddZeroed(TileSet.Num());

		for (int32 TileIndex = 0; TileIndex < TileSet.Num(); TileIndex++)
		{
			FTileSetItem& TileItem = TileSet[TileIndex];
			float BestDist2DSq = FLT_MAX;
			for (int32 SeedIndex = 0; SeedIndex < SeedLocations.Num(); SeedIndex++)
			{
				const float Dist2DSq = FVector2D::DistSquared(TileItem.TileCenter, SeedLocations[SeedIndex]);
				if (BestDist2DSq > Dist2DSq)
				{
					BestDist2DSq = Dist2DSq;
				}
			}

			TileItem.Dist2DSq = BestDist2DSq;
			SortedIndices[TileIndex] = TileIndex;
		}

		// always sort to keep array ordered (tile removals by workers)
		SortedIndices.Sort(FTileSetItemsSortPredicate(TileSet));

		for (int32 SortIndex = 0; SortIndex < TileSet.Num(); SortIndex++)
		{
			FTileSetItem& TileItem = TileSet[SortedIndices[SortIndex]];
			TileItem.SortOrder = SortIndex;
		}
	}
}

#endif // WITH_NAVIGATION_GENERATOR

ARecastNavMesh* ARecastNavMesh::SpawnInstance(UNavigationSystem* NavSys, const FNavDataConfig* AgentProps)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.OverrideLevel = NavSys->GetWorld()->PersistentLevel;
	ARecastNavMesh* Instance = NavSys->GetWorld()->SpawnActor<ARecastNavMesh>( SpawnInfo );

	if (Instance != NULL && AgentProps != NULL)
	{
		Instance->SetConfig(*AgentProps);
		if (AgentProps->Name != NAME_None)
		{
			FString StrName = FString::Printf(TEXT("%s-%s"), *(Instance->GetFName().GetPlainNameString()), *(AgentProps->Name.ToString()));
			// temporary solution to make sure we don't try to change name while there's already
			// an object with this name
			UObject* ExistingObject = StaticFindObject(/*Class=*/ NULL, Instance->GetOuter(), *StrName, true);
			if (ExistingObject != NULL)
			{
				ExistingObject->Rename(NULL, NULL, REN_DontCreateRedirectors | REN_ForceGlobalUnique | REN_DoNotDirty | REN_NonTransactional);
			}

			// Set descriptive name
			Instance->Rename(*StrName);
#if WITH_EDITOR
			Instance->SetActorLabel(StrName);
#endif // WITH_EDITOR
		}
	}

	return Instance;
}

ANavigationData* ARecastNavMesh::CreateNavigationInstances(UNavigationSystem* NavSys)
{
	if (NavSys == NULL || NavSys->GetWorld() == NULL)
	{
		return NULL;
	}

	const TArray<FNavDataConfig>* SupportedAgents = &NavSys->SupportedAgents;
	const int SupportedAgentsCount = SupportedAgents->Num();
	bool bShouldNotifyEditor = false;

	if (SupportedAgentsCount > 0)
	{
		// Bit array might be a bit of an overkill here, but this function will be called very rarely
		TBitArray<> AlreadyInstantiated(false, SupportedAgentsCount);
		uint8 NumberFound = 0;

		// 1. check whether any of required navmeshes has already been instantiated
		for (FActorIterator It(NavSys->GetWorld()); It && NumberFound < SupportedAgentsCount; ++It)
		{
			ARecastNavMesh* Nav = Cast<ARecastNavMesh>(*It);
			if (Nav != NULL && Nav->GetTypedOuter<UWorld>() == NavSys->GetWorld() && Nav->IsPendingKill() == false)
			{
				// find out which one it is
				const FNavDataConfig* AgentProps = SupportedAgents->GetTypedData();
				for (int i = 0; i < SupportedAgentsCount; ++i, ++AgentProps)
				{
					if (AlreadyInstantiated[i] == true)
					{
						// already present, skip
						continue;
					}

					if (Nav->IsGeneratedFor(*AgentProps) == true)
					{
						AlreadyInstantiated[i] = true;
						++NumberFound;
						break;
					}
				}				
			}
		}

		// 2. for all those that are required and don't have their instances yet just create them
		if (NumberFound < SupportedAgentsCount)
		{
			for (int i = 0; i < SupportedAgentsCount; ++i)
			{
				if (AlreadyInstantiated[i] == false)
				{
					ARecastNavMesh* Instance = SpawnInstance(NavSys, &(*SupportedAgents)[i]);
					if (Instance != NULL)
					{
						NavSys->RequestRegistration(Instance);
						bShouldNotifyEditor = true;
					}
				}
			}
		}
	}
	else
	{
		ARecastNavMesh* Instance = NULL;
		for( FActorIterator It(NavSys->GetWorld()) ; It; ++It )
		{
			ARecastNavMesh* Nav = Cast<ARecastNavMesh>(*It);
			if( Nav != NULL && Nav->GetTypedOuter<UWorld>() == NavSys->GetWorld())
			{
				Instance = Nav;
				break;
			}
		}

		if (Instance == NULL)
		{
			Instance = SpawnInstance(NavSys);
			bShouldNotifyEditor = true;
		}
		else
		{
			NavSys->RequestRegistration(Instance);
		}
	}

	return NULL;
}

UPrimitiveComponent* ARecastNavMesh::ConstructRenderingComponent() 
{
	return ConstructRenderingComponentImpl();
}

UPrimitiveComponent* ARecastNavMesh::ConstructRenderingComponentImpl() 
{
#if DO_NAVMESH_DEBUG_DRAWING_PER_TILE
	const bool bIsGameThread = IsInGameThread();

	if (RecastNavMeshImpl != NULL)
	{
		bool bComponentsRemoved = false;

		for (int i = 0; i < TileRenderingComponents.Num(); ++i)
		{
			if (TileRenderingComponents[i] != NULL)
			{
				TileRenderingComponents[i]->UnregisterComponent();
				Components.Remove(TileRenderingComponents[i]);
				bComponentsRemoved = true;
			}
		}

		TileRenderingComponents.Reset();
		RecastNavMeshImpl->GenerateTileRenderingComponents(TileRenderingComponents);

		//Components.Append(TileRenderingComponents);
		for (int i = 0; i < TileRenderingComponents.Num(); ++i)
		{
			if (TileRenderingComponents[i] != NULL)
			{
				TileRenderingComponents[i]->RegisterComponent();
			}
		}

		if (bComponentsRemoved || TileRenderingComponents.Num() > 0)
		{
			//RegisterAllComponents();
			//MarkComponentsRenderStateDirty();
		}
	}

	return NULL;
#else
	return NewNamedObject<UNavMeshRenderingComponent>(this, TEXT("NavMeshRenderer"));
#endif // DO_NAVMESH_DEBUG_DRAWING_PER_TILE
}

void ARecastNavMesh::UpdateNavMeshDrawing()
{
#if !UE_BUILD_SHIPPING
#if DO_NAVMESH_DEBUG_DRAWING_PER_TILE
	// @todo - we shouldn't be updating _every_ tile renderer, just the ones that have been changed
	for (int i = 0; i < TileRenderingComponents.Num(); ++i)
	{
		if (TileRenderingComponents[i] != NULL && TileRenderingComponents[i]->bVisible)
		{
			TileRenderingComponents[i]->MarkRenderStateDirty();
		}
	}
#else // DO_NAVMESH_DEBUG_DRAWING_PER_TILE
	if (RenderingComp != NULL && RenderingComp->bVisible)
	{
		RenderingComp->MarkRenderStateDirty();
	}
#endif // DO_NAVMESH_DEBUG_DRAWING_PER_TILE
#endif // UE_BUILD_SHIPPING
}

#if WITH_NAVIGATION_GENERATOR
FNavDataGenerator* ARecastNavMesh::ConstructGenerator(const FNavAgentProperties& AgentProps)
{
	FRecastNavMeshGenerator* Generator = NULL;
	// Need to know properties to generate navmesh
	if (AgentProps.IsValid())
	{
		Generator = new FRecastNavMeshGenerator(this);
		if (Generator != NULL)
		{
#if RECAST_ASYNC_REBUILDING
			NavDataReadWriteLock = Generator->GetNavMeshModificationLock();
#endif // RECAST_ASYNC_REBUILDING
		}
	}

	return Generator;
}
#endif

void ARecastNavMesh::DestroyGenerator()
{
#if RECAST_ASYNC_REBUILDING
	// don't try to destroy generator with running async workers
	FRecastNavMeshGenerator* MyGenerator = NavDataGenerator.IsValid() ? (FRecastNavMeshGenerator*)NavDataGenerator.Get() : NULL;
	if (MyGenerator)
	{
		while (MyGenerator->IsAsyncBuildInProgress())
		{
			UE_LOG(LogNavigation, Log, TEXT("Can't destroy generator with active async worker, waiting..."));
			FPlatformProcess::Sleep(0.010f);
		}
	}
#endif

	Super::DestroyGenerator();

#if RECAST_ASYNC_REBUILDING
	NavDataReadWriteLock = &NavDataReadWriteLockDummy;
#endif // RECAST_ASYNC_REBUILDING
}

void ARecastNavMesh::CleanUp()
{
	Super::CleanUp();

	DestroyRecastPImpl();
}

void ARecastNavMesh::PostInitProperties()
{
	if (HasAnyFlags(RF_ClassDefaultObject) == true)
	{
		SetDrawDistance(DefaultDrawDistance);
	}
	else if(GetWorld() != NULL)
	{
		// get rid of instances saved within levels that are streamed-in
		if ((GEngine->IsSettingUpPlayWorld() == false) // this is a @HACK
			&&	(GetWorld()->GetOutermost() != GetOutermost())
			// If we are cooking, then let them all pass.
			// They will be handled at load-time when running.
			&&	(IsRunningCommandlet() == false))
		{
			// marking self for deletion 
			CleanUpAndMarkPendingKill();
		}
	}
	
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
#if RECAST_ASYNC_REBUILDING
		NavDataReadWriteLock = &NavDataReadWriteLockDummy;		
#endif // RECAST_ASYNC_REBUILDING

		DefaultQueryFilter->SetMaxSearchNodes(DefaultMaxSearchNodes);

		FRecastQueryFilter* DetourFilter = static_cast<FRecastQueryFilter*>(DefaultQueryFilter->GetImplementation());
		DetourFilter->setHeuristicScale(HeuristicScale);
	}

	// voxel cache requires the same rasterization setup for all navmeshes, as it's stored in octree
	if (IsVoxelCacheEnabled() && !HasAnyFlags(RF_ClassDefaultObject))
	{
		ARecastNavMesh* DefOb = (ARecastNavMesh*)ARecastNavMesh::StaticClass()->GetDefaultObject();

		if (TileSizeUU != DefOb->TileSizeUU)
		{
			UE_LOG(LogNavigation, Warning, TEXT("%s param: TileSizeUU(%f) differs from config settings, forcing value %f so it can be used with voxel cache!"),
				*GetNameSafe(this), TileSizeUU, DefOb->TileSizeUU);
			
			TileSizeUU = DefOb->TileSizeUU;
		}

		if (CellSize != DefOb->CellSize)
		{
			UE_LOG(LogNavigation, Warning, TEXT("%s param: CellSize(%f) differs from config settings, forcing value %f so it can be used with voxel cache!"),
				*GetNameSafe(this), CellSize, DefOb->CellSize);

			CellSize = DefOb->CellSize;
		}

		if (CellHeight != DefOb->CellHeight)
		{
			UE_LOG(LogNavigation, Warning, TEXT("%s param: CellHeight(%f) differs from config settings, forcing value %f so it can be used with voxel cache!"),
				*GetNameSafe(this), CellHeight, DefOb->CellHeight);

			CellHeight = DefOb->CellHeight;
		}

		if (AgentMaxSlope != DefOb->AgentMaxSlope)
		{
			UE_LOG(LogNavigation, Warning, TEXT("%s param: AgentMaxSlope(%f) differs from config settings, forcing value %f so it can be used with voxel cache!"),
				*GetNameSafe(this), AgentMaxSlope, DefOb->AgentMaxSlope);

			AgentMaxSlope = DefOb->AgentMaxSlope;
		}

		if (AgentMaxStepHeight != DefOb->AgentMaxStepHeight)
		{
			UE_LOG(LogNavigation, Warning, TEXT("%s param: AgentMaxStepHeight(%f) differs from config settings, forcing value %f so it can be used with voxel cache!"),
				*GetNameSafe(this), AgentMaxStepHeight, DefOb->AgentMaxStepHeight);

			AgentMaxStepHeight = DefOb->AgentMaxStepHeight;
		}
	}
}

void ARecastNavMesh::OnNavAreaAdded(const UClass* NavAreaClass, int32 AgentIndex)
{
	Super::OnNavAreaAdded(NavAreaClass, AgentIndex);

	// update navmesh query filter with area costs
	const int32 AreaID = GetAreaID(NavAreaClass);
	if (AreaID != INDEX_NONE)
	{
		const UNavArea* DefArea = ((UClass*)NavAreaClass)->GetDefaultObject<UNavArea>();

		DefaultQueryFilter->SetAreaCost(AreaID, DefArea->DefaultCost);
		DefaultQueryFilter->SetFixedAreaEnteringCost(AreaID, DefArea->FixedAreaEnteringCost);
	}

#if WITH_NAVIGATION_GENERATOR
	// update generator's cached data
	FRecastNavMeshGenerator* MyGenerator = NavDataGenerator.IsValid() ? (FRecastNavMeshGenerator*)NavDataGenerator.Get() : NULL;
	if (MyGenerator)
	{
		MyGenerator->OnAreaAdded(NavAreaClass, AreaID);
	}
#endif // WITH_NAVIGATION_GENERATOR
}

int32 ARecastNavMesh::GetNewAreaID(const UClass* AreaClass) const
{
	if (AreaClass == UNavigationSystem::GetDefaultWalkableArea())
	{
		return RECAST_DEFAULT_AREA;
	}

	if (AreaClass == UNavArea_Null::StaticClass())
	{
		return RECAST_NULL_AREA;
	}

	int32 FreeAreaID = Super::GetNewAreaID(AreaClass);
	while (FreeAreaID == RECAST_NULL_AREA || FreeAreaID == RECAST_DEFAULT_AREA)
	{
		FreeAreaID++;
	}

	check(FreeAreaID < GetMaxSupportedAreas());
	return FreeAreaID;
}

FColor ARecastNavMesh::GetAreaIDColor(uint8 AreaID) const
{
	const UClass* AreaClass = GetAreaClass(AreaID);
	const UNavArea* DefArea = AreaClass ? ((UClass*)AreaClass)->GetDefaultObject<UNavArea>() : NULL;
	return DefArea ? DefArea->DrawColor : NavDataConfig.Color;
}

void ARecastNavMesh::SerializeRecastNavMesh(FArchive& Ar, FPImplRecastNavMesh*& NavMesh)
{
	if (!Ar.IsLoading() 
		&& (NavMesh == NULL 
#if WITH_NAVIGATION_GENERATOR
			|| (GetGenerator() != NULL && GetGenerator()->IsBuildInProgress(/*bCheckDirtyToo=*/true) == true)
#endif // WITH_NAVIGATION_GENERATOR
			)
		)
	{
		//disable warning during PIE, we'll have information about wrong navmesh on screen if dynamic building is disabled
		if (NavMesh != NULL && Ar.IsSaving())
		{
			// nothing to write or while building, which in turn could result in corrupted save
			UE_LOG(LogNavigation, Warning, TEXT("ARecastNavMesh: Unable to serialize navmesh when build is in progress or navmesh is not up to date."));
		}
		return;
	}

	if (Ar.IsLoading())
	{
		// allocate if necessary
		if (RecastNavMeshImpl == NULL)
		{
			RecastNavMeshImpl = new FPImplRecastNavMesh(this);
		}
	}

	if (RecastNavMeshImpl)
	{
		RecastNavMeshImpl->Serialize(Ar);
	}	
}


void ARecastNavMesh::Serialize( FArchive& Ar )
{
	Super::Serialize(Ar);

	Ar << NavMeshVersion;

	//@todo: How to handle loading nav meshes saved w/ recast when recast isn't present????
	
	// when writing, write a zero here for now.  will come back and fill it in later.
	uint32 RecastNavMeshSizeBytes = 0;
	int32 RecastNavMeshSizePos = Ar.Tell();
	Ar << RecastNavMeshSizeBytes;

	if (Ar.IsLoading())
	{
		if (NavMeshVersion < NAVMESHVER_MIN_COMPATIBLE)
		{
			// incompatible, just skip over this data.  navmesh needs rebuilt.
			Ar.Seek( RecastNavMeshSizePos + RecastNavMeshSizeBytes );

			// Mark self for delete
			CleanUpAndMarkPendingKill();
		}
		else if (RecastNavMeshSizeBytes > 4)
		{
			SerializeRecastNavMesh(Ar, RecastNavMeshImpl);
#if !(UE_BUILD_SHIPPING)
			RequestDrawingUpdate();
#endif //!(UE_BUILD_SHIPPING)
		}
		else
		{
			// empty, just skip over this data
			Ar.Seek( RecastNavMeshSizePos + RecastNavMeshSizeBytes );
		}
	}
	else
	{
		SerializeRecastNavMesh(Ar, RecastNavMeshImpl);

		if (Ar.IsSaving())
		{
			int32 CurPos = Ar.Tell();
			RecastNavMeshSizeBytes = CurPos - RecastNavMeshSizePos;
			Ar.Seek(RecastNavMeshSizePos);
			Ar << RecastNavMeshSizeBytes;
			Ar.Seek(CurPos);
		}
	}
}

void ARecastNavMesh::SetConfig(const FNavDataConfig& Src) 
{ 
	NavDataConfig = Src; 
	AgentMaxHeight = AgentHeight = Src.AgentHeight;
	AgentRadius = Src.AgentRadius;
}

void ARecastNavMesh::FillConfig(FNavDataConfig& Dest)
{
	Dest = NavDataConfig;
	Dest.AgentHeight = AgentHeight;
	Dest.AgentRadius = AgentRadius;
}

bool ARecastNavMesh::IsGeneratedFor(const FNavAgentProperties& AgentProps) const
{
	return NavDataConfig.IsEquivalent(AgentProps);
}

void ARecastNavMesh::BeginBatchQuery() const
{
#if RECAST_ASYNC_REBUILDING
	// lock critical section when no other batch queries are active
	if (BatchQueryCounter <= 0)
	{
		BatchQueryCounter = 0;
		NavDataReadWriteLock->Lock();
	}

	BatchQueryCounter++;
#endif // RECAST_ASYNC_REBUILDING
}

void ARecastNavMesh::FinishBatchQuery() const
{
#if RECAST_ASYNC_REBUILDING
	BatchQueryCounter--;

	// release critical section when no other batch queries are active
	NavDataReadWriteLock->Unlock();
#endif // RECAST_ASYNC_REBUILDING
}

FBox ARecastNavMesh::GetNavMeshBounds() const
{
	FBox Bounds;
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		Bounds = RecastNavMeshImpl->GetNavMeshBounds();
	}

	return Bounds;
}

FBox ARecastNavMesh::GetNavMeshTileBounds(int32 TileIndex) const
{
	FBox Bounds;
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		Bounds = RecastNavMeshImpl->GetNavMeshTileBounds(TileIndex);
	}

	return Bounds;
}

void ARecastNavMesh::GetNavMeshTileXY(int32 TileIndex, int32& OutX, int32& OutY, int32& OutLayer) const
{
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		RecastNavMeshImpl->GetNavMeshTileXY(TileIndex, OutX, OutY, OutLayer);
	}
}

bool ARecastNavMesh::GetPolysInTile(int32 TileIndex, TArray<FNavPoly>& Polys) const
{
	bool bSuccess = false;
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		bSuccess = RecastNavMeshImpl->GetPolysInTile(TileIndex, Polys);
	}

	return bSuccess;
}

int32 ARecastNavMesh::GetNavMeshTilesCount() const
{
	int32 NumTiles = 0;
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		NumTiles = RecastNavMeshImpl->GetNavMeshTilesCount();
	}

	return NumTiles;
}

void ARecastNavMesh::GetEdgesForPathCorridor(TArray<NavNodeRef>* PathCorridor, TArray<FNavigationPortalEdge>* PathCorridorEdges) const
{
	check(PathCorridor != NULL && PathCorridorEdges != NULL);

	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		RecastNavMeshImpl->GetEdgesForPathCorridor(PathCorridor, PathCorridorEdges);
	}
}

FNavLocation ARecastNavMesh::GetRandomPoint(TSharedPtr<const FNavigationQueryFilter> Filter) const
{
	FNavLocation RandomPt;
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		RandomPt = RecastNavMeshImpl->GetRandomPoint(GetRightFilterRef(Filter));
	}

	return RandomPt;
}

bool ARecastNavMesh::GetRandomPointInRadius(const FVector& Origin, float Radius, FNavLocation& OutResult, TSharedPtr<const FNavigationQueryFilter> Filter) const
{
	bool bSuccess = false;
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		bSuccess = RecastNavMeshImpl->GetRandomPointInRadius(Origin, Radius, OutResult, GetRightFilterRef(Filter));
	}

	return bSuccess;
}

bool ARecastNavMesh::GetRandomPointInCluster(NavNodeRef ClusterRef, FNavLocation& OutLocation) const
{
	bool bSuccess = false;
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		bSuccess = RecastNavMeshImpl->GetRandomPointInCluster(ClusterRef, OutLocation);
	}

	return bSuccess;
}

NavNodeRef ARecastNavMesh::GetClusterRef(NavNodeRef PolyRef) const
{
	NavNodeRef ClusterRef = 0;
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		ClusterRef = RecastNavMeshImpl->GetClusterRefFromPolyRef(PolyRef);
	}

	return ClusterRef;
}

bool ARecastNavMesh::ProjectPoint(const FVector& Point, FNavLocation& OutLocation, const FVector& Extent, TSharedPtr<const FNavigationQueryFilter> Filter) const
{
	bool bSuccess = false;
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		bSuccess = RecastNavMeshImpl->ProjectPointToNavMesh(Point, OutLocation, Extent, GetRightFilterRef(Filter));
	}

	return bSuccess;
}

bool ARecastNavMesh::ProjectPointMulti(const FVector& Point, TArray<FNavLocation>& OutLocations, const FVector& Extent,
									   float MinZ, float MaxZ, TSharedPtr<const FNavigationQueryFilter> Filter) const
{
	bool bSuccess = false;
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		bSuccess = RecastNavMeshImpl->ProjectPointMulti(Point, OutLocations, Extent, MinZ, MaxZ, GetRightFilterRef(Filter));
	}

	return bSuccess;
}

ENavigationQueryResult::Type ARecastNavMesh::CalcPathCost(const FVector& PathStart, const FVector& PathEnd, float& OutPathCost, TSharedPtr<const FNavigationQueryFilter> QueryFilter) const 
{
	float TmpPathLength = 0.f;
	ENavigationQueryResult::Type Result = CalcPathLengthAndCost(PathStart, PathEnd, TmpPathLength, OutPathCost, QueryFilter);
	return Result;
}

ENavigationQueryResult::Type ARecastNavMesh::CalcPathLength(const FVector& PathStart, const FVector& PathEnd, float& OutPathLength, TSharedPtr<const FNavigationQueryFilter> QueryFilter) const 
{
	float TmpPathCost = 0.f;
	ENavigationQueryResult::Type Result = CalcPathLengthAndCost(PathStart, PathEnd, OutPathLength, TmpPathCost, QueryFilter);
	return Result;
}

ENavigationQueryResult::Type ARecastNavMesh::CalcPathLengthAndCost(const FVector& PathStart, const FVector& PathEnd, float& OutPathLength, float& OutPathCost, TSharedPtr<const FNavigationQueryFilter> QueryFilter) const
{
	ENavigationQueryResult::Type Result = ENavigationQueryResult::Invalid;

	if (RecastNavMeshImpl)
	{
		if ((PathStart - PathEnd).IsNearlyZero() == true)
		{
			OutPathLength = 0.f;
			Result = ENavigationQueryResult::Success;
		}
		else
		{
			TSharedRef<FNavMeshPath> Path = MakeShareable(new FNavMeshPath());
			Path->SetWantsStringPulling(false);
			Path->SetWantsPathCorridor(true);
			
			{
				SECTION_LOCK_TILES;
				Result = RecastNavMeshImpl->FindPath(PathStart, PathEnd, Path.Get(), GetRightFilterRef(QueryFilter));
			}

			if (Result == ENavigationQueryResult::Success || (Result == ENavigationQueryResult::Fail && Path->IsPartial()))
			{
				OutPathLength = Path->GetTotalPathLength();
				OutPathCost = Path->GetCost();
			}
		}
	}

	return Result;
}

NavNodeRef ARecastNavMesh::FindNearestPoly(FVector const& Loc, FVector const& Extent, TSharedPtr<const FNavigationQueryFilter> Filter) const
{
	NavNodeRef PolyRef = 0;
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		PolyRef = RecastNavMeshImpl->FindNearestPoly(Loc, Extent, GetRightFilterRef(Filter));
	}

	return PolyRef;
}

void ARecastNavMesh::UpdateSmartLink(class USmartNavLinkComponent* LinkComp)
{
	TSubclassOf<UNavArea> AreaClass = LinkComp->IsEnabled() ? LinkComp->GetEnabledArea() : LinkComp->GetDisabledArea();
	UpdateNavigationLinkArea(LinkComp->GetLinkId(), AreaClass);
}

void ARecastNavMesh::UpdateNavigationLinkArea(int32 UserId, TSubclassOf<class UNavArea> AreaClass) const
{
	int32 AreaId = GetAreaID(AreaClass);
	if (AreaId >= 0 && RecastNavMeshImpl)
	{
		UNavArea* DefArea = (UNavArea*)(AreaClass->GetDefaultObject());

		SECTION_LOCK_TILES;
		RecastNavMeshImpl->UpdateNavigationLinkArea(UserId, AreaId, DefArea->GetAreaFlags());
	}
}

void ARecastNavMesh::UpdateSegmentLinkArea(int32 UserId, TSubclassOf<class UNavArea> AreaClass) const
{
	int32 AreaId = GetAreaID(AreaClass);
	if (AreaId >= 0 && RecastNavMeshImpl)
	{
		UNavArea* DefArea = (UNavArea*)(AreaClass->GetDefaultObject());

		SECTION_LOCK_TILES;
		RecastNavMeshImpl->UpdateSegmentLinkArea(UserId, AreaId, DefArea->GetAreaFlags());
	}
}

bool ARecastNavMesh::GetPolyCenter(NavNodeRef PolyID, FVector& OutCenter) const
{
	bool bSuccess = false;
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		bSuccess = RecastNavMeshImpl->GetPolyCenter(PolyID, OutCenter);
	}

	return bSuccess;
}

bool ARecastNavMesh::GetPolyVerts(NavNodeRef PolyID, TArray<FVector>& OutVerts) const
{
	bool bSuccess = false;
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		bSuccess = RecastNavMeshImpl->GetPolyVerts(PolyID, OutVerts);
	}

	return bSuccess;
}

uint32 ARecastNavMesh::GetPolyAreaID(NavNodeRef PolyID) const
{
	uint32 AreaID = RECAST_DEFAULT_AREA;
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		AreaID = RecastNavMeshImpl->GetPolyAreaID(PolyID);
	}

	return AreaID;
}

bool ARecastNavMesh::GetClusterCenter(NavNodeRef ClusterRef, bool bUseCenterPoly, FVector& OutCenter) const
{
	bool bSuccess = false;
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		bSuccess = RecastNavMeshImpl->GetClusterCenter(ClusterRef, bUseCenterPoly, OutCenter);
	}

	return bSuccess;
}

bool ARecastNavMesh::GetClusterBounds(NavNodeRef ClusterRef, FBox& OutBounds) const
{
	bool bSuccess = false;
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		bSuccess = RecastNavMeshImpl->GetClusterBounds(ClusterRef, OutBounds);
	}

	return bSuccess;
}

bool ARecastNavMesh::GetPolysWithinPathingDistance(FVector const& StartLoc, const float PathingDistance, TArray<NavNodeRef>& FoundPolys, TSharedPtr<const FNavigationQueryFilter> Filter) const
{
	bool bSuccess = false;
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		bSuccess = RecastNavMeshImpl->GetPolysWithinPathingDistance(StartLoc, PathingDistance, GetRightFilterRef(Filter), FoundPolys);
	}

	return bSuccess;
}

bool ARecastNavMesh::GetClustersWithinPathingDistance(FVector const& StartLoc, const float PathingDistance, TArray<NavNodeRef>& FoundClusters, bool bBackTracking) const
{
	bool bSuccess = false;
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		bSuccess = RecastNavMeshImpl->GetClustersWithinPathingDistance(StartLoc, PathingDistance, bBackTracking, FoundClusters);
	}

	return bSuccess;
}

void ARecastNavMesh::GetDebugGeometry(FRecastDebugGeometry& OutGeometry, int32 TileIndex) const
{
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		RecastNavMeshImpl->GetDebugGeometry(OutGeometry, TileIndex);
	}
}
void ARecastNavMesh::GetDebugTileBounds(FBox& OuterBox, int32& NumTilesX, int32& NumTilesY) const
{
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		RecastNavMeshImpl->GetDebugTileBounds(OuterBox, NumTilesX, NumTilesY);
	}
}

void ARecastNavMesh::RequestDrawingUpdate()
{
#if !UE_BUILD_SHIPPING
	FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
		FSimpleDelegateGraphTask::FDelegate::CreateUObject(this, &ARecastNavMesh::UpdateDrawing)
		, TEXT("Requesting navmesh redraw")
		, NULL
		, ENamedThreads::GameThread
		);
#endif // !UE_BUILD_SHIPPING
}

void ARecastNavMesh::UpdateDrawing()
{
#if DO_NAVMESH_DEBUG_DRAWING_PER_TILE
	ConstructRenderingComponentImpl();
#endif
	UpdateNavMeshDrawing();

#if WITH_EDITOR
	if (GEditor != NULL)
	{
		GEditor->RedrawLevelEditingViewports();
	}
#endif // WITH_EDITOR
}

void ARecastNavMesh::DrawDebugPathCorridor(NavNodeRef const* PathPolys, int32 NumPathPolys, bool bPersistent) const
{
	static const FColor PathLineColor(255, 128, 0);

	// draw poly outlines
	TArray<FVector> PolyVerts;
	for (int32 PolyIdx=0; PolyIdx < NumPathPolys; ++PolyIdx)
	{
		if ( GetPolyVerts(PathPolys[PolyIdx], PolyVerts) )
		{
			for (int32 VertIdx=0; VertIdx < PolyVerts.Num()-1; ++VertIdx)
			{
				DrawDebugLine(GetWorld(), PolyVerts[VertIdx], PolyVerts[VertIdx+1], PathLineColor, bPersistent);
			}
			DrawDebugLine(GetWorld(), PolyVerts[PolyVerts.Num()-1], PolyVerts[0], PathLineColor, bPersistent);
		}
	}

	// draw ordered poly links
	if (NumPathPolys > 0)
	{
		FVector PolyCenter;
		FVector NextPolyCenter;
		if ( GetPolyCenter(PathPolys[0], NextPolyCenter) )			// prime the pump
		{
			for (int32 PolyIdx=0; PolyIdx < NumPathPolys-1; ++PolyIdx)
			{
				PolyCenter = NextPolyCenter;
				if ( GetPolyCenter(PathPolys[PolyIdx+1], NextPolyCenter) )
				{
					DrawDebugLine(GetWorld(), PolyCenter, NextPolyCenter, PathLineColor, bPersistent);
					DrawDebugBox(GetWorld(), PolyCenter, FVector(5.f), PathLineColor, bPersistent);
				}
			}
		}
	}
}

#if !UE_BUILD_SHIPPING
uint32 ARecastNavMesh::LogMemUsed() const 
{
	const uint32 SuperMemUsed = Super::LogMemUsed();
	uint32 MemUsed = FreshTiles.GetAllocatedSize();

	UE_LOG(LogNavigation, Display, TEXT("%s: ARecastNavMesh: %u\n    self: %d"), *GetName(), MemUsed, sizeof(ARecastNavMesh));	

	return MemUsed + SuperMemUsed;
}

#endif // !UE_BUILD_SHIPPING

SIZE_T ARecastNavMesh::GetResourceSize(EResourceSizeMode::Type Mode)
{
	return 0;
}

uint16 ARecastNavMesh::GetDefaultForbiddenFlags() const
{
	return FPImplRecastNavMesh::GetFilterForbiddenFlags((const FRecastQueryFilter*)DefaultQueryFilter->GetImplementation());
}

void ARecastNavMesh::SetDefaultForbiddenFlags(uint16 ForbiddenAreaFlags)
{
	FPImplRecastNavMesh::SetFilterForbiddenFlags((FRecastQueryFilter*)DefaultQueryFilter->GetImplementation(), ForbiddenAreaFlags);
}

bool ARecastNavMesh::FilterPolys(TArray<NavNodeRef>& PolyRefs, const class FRecastQueryFilter* Filter) const
{
	bool bSuccess = false;
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		bSuccess = RecastNavMeshImpl->FilterPolys(PolyRefs, Filter);
	}

	return bSuccess;
}

void ARecastNavMesh::ApplyWorldOffset(const FVector& InOffset, bool bWorldShift)
{
	if (RecastNavMeshImpl)
	{
		SECTION_LOCK_TILES;
		RecastNavMeshImpl->ApplyWorldOffset(InOffset, bWorldShift);
	}

	Super::ApplyWorldOffset(InOffset, bWorldShift);
	RequestDrawingUpdate();
}

FPathFindingResult ARecastNavMesh::FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query)
{
	const ANavigationData* Self = Query.NavData.Get();
	check(Cast<const ARecastNavMesh>(Self));

	const ARecastNavMesh* RecastNavMesh = (const ARecastNavMesh*)Self;
	if (Self == NULL || RecastNavMesh->RecastNavMeshImpl == NULL)
	{
		return ENavigationQueryResult::Error;
	}
		
	FPathFindingResult Result;
	Result.Path = Self->CreatePathInstance<FNavMeshPath>();
	
	if ((Query.StartLocation - Query.EndLocation).IsNearlyZero() == true)
	{
		Result.Path->PathPoints.Reset();
		Result.Path->PathPoints.Add(FNavPathPoint(Query.EndLocation));
		Result.Result = ENavigationQueryResult::Success;
	}
	else
	{
		SECTION_LOCK_TILES_FOR(RecastNavMesh);

		if(Query.QueryFilter.IsValid())
		{
			Result.Result = RecastNavMesh->RecastNavMeshImpl->FindPath(Query.StartLocation, Query.EndLocation,
				*((FNavMeshPath*)(Result.Path.Get())), *(Query.QueryFilter.Get()));
		}
		else
		{
			Result.Result = ENavigationQueryResult::Error;
		}
	}

	return Result;
}

FPathFindingResult ARecastNavMesh::FindHierarchicalPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query)
{
	const ANavigationData* Self = Query.NavData.Get();
	check(Cast<const ARecastNavMesh>(Self));

	const ARecastNavMesh* RecastNavMesh = (const ARecastNavMesh*)Self;
	if (Self == NULL || RecastNavMesh->RecastNavMeshImpl == NULL)
	{
		return ENavigationQueryResult::Error;
	}

	const bool bCanUseHierachicalPath = (Query.QueryFilter == RecastNavMesh->GetDefaultQueryFilter());

	FPathFindingResult Result;
	Result.Path = Self->CreatePathInstance<FNavMeshPath>();

	if ((Query.StartLocation - Query.EndLocation).IsNearlyZero() == true)
	{
		Result.Path->PathPoints.Reset();
		Result.Path->PathPoints.Add(FNavPathPoint(Query.EndLocation));
		Result.Result = ENavigationQueryResult::Success;
	}
	else
	{
		SECTION_LOCK_TILES_FOR(RecastNavMesh);

		bool bUseFallbackSearch = false;
		if (bCanUseHierachicalPath)
		{
			Result.Result = RecastNavMesh->RecastNavMeshImpl->FindClusterPath(Query.StartLocation, Query.EndLocation,
				*((FNavMeshPath*)(Result.Path.Get())));

			if (Result.Result == ENavigationQueryResult::Error)
			{
				bUseFallbackSearch = true;
			}
		}
		else
		{
			UE_LOG(LogNavigation, Warning, TEXT("Hierarchical path finding request failed: filter doesn't match!"));
			bUseFallbackSearch = true;
		}

		if (bUseFallbackSearch)
		{
			Result.Result = RecastNavMesh->RecastNavMeshImpl->FindPath(Query.StartLocation, Query.EndLocation,
				*((FNavMeshPath*)(Result.Path.Get())), *(Query.QueryFilter.Get()));
		}
	}

	return Result;
}

bool ARecastNavMesh::TestPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query)
{
	const ANavigationData* Self = Query.NavData.Get();
	check(Cast<const ARecastNavMesh>(Self));

	const ARecastNavMesh* RecastNavMesh = (const ARecastNavMesh*)Self;
	if (Self == NULL || RecastNavMesh->RecastNavMeshImpl == NULL)
	{
		return false;
	}

	bool bPathExists = true;
	if ((Query.StartLocation - Query.EndLocation).IsNearlyZero() == false)
	{
		SECTION_LOCK_TILES_FOR(RecastNavMesh);

		ENavigationQueryResult::Type Result = RecastNavMesh->RecastNavMeshImpl->TestPath(Query.StartLocation, Query.EndLocation, *(Query.QueryFilter.Get()));
		bPathExists = (Result == ENavigationQueryResult::Success);
	}

	return bPathExists;
}

bool ARecastNavMesh::TestHierarchicalPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query)
{
	const ANavigationData* Self = Query.NavData.Get();
	check(Cast<const ARecastNavMesh>(Self));

	const ARecastNavMesh* RecastNavMesh = (const ARecastNavMesh*)Self;
	if (Self == NULL || RecastNavMesh->RecastNavMeshImpl == NULL)
	{
		return false;
	}

	const bool bCanUseHierachicalPath = (Query.QueryFilter == RecastNavMesh->GetDefaultQueryFilter());
	bool bPathExists = true;

	if ((Query.StartLocation - Query.EndLocation).IsNearlyZero() == false)
	{
		SECTION_LOCK_TILES_FOR(RecastNavMesh);

		bool bUseFallbackSearch = false;
		if (bCanUseHierachicalPath)
		{
			ENavigationQueryResult::Type Result = RecastNavMesh->RecastNavMeshImpl->TestClusterPath(Query.StartLocation, Query.EndLocation);
			bPathExists = (Result == ENavigationQueryResult::Success);

			if (Result == ENavigationQueryResult::Error)
			{
				bUseFallbackSearch = true;
			}
		}
		else
		{
			UE_LOG(LogNavigation, Log, TEXT("Hierarchical path finding test failed: filter doesn't match!"));
			bUseFallbackSearch = true;
		}

		if (bUseFallbackSearch)
		{
			ENavigationQueryResult::Type Result = RecastNavMesh->RecastNavMeshImpl->TestPath(Query.StartLocation, Query.EndLocation, *(Query.QueryFilter.Get()));
			bPathExists = (Result == ENavigationQueryResult::Success);
		}
	}

	return bPathExists;
}

bool ARecastNavMesh::NavMeshRaycast(const ANavigationData* Self, const FVector& RayStart, const FVector& RayEnd, FVector& HitLocation, TSharedPtr<const FNavigationQueryFilter> QueryFilter)
{
	check(Cast<const ARecastNavMesh>(Self));

	const ARecastNavMesh* RecastNavMesh = (const ARecastNavMesh*)Self;
	if (Self == NULL || RecastNavMesh->RecastNavMeshImpl == NULL)
	{
		HitLocation = RayStart;
		return true;
	}

	FRaycastResult Result;
	{
		SECTION_LOCK_TILES_FOR(RecastNavMesh);		
		RecastNavMesh->RecastNavMeshImpl->Raycast2D(RayStart, RayEnd, RecastNavMesh->GetRightFilterRef(QueryFilter), Result);
	}

	HitLocation = Result.HasHit() ? (RayStart + (RayEnd - RayStart) * Result.HitTime) : RayEnd;

	return Result.HasHit();
}

bool ARecastNavMesh::IsSegmentOnNavmesh(const FVector& SegmentStart, const FVector& SegmentEnd, TSharedPtr<const FNavigationQueryFilter> Filter) const
{
	if (RecastNavMeshImpl == NULL)
	{
		return false;
	}
	
	FRaycastResult Result;
	{
		SECTION_LOCK_TILES_FOR(this);		
		RecastNavMeshImpl->Raycast2D(SegmentStart, SegmentEnd, GetRightFilterRef(Filter), Result);
	}

	return Result.HasHit() == false;
}

int32 ARecastNavMesh::DebugPathfinding(const FPathFindingQuery& Query, TArray<FRecastDebugPathfindingStep>& Steps)
{
	int32 NumSteps = 0;

	const ANavigationData* Self = Query.NavData.Get();
	check(Cast<const ARecastNavMesh>(Self));

	const ARecastNavMesh* RecastNavMesh = (const ARecastNavMesh*)Self;
	if (Self == NULL || RecastNavMesh->RecastNavMeshImpl == NULL)
	{
		return false;
	}

	if ((Query.StartLocation - Query.EndLocation).IsNearlyZero() == false)
	{
		SECTION_LOCK_TILES_FOR(RecastNavMesh);

		NumSteps = RecastNavMesh->RecastNavMeshImpl->DebugPathfinding(Query.StartLocation, Query.EndLocation, *(Query.QueryFilter.Get()), Steps);
	}

	return NumSteps;
}

void ARecastNavMesh::UpdateNavTileData(uint8* OldTileData, FNavMeshTileData NewTileData)
{
	check(IsInGameThread());
	static const float SecondsToHoldOldData = 1.f;

	if (OldTileData != NULL)
	{
		int32 OldDataIndex = INDEX_NONE;
		int32 OldDataSize = 0;

		// find shared pointer holding OldTileData
		const FNavMeshTileData* const RESTRICT DataEnd = TileNavDataContainer.GetTypedData() + TileNavDataContainer.Num();
		for(const FNavMeshTileData* RESTRICT Data = TileNavDataContainer.GetTypedData(); Data < DataEnd; ++Data)
		{
			if( *Data == OldTileData )
			{
				OldDataIndex = (int32)(Data - TileNavDataContainer.GetTypedData());
				OldDataSize = Data->DataSize;
				break;
			}
		}

		ensure(OldDataIndex != INDEX_NONE);
		if (OldDataIndex != INDEX_NONE)
		{
			DEC_MEMORY_STAT_BY(STAT_Navigation_NavDataMemory, OldDataSize);

			//	remove from list of referenced memory.
			// this should result in releasing OldTileData (unless it's being 
			//	referenced by some other shared pointer)
			//	bAllowShrinking=false since TileNavDataContainer will consistently retain 
			// its size as long as navmesh has the same number of tiles
			TileNavDataContainer.RemoveAtSwap(OldDataIndex, 1, /*bAllowShrinking=*/false);
		}
	}

	if (NewTileData.IsValid()) 
	{
		INC_MEMORY_STAT_BY(STAT_Navigation_NavDataMemory, NewTileData.DataSize);
		TileNavDataContainer.Add(NewTileData);
	}
}

void ARecastNavMesh::ReserveTileSet(int32 NewGridWidth, int32 NewGridHeight)
{
	const int32 NewSize = NewGridWidth * NewGridHeight;	

	if (NewGridWidth != GridWidth || NewGridHeight != GridHeight)
	{
		TileSet.Reset();		
		if (TileSet.Max() < NewSize)
		{
			TileSet.Reserve(NewSize);
		}

		for (int32 Index = 0; Index < NewSize; ++Index)
		{
			const FVector2D Location = TileIndexToXY(NewGridWidth, Index);
			TileSet.Add(FTileSetItem(Location.X, Location.Y, Index));
		}
		
		GridWidth = NewGridWidth;
		GridHeight = NewGridHeight;
	}
}

FTileSetItem* ARecastNavMesh::GetTileSetItemAt(int32 TileX, int32 TileY)
{
	return &TileSet[TileXYToIndex(GridWidth, TileX, TileY)];
}

void ARecastNavMesh::UpdateNavmeshOffset( const FBox& NavBounds )
{
	if (RecastNavMeshImpl != NULL)
	{
		if (dtNavMesh* RecastNavMesh = RecastNavMeshImpl->GetRecastMesh())
		{
			const FBox RCNavBounds = Unreal2RecastBox(NavBounds);
			const float moveBy[3] = { 
				RCNavBounds.Min.X - RecastNavMeshImpl->GetRecastMesh()->getParams()->orig[0],
				RCNavBounds.Min.Y - RecastNavMeshImpl->GetRecastMesh()->getParams()->orig[1],
				RCNavBounds.Min.Z - RecastNavMeshImpl->GetRecastMesh()->getParams()->orig[2]
			};
			RecastNavMesh->applyWorldOffset(&moveBy[0]);
		}
	}
}


void ARecastNavMesh::UpdateNavVersion() 
{ 
#if WITH_NAVIGATION_GENERATOR
	NavMeshVersion = NAVMESHVER_LATEST; 
#endif // WITH_NAVIGATION_GENERATOR
}

#if WITH_EDITOR

void ARecastNavMesh::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent)
{
	static const FName NAME_Generation = FName(TEXT("Generation"));
	static const FName NAME_Display = FName(TEXT("Display"));

	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property != NULL)
	{
		if (FObjectEditorUtils::GetCategoryFName(PropertyChangedEvent.Property) == NAME_Generation)
		{
			FName PropName = PropertyChangedEvent.Property->GetFName();
			
			if (PropName == GET_MEMBER_NAME_CHECKED(ARecastNavMesh,AgentRadius) ||
				PropName == GET_MEMBER_NAME_CHECKED(ARecastNavMesh,TileSizeUU) ||
				PropName == GET_MEMBER_NAME_CHECKED(ARecastNavMesh,CellSize))
			{
				// rule of thumb, dimension tile shouldn't be less than 16 * AgentRadius
				if (TileSizeUU < 16.f * AgentRadius)
				{
					TileSizeUU = FMath::Max(16.f * AgentRadius, RECAST_MIN_TILE_SIZE);
				}

				// tile's dimension can't exceed 2^16 x cell size, as it's being stored on 2 bytes
				const int32 DimensionVX = FMath::Ceil(TileSizeUU / CellSize);
				if (DimensionVX > MAX_uint16)
				{
					TileSizeUU = MAX_uint16 * CellSize;
				}
			}

#if WITH_NAVIGATION_GENERATOR
			FNavDataGenerator* Generator = GetGenerator(NavigationSystem::DontCreate);
			if (Generator != NULL)
			{
				Generator->Generate();
			}
#endif // WITH_NAVIGATION_GENERATOR
		}
		else if (FObjectEditorUtils::GetCategoryFName(PropertyChangedEvent.Property) == NAME_Display)
		{
			RequestDrawingUpdate();
		}
	}
}

#endif // WITH_EDITOR

bool ARecastNavMesh::NeedsRebuild()
{
	bool bLooksLikeNeeded = !RecastNavMeshImpl || RecastNavMeshImpl->GetRecastMesh() == 0;
#if WITH_NAVIGATION_GENERATOR
	FRecastNavMeshGenerator* MyGenerator = NavDataGenerator.IsValid() ? (FRecastNavMeshGenerator*)NavDataGenerator.Get() : NULL;
	if (MyGenerator)
	{
		return bLooksLikeNeeded || MyGenerator->HasDirtyTiles() || MyGenerator->HasDirtyAreas();
	}
#endif // WITH_NAVIGATION_GENERATOR

	return bLooksLikeNeeded;
}

bool ARecastNavMesh::IsVoxelCacheEnabled()
{
#if RECAST_ASYNC_REBUILDING
	// voxel cache is using static buffers to minimize memory impact
	// therefore it can run only with synchronous navmesh rebuilds
	return false;
#endif

	ARecastNavMesh* DefOb = (ARecastNavMesh*)ARecastNavMesh::StaticClass()->GetDefaultObject();
	return DefOb && DefOb->bUseVoxelCache;
}

const class FRecastQueryFilter* ARecastNavMesh::GetNamedFilter(ERecastNamedFilter::Type FilterType)
{
	check(FilterType < ERecastNamedFilter::NamedFiltersCount); 
	return NamedFilters[FilterType];
}

#endif	//WITH_RECAST
