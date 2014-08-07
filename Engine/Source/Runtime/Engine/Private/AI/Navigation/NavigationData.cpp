// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AI/NavDataGenerator.h"
#include "AI/Navigation/NavAreas/NavAreaMeta.h"
#include "AI/Navigation/NavigationData.h"

//----------------------------------------------------------------------//
// FPathFindingQuery
//----------------------------------------------------------------------//
FPathFindingQuery::FPathFindingQuery(const UObject* InOwner, const class ANavigationData* InNavData, const FVector& Start, const FVector& End, TSharedPtr<const FNavigationQueryFilter> SourceQueryFilter, FNavPathSharedPtr InPathInstanceToFill)
: NavData(InNavData)
, Owner(InOwner)
, StartLocation(Start)
, EndLocation(End)
, QueryFilter(SourceQueryFilter)
, PathInstanceToFill(InPathInstanceToFill)
, NavDataFlags(0)
{
	if (SourceQueryFilter.IsValid() == false && NavData.IsValid() == true)
	{
		QueryFilter = NavData->GetDefaultQueryFilter();
	}
}

FPathFindingQuery::FPathFindingQuery(const FPathFindingQuery& Source)
: NavData(Source.NavData)
, Owner(Source.Owner)
, StartLocation(Source.StartLocation)
, EndLocation(Source.EndLocation)
, QueryFilter(Source.QueryFilter)
, PathInstanceToFill(Source.PathInstanceToFill)
, NavDataFlags(Source.NavDataFlags)
{
	if (Source.QueryFilter.IsValid() == false && NavData.IsValid() == true)
	{
		QueryFilter = NavData->GetDefaultQueryFilter();
	}
}

FPathFindingQuery::FPathFindingQuery(FNavPathSharedRef PathToRecalculate, const ANavigationData* NavDataOverride)
: NavData(NavDataOverride != NULL ? NavDataOverride : PathToRecalculate->GetNavigationDataUsed())
, Owner(PathToRecalculate->GetQuerier())
, StartLocation(PathToRecalculate->GetPathFindingStartLocation())
, EndLocation(PathToRecalculate->GetGoalLocation())
, QueryFilter(PathToRecalculate->GetFilter())
, PathInstanceToFill(PathToRecalculate)
, NavDataFlags(0)
{
	if (QueryFilter.IsValid() == false && NavData.IsValid() == true)
	{
		QueryFilter = NavData->GetDefaultQueryFilter();
	}
}

//----------------------------------------------------------------------//
// FAsyncPathFindingQuery
//----------------------------------------------------------------------//
uint32 FAsyncPathFindingQuery::LastPathFindingUniqueID = INVALID_NAVQUERYID;

FAsyncPathFindingQuery::FAsyncPathFindingQuery(const UObject* InOwner, const class ANavigationData* InNavData, const FVector& Start, const FVector& End, const FNavPathQueryDelegate& Delegate, TSharedPtr<const FNavigationQueryFilter> SourceQueryFilter)
: FPathFindingQuery(InOwner, InNavData, Start, End, SourceQueryFilter)
, QueryID(GetUniqueID())
, OnDoneDelegate(Delegate)
{

}

FAsyncPathFindingQuery::FAsyncPathFindingQuery(const FPathFindingQuery& Query, const FNavPathQueryDelegate& Delegate, const EPathFindingMode::Type QueryMode)
: FPathFindingQuery(Query)
, QueryID(GetUniqueID())
, OnDoneDelegate(Delegate)
, Mode(QueryMode)
{

}
//----------------------------------------------------------------------//
// FSupportedAreaData
//----------------------------------------------------------------------//
FSupportedAreaData::FSupportedAreaData(TSubclassOf<UNavArea> NavAreaClass, int32 InAreaID)
	: AreaID(InAreaID), AreaClass(NavAreaClass)
{
	if (AreaClass != NULL)
	{
		AreaClassName = AreaClass->GetName();
	}
	else
	{
		AreaClassName = TEXT("Invalid");
	}
}

//----------------------------------------------------------------------//
// FNavDataGenerator
//----------------------------------------------------------------------//
void FNavDataGenerator::TriggerGeneration()
{
}

