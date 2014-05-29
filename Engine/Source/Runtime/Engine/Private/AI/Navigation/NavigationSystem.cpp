// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "NavDataGenerator.h"
#include "NavigationOctree.h"
#include "AI/Navigation/NavMeshBoundsVolume.h"

#if WITH_RECAST
#include "RecastNavMeshGenerator.h"
#endif // WITH_RECAST
#if WITH_EDITOR
#include "UnrealEd.h"
#include "Editor/GeometryMode/Public/GeometryEdMode.h"
#include "Editor/GeometryMode/Public/EditorGeometry.h"
#endif

// @todo this is here only due to circular dependency to AIModule. To be removed
#include "Navigation/CrowdManager.h"
#include "Navigation/PathFollowingComponent.h"
#include "Navigation/NavigationComponent.h"
#include "AI/Navigation/NavAreas/NavArea_Null.h"
#include "AI/Navigation/NavAreas/NavArea_Default.h"
#include "AI/Navigation/SmartNavLinkComponent.h"
#include "AI/Navigation/NavigationSystem.h"

static const uint32 INITIAL_ASYNC_QUERIES_SIZE = 32;
static const uint32 REGISTRATION_QUEUE_SIZE = 16;	// and we'll not reallocate
#if WITH_RECAST
static const uint32 MAX_NAV_SEARCH_NODES = RECAST_MAX_SEARCH_NODES;
#else // WITH_RECAST
static const uint32 MAX_NAV_SEARCH_NODES = 2048;
#endif // WITH_RECAST

DEFINE_LOG_CATEGORY(LogNavigation);

DECLARE_CYCLE_STAT(TEXT("Rasterize triangles"), STAT_Navigation_RasterizeTriangles,STATGROUP_Navigation);

//----------------------------------------------------------------------//
// Stats
//----------------------------------------------------------------------//

DEFINE_STAT(STAT_Navigation_QueriesTimeSync);
DEFINE_STAT(STAT_Navigation_RequestingAsyncPathfinding);
DEFINE_STAT(STAT_Navigation_PathfindingSync);
DEFINE_STAT(STAT_Navigation_PathfindingAsync);
DEFINE_STAT(STAT_Navigation_AddTile);
DEFINE_STAT(STAT_Navigation_TileNavAreaSorting);
DEFINE_STAT(STAT_Navigation_TileGeometryExportToObjAsync);
DEFINE_STAT(STAT_Navigation_TileVoxelFilteringAsync);
DEFINE_STAT(STAT_Navigation_TileBuildAsync);
DEFINE_STAT(STAT_Navigation_MetaAreaTranslation);
DEFINE_STAT(STAT_Navigation_TileBuildPreparationSync);
DEFINE_STAT(STAT_Navigation_BSPExportSync);
DEFINE_STAT(STAT_Navigation_GatheringNavigationModifiersSync);
DEFINE_STAT(STAT_Navigation_ActorsGeometryExportSync);
DEFINE_STAT(STAT_Navigation_ProcessingActorsForNavMeshBuilding);
DEFINE_STAT(STAT_Navigation_AdjustingNavLinks);
DEFINE_STAT(STAT_Navigation_AddingActorsToNavOctree);
DEFINE_STAT(STAT_Navigation_RecastTick);
DEFINE_STAT(STAT_Navigation_RecastBuildCompressedLayers);
DEFINE_STAT(STAT_Navigation_RecastBuildNavigation);
DEFINE_STAT(STAT_Navigation_DestructiblesShapesExported);
DEFINE_STAT(STAT_Navigation_UpdateNavOctree);
DEFINE_STAT(STAT_Navigation_CollisionTreeMemory);
DEFINE_STAT(STAT_Navigation_NavDataMemory);
DEFINE_STAT(STAT_Navigation_TileCacheMemory);
DEFINE_STAT(STAT_Navigation_OutOfNodesPath);
DEFINE_STAT(STAT_Navigation_PartialPath);
DEFINE_STAT(STAT_Navigation_CumulativeBuildTime);
DEFINE_STAT(STAT_Navigation_BuildTime);
DEFINE_STAT(STAT_Navigation_OffsetFromCorners);
DEFINE_STAT(STAT_Navigation_PathVisibilityOptimisation);

//----------------------------------------------------------------------//
// consts
//----------------------------------------------------------------------//

const uint32 FNavigationQueryFilter::DefaultMaxSearchNodes = MAX_NAV_SEARCH_NODES;

namespace FNavigationSystem
{
	// these are totally arbitrary values, and it should haven happen these are ever used.
	// in any reasonable case UNavigationSystem::SupportedAgents should be filled in ini file
	// and only those values will be used
	const float FallbackAgentRadius = 35.f;
	const float FallbackAgentHeight = 144.f;
		
	FORCEINLINE bool IsValidExtent(const FVector& Extent)
	{
		return Extent != INVALID_NAVEXTENT;
	}
}

namespace NavigationDebugDrawing
{
	const float PathLineThickness = 3.f;
	const FVector PathOffeset(0,0,15);
	const FVector PathNodeBoxExtent(16.f);
}

//----------------------------------------------------------------------//
// UNavigationSystem                                                                
//----------------------------------------------------------------------//
bool UNavigationSystem::bNavigationAutoUpdateEnabled = true;
TArray<TSubclassOf<class ANavigationData> > UNavigationSystem::NavDataClasses;
TArray<const UClass*> UNavigationSystem::NavAreaClasses;
TArray<UClass*> UNavigationSystem::PendingNavAreaRegistration;
TSubclassOf<class UNavArea> UNavigationSystem::DefaultWalkableArea = NULL;
TSubclassOf<class UNavArea> UNavigationSystem::DefaultObstacleArea = NULL;
FOnNavAreaChanged UNavigationSystem::NavAreaAddedObservers;
FOnNavAreaChanged UNavigationSystem::NavAreaRemovedObservers;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
FNavigationSystemExec UNavigationSystem::ExecHandler;
#endif // !UE_BUILD_SHIPPING

/** called after navigation influencing event takes place*/
UNavigationSystem::FOnNavigationDirty UNavigationSystem::NavigationDirtyEvent;

bool UNavigationSystem::bUpdateNavOctreeOnPrimitiveComponentChange = true;
//----------------------------------------------------------------------//
// life cycle stuff                                                                
//----------------------------------------------------------------------//

UNavigationSystem::UNavigationSystem(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, bWholeWorldNavigable(false)
	, bAddPlayersToGenerationSeeds(true)
	, bSkipAgentHeightCheckWhenPickingNavData(false)
	, DirtyAreasUpdateFreq(60)
	, OperationMode(FNavigationSystem::InvalidMode)
	, NavOctree(NULL)
	, bNavigationBuildingLocked(false)
	, bInitialBuildingLockActive(false)
	, bInitialSetupHasBeenPerformed(false)
	, CurrentlyDrawnNavDataIndex(0)
	, DirtyAreasUpdateTime(0)
{
#if WITH_EDITOR
	bFakeComponentChangesBeingApplied = false;
#endif

	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		// reserve some arbitrary size
		AsyncPathFindingQueries.Reserve( INITIAL_ASYNC_QUERIES_SIZE );
		NavDataRegistrationQueue.Reserve( REGISTRATION_QUEUE_SIZE );
	
		NavOctree = new FNavigationOctree(FVector(0,0,0), 64000);
#if WITH_NAVIGATION_GENERATOR
#if WITH_RECAST
		NavOctree->NavigableGeometryExportDelegate = FNavigationOctree::FNavigableGeometryExportDelegate::CreateStatic(&FRecastNavMeshGenerator::ExportActorGeometry);
		NavOctree->NavigableGeometryComponentExportDelegate = FNavigationOctree::FNavigableGeometryComponentExportDelegate::CreateStatic(&FRecastNavMeshGenerator::ExportComponentGeometry);
#endif // WITH_RECAST
		FWorldDelegates::LevelAddedToWorld.AddUObject(this, &UNavigationSystem::OnLevelAddedToWorld);
		FWorldDelegates::LevelRemovedFromWorld.AddUObject(this, &UNavigationSystem::OnLevelRemovedFromWorld);
#endif // WITH_NAVIGATION_GENERATOR	
	}
	else
	{
		DefaultWalkableArea = UNavArea_Default::StaticClass();
		DefaultObstacleArea = UNavArea_Null::StaticClass();
	}

#if WITH_EDITOR
	if (GIsEditor && HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		GEditorModeTools().OnEditorModeChanged().AddUObject(this, &UNavigationSystem::OnEditorModeChanged);
	}
#endif // WITH_EDITOR
}

UNavigationSystem::~UNavigationSystem()
{
	CleanUp();
#if WITH_EDITOR
	if (GIsEditor)
	{
		GEditorModeTools().OnEditorModeChanged().RemoveAll(this);
	}
#endif // WITH_EDITOR
}

void UNavigationSystem::DoInitialSetup()
{
	if (bInitialSetupHasBeenPerformed)
	{
		return;
	}

	// gather navigation creators
	NavDataClasses.Empty(RequiredNavigationDataClassNames.Num());
	for (int32 Index = 0; Index < RequiredNavigationDataClassNames.Num(); ++Index)
	{
		TSubclassOf<class ANavigationData> NavDataClass = LoadClass<ANavigationData>(NULL, *RequiredNavigationDataClassNames[Index].ClassName, NULL, LOAD_None, NULL);
		if (NavDataClass)
		{
			NavDataClasses.AddUnique(NavDataClass);
		}
		else
		{
			UE_LOG(LogNavigation, Warning, TEXT("Unable to find navigation data class \'%s\' while setting up require navigation types")
				, *RequiredNavigationDataClassNames[Index].ClassName);
		}
	}

	if (NavDataClasses.Num() == 0)
	{
		// @note: if you don't want navigation system to be created at all you can disable it by 
		// setting AWorldSettings.bEnableNavigationSystem to false
		UE_LOG(LogNavigation, Error, TEXT("No navigation data types found while setting up required navigation types!"));
	}

	CreateCrowdManager();

	bInitialSetupHasBeenPerformed = true;
}

void UNavigationSystem::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		// make sure there's at leas one supported navigation agent size
		if (SupportedAgents.Num() == 0)
		{
			SupportedAgents.Add(FNavDataConfig(FNavigationSystem::FallbackAgentRadius, FNavigationSystem::FallbackAgentHeight));
		}

		const bool bShouldStoreNavigableGeometry = bBuildNavigationAtRuntime || !GetWorld()->IsGameWorld();
		NavOctree->SetNavigableGeometryStoringMode(bShouldStoreNavigableGeometry 
			? FNavigationOctree::StoreNavGeometry
			: FNavigationOctree::SkipNavGeometry);
	
		bInitialBuildingLockActive = bInitialBuildingLocked;

#if WITH_NAVIGATION_GENERATOR
		UWorld* World = GetWorld();
		if (World && World->PersistentLevel)
		{
			OnLevelAddedToWorld(World->PersistentLevel, World);
		}
#endif

		// register for any actor move change
		GEngine->OnActorMoved().AddUObject(this, &UNavigationSystem::OnActorMoved);
		FCoreDelegates::PostLoadMap.AddUObject(this, &UNavigationSystem::OnPostLoadMap);
#if WITH_NAVIGATION_GENERATOR
		UNavigationSystem::NavigationDirtyEvent.AddUObject(this, &UNavigationSystem::OnNavigationDirtied);
#endif // WITH_NAVIGATION_GENERATOR
	}
}

void UNavigationSystem::OnInitializeActors()
{
	
}