//----------------------------------------------------------------------//
// ANavigationData                                                                
//----------------------------------------------------------------------//
ANavigationData::ANavigationData(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, bEnableDrawing(false)
	, bRebuildAtRuntime(false)
	, DataVersion(NAVMESHVER_LATEST)
	, FindPathImplementation(NULL)
	, FindHierarchicalPathImplementation(NULL)
	, bRegistered(false)
	, NavDataUniqueID(GetNextUniqueID())
{
	PrimaryActorTick.bCanEverTick = true;
	bNetLoadOnClient = false;
	bCanBeDamaged = false;
	DefaultQueryFilter = MakeShareable(new FNavigationQueryFilter());
	ObservedPathsTickInterval = 0.5;
}

ANavigationData::~ANavigationData()
{
	CleanUp();
}

uint16 ANavigationData::GetNextUniqueID()
{
	check(IsInGameThread());
	static uint16 StaticID = INVALID_NAVDATA;
	return ++StaticID;
}

void ANavigationData::PostInitProperties()
{
	Super::PostInitProperties();

	if (IsPendingKill() == true)
	{
		return;
	}
	
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		UWorld* WorldOuter = GetWorld();
		
		if (WorldOuter != NULL && WorldOuter->GetNavigationSystem() != NULL)
		{
			WorldOuter->GetNavigationSystem()->RequestRegistration(this);
		}

		RenderingComp = ConstructRenderingComponent();
	}
}

void ANavigationData::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	if (GetWorld() == NULL || GetWorld()->GetNavigationSystem() == NULL)
	{
		CleanUpAndMarkPendingKill();
		return;
	}

	if (IsPendingKill() == true)
	{
		return;
	}

#if WITH_NAVIGATION_GENERATOR
	if (bRebuildAtRuntime == true && NavDataGenerator.IsValid() == false)
	{
		GetGenerator(FNavigationSystem::Create);
	}
#endif
}

void ANavigationData::PostLoad() 
{
	Super::PostLoad();

	InstantiateAndRegisterRenderingComponent();
}

void ANavigationData::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	PurgeUnusedPaths();
	if (NextObservedPathsTickInSeconds >= 0.f)
	{
		NextObservedPathsTickInSeconds -= DeltaTime;
		if (NextObservedPathsTickInSeconds <= 0.f)
		{
			RepathRequests.Reserve(ObservedPaths.Num());

			for (int32 PathIndex = ObservedPaths.Num() - 1; PathIndex >= 0; --PathIndex)
			{
				if (ObservedPaths[PathIndex].IsValid())
				{
					FNavPathSharedPtr SharedPath = ObservedPaths[PathIndex].Pin();
					FNavigationPath* Path = SharedPath.Get();
					EPathObservationResult::Type Result = Path->TickPathObservation();
					switch (Result)
					{
					case EPathObservationResult::NoLongerObserving:
						ObservedPaths.RemoveAtSwap(PathIndex, 1, /*bAllowShrinking=*/false);
						break;

					case EPathObservationResult::NoChange:
						// do nothing
						break;

					case EPathObservationResult::RequestRepath:
						RepathRequests.Add(FNavPathRecalculationRequest(SharedPath, ENavPathUpdateType::GoalMoved));
						break;
					
					default:
						check(false && "unhandled EPathObservationResult::Type in ANavigationData::TickActor");
						break;
					}
				}
				else
				{
					ObservedPaths.RemoveAtSwap(PathIndex, 1, /*bAllowShrinking=*/false);
				}
			}

			if (ObservedPaths.Num() > 0)
			{
				NextObservedPathsTickInSeconds = ObservedPathsTickInterval;
			}
		}
	}

	if (RepathRequests.Num() > 0)
	{
		// @todo batch-process it!
		for (auto RecalcRequest : RepathRequests)
		{
			FPathFindingQuery Query(RecalcRequest.Path);
			// @todo consider supplying NavAgentPropertied from path's querier
			const FPathFindingResult Result = FindPath(FNavAgentProperties(), Query.SetPathInstanceToUpdate(RecalcRequest.Path));

			// partial paths are still valid and can change to full path when moving goal gets back on navmesh
			if (Result.IsSuccessful() || Result.IsPartial())
			{
				RecalcRequest.Path->UpdateLastRepathGoalLocation();
				RecalcRequest.Path->DoneUpdating(RecalcRequest.Reason);
				if (RecalcRequest.Reason == ENavPathUpdateType::NavigationChanged)
				{
					RegisterActivePath(RecalcRequest.Path);
				}
			}
			else
			{
				RecalcRequest.Path->RePathFailed();
			}
		}

		RepathRequests.Reset();
	}
}

void ANavigationData::RerunConstructionScripts()
{
	Super::RerunConstructionScripts();

	InstantiateAndRegisterRenderingComponent();
}

void ANavigationData::OnRegistered() 
{ 
	InstantiateAndRegisterRenderingComponent();

	bRegistered = true; 
}

void ANavigationData::InstantiateAndRegisterRenderingComponent()
{
#if !UE_BUILD_SHIPPING
	if (!IsPendingKill() && (RenderingComp == NULL || RenderingComp->IsPendingKill()))
	{
		if (RenderingComp)
		{
			// rename the old rendering component out of the way
			RenderingComp->Rename(NULL, NULL, REN_DontCreateRedirectors | REN_ForceGlobalUnique | REN_DoNotDirty | REN_NonTransactional);
		}

		RenderingComp = ConstructRenderingComponent();
		if (RenderingComp != NULL)
		{
			RenderingComp->RegisterComponent();
		}
	}
#endif // !UE_BUILD_SHIPPING
}

void ANavigationData::DestroyGenerator()
{
#if WITH_NAVIGATION_GENERATOR
	if (NavDataGenerator.IsValid() == true)
	{
		NavDataGenerator->OnNavigationDataDestroyed(this);
		NavDataGenerator.Reset();
	}
#endif // WITH_NAVIGATION_GENERATOR
};

void ANavigationData::PurgeUnusedPaths()
{
	check(IsInGameThread());

	const int32 Count = ActivePaths.Num();
	FNavPathWeakPtr* WeakPathPtr = (ActivePaths.GetTypedData() + Count - 1);
	for (int32 i = Count - 1; i >= 0; --i, --WeakPathPtr)
	{
		if (WeakPathPtr->IsValid() == false)
		{
			ActivePaths.RemoveAtSwap(i, 1, /*bAllowShrinking=*/false);
		}
	}
}

#if WITH_EDITOR
void ANavigationData::PostEditUndo()
{
	Super::PostEditUndo();
#if WITH_NAVIGATION_GENERATOR
	GetGenerator(FNavigationSystem::Create);
#endif // WITH_NAVIGATION_GENERATOR

	UWorld* WorldOuter = GetWorld();
	if (WorldOuter != NULL && WorldOuter->GetNavigationSystem() != NULL)
	{
		WorldOuter->GetNavigationSystem()->RequestRegistration(this);
	}
}
#endif // WITH_EDITOR

void ANavigationData::Destroyed()
{
	UWorld* WorldOuter = GetWorld();

	bRegistered = false;
	if (WorldOuter != NULL && WorldOuter->GetNavigationSystem() != NULL)
	{
		WorldOuter->GetNavigationSystem()->UnregisterNavData(this);
	}
	
	CleanUp();

	Super::Destroyed();
}

void ANavigationData::CleanUp()
{
	bRegistered = false;
	DestroyGenerator();
}

void ANavigationData::CleanUpAndMarkPendingKill()
{
	CleanUp();
	SetActorHiddenInGame(true);

	// do NOT destroy here! it can be called from PostLoad and will crash in DestroyActor()
	MarkPendingKill();
	MarkComponentsAsPendingKill();
}

bool ANavigationData::CanRebuild() const
{
#if WITH_NAVIGATION_GENERATOR
	return NavDataGenerator.IsValid();
#endif

	return false;
}

#if WITH_NAVIGATION_GENERATOR