void UNavigationSystem::OnWorldInitDone(FNavigationSystem::EMode Mode)
{
	OperationMode = Mode;

	UWorld* World = GetWorld();

	if (IsThereAnywhereToBuildNavigation() == false
		// Simulation mode is a special case - better not do it in this case
		&& OperationMode != FNavigationSystem::SimulationMode)
	{
		// remove all navigation data instances
		for (TActorIterator<ANavigationData> It(World); It; ++It)
		{
			ANavigationData* Nav = (*It);
			if (Nav != NULL && Nav->IsPendingKill() == false)
			{
				UnregisterNavData(Nav);
				Nav->CleanUpAndMarkPendingKill();
			}
		}

		if (OperationMode == FNavigationSystem::EditorMode)
		{
			bInitialBuildingLockActive = false;
		}

		bNavDataRemovedDueToMissingNavBounds = true;
	}
	else
	{
		PopulateNavOctree();

		// gather all navigation data instances and register all not-yet-registered
		// (since it's quite possible navigation system was not ready by the time
		// those instances were serialized-in or spawned)
		bool bProcessRegistration = false;
		for (TActorIterator<ANavigationData> It(World); It; ++It)
		{
			ANavigationData* Nav = (*It);
			if (Nav != NULL && Nav->IsPendingKill() == false && Nav->IsRegistered() == false)
			{
				RequestRegistration(Nav, false);
				bProcessRegistration = true;
			}
		}
		if (bProcessRegistration == true)
		{
			ProcessRegistrationCandidates();
		}

		if (OperationMode == FNavigationSystem::EditorMode)
		{
			// don't lock navigation building in editor
			bInitialBuildingLockActive = false;
		}
		
#if WITH_NAVIGATION_GENERATOR
		if (bAutoCreateNavigationData == true)
		{
			SpawnMissingNavigationData();
			// in case anything spawned has registered
			ProcessRegistrationCandidates();

			for (int32 NavDataIndex = 0; NavDataIndex < NavDataSet.Num(); ++NavDataIndex)
			{
				ANavigationData* NavData = NavDataSet[NavDataIndex];
				if (NavData != NULL)
				{
					FNavDataGenerator* Generator = NavData->GetGenerator(FNavigationSystem::Create);
					if (Generator != NULL)
					{
						NavData->GetGenerator(FNavigationSystem::DontCreate)->OnWorldInitDone(bInitialBuildingLockActive == false && bNavigationAutoUpdateEnabled);
					}
				}
			}
		}
		else
		{
			if (GetMainNavData(FNavigationSystem::DontCreate) != NULL)
			{
				// trigger navmesh update
				for (TActorIterator<ANavigationData> It(World); It; ++It)
				{
					ANavigationData* NavData = (*It);
					if (NavData != NULL)
					{
						ERegistrationResult Result = RegisterNavData(NavData);

						if (Result == RegistrationSuccessful)
						{
#if WITH_RECAST
							if (NavData->GetGenerator(FNavigationSystem::Create) != NULL
								&& Cast<ARecastNavMesh>(NavData) != NULL)
							{
								NavData->GetGenerator(FNavigationSystem::DontCreate)->OnWorldInitDone(bInitialBuildingLockActive == false && bNavigationAutoUpdateEnabled);
							}
#endif // WITH_RECAST
						}
						else if (Result != RegistrationFailed_DataPendingKill
							&& Result != RegistrationFailed_AgentNotValid
							)
						{
							NavData->CleanUpAndMarkPendingKill();					
						}
					}
				}
			}
		}
#endif
	}
}

void UNavigationSystem::CreateCrowdManager()
{
	SetCrowdManager(NewObject<UCrowdManager>(this));
}

void UNavigationSystem::SetCrowdManager(UCrowdManager* NewCrowdManager)
{
	if (NewCrowdManager == CrowdManager.Get())
	{
		return;
	}

	if (CrowdManager.IsValid())
	{
		CrowdManager->RemoveFromRoot();
	}
	CrowdManager = NewCrowdManager;
	if (NewCrowdManager != NULL)
	{
		CrowdManager->AddToRoot();
	}
}

void UNavigationSystem::Tick(float DeltaSeconds)
{
	// Register any pending nav areas
	if (PendingNavAreaRegistration.Num() > 0)
	{
		ProcessNavAreaPendingRegistration();
	}

	if (PendingOctreeUpdates.Num() > 0)
	{
		SCOPE_CYCLE_COUNTER(STAT_Navigation_AddingActorsToNavOctree);

		SCOPE_CYCLE_COUNTER(STAT_Navigation_BuildTime)
		STAT(double ThisTime = 0);
		{
			SCOPE_SECONDS_COUNTER(ThisTime);
			for (TSet<FNavigationDirtyElement>::TIterator It(PendingOctreeUpdates); It; ++It)
			{
				AddElementToNavOctree(*It);
			}
			PendingOctreeUpdates.Empty(32);
		}
		INC_FLOAT_STAT_BY(STAT_Navigation_CumulativeBuildTime,(float)ThisTime*1000);
	}

#if WITH_NAVIGATION_GENERATOR
	DirtyAreasUpdateTime += DeltaSeconds;
	const float DirtyAreasUpdateDeltaTime = 1.0f / DirtyAreasUpdateFreq;
	if (DirtyAreas.Num() > 0 && DirtyAreasUpdateTime >= DirtyAreasUpdateDeltaTime)
	{
		for (int32 NavDataIndex = 0; NavDataIndex < NavDataSet.Num(); ++NavDataIndex)
		{
			ANavigationData* NavData = NavDataSet[NavDataIndex];
			if (NavData != NULL)
			{
				FNavDataGenerator* Generator = NavData->GetGenerator(FNavigationSystem::DontCreate);
				if (Generator != NULL)
				{
					Generator->RebuildDirtyAreas(DirtyAreas);
				}
			}
		}

		DirtyAreasUpdateTime = 0;
		DirtyAreas.Reset();
	}
#endif // WITH_NAVIGATION_GENERATOR

	if (AsyncPathFindingQueries.Num() > 0)
	{
		TriggerAsyncQueries(AsyncPathFindingQueries);
		AsyncPathFindingQueries.Reset();
	}

	if (CrowdManager.IsValid())
	{
		CrowdManager->Tick(DeltaSeconds);
	}
}

void UNavigationSystem::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	// don't reference NavAreaClasses in editor (unless PIE is active)
	// to allow deleting assets
	if (!GIsEditor || GIsPlayInEditorWorld)
	{
		for (int32 NavAreaIndex = 0; NavAreaIndex < NavAreaClasses.Num(); NavAreaIndex++)
		{
			Collector.AddReferencedObject(NavAreaClasses[NavAreaIndex], InThis);
		}
	}

	for (int32 PendingAreaIndex = 0; PendingAreaIndex < PendingNavAreaRegistration.Num(); PendingAreaIndex++)
	{
		Collector.AddReferencedObject(PendingNavAreaRegistration[PendingAreaIndex], InThis);
	}
	
	UNavigationSystem* This = CastChecked<UNavigationSystem>(InThis);
	UCrowdManager* CrowdManager = This->GetCrowdManager();
	Collector.AddReferencedObject(CrowdManager, InThis);
}

#if WITH_EDITOR
void UNavigationSystem::SetNavigationAutoUpdateEnabled(bool bNewEnable,UNavigationSystem* InNavigationsystem) 
{ 
	if(bNewEnable != bNavigationAutoUpdateEnabled)
	{
		bNavigationAutoUpdateEnabled = bNewEnable; 
#if WITH_NAVIGATION_GENERATOR
		if (InNavigationsystem)
		{
			InNavigationsystem->EnableAllGenerators(bNewEnable, /*bForce=*/true);
		}
#endif // WITH_NAVIGATION_GENERATOR
	}
}
#endif // WITH_EDITOR

//----------------------------------------------------------------------//
// Public querying interface                                                                
//----------------------------------------------------------------------//
FPathFindingResult UNavigationSystem::FindPathSync(const FNavAgentProperties& AgentProperties, FPathFindingQuery Query, EPathFindingMode::Type Mode)
{
	SCOPE_CYCLE_COUNTER(STAT_Navigation_PathfindingSync);

	if (Query.NavData.IsValid() == false)
	{
		Query.NavData = GetNavDataForProps(AgentProperties);
	}

	FPathFindingResult Result(ENavigationQueryResult::Error);
	if (Query.NavData.IsValid())
	{
		if (Mode == EPathFindingMode::Hierarchical)
		{
			Result = Query.NavData->FindHierarchicalPath(AgentProperties, Query);
		}
		else
		{
			Result = Query.NavData->FindPath(AgentProperties, Query);
		}
	}

	return Result;
}

FPathFindingResult UNavigationSystem::FindPathSync(FPathFindingQuery Query, EPathFindingMode::Type Mode)
{
	SCOPE_CYCLE_COUNTER(STAT_Navigation_PathfindingSync);

	if (Query.NavData.IsValid() == false)
	{
		Query.NavData = GetMainNavData(FNavigationSystem::DontCreate);
	}
	
	FPathFindingResult Result(ENavigationQueryResult::Error);
	if (Query.NavData.IsValid())
	{
		if (Mode == EPathFindingMode::Hierarchical)
		{
			Result = Query.NavData->FindHierarchicalPath(FNavAgentProperties(), Query);
		}
		else
		{
			Result = Query.NavData->FindPath(FNavAgentProperties(), Query);
		}
	}

	return Result;
}

bool UNavigationSystem::TestPathSync(FPathFindingQuery Query, EPathFindingMode::Type Mode) const
{
	SCOPE_CYCLE_COUNTER(STAT_Navigation_PathfindingSync);

	if (Query.NavData.IsValid() == false)
	{
		Query.NavData = GetMainNavData();
	}
	
	bool bExists = false;
	if (Query.NavData.IsValid())
	{
		if (Mode == EPathFindingMode::Hierarchical)
		{
			bExists = Query.NavData->TestHierarchicalPath(FNavAgentProperties(), Query);
		}
		else
		{
			bExists = Query.NavData->TestPath(FNavAgentProperties(), Query);
		}
	}

	return bExists;
}

void UNavigationSystem::AddAsyncQuery(const FAsyncPathFindingQuery& Query)
{
	check(IsInGameThread());
	AsyncPathFindingQueries.Add(Query);
}

uint32 UNavigationSystem::FindPathAsync(const FNavAgentProperties& AgentProperties, FPathFindingQuery Query, const FNavPathQueryDelegate& ResultDelegate, EPathFindingMode::Type Mode)
{
	SCOPE_CYCLE_COUNTER(STAT_Navigation_RequestingAsyncPathfinding);

	if (Query.NavData.IsValid() == false)
	{
		Query.NavData = GetNavDataForProps(AgentProperties);
	}

	if (Query.NavData.IsValid())
	{
		FAsyncPathFindingQuery AsyncQuery(Query, ResultDelegate, Mode);

		if (AsyncQuery.QueryID != INVALID_NAVQUERYID)
		{
			AddAsyncQuery(AsyncQuery);
		}

		return AsyncQuery.QueryID;
	}

	return INVALID_NAVQUERYID;
}

void UNavigationSystem::AbortAsyncFindPathRequest(uint32 AsynPathQueryID)
{
	check(IsInGameThread());
	FAsyncPathFindingQuery* Query = AsyncPathFindingQueries.GetTypedData();
	for (int32 Index = 0; Index < AsyncPathFindingQueries.Num(); ++Index, ++Query)
	{
		if (Query->QueryID == AsynPathQueryID)
		{
			AsyncPathFindingQueries.RemoveAtSwap(Index);
			break;
		}
	}
}

void UNavigationSystem::TriggerAsyncQueries(TArray<FAsyncPathFindingQuery>& PathFindingQueries)
{
	FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
		FSimpleDelegateGraphTask::FDelegate::CreateUObject(this, &UNavigationSystem::PerformAsyncQueries, PathFindingQueries)
		, TEXT("NavigationSystem batched async queries")
		);
}

static void AsyncQueryDone(FAsyncPathFindingQuery Query)
{
	Query.OnDoneDelegate.ExecuteIfBound(Query.QueryID, Query.Result.Result, Query.Result.Path);
}

void UNavigationSystem::PerformAsyncQueries(TArray<FAsyncPathFindingQuery> PathFindingQueries)
{
	SCOPE_CYCLE_COUNTER(STAT_Navigation_PathfindingAsync);

	if (PathFindingQueries.Num() == 0)
	{
		return;
	}
	
	const int32 QueriesCount = PathFindingQueries.Num();
	FAsyncPathFindingQuery* Query = PathFindingQueries.GetTypedData();

	for (int32 QueryIndex = 0; QueryIndex < QueriesCount; ++QueryIndex, ++Query)
	{
		// @todo this is not necessarily the safest way to use UObjects outside of main thread. 
		//	think about something else.
		const ANavigationData* NavData = Query->NavData.IsValid() ? Query->NavData.Get() : GetMainNavData(FNavigationSystem::DontCreate);

		// perform query
		if (NavData)
		{
			if (Query->Mode == EPathFindingMode::Hierarchical)
			{
				Query->Result = NavData->FindHierarchicalPath(FNavAgentProperties(), *Query);
			}
			else
			{
				Query->Result = NavData->FindPath(FNavAgentProperties(), *Query);
			}
		}
		else
		{
			Query->Result = ENavigationQueryResult::Error;
		}

		// @todo make it return more informative results (bResult == false)
		// trigger calling delegate on main thread - otherwise it may depend too much on stuff being thread safe
		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
			FSimpleDelegateGraphTask::FDelegate::CreateStatic(AsyncQueryDone, *Query)
			, TEXT("Async nav query finished")
			, NULL
			, ENamedThreads::GameThread
			);
	}
}