FNavDataGenerator* ANavigationData::GetGenerator(FNavigationSystem::ECreateIfEmpty CreateIfNone)
{
	if ((NavDataGenerator.IsValid() == false || NavDataGenerator.GetSharedReferenceCount() <= 0)
		&& CreateIfNone == FNavigationSystem::Create
		&& (bRebuildAtRuntime == true || GetWorld()->IsGameWorld() == false))
	{
		// @todo this is not-that-clear design and is subject to change
		if (NavDataConfig.IsValid() == false 
			&& GetWorld()->GetNavigationSystem() != NULL
			&& GetWorld()->GetNavigationSystem()->SupportedAgents.Num() < 2
			)
		{
			// fill in AgentProps with whatever is the instance's setup
			FillConfig(NavDataConfig);
		}

		FNavDataGenerator* Generator = ConstructGenerator(NavDataConfig);

		if (Generator != NULL)
		{
			// make sure no one has made this sharable yet
			// it's a "check" not an "ensure" since it's really crucial
			check(Generator->HasBeenAlreadyMadeSharable() == false);
			NavDataGenerator = MakeShareable(Generator);
		}
	}

	return NavDataGenerator.IsValid() && NavDataGenerator.GetSharedReferenceCount() > 0 ? NavDataGenerator.Get() : NULL;
}

void ANavigationData::RebuildDirtyAreas(const TArray<FNavigationDirtyArea>& DirtyAreas)
{
	FNavDataGenerator* Generator = GetGenerator(FNavigationSystem::DontCreate);
	if (Generator)
	{
		Generator->RebuildDirtyAreas(DirtyAreas);
	}
}

#endif // WITH_NAVIGATION_GENERATOR

void ANavigationData::DrawDebugPath(FNavigationPath* Path, FColor PathColor, UCanvas* Canvas, bool bPersistent, const uint32 NextPathPointIndex) const
{
	Path->DebugDraw(this, PathColor, Canvas, bPersistent, NextPathPointIndex);
}

void ANavigationData::OnNavAreaAdded(const UClass* NavAreaClass, int32 AgentIndex)
{
	// check if area can be added
	const UNavArea* DefArea = NavAreaClass ? ((UClass*)NavAreaClass)->GetDefaultObject<UNavArea>() : NULL;
	const bool bIsMetaArea = NavAreaClass && NavAreaClass->IsChildOf(UNavAreaMeta::StaticClass());
	if (!DefArea || bIsMetaArea || !DefArea->IsSupportingAgent(AgentIndex))
	{
		UE_LOG(LogNavigation, Verbose, TEXT("%s discarded area %s (valid:%s meta:%s validAgent[%d]:%s)"),
			*GetName(), *GetNameSafe(NavAreaClass),
			DefArea ? TEXT("yes") : TEXT("NO"),
			bIsMetaArea ? TEXT("YES") : TEXT("no"),
			AgentIndex, DefArea->IsSupportingAgent(AgentIndex) ? TEXT("yes") : TEXT("NO"));
		return;
	}

	// check if area is already on supported list
	FString AreaClassName = NavAreaClass->GetName();
	for (int32 i = 0; i < SupportedAreas.Num(); i++)
	{
		if (SupportedAreas[i].AreaClassName == AreaClassName)
		{
			SupportedAreas[i].AreaClass = NavAreaClass;
			AreaClassToIdMap.Add(NavAreaClass, SupportedAreas[i].AreaID);
			UE_LOG(LogNavigation, Verbose, TEXT("%s updated area %s with ID %d"), *GetName(), *AreaClassName, SupportedAreas[i].AreaID);
			return;
		}
	}

	// try adding new one
	const int32 MaxSupported = GetMaxSupportedAreas();
	if (SupportedAreas.Num() >= MaxSupported)
	{
		UE_LOG(LogNavigation, Error, TEXT("%s can't support area %s - limit reached! (%d)"), *GetName(), *AreaClassName, MaxSupported);
		return;
	}

	FSupportedAreaData NewAgentData;
	NewAgentData.AreaClass = NavAreaClass;
	NewAgentData.AreaClassName = AreaClassName;
	NewAgentData.AreaID = GetNewAreaID(NavAreaClass);
	SupportedAreas.Add(NewAgentData);
	AreaClassToIdMap.Add(NavAreaClass, NewAgentData.AreaID);

	UE_LOG(LogNavigation, Verbose, TEXT("%s registered area %s with ID %d"), *GetName(), *AreaClassName, NewAgentData.AreaID);
}