bool UNavigationSystem::GetRandomPoint(FNavLocation& ResultLocation, ANavigationData* NavData, TSharedPtr<const FNavigationQueryFilter> QueryFilter)
{
	SCOPE_CYCLE_COUNTER(STAT_Navigation_QueriesTimeSync);

	if (NavData == NULL)
	{
		NavData = MainNavData;
	}

	if (NavData != NULL)
	{
		ResultLocation = NavData->GetRandomPoint(QueryFilter);
		return true;
	}

	return false;
}

bool UNavigationSystem::GetRandomPointInRadius(const FVector& Origin, float Radius, FNavLocation& ResultLocation, ANavigationData* NavData, TSharedPtr<const FNavigationQueryFilter> QueryFilter) const
{
	SCOPE_CYCLE_COUNTER(STAT_Navigation_QueriesTimeSync);

	if (NavData == NULL)
	{
		NavData = MainNavData;
	}

	if (NavData != NULL)
	{
		NavData->GetRandomPointInRadius(Origin, Radius, ResultLocation, QueryFilter);
		return true;
	}

	return false;
}

ENavigationQueryResult::Type UNavigationSystem::GetPathCost(const FVector& PathStart, const FVector& PathEnd, float& OutPathCost, const class ANavigationData* NavData, TSharedPtr<const FNavigationQueryFilter> QueryFilter) const
{
	SCOPE_CYCLE_COUNTER(STAT_Navigation_QueriesTimeSync);

	if (NavData == NULL)
	{
		NavData = GetMainNavData();
	}

	return NavData != NULL ? NavData->CalcPathCost(PathStart, PathEnd, OutPathCost, QueryFilter) : ENavigationQueryResult::Error;
}

ENavigationQueryResult::Type UNavigationSystem::GetPathLength(const FVector& PathStart, const FVector& PathEnd, float& OutPathLength, const ANavigationData* NavData, TSharedPtr<const FNavigationQueryFilter> QueryFilter) const
{
	SCOPE_CYCLE_COUNTER(STAT_Navigation_QueriesTimeSync);

	if (NavData == NULL)
	{
		NavData = GetMainNavData();
	}

	return NavData != NULL ? NavData->CalcPathLength(PathStart, PathEnd, OutPathLength, QueryFilter) : ENavigationQueryResult::Error;
}

ENavigationQueryResult::Type UNavigationSystem::GetPathLengthAndCost(const FVector& PathStart, const FVector& PathEnd, float& OutPathLength, float& OutPathCost, const ANavigationData* NavData, TSharedPtr<const FNavigationQueryFilter> QueryFilter) const
{
	SCOPE_CYCLE_COUNTER(STAT_Navigation_QueriesTimeSync);

	if (NavData == NULL)
	{
		NavData = GetMainNavData();
	}

	return NavData != NULL ? NavData->CalcPathLengthAndCost(PathStart, PathEnd, OutPathLength, OutPathCost, QueryFilter) : ENavigationQueryResult::Error;
}

bool UNavigationSystem::ProjectPointToNavigation(const FVector& Point, FNavLocation& OutLocation, const FVector& Extent, const class ANavigationData* NavData, TSharedPtr<const FNavigationQueryFilter> QueryFilter) const
{
	SCOPE_CYCLE_COUNTER(STAT_Navigation_QueriesTimeSync);

	if (NavData == NULL)
	{
		NavData = GetMainNavData();
	}

	return NavData != NULL && NavData->ProjectPoint(Point, OutLocation
		, FNavigationSystem::IsValidExtent(Extent) ? Extent : NavData->NavDataConfig.DefaultQueryExtent
		, QueryFilter);
}

void UNavigationSystem::SimpleMoveToActor(AController* Controller, const AActor* Goal)
{
	if (Goal == NULL)
	{
		UE_LOG(LogNavigation, Log, TEXT("UNavigationSystem::SimpleMoveToActor called for %s with NULL goal actor")
			, *GetNameSafe(Controller));
		return;
	}

	UNavigationComponent* PFindComp = NULL;
	UPathFollowingComponent* PFollowComp = NULL;
	
	if (Controller)
	{
		Controller->InitNavigationControl(PFindComp, PFollowComp);
	}

	if (PFindComp && PFollowComp && !PFollowComp->HasReached(Goal))
	{
		const bool bPathExists = PFindComp->FindPathToActor(Goal);
		if (bPathExists)
		{
			PFollowComp->RequestMove(PFindComp->GetPath(), Goal);
		}
	}
}

void UNavigationSystem::SimpleMoveToLocation(AController* Controller, const FVector& Goal)
{
	UNavigationComponent* PFindComp = NULL;
	UPathFollowingComponent* PFollowComp = NULL;

	if (Controller)
	{
		Controller->InitNavigationControl(PFindComp, PFollowComp);
	}

	if (PFindComp && PFollowComp && !PFollowComp->HasReached(Goal))
	{
		const bool bPathExists = PFindComp->FindPathToLocation(Goal);
		if (bPathExists)
		{
			PFollowComp->RequestMove(PFindComp->GetPath(), NULL);
		}
	}
}

bool UNavigationSystem::NavigationRaycast(UObject* WorldContextObject, const FVector& RayStart, const FVector& RayEnd, FVector& HitLocation, TSubclassOf<class UNavigationQueryFilter> FilterClass, AController* Querier)
{
	UWorld* World = NULL;

	if (WorldContextObject != NULL)
	{
		World = GEngine->GetWorldFromContextObject(WorldContextObject);
	}
	if (World == NULL && Querier != NULL)
	{
		World = GEngine->GetWorldFromContextObject(Querier);
	}

	// blocked, i.e. not traversable, by default
	bool bRaycastBlocked = true;
	HitLocation = RayStart;

	if (World != NULL && World->GetNavigationSystem() != NULL)
	{
		const UNavigationSystem* NavSys = World->GetNavigationSystem();

		// figure out which navigation data to use
		const ANavigationData* NavData = NULL;
		INavAgentInterface* MyNavAgent = InterfaceCast<INavAgentInterface>(Querier);
		if (MyNavAgent)
		{
			const FNavAgentProperties* AgentProps = MyNavAgent->GetNavAgentProperties();
			NavData = NavSys->GetNavDataForProps(*AgentProps);
		}
		if (NavData == NULL)
		{
			NavData = NavSys->GetMainNavData();
		}

		if (NavData != NULL)
		{
			bRaycastBlocked = NavData->Raycast(RayStart, RayEnd, HitLocation, UNavigationQueryFilter::GetQueryFilter(NavData, FilterClass));
		}
	}

	return bRaycastBlocked;
}

void UNavigationSystem::GetNavAgentPropertiesArray(TArray<FNavAgentProperties>& OutNavAgentProperties) const
{
	AgentToNavDataMap.GetKeys(OutNavAgentProperties);
}

ANavigationData* UNavigationSystem::GetNavDataForProps(const FNavAgentProperties& AgentProperties)
{
	const UNavigationSystem* ConstThis = const_cast<const UNavigationSystem*>(this);
	return const_cast<ANavigationData*>(ConstThis->GetNavDataForProps(AgentProperties));
}

const ANavigationData* UNavigationSystem::GetNavDataForProps(const FNavAgentProperties& AgentProperties) const
{
	if (SupportedAgents.Num() <= 1)
	{
		return MainNavData;
	}
	
	const ANavigationData* const* NavDataForAgent = AgentToNavDataMap.Find(AgentProperties);

	if (NavDataForAgent == NULL)
	{
		TArray<FNavAgentProperties> AgentPropertiesList;
		int32 NumNavDatas = AgentToNavDataMap.GetKeys(AgentPropertiesList);
		
		FNavAgentProperties BestFitNavAgent;
		float BestExcessHeight = -FLT_MAX;
		float BestExcessRadius = -FLT_MAX;
		float ExcessRadius = -FLT_MAX;
		float ExcessHeight = -FLT_MAX;
		const float AgentHeight = bSkipAgentHeightCheckWhenPickingNavData ? 0.f : AgentProperties.AgentHeight;

		for (TArray<FNavAgentProperties>::TConstIterator It(AgentPropertiesList); It; ++It)
		{
			const FNavAgentProperties& NavIt = *It;
			ExcessRadius = NavIt.AgentRadius - AgentProperties.AgentRadius;
			ExcessHeight = NavIt.AgentHeight - AgentHeight;

			const bool bExcessRadiusIsBetter = ((ExcessRadius == 0) && (BestExcessRadius != 0)) 
				|| ((ExcessRadius > 0) && (BestExcessRadius < 0))
				|| ((ExcessRadius > 0) && (BestExcessRadius > 0) && (ExcessRadius < BestExcessRadius))
				|| ((ExcessRadius < 0) && (BestExcessRadius < 0) && (ExcessRadius > BestExcessRadius));
			const bool bExcessHeightIsBetter = ((ExcessHeight == 0) && (BestExcessHeight != 0))
				|| ((ExcessHeight > 0) && (BestExcessHeight < 0))
				|| ((ExcessHeight > 0) && (BestExcessHeight > 0) && (ExcessHeight < BestExcessHeight))
				|| ((ExcessHeight < 0) && (BestExcessHeight < 0) && (ExcessHeight > BestExcessHeight));
			const bool bBestIsValid = (BestExcessRadius >= 0) && (BestExcessHeight >= 0);
			const bool bRadiusEquals = (ExcessRadius == BestExcessRadius);
			const bool bHeightEquals = (ExcessHeight == BestExcessHeight);

			bool bValuesAreBest = ((bExcessRadiusIsBetter || bRadiusEquals) && (bExcessHeightIsBetter || bHeightEquals));
			if (!bValuesAreBest && !bBestIsValid)
			{
				bValuesAreBest = bExcessRadiusIsBetter || (bRadiusEquals && bExcessHeightIsBetter);
			}

			if (bValuesAreBest)
			{
				BestFitNavAgent = NavIt;
				BestExcessHeight = ExcessHeight;
				BestExcessRadius = ExcessRadius;
			}
		}

		if (BestFitNavAgent.IsValid())
		{
			NavDataForAgent = AgentToNavDataMap.Find(BestFitNavAgent);
		}
	}

	return NavDataForAgent != NULL && *NavDataForAgent != NULL ? *NavDataForAgent : MainNavData;
}

ANavigationData* UNavigationSystem::GetMainNavData(FNavigationSystem::ECreateIfEmpty CreateNewIfNoneFound)
{
	checkSlow(IsInGameThread() == true);

	if (MainNavData == NULL || MainNavData->IsPendingKill())
	{
		MainNavData = NULL;

		// @TODO this should be done a differently. There should be specified a "default agent"
		for (int32 NavDataIndex = 0; NavDataIndex < NavDataSet.Num(); ++NavDataIndex)
		{
			ANavigationData* NavData = NavDataSet[NavDataIndex];
			if (NavData != NULL && NavData->IsPendingKill() == false)
			{
				MainNavData = NavData;
				break;
			}
		}

#if WITH_RECAST
		if ( /*GIsEditor && */(MainNavData == NULL) && CreateNewIfNoneFound == FNavigationSystem::Create )
		{
			// Spawn a new one if we're in the editor.  In-game, either we loaded one or we don't get one.
			MainNavData = GetWorld()->SpawnActor<ANavigationData>(ARecastNavMesh::StaticClass());
		}
#endif // WITH_RECAST
		// either way make sure it's registered. Registration stores unique
		// navmeshes, so we have nothing to loose
		RegisterNavData(MainNavData);
	}

	return MainNavData;
}

TSharedPtr<FNavigationQueryFilter> UNavigationSystem::CreateDefaultQueryFilterCopy() const 
{ 
	return MainNavData ? MainNavData->GetDefaultQueryFilter()->GetCopy() : NULL; 
}

bool UNavigationSystem::IsNavigationBuilt(const AWorldSettings* Settings) const
{
	if (IsThereAnywhereToBuildNavigation() == false)
	{
		return true;
	}

	bool bIsBuilt = true;
	// look for all ANavigationDatas that belong to specified world, and check if they're dirty
	// trigger navmesh update
	for (int32 NavDataIndex = 0; NavDataIndex < NavDataSet.Num(); ++NavDataIndex)
	{
		ANavigationData* NavData = NavDataSet[NavDataIndex];
		if (NavData != NULL && NavData->GetWorldSettings() == Settings)
		{
#if WITH_NAVIGATION_GENERATOR
			FNavDataGenerator* Generator = NavData->GetGenerator(FNavigationSystem::DontCreate);
			if ( NavData->bRebuildAtRuntime == false && ( Generator == NULL || Generator->IsBuildInProgress(true) == true ) )
			{
				bIsBuilt = false;
				break;
			}
#endif
		}
	}

	return bIsBuilt;
}

bool UNavigationSystem::IsThereAnywhereToBuildNavigation() const
{
	// not check if there are any volumes or other structures requiring/supporting navigation building
	if (bWholeWorldNavigable == true)
	{
		return true;
	}

	// @TODO this should be done more flexible to be able to trigger this from game-specific 
	// code (like Navigation System's subclass maybe)
	bool bCreateNavigation = false;

	for (TActorIterator<ANavMeshBoundsVolume> It(GetWorld()); It; ++It)
	{
		ANavMeshBoundsVolume const* const V = (*It);
		if (V != NULL)
		{
			bCreateNavigation = true;
			break;
		}
	}

	return bCreateNavigation;
}

FBox UNavigationSystem::GetWorldBounds() const
{
	checkSlow(IsInGameThread() == true);

	NavigableWorldBounds = FBox(0);

	if (GetWorld() != NULL && bWholeWorldNavigable == true)
	{
		// @TODO - super slow! Need to ask tech guys where I can get this from
		for( FActorIterator It(GetWorld()); It; ++It )
		{
			if ((*It)->IsNavigationRelevant() == true)
			{
				NavigableWorldBounds += (*It)->GetComponentsBoundingBox();
			}
		}
	}

	return NavigableWorldBounds;
}

FBox UNavigationSystem::GetLevelBounds(ULevel* InLevel) const
{
	FBox NavigableLevelBounds(0);

	if (InLevel)
	{
		AActor** Actor = InLevel->Actors.GetTypedData();
		const int32 ActorCount = InLevel->Actors.Num();
		for (int32 ActorIndex = 0; ActorIndex < ActorCount; ++ActorIndex, ++Actor)
		{
			if (*Actor != NULL && (*Actor)->IsNavigationRelevant() == true)
			{
				NavigableLevelBounds += (*Actor)->GetComponentsBoundingBox();
			}
		}
	}

	return NavigableLevelBounds;
}

void UNavigationSystem::ApplyWorldOffset(const FVector& InOffset, bool bWorldShift)
{
	for (int32 NavDataIndex = 0; NavDataIndex < NavDataSet.Num(); ++NavDataIndex)
	{
		NavDataSet[NavDataIndex]->ApplyWorldOffset(InOffset, bWorldShift);
	}
}

//----------------------------------------------------------------------//
// Bookkeeping 
//----------------------------------------------------------------------//
void UNavigationSystem::RequestRegistration(ANavigationData* NavData, bool bTriggerRegistrationProcessing)
{
	FScopeLock RegistrationLock(&NavDataRegistrationSection);

	if (NavDataRegistrationQueue.Num() < REGISTRATION_QUEUE_SIZE)
	{
		NavDataRegistrationQueue.AddUnique(NavData);

		if (bTriggerRegistrationProcessing == true)
		{
			// trigger registration candidates processing
			FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
				FSimpleDelegateGraphTask::FDelegate::CreateUObject(this, &UNavigationSystem::ProcessRegistrationCandidates)
				, TEXT("Process registration candidates")
				, NULL
				, ENamedThreads::GameThread
				);
		}
	}
	else
	{
		UE_LOG(LogNavigation, Error, TEXT("Navigation System: registration queue full!"));
	}
}

void UNavigationSystem::ProcessRegistrationCandidates()
{
	FScopeLock RegistrationLock(&NavDataRegistrationSection);

	if (NavDataRegistrationQueue.Num() == 0)
	{
		return;
	}

	ANavigationData** NavDataPtr = NavDataRegistrationQueue.GetTypedData();
	const int CandidatesCount = NavDataRegistrationQueue.Num();

	for (int32 CandidateIndex = 0; CandidateIndex < CandidatesCount; ++CandidateIndex, ++NavDataPtr)
	{
		if (*NavDataPtr != NULL)
		{
			ERegistrationResult Result = RegisterNavData(*NavDataPtr);

			if (Result == RegistrationSuccessful)
			{
				continue;
			}
			else if (Result != RegistrationFailed_DataPendingKill)
			{
				(*NavDataPtr)->CleanUpAndMarkPendingKill();
				if ((*NavDataPtr) == MainNavData)
				{
					MainNavData = NULL;
				}
			}
		}
	}

	MainNavData = GetMainNavData(FNavigationSystem::DontCreate);
	
	// we processed all candidates so clear the queue
	NavDataRegistrationQueue.Reset();
}

void UNavigationSystem::ProcessNavAreaPendingRegistration()
{
	TArray<UClass*> TempPending = PendingNavAreaRegistration;

	// Clear out old array, will fill in if still pending
	PendingNavAreaRegistration.Empty();

	for (int32 PendingAreaIndex = 0; PendingAreaIndex < TempPending.Num(); PendingAreaIndex++)
	{
		RegisterNavAreaClass(TempPending[PendingAreaIndex]);
	}
}

UNavigationSystem::ERegistrationResult UNavigationSystem::RegisterNavData(ANavigationData* NavData)
{
	if (NavData == NULL)
	{
		return RegistrationError;
	}
	else if (NavData->IsPendingKill() == true)
	{
		return RegistrationFailed_DataPendingKill;
	}
	// still to be seen if this is really true, but feels right
	else if (NavData->IsRegistered() == true)
	{
		return RegistrationSuccessful;
	}

	FScopeLock Lock(&NavDataRegistration);

	UNavigationSystem::ERegistrationResult Result = RegistrationError;

	// find out which, if any, navigation agents are supported by this nav data
	// if none then fail the registration
	FNavDataConfig NavConfig = NavData->GetConfig();

	// not discarding navmesh when there's only one Supported Agent
	if (NavConfig.IsValid() == false && SupportedAgents.Num() == 1)
	{
		// fill in AgentProps with whatever is the instance's setup
		NavData->SetConfig((NavConfig = SupportedAgents[0]));
		NavData->SetSupportsDefaultAgent(true);	
		NavData->ProcessNavAreas(NavAreaClasses, 0);
	}

	if (NavConfig.IsValid() == true)
	{
		// check if this kind of agent has already its navigation implemented
		ANavigationData** NavDataForAgent = AgentToNavDataMap.Find(NavConfig);
		if (NavDataForAgent == NULL || *NavDataForAgent == NULL || (*NavDataForAgent)->IsPendingKill() == true)
		{
			// ok, so this navigation agent doesn't have its navmesh registered yet, but do we want to support it?
			bool bAgentSupported = false;
			FNavDataConfig* Agent = SupportedAgents.GetTypedData();
			for (int32 AgentIndex = 0; AgentIndex < SupportedAgents.Num(); ++AgentIndex, ++Agent)
			{
				if (Agent->IsEquivalent(NavConfig) == true)
				{
					// it's supported, then just in case it's not a precise match (IsEquivalent succeeds with some precision) 
					// update NavData with supported Agent
					bAgentSupported = true;

					NavData->SetConfig(*Agent);
					AgentToNavDataMap.Add(*Agent, NavData);

					NavData->SetSupportsDefaultAgent(AgentIndex == 0);
					NavData->ProcessNavAreas(NavAreaClasses, AgentIndex);

					OnNavDataRegisteredEvent.Broadcast(NavData);
					break;
				}
			}

			Result = bAgentSupported == true ? RegistrationSuccessful : RegistrationFailed_AgentNotValid;
		}
		else if (*NavDataForAgent == NavData)
		{
			// let's treat double registration of the same nav data with the same agent as a success
			Result = RegistrationSuccessful;
		}
		else
		{
			// otherwise specified agent type already has its navmesh implemented, fail redundant instance
			Result = RegistrationFailed_AgentAlreadySupported;
		}
	}
	else
	{
		Result = RegistrationFailed_AgentNotValid;
	}

	if (Result == RegistrationSuccessful)
	{
		NavDataSet.AddUnique(NavData);

		NavAreaAddedObservers.AddUObject(NavData, &ANavigationData::OnNavAreaAddedNotify);
		NavAreaRemovedObservers.AddUObject(NavData, &ANavigationData::OnNavAreaRemovedNotify);

		NavData->OnRegistered();
	}
	// @todo else might consider modifying this NavData to implement navigation for one of the supported agents
	// care needs to be taken to not make it implement navigation for agent who's real implementation has 
	// not been loaded yet.

	return Result;
}

void UNavigationSystem::UnregisterNavData(ANavigationData* NavData)
{
	if (NavData == NULL)
	{
		return;
	}

	FScopeLock Lock(&NavDataRegistration);

	NavDataSet.RemoveSingle(NavData);

	NavAreaAddedObservers.RemoveUObject(NavData, &ANavigationData::OnNavAreaAddedNotify);
	NavAreaRemovedObservers.RemoveUObject(NavData, &ANavigationData::OnNavAreaRemovedNotify);
}

void UNavigationSystem::RegisterSmartLink(class USmartNavLinkComponent* LinkComp)
{
	SmartLinksMap.Add(LinkComp->GetLinkId(), LinkComp);
}

void UNavigationSystem::UnregisterSmartLink(class USmartNavLinkComponent* LinkComp)
{
	SmartLinksMap.Remove(LinkComp->GetLinkId());
}

uint32 UNavigationSystem::FindFreeSmartLinkId() const
{
	uint32 NextFreeId = 1;
	while (SmartLinksMap.Contains(NextFreeId))
	{
		NextFreeId++;
	}

	return NextFreeId;
}

class USmartNavLinkComponent* UNavigationSystem::GetSmartLink(uint32 SmartLinkId) const
{
	return SmartLinksMap.FindRef(SmartLinkId);
}

void UNavigationSystem::UpdateSmartLink(class USmartNavLinkComponent* LinkComp)
{
	for (TMap<FNavAgentProperties, ANavigationData*>::TIterator It(AgentToNavDataMap); It; ++It)
	{
		ANavigationData* NavData = It.Value();
		NavData->UpdateSmartLink(LinkComp);
	}
}

void UNavigationSystem::RequestAreaUnregistering(UClass* NavAreaClass)
{
	check(IsInGameThread());

	if (NavAreaClasses.Contains(NavAreaClass))
	{
		// remove from known areas
		NavAreaClasses.RemoveSingleSwap(NavAreaClass);
		PendingNavAreaRegistration.RemoveSingleSwap(NavAreaClass);

		// notify navigation data
		NavAreaRemovedObservers.Broadcast(NavAreaClass);
	}
}

void UNavigationSystem::RequestAreaRegistering(UClass* NavAreaClass)
{
	check(IsInGameThread());

	// can't be null
	if (NavAreaClass == NULL)
	{
		return;
	}

	// can't be abstract
	if (NavAreaClass->HasAnyClassFlags(CLASS_Abstract))
	{
		return;
	}

	// special handling of blueprint based areas
	if (NavAreaClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
	{
		// can't be skeleton of blueprint class
		if (NavAreaClass->GetName().Contains(TEXT("SKEL_")))
		{
			return;
		}

		// can't be class from Developers folder (won't be saved properly anyway)
		const UPackage* Package = NavAreaClass->GetOutermost();
		if (Package && Package->GetName().Contains(TEXT("/Developers/")) )
		{
			return;
		}
	}

	PendingNavAreaRegistration.Add(NavAreaClass);
}

void UNavigationSystem::RegisterNavAreaClass(UClass* AreaClass)
{
#if WITH_EDITORONLY_DATA
	if (AreaClass->ClassGeneratedBy && AreaClass->ClassGeneratedBy->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad))
	{
		// Class isn't done loading, try again later
		PendingNavAreaRegistration.Add(AreaClass);
		return;
	}
#endif //WITH_EDITORONLY_DATA

	// add to know areas
	NavAreaClasses.AddUnique(AreaClass);

	// notify existing nav data
	NavAreaAddedObservers.Broadcast(AreaClass);

#if WITH_EDITOR
	// update area properties
	AreaClass->GetDefaultObject<UNavArea>()->UpdateAgentConfig();
#endif
}

int32 UNavigationSystem::GetSupportedAgentIndex(const ANavigationData* NavData) const
{
	if (SupportedAgents.Num() < 2)
	{
		return 0;
	}

	const FNavDataConfig& TestConfig = NavData->GetConfig();
	for (int32 AgentIndex = 0; AgentIndex < SupportedAgents.Num(); AgentIndex++)
	{
		if (SupportedAgents[AgentIndex].IsEquivalent(TestConfig))
		{
			return AgentIndex;
		}
	}
	
	return INDEX_NONE;
}

int32 UNavigationSystem::GetSupportedAgentIndex(const FNavAgentProperties& NavAgent) const
{
	if (SupportedAgents.Num() < 2)
	{
		return 0;
	}

	for (int32 AgentIndex = 0; AgentIndex < SupportedAgents.Num(); AgentIndex++)
	{
		if (SupportedAgents[AgentIndex].IsEquivalent(NavAgent))
		{
			return AgentIndex;
		}
	}

	return INDEX_NONE;
}