void ANavigationData::OnNavAreaAddedNotify(const UClass* NavAreaClass)
{
	const UNavigationSystem* NavSys = GetWorld()->GetNavigationSystem();
	const int32 AgentIndex = NavSys->GetSupportedAgentIndex(this);

	OnNavAreaAdded(NavAreaClass, AgentIndex);
}

void ANavigationData::OnNavAreaRemoved(const UClass* NavAreaClass)
{
	for (int32 i = 0; i < SupportedAreas.Num(); i++)
	{
		if (SupportedAreas[i].AreaClass == NavAreaClass)
		{
			AreaClassToIdMap.Remove(NavAreaClass);
			SupportedAreas.RemoveAt(i);
			break;
		}
	}
}

void ANavigationData::OnNavAreaRemovedNotify(const UClass* NavAreaClass)
{
	OnNavAreaRemoved(NavAreaClass);
}

void ANavigationData::ProcessNavAreas(const TArray<const UClass*>& AreaClasses, int32 AgentIndex)
{
	for (int32 i = 0; i < AreaClasses.Num(); i++)
	{
		OnNavAreaAdded(AreaClasses[i], AgentIndex);
	}
}

int32 ANavigationData::GetNewAreaID(const UClass* AreaClass) const
{
	int TestId = 0;
	while (TestId < SupportedAreas.Num())
	{
		const bool bIsTaken = IsAreaAssigned(TestId);
		if (bIsTaken)
		{
			TestId++;
		}
		else
		{
			break;
		}
	}

	return TestId;
}

const UClass* ANavigationData::GetAreaClass(int32 AreaID) const
{
	for (int32 i = 0; i < SupportedAreas.Num(); i++)
	{
		if (SupportedAreas[i].AreaID == AreaID)
		{
			return SupportedAreas[i].AreaClass;
		}
	}

	return NULL;
}

bool ANavigationData::IsAreaAssigned(int32 AreaID) const
{
	for (int32 i = 0; i < SupportedAreas.Num(); i++)
	{
		if (SupportedAreas[i].AreaID == AreaID)
		{
			return true;
		}
	}

	return false;
}

int32 ANavigationData::GetAreaID(const UClass* AreaClass) const
{
	const int32* PtrId = AreaClassToIdMap.Find(AreaClass);
	return PtrId ? *PtrId : INDEX_NONE;
}

void ANavigationData::UpdateCustomLink(const class INavLinkCustomInterface* CustomLink)
{
	// no implementation for abstract class
}

TSharedPtr<const FNavigationQueryFilter> ANavigationData::GetQueryFilter(TSubclassOf<class UNavigationQueryFilter> FilterClass) const
{
	return QueryFilters.FindRef(FilterClass);
}

void ANavigationData::StoreQueryFilter(TSubclassOf<class UNavigationQueryFilter> FilterClass, TSharedPtr<const FNavigationQueryFilter> NavFilter)
{
	QueryFilters.Add(FilterClass, NavFilter);
}

void ANavigationData::RemoveQueryFilter(TSubclassOf<class UNavigationQueryFilter> FilterClass)
{
	QueryFilters.Remove(FilterClass);
}

uint32 ANavigationData::LogMemUsed() const
{
	const uint32 MemUsed = ActivePaths.GetAllocatedSize() + SupportedAreas.GetAllocatedSize() +
		QueryFilters.GetAllocatedSize() + AreaClassToIdMap.GetAllocatedSize();

	UE_LOG(LogNavigation, Display, TEXT("%s: ANavigationData: %u\n    self: %d"), *GetName(), MemUsed, sizeof(ANavigationData));	

#if WITH_NAVIGATION_GENERATOR
	if (NavDataGenerator.IsValid())
	{
		NavDataGenerator->LogMemUsed();
	}
#endif // WITH_NAVIGATION_GENERATOR

	return MemUsed;
}