void UNavigationSystem::DescribeFilterFlags(class UEnum* FlagsEnum) const
{
#if WITH_EDITOR
	TArray<FString> FlagDesc;
	FString EmptyStr;
	FlagDesc.Init(EmptyStr, 16);

	const int32 NumEnums = FMath::Min(16, FlagsEnum->NumEnums() - 1);	// skip _MAX
	for (int32 FlagIndex = 0; FlagIndex < NumEnums; FlagIndex++)
	{
		FlagDesc[FlagIndex] = FlagsEnum->GetEnumText(FlagIndex).ToString();
	}

	DescribeFilterFlags(FlagDesc);
#endif
}

void UNavigationSystem::DescribeFilterFlags(const TArray<FString>& FlagsDesc) const
{
#if WITH_EDITOR
	const int32 MaxFlags = 16;
	TArray<FString> UseDesc = FlagsDesc;

	FString EmptyStr;
	while (UseDesc.Num() < MaxFlags)
	{
		UseDesc.Add(EmptyStr);
	}

	// get special value from recast's navmesh
#if WITH_RECAST
	uint16 NavLinkFlag = ARecastNavMesh::GetNavLinkFlag();
	for (int32 FlagIndex = 0; FlagIndex < MaxFlags; FlagIndex++)
	{
		if ((NavLinkFlag >> FlagIndex) & 1)
		{
			UseDesc[FlagIndex] = TEXT("Navigation link");
			break;
		}
	}
#endif

	// setup properties
	UStructProperty* StructProp1 = FindField<UStructProperty>(UNavigationQueryFilter::StaticClass(), TEXT("IncludeFlags"));
	UStructProperty* StructProp2 = FindField<UStructProperty>(UNavigationQueryFilter::StaticClass(), TEXT("ExcludeFlags"));
	check(StructProp1);
	check(StructProp2);

	UStruct* Structs[] = { StructProp1->Struct, StructProp2->Struct };
	const FString CustomNameMeta = TEXT("DisplayName");

	for (int32 StructIndex = 0; StructIndex < ARRAY_COUNT(Structs); StructIndex++)
	{
		for (int32 FlagIndex = 0; FlagIndex < MaxFlags; FlagIndex++)
		{
			FString PropName = FString::Printf(TEXT("bNavFlag%d"), FlagIndex);
			UProperty* Prop = FindField<UProperty>(Structs[StructIndex], *PropName);
			check(Prop);

			if (UseDesc[FlagIndex].Len())
			{
				Prop->SetPropertyFlags(CPF_Edit);
				Prop->SetMetaData(*CustomNameMeta, *UseDesc[FlagIndex]);
			}
			else
			{
				Prop->ClearPropertyFlags(CPF_Edit);
			}
		}
	}

#endif
}

void UNavigationSystem::ResetCachedFilter(TSubclassOf<UNavigationQueryFilter> FilterClass)
{
	for (int32 NavDataIndex = 0; NavDataIndex < NavDataSet.Num(); NavDataIndex++)
	{
		if (NavDataSet[NavDataIndex])
		{
			NavDataSet[NavDataIndex]->RemoveQueryFilter(FilterClass);
		}
	}
}

void UNavigationSystem::RegisterGenerationSeed(AActor* SeedActor)
{
	GenerationSeeds.Add(SeedActor);
}

void UNavigationSystem::UnregisterGenerationSeed(AActor* SeedActor)
{
	GenerationSeeds.RemoveSingleSwap(SeedActor);
}

void UNavigationSystem::GetGenerationSeeds(TArray<FVector>& SeedLocations) const
{
	if (bAddPlayersToGenerationSeeds)
	{
		for (FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator(); it ;it++)
		{
			if (*it && (*it)->GetPawn() != NULL)
			{
				const FVector SeedLoc((*it)->GetPawn()->GetActorLocation());
				SeedLocations.Add(SeedLoc);
			}
		}
	}

	for (int32 SeedIndex = 0; SeedIndex < GenerationSeeds.Num(); SeedIndex++)
	{
		if (GenerationSeeds[SeedIndex].IsValid())
		{
			SeedLocations.Add(GenerationSeeds[SeedIndex]->GetActorLocation());
		}
	}
}

UNavigationSystem* UNavigationSystem::CreateNavigationSystem(class UWorld* WorldOwner)
{
	UNavigationSystem* NavSys = NULL;

#if WITH_SERVER_CODE || WITH_EDITOR
	// create navigation system for editor and server targets, but remove it from game clients
	if (WorldOwner && (WorldOwner->GetNetMode() != NM_Client) )
	{
		AWorldSettings* WorldSettings = WorldOwner->GetWorldSettings();
		if (WorldSettings == NULL || WorldSettings->bEnableNavigationSystem)
		{
			NavSys = NewObject<UNavigationSystem>(WorldOwner, GEngine->NavigationSystemClass);		
			WorldOwner->SetNavigationSystem(NavSys);
		}
	}
#endif

	return NavSys;
}

void UNavigationSystem::InitializeForWorld(class UWorld* World, FNavigationSystem::EMode Mode)
{
	if (World)
	{
		UNavigationSystem* NavSys = World->GetNavigationSystem();
		if (NavSys == NULL)
		{
			NavSys = CreateNavigationSystem(World);
		}

		if (NavSys)
		{
			NavSys->OnWorldInitDone(Mode);
		}
	}
}

UNavigationSystem* UNavigationSystem::GetCurrent(UWorld* World)
{
	return World ? World->GetNavigationSystem() : NULL;
}

UNavigationSystem* UNavigationSystem::GetCurrent(class UObject* WorldContextObject)
{
	UWorld* World = NULL;

	if (WorldContextObject != NULL)
	{
		World = GEngine->GetWorldFromContextObject(WorldContextObject);
	}

	return World ? World->GetNavigationSystem() : NULL;
}

ANavigationData* UNavigationSystem::GetNavDataWithID(const uint16 NavDataID) const
{
	for (int32 NavDataIndex = 0; NavDataIndex < NavDataSet.Num(); ++NavDataIndex)
	{
		const ANavigationData* NavData = NavDataSet[NavDataIndex];
		if (NavData != NULL && NavData->GetNavDataUniqueID() == NavDataID)
		{
			return const_cast<ANavigationData*>(NavData);
		}
	}

	return NULL;
}

#if WITH_NAVIGATION_GENERATOR
void UNavigationSystem::AddDirtyArea(const FBox& NewArea, int32 Flags)
{
	if (Flags > 0)
	{
		DirtyAreas.Add(FNavigationDirtyArea(NewArea, Flags));
	}
}

void UNavigationSystem::AddDirtyAreas(const TArray<FBox>& NewAreas, int32 Flags)
{ 
	for (int32 NewAreaIndex = 0; NewAreaIndex < NewAreas.Num(); NewAreaIndex++)
	{
		AddDirtyArea(NewAreas[NewAreaIndex], Flags);
	}
}
#endif // WITH_NAVIGATION_GENERATOR

int32 GetDirtyFlagHelper(int32 UpdateFlags, int32 DefaultValue)
{
	return ((UpdateFlags & UNavigationSystem::OctreeUpdate_Modifiers) != 0) ? ENavigationDirtyFlag::DynamicModifier :
		((UpdateFlags & UNavigationSystem::OctreeUpdate_Geometry) != 0) ? ENavigationDirtyFlag::All :
		DefaultValue;
}

FSetElementId UNavigationSystem::RegisterNavigationRelevantActor(AActor* Actor, int32 UpdateFlags) 
{ 
	FSetElementId RequestId;

#if WITH_EDITOR
	if (AreFakeComponentChangesBeingApplied())
	{
		// SMALL HACK: don't have to unregister actor, Static lighting system is refreshing components in world
		return RequestId;
	}
#endif
	
	if (Actor->IsNavigationRelevant())
	{
		INavRelevantActorInterface* NavRelevantActor = InterfaceCast<INavRelevantActorInterface>(Actor);
		UNavigationProxy* ProxyOb = NavRelevantActor ? NavRelevantActor->GetNavigationProxy() : NULL;
		const bool bPerComponentNavigation = NavRelevantActor ? NavRelevantActor->DoesSupplyPerComponentNavigationCollision() : false;
		
		// actor's shouldn't use proxy and per component navigation at the same time (conflict ownership of octree element)
		if (bPerComponentNavigation && ProxyOb)
		{
			UE_LOG(LogNavigation, Warning, TEXT("Clearing navigation proxy of %s (provides per component data!)"), *GetNameSafe(Actor));
			ProxyOb = NULL;
		}

		// dirty proxy will force update
		if (ProxyOb && ProxyOb->bDirty)
		{
			ProxyOb->bDirty = false;

			UnregisterNavigationRelevantActor(Actor, UpdateFlags | OctreeUpdate_Refresh);
		}

		UObject* ElementOwner = ProxyOb ? (UObject*)ProxyOb : Actor;
		RequestId = RegisterNavOctreeElement(ElementOwner, UpdateFlags, FNavigationSystem::InvalidBoundingBox);
	}

	return RequestId;
}

FSetElementId UNavigationSystem::RegisterNavOctreeElement(class UObject* ElementOwner, int32 UpdateFlags, const FBox& Bounds)
{
	FSetElementId SetId;

#if WITH_EDITOR
	if (AreFakeComponentChangesBeingApplied())
	{
		// SMALL HACK: don't have to unregister actor, Static lighting system is refreshing components in world
		return SetId;
	}
#endif

	if (NavOctree == NULL || ElementOwner == NULL)
	{
		return SetId;
	}

	const FOctreeElementId* ElementId = GetObjectsNavOctreeId(ElementOwner);
	if (ElementId == NULL)
	{
		FNavigationDirtyElement UpdateInfo(ElementOwner, GetDirtyFlagHelper(UpdateFlags, 0));
		if (!(Bounds == FNavigationSystem::InvalidBoundingBox))
		{
			UpdateInfo.SetBounds(Bounds);
		}

		SetId = PendingOctreeUpdates.FindId(UpdateInfo);
		if (SetId.IsValidId())
		{
			// make sure this request stays, in case it has been invalidated already
			PendingOctreeUpdates[SetId] = UpdateInfo;
		}
		else
		{
			SetId = PendingOctreeUpdates.Add(UpdateInfo);
		}
	}

	return SetId;
}

void UNavigationSystem::AddElementToNavOctree(const FNavigationDirtyElement& DirtyElement)
{
	// handle invalidated requests first
	if (DirtyElement.bInvalidRequest)
	{
		if (DirtyElement.bHasPrevData)
		{
#if WITH_NAVIGATION_GENERATOR
			AddDirtyArea(DirtyElement.PrevBounds, DirtyElement.PrevFlags);
#endif // WITH_NAVIGATION_GENERATOR
		}
		
		return;
	}

	// no check for owner's validity are performed, nor its presence
	// in NavOctree - function assumes callee responsibility in this 
	// regard

	UObject* ElementOwner = DirtyElement.Owner.Get();
	AActor* ActorOwner = Cast<AActor>(ElementOwner);
	UActorComponent* CompOwner = Cast<UActorComponent>(ElementOwner);
	
	UNavigationProxy* NavProxy = Cast<UNavigationProxy>(ElementOwner);
	if (NavProxy)
	{
		ActorOwner = NavProxy->MyOwner;
	}

	if (ElementOwner == NULL ||
		(ActorOwner && ActorOwner->GetWorld() == NULL) ||
		(CompOwner && CompOwner->GetWorld() == NULL))
	{
		return;
	}

	if (ActorOwner)
	{
		INavRelevantActorInterface* NavRelevantActor = InterfaceCast<INavRelevantActorInterface>(ActorOwner);
		const bool bPerComponentNavigation = NavRelevantActor ? NavRelevantActor->DoesSupplyPerComponentNavigationCollision() : false;

		if (bPerComponentNavigation)
		{
			TArray<UActorComponent*> Components;
			ActorOwner->GetComponents(Components);
			for (int32 ComponentIndex = 0; ComponentIndex < Components.Num(); ComponentIndex++)
			{
				UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Components[ComponentIndex]);
				if (PrimComp && PrimComp->CanEverAffectNavigation())
				{
					AddComponentElementToNavOctree(PrimComp, DirtyElement, PrimComp->Bounds.GetBox());
				}
				else
				{
					UNavRelevantComponent* NavRelevantComponent = Cast<UNavRelevantComponent>(Components[ComponentIndex]);
					if (NavRelevantComponent && NavRelevantComponent->IsNavigationRelevant())
					{
						NavRelevantComponent->OnOwnerRegistered();
						AddComponentElementToNavOctree(NavRelevantComponent, DirtyElement, NavRelevantComponent->Bounds);
					}
				}
			}
		}
		else
		{
			AddActorElementToNavOctree(ActorOwner, DirtyElement);
		}
	}
	else
	{
		AddComponentElementToNavOctree(CompOwner, DirtyElement, DirtyElement.BoundsOverride);
	}
}

void UNavigationSystem::AddActorElementToNavOctree(class AActor* Actor, const FNavigationDirtyElement& DirtyElement)
{
	ensure(GetObjectsNavOctreeId(Actor) == NULL);

	// notify relevant components about adding owner to octree
	TArray<UNavRelevantComponent*> Components;
	Actor->GetComponents(Components);
	for (int32 ComponentIndex = 0; ComponentIndex < Components.Num(); ComponentIndex++)
	{
		UNavRelevantComponent* NavRelevantComponent = Components[ComponentIndex];
		if (NavRelevantComponent->IsNavigationRelevant())
		{
			NavRelevantComponent->OnOwnerRegistered();
		}
	}

	FNavigationOctreeElement GeneratedData;
	NavOctree->AddActor(Actor, GeneratedData);

#if WITH_NAVIGATION_GENERATOR && NAVOCTREE_CONTAINS_COLLISION_DATA
	const FBox BBox = GeneratedData.Bounds.GetBox();
	const bool bValidBBox = BBox.IsValid && !BBox.GetSize().IsNearlyZero();
	if (bValidBBox && !GeneratedData.IsEmpty())
	{
		const int32 DirtyFlag = DirtyElement.FlagsOverride ? DirtyElement.FlagsOverride : GeneratedData.Data.GetDirtyFlag();
		AddDirtyArea(BBox, DirtyFlag);
	}
#endif // WITH_NAVIGATION_GENERATOR
}

void UNavigationSystem::AddComponentElementToNavOctree(class UActorComponent* ActorComp, const FNavigationDirtyElement& DirtyElement, const FBox& Bounds)
{
	ensure(GetObjectsNavOctreeId(ActorComp) == NULL);

	FNavigationOctreeElement GeneratedData;
	NavOctree->AddComponent(ActorComp, Bounds, GeneratedData);

#if WITH_NAVIGATION_GENERATOR && NAVOCTREE_CONTAINS_COLLISION_DATA
	const FBox BBox = GeneratedData.Bounds.GetBox();
	const bool bValidBBox = BBox.IsValid && !BBox.GetSize().IsNearlyZero();
	if (bValidBBox && !GeneratedData.IsEmpty())
	{
		const int32 DirtyFlag = DirtyElement.FlagsOverride ? DirtyElement.FlagsOverride : GeneratedData.Data.GetDirtyFlag();
		AddDirtyArea(BBox, DirtyFlag);
	}
#endif // WITH_NAVIGATION_GENERATOR
}

bool UNavigationSystem::GetNavOctreeElementData(class UObject* NodeOwner, int32& DirtyFlags, FBox& DirtyBounds)
{
	const FOctreeElementId* ElementId = GetObjectsNavOctreeId(NodeOwner);
	if (ElementId != NULL)
	{
		if (ElementId->IsValidId() && NavOctree->IsValidElementId(*ElementId))
		{
#if WITH_NAVIGATION_GENERATOR && NAVOCTREE_CONTAINS_COLLISION_DATA
			// mark area occupied by given actor as dirty
			FNavigationOctreeElement& ElementData = NavOctree->GetElementById(*ElementId);
			DirtyFlags = ElementData.Data.GetDirtyFlag();
			DirtyBounds = ElementData.Bounds.GetBox();
#endif // WITH_NAVIGATION_GENERATOR

			return true;
		}
	}

	return false;
}

void UNavigationSystem::UnregisterNavigationRelevantActor(AActor* Actor, int32 UpdateFlags)
{
#if WITH_EDITOR
	if (AreFakeComponentChangesBeingApplied())
	{
		// SMALL HACK: don't have to unregister actor, Static lighting system is refreshing components in world
		return;
	}
#endif

	INavRelevantActorInterface* NavRelevantActor = InterfaceCast<INavRelevantActorInterface>(Actor);
	UNavigationProxy* ProxyOb = NavRelevantActor ? NavRelevantActor->GetNavigationProxy() : NULL;
	const bool bPerComponentNavigation = NavRelevantActor ? NavRelevantActor->DoesSupplyPerComponentNavigationCollision() : false;

	// actor's shouldn't use proxy and per component navigation at the same time (conflict ownership of octree element)
	if (bPerComponentNavigation && ProxyOb)
	{
		UE_LOG(LogNavigation, Warning, TEXT("Clearing navigation proxy of %s (provides per component data!)"), *GetNameSafe(Actor));
		ProxyOb = NULL;
	}

	if (bPerComponentNavigation)
	{
		TArray<UPrimitiveComponent*> Components;
		Actor->GetComponents(Components);
		for (int32 ComponentIndex = 0; ComponentIndex < Components.Num(); ComponentIndex++)
		{
			UPrimitiveComponent* PrimComp = Components[ComponentIndex];
			if (PrimComp && PrimComp->CanEverAffectNavigation())
			{
				UnregisterNavOctreeElement(PrimComp, UpdateFlags, PrimComp->Bounds.GetBox());
			}
			else
			{
				UNavRelevantComponent* NavRelevantComponent = Cast<UNavRelevantComponent>(Components[ComponentIndex]);
				if (NavRelevantComponent && NavRelevantComponent->IsNavigationRelevant())
				{
					NavRelevantComponent->OnOwnerUnregistered();

					UnregisterNavOctreeElement(NavRelevantComponent, UpdateFlags, NavRelevantComponent->Bounds);
				}
			}
		}
	}
	else
	{
		TArray<UNavRelevantComponent*> Components;
		Actor->GetComponents(Components);
		// notify relevant components about removing owner from octree
		for (int32 ComponentIndex = 0; ComponentIndex < Components.Num(); ComponentIndex++)
		{
			UNavRelevantComponent* NavRelevantComponent = Components[ComponentIndex];
			if (NavRelevantComponent && NavRelevantComponent->IsNavigationRelevant())
			{
				NavRelevantComponent->OnOwnerUnregistered();
			}
		}

		UObject* ElementOwner = ProxyOb ? (UObject*)ProxyOb : Actor;
		UnregisterNavOctreeElement(ElementOwner, UpdateFlags, FNavigationSystem::InvalidBoundingBox);
	}
}

void UNavigationSystem::UnregisterNavOctreeElement(class UObject* ElementOwner, int32 UpdateFlags, const FBox& Bounds)
{
#if WITH_EDITOR
	if (AreFakeComponentChangesBeingApplied())
	{
		// SMALL HACK: don't have to unregister actor, Static lighting system is refreshing components in world
		return;
	}
#endif
	if (NavOctree == NULL || ElementOwner == NULL)
	{
		return;
	}

	const FOctreeElementId* ElementId = GetObjectsNavOctreeId(ElementOwner);
	if (ElementId != NULL)
	{
		if (ElementId->IsValidId() && NavOctree->IsValidElementId(*ElementId))
		{
#if WITH_NAVIGATION_GENERATOR && NAVOCTREE_CONTAINS_COLLISION_DATA
			// mark area occupied by given actor as dirty
			FNavigationOctreeElement& ElementData = NavOctree->GetElementById(*ElementId);
			const int32 DirtyFlag = GetDirtyFlagHelper(UpdateFlags, ElementData.Data.GetDirtyFlag());
			AddDirtyArea(ElementData.Bounds.GetBox(), DirtyFlag);
#endif // WITH_NAVIGATION_GENERATOR
			NavOctree->RemoveNode(ElementId);
		}
		// @todo could use a way to "pop" an element from ObjectToOctreeId map
		// co that above I could get a copy of FOctreeElementId while destroying 
		// map's copy.
		RemoveObjectsNavOctreeId(ElementOwner);
	}

	// mark pending update as invalid, it will be dirtied according to currently active settings
	const bool bCanInvalidateQueue = (UpdateFlags & OctreeUpdate_Refresh) == 0;
	if (bCanInvalidateQueue)
	{
		const FSetElementId RequestId = PendingOctreeUpdates.FindId(ElementOwner);
		if (RequestId.IsValidId())
		{
			PendingOctreeUpdates[RequestId].bInvalidRequest = true;
		}
	}
}

void UNavigationSystem::UpdateNavOctree(class AActor* Actor, int32 UpdateFlags)
{
	INC_DWORD_STAT(STAT_Navigation_UpdateNavOctree);

	if (Actor->IsNavigationRelevant())
	{
		const INavRelevantActorInterface* NavRelevantActor = InterfaceCast<INavRelevantActorInterface>(Actor);
		const bool bPerComponentNavigation = NavRelevantActor ? NavRelevantActor->DoesSupplyPerComponentNavigationCollision() : false;
		if (bPerComponentNavigation)
		{
			TArray<UActorComponent*> Components;
			Actor->GetComponents(Components);
			// make sure it's unregistered correctly
			for (int32 ComponentIndex = 0; ComponentIndex < Components.Num(); ComponentIndex++)
			{
				UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Components[ComponentIndex]);
				if (PrimComp && PrimComp->CanEverAffectNavigation())
				{
					UnregisterNavOctreeElement(PrimComp, UpdateFlags | OctreeUpdate_Refresh, PrimComp->Bounds.GetBox());
				}
				else
				{
					UNavRelevantComponent* NavRelevantComponent = Cast<UNavRelevantComponent>(Components[ComponentIndex]);
					if (NavRelevantComponent && NavRelevantComponent->IsNavigationRelevant())
					{
						UnregisterNavOctreeElement(NavRelevantComponent, UpdateFlags | OctreeUpdate_Refresh, NavRelevantComponent->Bounds);
					}
				}
			}
		}

		UNavigationProxy* ProxyOb = NavRelevantActor ? NavRelevantActor->GetNavigationProxy() : NULL;
		UObject* NodeOwner = (ProxyOb && !bPerComponentNavigation) ? (UObject*)ProxyOb : Actor;

		UpdateNavOctreeElement(NodeOwner, UpdateFlags, FNavigationSystem::InvalidBoundingBox);
	}
}

void UNavigationSystem::UpdateNavOctree(class UActorComponent* ActorComp, int32 UpdateFlags)
{
	INC_DWORD_STAT(STAT_Navigation_UpdateNavOctree);

	UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(ActorComp);
	UNavRelevantComponent* NavComp = Cast<UNavRelevantComponent>(ActorComp);

	if (PrimComp)
	{
		UpdateNavOctreeElement(ActorComp, UpdateFlags, PrimComp->Bounds.GetBox());
	}
	else if (NavComp)
	{
		UpdateNavOctreeElement(ActorComp, UpdateFlags, NavComp->Bounds);
	}	
}

void UNavigationSystem::UpdateNavOctreeElement(class UObject* ElementOwner, int32 UpdateFlags, const FBox& Bounds)
{
	// grab existing octree data
	FBox CurrentBounds;
	int32 CurrentFlags;
	const bool bAlreadyExists = GetNavOctreeElementData(ElementOwner, CurrentFlags, CurrentBounds);

	if (bAlreadyExists)
	{
		// don't invalidate pending requests
		UpdateFlags |= OctreeUpdate_Refresh;

		UnregisterNavOctreeElement(ElementOwner, UpdateFlags, CurrentBounds);
	}

	const FSetElementId RequestId = RegisterNavOctreeElement(ElementOwner, UpdateFlags, Bounds);

	// add original data to pending registration request
	// so it could be dirtied properly when system receive unregister request while actor is still queued
	if (RequestId.IsValidId())
	{
		FNavigationDirtyElement& UpdateInfo = PendingOctreeUpdates[RequestId];
		UpdateInfo.PrevFlags = CurrentFlags;
		UpdateInfo.PrevBounds = CurrentBounds;
		UpdateInfo.bHasPrevData = bAlreadyExists;
	}
}

void UNavigationSystem::PopulateNavOctree()
{
	UWorld* World = GetWorld();

	check(World && NavOctree);

	// now process all actors on all levels
	for (int32 LevelIndex = 0; LevelIndex < World->GetNumLevels(); ++LevelIndex) 
	{
		ULevel* Level = World->GetLevel(LevelIndex);

		for (int32 ActorIndex=0; ActorIndex<Level->Actors.Num(); ActorIndex++)
		{
			AActor* Actor = Level->Actors[ActorIndex];

			const bool bLegalActor = Actor && Actor->IsNavigationRelevant() && !Actor->IsPendingKill();
			if (bLegalActor)
			{
				UpdateNavOctree(Actor);
			}
		}
	}
}

void UNavigationSystem::FindElementsInNavOctree(const FBox& QueryBox, const FNavigationOctreeFilter& Filter, TArray<FNavigationOctreeElement>& Elements)
{
	for (FNavigationOctree::TConstElementBoxIterator<> It(*NavOctree, QueryBox); It.HasPendingElements(); It.Advance())
	{
		const FNavigationOctreeElement& Element = It.GetCurrentElement();
		if (Element.IsMatchingFilter(Filter))
		{
			Elements.Add(Element);
		}		
	}
}

void UNavigationSystem::ReleaseInitialBuildingLock()
{
	if (bInitialBuildingLocked == false)
	{
		return;
	}

	if (bInitialBuildingLockActive == true)
	{
		bInitialBuildingLockActive = false;
		if (bNavigationBuildingLocked == false)
		{
#if WITH_NAVIGATION_GENERATOR
			// apply pending changes
			{
				SCOPE_CYCLE_COUNTER(STAT_Navigation_AddingActorsToNavOctree);

				SCOPE_CYCLE_COUNTER(STAT_Navigation_BuildTime)
				STAT(double ThisTime = 0);
				{
					SCOPE_SECONDS_COUNTER(ThisTime);
					for (TSet<FNavigationDirtyElement>::TIterator It(PendingOctreeUpdates); It; ++It)
					{
						AddElementToNavOctree(*It);
					}
				}
				INC_FLOAT_STAT_BY(STAT_Navigation_CumulativeBuildTime,(float)ThisTime*1000);
			}

			PendingOctreeUpdates.Empty(32);
			// clear dirty areas - forced navigation unlocking is supposed to rebuild the whole navigation
			DirtyAreas.Reset();

			// if navigation building is not blocked for other reasons then rebuild
			// bForce == true to skip bNavigationBuildingLocked test
			NavigationBuildingUnlock(/*bForce = */true);
#endif // WITH_NAVIGATION_GENERATOR
		}
	}
}

#if WITH_EDITOR
void UNavigationSystem::UpdateLevelCollision(ULevel* InLevel)
{
	if (InLevel != NULL)
	{
		UWorld* World = GetWorld();
		OnLevelRemovedFromWorld(InLevel, World);
		OnLevelAddedToWorld(InLevel, World);
	}
}

void UNavigationSystem::OnEditorModeChanged(FEdMode* Mode, bool IsEntering)
{
	if (Mode == NULL)
	{
		return;
	}

	if (IsEntering == false && Mode->GetID() == FBuiltinEditorModes::EM_Geometry)
	{
		// check if any of modified brushes belongs to an ANavMeshBoundsVolume
		FEdModeGeometry* GeometryMode = (FEdModeGeometry*)Mode;
		for (auto GeomObjectIt = GeometryMode->GeomObjectItor(); GeomObjectIt; GeomObjectIt++)
		{
			ANavMeshBoundsVolume* Volume = Cast<ANavMeshBoundsVolume>((*GeomObjectIt)->GetActualBrush());
			if (Volume)
			{
				OnNavigationBoundsUpdated(Volume);
			}
		}
	}
}
#endif

#if WITH_NAVIGATION_GENERATOR
void UNavigationSystem::OnNavigationBoundsUpdated(AVolume* NavVolume)
{
	if (Cast<ANavMeshBoundsVolume>(NavVolume) != NULL)
	{
		const bool bIsInGame = GIsEditor == false || NavVolume->GetWorld()->IsPlayInEditor();

		if (bNavDataRemovedDueToMissingNavBounds)
		{
			PopulateNavOctree();
			bNavDataRemovedDueToMissingNavBounds = false;
		}
	
		if (NavDataSet.Num() == 0)
		{
			if (NavDataRegistrationQueue.Num() > 0)
			{
				ProcessRegistrationCandidates();
			}

			if (NavDataSet.Num() == 0)
			{
				SpawnMissingNavigationData();
				ProcessRegistrationCandidates();

				for (int32 NavDataIndex = 0; NavDataIndex < NavDataSet.Num(); ++NavDataIndex)
				{
					ANavigationData* NavData = NavDataSet[NavDataIndex];
					if (NavData != NULL && (NavData->bRebuildAtRuntime || (GIsEditor && !bIsInGame)))
					{
						NavData->GetGenerator(FNavigationSystem::Create);
					}
				}
			}
		}

#if WITH_RECAST
		if (IsNavigationBuildingLocked() == false)
		{
			// rebuild all navmeshes
			// NOTE: this will most probably be done in editor, if required to be performed at runtime may require some optimizations
			for (int32 NavDataIndex = 0; NavDataIndex < NavDataSet.Num(); ++NavDataIndex)
			{
				ANavigationData* NavData = NavDataSet[NavDataIndex];
				if (NavData != NULL && NavData->GetGenerator() != NULL)
				{
					if ( Cast<ARecastNavMesh>(NavData) != NULL && (bIsInGame == false || ((ARecastNavMesh*)NavData)->bRebuildAtRuntime) )
					{
						// there are two possible cases here:
						// 1. new volume is totally inside original generation bounds
						// 2. it expands current bounds.
						// @TODO currently handling every case as 1), 2) is an optimization
						NavData->GetGenerator(FNavigationSystem::Create)->OnNavigationBoundsUpdated(NavVolume);
					}
				}
			}
		}
#endif // WITH_RECAST
	}
}

void UNavigationSystem::Build()
{
	if (NavDataClasses.Num() == 0 || IsThereAnywhereToBuildNavigation() == false)
	{
		return;
	}

	const double BuildStartTime = FPlatformTime::Seconds();

	SpawnMissingNavigationData();

	// make sure freshly created navigation instances are registered before we try to build them
	ProcessRegistrationCandidates();

	// and now iterate through all registered and just build them
	RebuildAll();

	UE_LOG(LogNavigation, Display, TEXT("UNavigationSystem::Build total execution time: %.5f"), float(FPlatformTime::Seconds() - BuildStartTime));
}

void UNavigationSystem::SpawnMissingNavigationData()
{
	DoInitialSetup();

	const int32 SupportedAgentsCount = SupportedAgents.Num();
	check(SupportedAgentsCount >= 0);
	
	// Bit array might be a bit of an overkill here, but this function will be called very rarely
	TBitArray<> AlreadyInstantiated(false, SupportedAgentsCount);
	uint8 NumberFound = 0;
	UWorld* NavWorld = GetWorld();

	// 1. check whether any of required navigation data has already been instantiated
	for (TActorIterator<ARecastNavMesh> It(NavWorld); It && NumberFound < SupportedAgentsCount; ++It)
	{
		ARecastNavMesh* Nav = (*It);
		if (Nav != NULL && Nav->GetTypedOuter<UWorld>() == NavWorld && Nav->IsPendingKill() == false)
		{
			// find out which one it is
			const FNavDataConfig* AgentProps = SupportedAgents.GetTypedData();
			for (int32 AgentIndex = 0; AgentIndex < SupportedAgentsCount; ++AgentIndex, ++AgentProps)
			{
				if (AlreadyInstantiated[AgentIndex] == true)
				{
					// already present, skip
					continue;
				}

				if (Nav->DoesSupportAgent(*AgentProps) == true)
				{
					AlreadyInstantiated[AgentIndex] = true;
					++NumberFound;
					break;
				}
			}				
		}
	}

	// 2. for any not already instantiated navigation data call creator functions
	if (NumberFound < SupportedAgentsCount)
	{
		for (int32 AgentIndex = 0; AgentIndex < SupportedAgentsCount; ++AgentIndex)
		{
			if (AlreadyInstantiated[AgentIndex] == false)
			{
				bool bHandled = false;

				for (int32 NavClassIndex = 0; NavClassIndex < NavDataClasses.Num(); ++NavClassIndex)
				{
					ANavigationData* Instance = CreateNavigationDataInstance(NavDataClasses[NavClassIndex], NavWorld, SupportedAgents[AgentIndex]);

					if (Instance != NULL)
					{
						RequestRegistration(Instance);
						bHandled = true;
						break;
					}
				}

				if (bHandled == false)
				{
					UE_LOG(LogNavigation, Warning, TEXT("Was not able to create navigation data for SupportedAgent %s (index %d)")
						, *(SupportedAgents[AgentIndex].Name.ToString()), AgentIndex);
				}
			}
		}

		ProcessRegistrationCandidates();
	}
	
	if (MainNavData == NULL || MainNavData->IsPendingKill())
	{
		// update 
		MainNavData = GetMainNavData(FNavigationSystem::DontCreate);
	}
}

ANavigationData* UNavigationSystem::CreateNavigationDataInstance(TSubclassOf<class ANavigationData> NavDataClass, UWorld* World, const FNavDataConfig& NavConfig)
{
	check(World);

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.OverrideLevel = World->PersistentLevel;
	ANavigationData* Instance = World->SpawnActor<ANavigationData>(*NavDataClass, SpawnInfo);

	if (Instance != NULL)
	{
		Instance->SetConfig(NavConfig);
		if (NavConfig.Name != NAME_None)
		{
			FString StrName = FString::Printf(TEXT("%s-%s"), *(Instance->GetFName().GetPlainNameString()), *(NavConfig.Name.ToString()));
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

void UNavigationSystem::OnUpdateStreamingStarted()
{
	NavigationBuildingLock();
}

void UNavigationSystem::OnUpdateStreamingFinished()
{
	NavigationBuildingUnlock();

	if (bWholeWorldNavigable == true)
	{
		const FBox NavBounds = NavigableWorldBounds;
		const FBox NewNavBounds = GetWorldBounds();
		if (FVector::DistSquared(NavBounds.GetCenter(), NewNavBounds.GetCenter()) > KINDA_SMALL_NUMBER
			|| FVector::DistSquared(NavBounds.GetExtent(), NewNavBounds.GetExtent()) > KINDA_SMALL_NUMBER)
		{
			RebuildAll();
		}
	}
}

void UNavigationSystem::OnPIEStart()
{
	EnableAllGenerators(false);
}

void UNavigationSystem::OnPIEEnd()
{
	// it's set to true, but in editor it depends on user settings
	EnableAllGenerators(bNavigationAutoUpdateEnabled);
}

void UNavigationSystem::EnableAllGenerators(bool bEnable, bool bForce)
{
	if (bEnable)
	{
		NavigationBuildingUnlock(bForce);
	}
	else
	{
		NavigationBuildingLock();
	}
}

void UNavigationSystem::NavigationBuildingLock()
{
	if (bNavigationBuildingLocked == true)
	{
		return;
	}

	GetMainNavData(bBuildNavigationAtRuntime && IsThereAnywhereToBuildNavigation() ? FNavigationSystem::Create : FNavigationSystem::DontCreate);

	bNavigationBuildingLocked = true;

	// lock all generators
	for (int32 NavDataIndex = 0; NavDataIndex < NavDataSet.Num(); ++NavDataIndex)
	{
		ANavigationData* NavData = NavDataSet[NavDataIndex];
		if (NavData != NULL && NavData->GetGenerator() != NULL)
		{
			NavData->GetGenerator(FNavigationSystem::DontCreate)->OnNavigationBuildingLocked();
		}
	}
}

void UNavigationSystem::NavigationBuildingUnlock(bool bForce)
{
	if ((bNavigationBuildingLocked == true && bInitialBuildingLockActive == false) || bForce == true)
	{
		bNavigationBuildingLocked = false;
		bInitialBuildingLockActive = false;

		// unlock all generators
		for (int32 NavDataIndex = 0; NavDataIndex < NavDataSet.Num(); ++NavDataIndex)
		{
			ANavigationData* NavData = NavDataSet[NavDataIndex];
			if (NavData != NULL && NavData->GetGenerator() != NULL)
			{
				NavData->GetGenerator(FNavigationSystem::DontCreate)->OnNavigationBuildingUnlocked(bForce);
			}
		}
	}
	else if (bInitialBuildingLockActive == true)
	{
		// remember that other reasons to lock building are no longer there
		// so we can release building lock as soon as bInitialBuildingLockActive 
		// turns true
		bNavigationBuildingLocked = false;
	}
}

void UNavigationSystem::RebuildAll()
{
	for (int32 NavDataIndex = 0; NavDataIndex < NavDataSet.Num(); ++NavDataIndex)
	{
		ANavigationData* NavData = NavDataSet[NavDataIndex];
		if (NavData != NULL && NavData->GetGenerator(FNavigationSystem::Create) != NULL)
		{
			NavData->GetGenerator(FNavigationSystem::DontCreate)->RebuildAll();
		}
	}
}

bool UNavigationSystem::IsNavigationBuildInProgress(bool bCheckDirtyToo)
{
	bool bRet = false;

	if (NavDataSet.Num() == 0)
	{
		// update nav data. If none found this is the place to create one
		GetMainNavData(FNavigationSystem::DontCreate);
	}
	
	for (int32 NavDataIndex = 0; NavDataIndex < NavDataSet.Num(); ++NavDataIndex)
	{
		ANavigationData* NavData = NavDataSet[NavDataIndex];
		if (NavData != NULL && NavData->GetGenerator() != NULL 
			&& NavData->GetGenerator()->IsBuildInProgress(bCheckDirtyToo) == true)
		{
			bRet = true;
			break;
		}
	}

	return bRet;
}

void UNavigationSystem::OnLevelAddedToWorld(ULevel* InLevel, UWorld* InWorld)
{
	if (InWorld == GetWorld())
	{
		AddLevelCollisionToOctree(InLevel);

		FBox NavigableLevelBounds = GetLevelBounds(InLevel);
		if (NavigableLevelBounds.IsValid)
		{
			AddDirtyArea(NavigableLevelBounds, ENavigationDirtyFlag::All);
		}
	}
}

void UNavigationSystem::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
	if (InWorld == GetWorld())
	{
		RemoveLevelCollisionFromOctree(InLevel);

		FBox NavigableLevelBounds = GetLevelBounds(InLevel);
		if (NavigableLevelBounds.IsValid)
		{
			AddDirtyArea(NavigableLevelBounds, ENavigationDirtyFlag::All);
		}
	}
}

void UNavigationSystem::AddLevelCollisionToOctree(ULevel* Level)
{
#if NAVOCTREE_CONTAINS_COLLISION_DATA
	const TArray<FVector>* LevelGeom = Level ? Level->GetStaticNavigableGeometry() : NULL;
	if (LevelGeom && NavOctree)
	{
		FNavigationOctreeElement BSPElem;
		FRecastNavMeshGenerator::ExportVertexSoupGeometry(*LevelGeom, BSPElem.Data);

		BSPElem.Bounds = BSPElem.Data.Bounds;
		BSPElem.Owner = Level;
		
		NavOctree->AddNode(BSPElem);
	}
#endif // NAVOCTREE_CONTAINS_COLLISION_DATA
}

void UNavigationSystem::RemoveLevelCollisionFromOctree(ULevel* Level)
{
#if NAVOCTREE_CONTAINS_COLLISION_DATA
	const FOctreeElementId* ElementId = GetObjectsNavOctreeId(Level);
	if (ElementId != NULL)
	{
		NavOctree->RemoveNode(ElementId);
		RemoveObjectsNavOctreeId(Level);
	}
#endif // NAVOCTREE_CONTAINS_COLLISION_DATA
}

#endif // WITH_NAVIGATION_GENERATOR

void UNavigationSystem::OnPostLoadMap()
{
	UE_LOG(LogNavigation, Log, TEXT("UNavigationSystem::OnPostLoadMap"));

	// if map has been loaded and there are some navigation bounds volumes 
	// then create appropriate navigation structured
	ANavigationData* NavData = GetMainNavData(FNavigationSystem::DontCreate);

	// Do this if there's currently no navigation
	if (NavData == NULL && bAutoCreateNavigationData == true && IsThereAnywhereToBuildNavigation() == true)
	{
		NavData = GetMainNavData(FNavigationSystem::Create);
	}
}

void UNavigationSystem::OnActorMoved(AActor* Actor)
{
#if WITH_NAVIGATION_GENERATOR
	if (Cast<ANavMeshBoundsVolume>(Actor) != NULL)
	{
		OnNavigationBoundsUpdated((ANavMeshBoundsVolume*)Actor);
	}
#endif // WITH_NAVIGATION_GENERATOR
}

void UNavigationSystem::OnNavigationDirtied(const FBox& Bounds)
{
#if WITH_NAVIGATION_GENERATOR
	AddDirtyArea(Bounds, ENavigationDirtyFlag::All);
#endif //WITH_NAVIGATION_GENERATOR
}

void UNavigationSystem::CleanUp()
{
	UE_LOG(LogNavigation, Log, TEXT("UNavigationSystem::CleanUp"));

	if (GEngine)
	{
		GEngine->OnActorMoved().RemoveAll(this);
	}
	FCoreDelegates::PostLoadMap.RemoveAll(this);
#if WITH_NAVIGATION_GENERATOR
	UNavigationSystem::NavigationDirtyEvent.RemoveAll(this);
	FWorldDelegates::LevelAddedToWorld.RemoveAll(this);
	FWorldDelegates::LevelRemovedFromWorld.RemoveAll(this);
#endif // WITH_NAVIGATION_GENERATOR

	if (NavOctree != NULL)
	{
		NavOctree->Destroy();
		delete NavOctree;
		NavOctree = NULL;
	}

	ObjectToOctreeId.Empty();

	for (int32 NavDataIndex = 0; NavDataIndex < NavDataSet.Num(); ++NavDataIndex)
	{
		if (NavDataSet[NavDataIndex] != NULL)
		{
			NavDataSet[NavDataIndex]->CleanUp();
		}
	}	

	SetCrowdManager(NULL);

	NavDataSet.Reset();
}

//----------------------------------------------------------------------//
// Kismet functions
//----------------------------------------------------------------------//
FVector UNavigationSystem::ProjectPointToNavigation(UObject* WorldContextObject, const FVector& Point, ANavigationData* NavData, TSubclassOf<UNavigationQueryFilter> FilterClass)
{
	FNavLocation ProjectedPoint(Point);

	UWorld* World = GEngine->GetWorldFromContextObject( WorldContextObject );
	UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(World);
	if (NavSys)
	{
		ANavigationData* UseNavData = NavData ? NavData : NavSys->GetMainNavData(FNavigationSystem::DontCreate);
		NavSys->ProjectPointToNavigation(Point, ProjectedPoint, INVALID_NAVEXTENT, UseNavData, UNavigationQueryFilter::GetQueryFilter(UseNavData, FilterClass));
	}

	return ProjectedPoint.Location;
}

FVector UNavigationSystem::GetRandomPoint(UObject* WorldContextObject, ANavigationData* NavData, TSubclassOf<UNavigationQueryFilter> FilterClass)
{
	FNavLocation RandomPoint;

	UWorld* World = GEngine->GetWorldFromContextObject( WorldContextObject );
	UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(World);
	if (NavSys)
	{
		ANavigationData* UseNavData = NavData ? NavData : NavSys->GetMainNavData(FNavigationSystem::DontCreate);
		NavSys->GetRandomPoint(RandomPoint, UseNavData, UNavigationQueryFilter::GetQueryFilter(UseNavData, FilterClass));
	}

	return RandomPoint.Location;
}


FVector UNavigationSystem::GetRandomPointInRadius(UObject* WorldContextObject, const FVector& Origin, float Radius, ANavigationData* NavData, TSubclassOf<UNavigationQueryFilter> FilterClass)
{
	FNavLocation RandomPoint;

	UWorld* World = GEngine->GetWorldFromContextObject( WorldContextObject );
	UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(World);
	if (NavSys)
	{
		ANavigationData* UseNavData = NavData ? NavData : NavSys->GetMainNavData(FNavigationSystem::DontCreate);
		NavSys->GetRandomPointInRadius(Origin, Radius, RandomPoint, UseNavData, UNavigationQueryFilter::GetQueryFilter(UseNavData, FilterClass));
	}

	return RandomPoint.Location;
}

ENavigationQueryResult::Type UNavigationSystem::GetPathCost(UObject* WorldContextObject, const FVector& PathStart, const FVector& PathEnd, float& OutPathCost, ANavigationData* NavData, TSubclassOf<UNavigationQueryFilter> FilterClass)
{
	UWorld* World = GEngine->GetWorldFromContextObject( WorldContextObject );
	UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(World);
	if (NavSys)
	{
		ANavigationData* UseNavData = NavData ? NavData : NavSys->GetMainNavData(FNavigationSystem::DontCreate);
		return NavSys->GetPathCost(PathStart, PathEnd, OutPathCost, UseNavData, UNavigationQueryFilter::GetQueryFilter(UseNavData, FilterClass));
	}

	return ENavigationQueryResult::Error;
}

ENavigationQueryResult::Type UNavigationSystem::GetPathLength(UObject* WorldContextObject, const FVector& PathStart, const FVector& PathEnd, float& OutPathLength, class ANavigationData* NavData, TSubclassOf<UNavigationQueryFilter> FilterClass)
{
	float PathLength = 0.f;

	UWorld* World = GEngine->GetWorldFromContextObject( WorldContextObject );
	UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(World);
	if (NavSys)
	{
		ANavigationData* UseNavData = NavData ? NavData : NavSys->GetMainNavData(FNavigationSystem::DontCreate);
		return NavSys->GetPathLength(PathStart, PathEnd, OutPathLength, UseNavData, UNavigationQueryFilter::GetQueryFilter(UseNavData, FilterClass));
	}

	return ENavigationQueryResult::Error;
}

bool UNavigationSystem::IsNavigationBeingBuilt(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject); 
	if (World != NULL && World->GetNavigationSystem() != NULL)
	{
#if WITH_NAVIGATION_GENERATOR
		return World->GetNavigationSystem()->IsNavigationBuildInProgress();
#else
		return false;
#endif
	}

	return false;
}

//----------------------------------------------------------------------//
// HACKS!!!
//----------------------------------------------------------------------//
bool UNavigationSystem::ShouldGeneratorRun(const class FNavDataGenerator* Generator) const
{
#if WITH_NAVIGATION_GENERATOR
	if (Generator != NULL)
	{
		for (int32 NavDataIndex = 0; NavDataIndex < NavDataSet.Num(); ++NavDataIndex)
		{
			ANavigationData* NavData = NavDataSet[NavDataIndex];
			if (NavData != NULL && NavData->GetGenerator(FNavigationSystem::DontCreate) == Generator)
			{
				return true;
			}
		}
	}
#endif
	return false;
}

bool UNavigationSystem::HandleCycleNavDrawnCommand( const TCHAR* Cmd, FOutputDevice& Ar )
{
	CycleNavigationDataDrawn();

	return true;
}

bool UNavigationSystem::HandleCountNavMemCommand( const TCHAR* Cmd, FOutputDevice& Ar )
{
	for (int32 NavDataIndex = 0; NavDataIndex < NavDataSet.Num(); ++NavDataIndex)
	{
		ANavigationData* NavData = NavDataSet[NavDataIndex];
		if (NavData != NULL)
		{
			NavData->LogMemUsed();
		}
	}
	return true;
}

//----------------------------------------------------------------------//
// Commands
//----------------------------------------------------------------------//
bool FNavigationSystemExec::Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar )
{
	if (InWorld == NULL || InWorld->GetNavigationSystem() == NULL)
	{
		return false;
	}

	UNavigationSystem*  NavSys = InWorld->GetNavigationSystem();

	if (NavSys->NavDataSet.Num() > 0)
	{
		if (FParse::Command(&Cmd, TEXT("CYCLENAVDRAWN")))
		{
			NavSys->HandleCycleNavDrawnCommand( Cmd, Ar );
			// not returning true to enable all navigation systems to cycle their own data
			return false;
		}
		else if (FParse::Command(&Cmd, TEXT("CountNavMem")))
		{
			NavSys->HandleCountNavMemCommand( Cmd, Ar );
			return false;
		}
	}

	return false;
}

void UNavigationSystem::CycleNavigationDataDrawn()
{
	++CurrentlyDrawnNavDataIndex;
	if (CurrentlyDrawnNavDataIndex >= NavDataSet.Num())
	{
		CurrentlyDrawnNavDataIndex = INDEX_NONE;
	}

	for (int32 NavDataIndex = 0; NavDataIndex < NavDataSet.Num(); ++NavDataIndex)
	{
		ANavigationData* NavData = NavDataSet[NavDataIndex];
		if (NavData != NULL)
		{
			const bool bNewEnabledDrawing = (CurrentlyDrawnNavDataIndex == INDEX_NONE) || (NavDataIndex == CurrentlyDrawnNavDataIndex);
			if (bNewEnabledDrawing != NavData->bEnableDrawing)
			{
				NavData->bEnableDrawing = bNewEnabledDrawing;
				NavData->MarkComponentsRenderStateDirty();
			}
		}
	}
}

bool UNavigationSystem::IsNavigationDirty()
{
	for (int32 NavDataIndex=0; NavDataIndex < NavDataSet.Num(); ++NavDataIndex)
	{
		if (NavDataSet[NavDataIndex]->NeedsRebuild())
		{
			return true;
		}
	}

	return false;
}

bool UNavigationSystem::DoesPathIntersectBox(const FNavigationPath* Path, const FBox& Box)
{
	return Path != NULL && Path->DoesIntersectBox(Box);
}