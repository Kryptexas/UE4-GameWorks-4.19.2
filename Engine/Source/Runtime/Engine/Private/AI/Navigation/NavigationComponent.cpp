// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "VisualLog.h"

//----------------------------------------------------------------------//
// Life cycle                                                        
//----------------------------------------------------------------------//

UNavigationComponent::UNavigationComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, bIsPathPartial(false)
	, AsynPathQueryID(INVALID_NAVQUERYID)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	
	bNotifyPathFollowing = true;
	bRepathWhenInvalid = true;
	bUpdateForSmartLinks = true;
	bWantsInitializeComponent = true;
	bAutoActivate = true;
}

void UNavigationComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// setting up delegate here in case function used has virtual overrides
	PathObserverDelegate = FNavigationPath::FPathObserverDelegate::CreateUObject(this, &UNavigationComponent::OnPathInvalid);
	
	UpdateCachedComponents();
}

void UNavigationComponent::UpdateCachedComponents()
{
	// try to find path following component in owning actor
	AActor* MyOwner = GetOwner();
	if (MyOwner)
	{
		PathFollowComp = MyOwner->FindComponentByClass<UPathFollowingComponent>();
	}
}

void UNavigationComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	// already requested not to tick. 
	if (bShoudTick == false)
	{
		return;
	}

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	check( GetOuter() != NULL );

	bool bSuccess = false;

	if (GoalActor != NULL)
	{
		const FVector GoalActorLocation = GetCurrentMoveGoal(GoalActor, GetOwner());
		const float DistanceSq = FVector::DistSquared(GoalActorLocation, OriginalGoalActorLocation);

		if (DistanceSq > RepathDistanceSq)
		{
			if (RePathTo(GoalActorLocation, StoredQueryFilter))
			{
				bSuccess = true;
				OriginalGoalActorLocation = GoalActorLocation;
			}
			else
			{
				FAIMessage::Send(Cast<AController>(GetOwner()), FAIMessage(UBrainComponent::AIMessage_RepathFailed, this));
			}
		}
		else
		{
			bSuccess = true;
		}
	}
	
	if (bSuccess == false)
	{
		ResetTransientData();
	}
}

void UNavigationComponent::OnPathInvalid(FNavigationPath* InvalidatedPath)
{
	if (InvalidatedPath == NULL || InvalidatedPath != Path.Get())
    {
		return;
    }

	// try to repath automatically
	if (!bIsPaused && bRepathWhenInvalid)
	{
		RepathData.GoalActor = GoalActor;
		RepathData.GoalLocation = CurrentGoal;
		RepathData.bSimplePath = bUseSimplePath;

		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
			FSimpleDelegateGraphTask::FDelegate::CreateUObject(this, &UNavigationComponent::DeferredRepathToGoal)
			, TEXT("Deferred restore path")
			, NULL
			, ENamedThreads::GameThread
			);
	}

	// reset state
	const bool bWasPaused = bIsPaused;
	ResetTransientData();
	bIsPaused = bWasPaused;

	// and notify observer
	if (MyPathObserver && !bRepathWhenInvalid)
	{
		MyPathObserver->OnPathInvalid(this);
	}
}

bool UNavigationComponent::RePathTo(const FVector& GoalLocation, TSharedPtr<const FNavigationQueryFilter> InStoredQueryFilter)
{
	return bUseSimplePath ? GenerateSimplePath(GoalLocation) :
		bDoAsyncPathfinding ? AsyncGeneratePathTo(GoalLocation, StoredQueryFilter) : GeneratePathTo(GoalLocation, InStoredQueryFilter);
}

FNavLocation UNavigationComponent::GetRandomPointOnNavMesh() const
{
#if WITH_RECAST
	ARecastNavMesh* const Nav = (ARecastNavMesh*)GetWorld()->GetNavigationSystem()->GetMainNavData(NavigationSystem::Create);
	if (Nav)
	{
		return Nav->GetRandomPoint();
	}
#endif // WITH_RECAST
	return FNavLocation();
}

FNavLocation UNavigationComponent::ProjectPointToNavigation(const FVector& Location) const
{
	FNavLocation OutLocation(Location);
	
	if (GetNavData() != NULL)
	{
		if (!MyNavData->ProjectPoint(Location, OutLocation, MyNavData->GetDefaultQueryExtent()))
		{
			OutLocation = FNavLocation();
		}
	}

	return OutLocation;
}

bool UNavigationComponent::ProjectPointToNavigation(const FVector& Location, FNavLocation& ProjectedLocation) const
{
	return GetNavData() != NULL && MyNavData->ProjectPoint(Location, ProjectedLocation, MyNavData->GetDefaultQueryExtent());
}

bool UNavigationComponent::ProjectPointToNavigation(const FVector& Location, FVector& ProjectedLocation) const
{
	FNavLocation NavLoc;
	const bool bSuccess = ProjectPointToNavigation(Location, NavLoc);
	ProjectedLocation = NavLoc.Location;
	return bSuccess;
}

bool UNavigationComponent::AsyncGeneratePathTo(const FVector& GoalLocation, TSharedPtr<const FNavigationQueryFilter> QueryFilter)
{
	return GetOuter() && MyNavAgent && AsyncGeneratePath(MyNavAgent->GetNavAgentLocation(), GoalLocation, QueryFilter);
}

bool UNavigationComponent::AsyncGeneratePath(const FVector& FromLocation, const FVector& GoalLocation, TSharedPtr<const FNavigationQueryFilter> QueryFilter)
{
	if (GetWorld()->GetNavigationSystem() == NULL || GetOuter() == NULL || MyNavAgent == NULL || GetNavData() == NULL)
	{
		return false;
	}

	// Make sure we're trying to path to the closest valid location TO that location rather than the absolute location.
	// After talking with Steve P., if we need to ever allow pathing WITHOUT projecting to nav-mesh, it will probably be
	// best to do so using a flag while keeping this as the default behavior.  NOTE: If any changes to this behavior
	// are made, you should search for other places in UNavigationComponent using NavMeshGoalLocation and make sure the
	// behavior remains consistent.
	const FNavAgentProperties* NavAgentProps = MyNavAgent->GetNavAgentProperties();
	if (NavAgentProps != NULL)
	{
		FVector NavMeshGoalLocation;

		if (ProjectPointToNavigation(GoalLocation, NavMeshGoalLocation) == QuerySuccess)
		{
			const EPathFindingMode::Type Mode = bDoHierarchicalPathfinding ? EPathFindingMode::Hierarchical : EPathFindingMode::Regular;
			AsynPathQueryID = GetWorld()->GetNavigationSystem()->FindPathAsync(*MyNavAgent->GetNavAgentProperties()
				, FPathFindingQuery(MyNavData, FromLocation, NavMeshGoalLocation, QueryFilter)
				, FNavPathQueryDelegate::CreateUObject(this, &UNavigationComponent::AsyncGeneratePath_OnCompleteCallback)
				, Mode);
		}
		else
		{
			// projecting destination point failed
			AsynPathQueryID = INVALID_NAVQUERYID;

			UE_VLOG(GetOwner(), LogNavigation, Warning, TEXT("AI trying to generate path to point off of navmesh"));
			UE_VLOG_LOCATION(GetOwner(), GoalLocation, 30, FColor::Red, TEXT("Invalid pathing GoalLocation"));
			if (MyNavData != NULL)
			{
				UE_VLOG_BOX(GetOwner(), FBox(GoalLocation-MyNavData->GetDefaultQueryExtent(),GoalLocation+MyNavData->GetDefaultQueryExtent()), FColor::Red, TEXT_EMPTY);
			}
		}
	}
	else
	{
		AsynPathQueryID = INVALID_NAVQUERYID;
	}

	return AsynPathQueryID != INVALID_NAVQUERYID;
}

void UNavigationComponent::Pause()
{
	bIsPaused = true;
	SetComponentTickEnabledAsync(false);
}

void UNavigationComponent::AbortAsyncFindPathRequest()
{
	if (AsynPathQueryID == INVALID_NAVQUERYID || GetWorld()->GetNavigationSystem() == NULL)
	{
		return;
	}

	GetWorld()->GetNavigationSystem()->AbortAsyncFindPathRequest((uint32)AsynPathQueryID);

	AsynPathQueryID = INVALID_NAVQUERYID;
}

void UNavigationComponent::DeferredResumePath()
{
	ResumePath();
}

bool UNavigationComponent::ResumePath()
{
	const bool bWasPaused = bIsPaused;

	bIsPaused = false;
	
	if (bWasPaused && Path.IsValid() && Path->IsValid())
	{
		if (IsFollowing() == true)
		{
			// make sure ticking is enabled (and it shouldn't be enabled before _first_ async path finding)
			SetComponentTickEnabledAsync(true);
		}

		// don't notify about path update, it's the same path...
		CurrentGoal = Path->PathPoints[ Path->PathPoints.Num() - 1 ].Location;
		return true;
	}

	return false;
}

void UNavigationComponent::AsyncGeneratePath_OnCompleteCallback(uint32 QueryID, ENavigationQueryResult::Type Result, FNavPathSharedPtr ResultPath)
{
	if (QueryID != uint32(AsynPathQueryID))
	{
		UE_VLOG(GetOwner(), LogNavigation, Display, TEXT("Pathfinding query %u finished, but it's different from AsynPathQueryID (%d)"), QueryID, AsynPathQueryID);
		return;
	}

	check(GetOuter() != NULL);
	
	if (Result == ENavigationQueryResult::Success && ResultPath.IsValid() && ResultPath->IsValid())
	{
		const bool bIsSamePath = Path.IsValid() == true && ((const FNavMeshPath*)Path.Get())->ContainsWithSameEnd((const FNavMeshPath*)ResultPath.Get())
			&& FVector::DistSquared(Path->GetDestinationLocation(), ResultPath->GetDestinationLocation()) < KINDA_SMALL_NUMBER;

		if (bIsSamePath == false)
		{
			if (IsFollowing() == true)
			{
				// make sure ticking is enabled (and it shouldn't be enabled before _first_ async path finding)
				SetComponentTickEnabledAsync(true);
			}

			SetPath(ResultPath);
			CurrentGoal = ResultPath->PathPoints[ ResultPath->PathPoints.Num() - 1 ].Location;
			NotifyPathUpdate();
		}
		else
		{
			UE_VLOG(GetOwner(), LogNavigation, Display, TEXT("Skipping newly generated path due to it being the same as current one"));
		}
	}
	else
	{
		ResetTransientData();
		if (MyPathObserver != NULL)
		{
			MyPathObserver->OnPathFailed(this);  
		}
	}
}

bool UNavigationComponent::GeneratePathTo(const FVector& GoalLocation, TSharedPtr<const FNavigationQueryFilter> QueryFilter)
{
	if (GetWorld() == NULL || GetWorld()->GetNavigationSystem() == NULL || GetOuter() == NULL)
	{
		return false;
	}

	// Make sure we're trying to path to the closest valid location TO that location rather than the absolute location.
	// After talking with Steve P., if we need to ever allow pathing WITHOUT projecting to nav-mesh, it will probably be
	// best to do so using a flag while keeping this as the default behavior.  NOTE:` If any changes to this behavior
	// are made, you should search for other places in UNavigationComponent using NavMeshGoalLocation and make sure the
	// behavior remains consistent.
	
	// make sure that nav data is updated
	GetNavData();

	const FNavAgentProperties* NavAgentProps = MyNavAgent ? MyNavAgent->GetNavAgentProperties() : NULL;
	if (NavAgentProps != NULL)
	{
		FVector NavMeshGoalLocation;

#if ENABLE_VISUAL_LOG
		UE_VLOG_LOCATION(GetOwner(), GoalLocation, 34, FColor(0,127,14), TEXT("GoalLocation"));
		const FVector ProjectionExtent = MyNavData ? MyNavData->GetDefaultQueryExtent() : FVector(DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL, DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL, DEFAULT_NAV_QUERY_EXTENT_VERTICAL);
		UE_VLOG_BOX(GetOwner(), FBox(GoalLocation - ProjectionExtent, GoalLocation + ProjectionExtent), FColor::Green, TEXT_EMPTY);
#endif

		if (ProjectPointToNavigation(GoalLocation, NavMeshGoalLocation) == QuerySuccess)
		{
			UE_VLOG_SEGMENT(GetOwner(), GoalLocation, NavMeshGoalLocation, FColor::Blue, TEXT_EMPTY);
			UE_VLOG_LOCATION(GetOwner(), NavMeshGoalLocation, 34, FColor::Blue, TEXT_EMPTY);
			
			UNavigationSystem* NavSys = GetWorld()->GetNavigationSystem();
			FPathFindingQuery Query(MyNavData, MyNavAgent->GetNavAgentLocation(), NavMeshGoalLocation, QueryFilter);

			const EPathFindingMode::Type Mode = bDoHierarchicalPathfinding ? EPathFindingMode::Hierarchical : EPathFindingMode::Regular;
			FPathFindingResult Result = NavSys->FindPathSync(*MyNavAgent->GetNavAgentProperties(), Query, Mode);

			// try reversing direction
			if (bSearchFromGoalWhenOutOfNodes && !bDoHierarchicalPathfinding &&
				Result.Path.IsValid() && Result.Path->DidSearchReachedLimit())
			{
				// quick check if path exists
				bool bPathExists = false;
				{
					DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Pathfinding: HPA* test time"), STAT_Navigation_PathVerifyTime, STATGROUP_Navigation);
					bPathExists = NavSys->TestPathSync(Query, EPathFindingMode::Hierarchical);
				}

				UE_VLOG(GetOwner(), LogNavigation, Log, TEXT("GeneratePathTo: out of nodes, HPA* path: %s"),
					bPathExists ? TEXT("exists, trying reversed search") : TEXT("doesn't exist"));

				// reverse search
				if (bPathExists)
				{
					DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Pathfinding: reversed test time"), STAT_Navigation_PathReverseTime, STATGROUP_Navigation);

					TSharedPtr<FNavigationQueryFilter> Filter = QueryFilter.IsValid() ? QueryFilter->GetCopy() : MyNavData->GetDefaultQueryFilter()->GetCopy();					
					Filter->SetBacktrackingEnabled(true);

					FPathFindingQuery ReversedQuery(MyNavData, Query.EndLocation, Query.StartLocation, Filter);
					Result = NavSys->FindPathSync(*MyNavAgent->GetNavAgentProperties(), Query, Mode);
				}
			}

			if (Result.IsSuccessful() || Result.IsPartial())
			{
				CurrentGoal = NavMeshGoalLocation;
				SetPath(Result.Path);

				if (IsFollowing() == true)
				{
					// make sure ticking is enabled (and it shouldn't be enabled before _first_ async path finding)
					SetComponentTickEnabledAsync(true);
				}

				NotifyPathUpdate();
				return true;
			}
			else
			{
				UE_VLOG(GetOwner(), LogNavigation, Display, TEXT("Failed to generate path to destination"));
			}
		}
		else
		{
			UE_VLOG(GetOwner(), LogNavigation, Display, TEXT("Destination not on navmesh (and nowhere near!)"));
		}
	}
	else
	{
		UE_VLOG(GetOwner(), LogNavigation, Display, TEXT("UNavigationComponent::GeneratePathTo: NavAgentProps == NULL (Probably Pawn died)"));
	}

	return false;
}

bool UNavigationComponent::GeneratePath(const FVector& FromLocation, const FVector& GoalLocation, TSharedPtr<const FNavigationQueryFilter> QueryFilter)
{
	if (GetWorld() == NULL || GetWorld()->GetNavigationSystem() == NULL || GetOuter() == NULL || MyNavAgent == NULL || GetNavData() == NULL)
	{
		return false;
	}

	const FNavLocation NavMeshGoalLocation = ProjectPointToNavigation(GoalLocation);
	const EPathFindingMode::Type Mode = bDoHierarchicalPathfinding ? EPathFindingMode::Hierarchical : EPathFindingMode::Regular;
	FPathFindingResult Result = GetWorld()->GetNavigationSystem()->FindPathSync(*MyNavAgent->GetNavAgentProperties()
		, FPathFindingQuery(GetNavData(), FromLocation, NavMeshGoalLocation.Location, QueryFilter)
		, Mode);

	if (Result.IsSuccessful() || Result.IsPartial())
	{
		CurrentGoal = NavMeshGoalLocation.Location;
		SetPath(Result.Path);
		return true;
	}

	return false;
}

bool UNavigationComponent::GenerateSimplePath(const FVector& Destination)
{
	if (GetWorld() == NULL || GetOwner() == NULL || MyNavAgent == NULL)
	{
		return false;
	}
	
	TArray<FVector> PathPoints;
	PathPoints.Add(MyNavAgent->GetNavAgentLocation());
	PathPoints.Add(Destination);

	FNavPathSharedPtr SimplePath = MakeShareable(new FNavigationPath(PathPoints, NULL));
	
	CurrentGoal = Destination;
	SetPath(SimplePath);
	return true;
}

bool UNavigationComponent::GetPathPoint(uint32 PathVertIdx, FNavPathPoint& PathPoint) const
{
	if (Path.IsValid() && Path->IsValid() && (int32)PathVertIdx < Path->PathPoints.Num())
	{
		PathPoint = Path->PathPoints[PathVertIdx];
		return true;
	}

	return false;
}

bool UNavigationComponent::HasValidPath() const
{ 
	return Path.IsValid() && Path->IsValid(); 
}

bool UNavigationComponent::FindSimplePathToActor(const AActor* NewGoalActor, float RepathDistance)
{
	bool bPathGenerationSucceeded = false;

	if (bIsPaused && NewGoalActor != NULL && GoalActor == NewGoalActor
		&& Path.IsValid() && Path->IsValid() && Path->GetOwner() == NULL)
	{
		RepathDistanceSq = FMath::Square(RepathDistance);
		OriginalGoalActorLocation = GetCurrentMoveGoal(NewGoalActor, GetOwner());

		bPathGenerationSucceeded = ResumePath();
	}
	else 
	{
		if (NewGoalActor != NULL)
		{
			const FVector NewGoalActorLocation = GetCurrentMoveGoal(NewGoalActor, GetOwner());
			bPathGenerationSucceeded = GenerateSimplePath(NewGoalActorLocation);

			if (bPathGenerationSucceeded)
			{
				GoalActor = NewGoalActor;
				RepathDistanceSq = FMath::Square(RepathDistance);
				OriginalGoalActorLocation = NewGoalActorLocation;
				bUseSimplePath = true;

				// if doing sync pathfinding enabling component's ticking needs to be done now
				SetComponentTickEnabledAsync(true);
			}
		}

		if (!bPathGenerationSucceeded)
		{
			UE_VLOG(GetOwner(), LogNavigation, Warning, TEXT("Failed to generate simple path to %s"), *GetNameSafe(NewGoalActor));
		}
	}

	if (bPathGenerationSucceeded == false)
	{
		ResetTransientData();
	}

	return bPathGenerationSucceeded;
}

bool UNavigationComponent::FindPathToActor(const AActor* NewGoalActor, TSharedPtr<const FNavigationQueryFilter> QueryFilter, float RepathDistance)
{
	bool bPathGenerationSucceeded = false;

	if (bIsPaused && NewGoalActor != NULL && GoalActor == NewGoalActor 
		&& Path.IsValid() && Path->IsValid())
	{
		RepathDistanceSq = FMath::Square(RepathDistance);
		OriginalGoalActorLocation = GetCurrentMoveGoal(NewGoalActor, GetOwner());

		if (bDoAsyncPathfinding == true)
		{
			FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
				FSimpleDelegateGraphTask::FDelegate::CreateUObject(this, &UNavigationComponent::DeferredResumePath)
				, TEXT("Deferred resume path")
				, NULL
				, ENamedThreads::GameThread
				);
			bPathGenerationSucceeded = true;
		}
		else
		{
			bPathGenerationSucceeded = ResumePath();
		}
	}
	else
	{
		if (NewGoalActor != NULL)
		{
			const FVector NewGoalActorLocation = GetCurrentMoveGoal(NewGoalActor, GetOwner());
			if (bDoAsyncPathfinding == true)
			{
				bPathGenerationSucceeded = AsyncGeneratePathTo(NewGoalActorLocation, QueryFilter);
			}
			else
			{
				bPathGenerationSucceeded = GeneratePathTo(NewGoalActorLocation, QueryFilter);
			}

			if (bPathGenerationSucceeded)
			{
				// if there's a specific query filter store it
				StoredQueryFilter = QueryFilter;

				GoalActor = NewGoalActor;
				RepathDistanceSq = FMath::Square(RepathDistance);
				OriginalGoalActorLocation = NewGoalActorLocation;
				bUseSimplePath = false;

				if (bDoAsyncPathfinding == false)
				{
					// if doing sync pathfinding enabling component's ticking needs to be done now
					SetComponentTickEnabledAsync(true);
				}
			}
		}

		if (!bPathGenerationSucceeded)
		{
			UE_VLOG(GetOwner(), LogNavigation, Log, TEXT("%s failed to generate path to %s"), *GetNameSafe(GetOwner()), *GetNameSafe(NewGoalActor));
		}
	}

	if (bPathGenerationSucceeded == false)
	{
		ResetTransientData();
	}

	return bPathGenerationSucceeded;
}

bool UNavigationComponent::FindSimplePathToLocation(const FVector& DestLocation)
{
	bool bPathGenerationSucceeded = false;

	if (bIsPaused && Path.IsValid() && Path->IsValid() && Path->GetOwner() == NULL
		&& FVector::DistSquared(CurrentGoal, DestLocation) < SMALL_NUMBER)
	{
		bPathGenerationSucceeded = ResumePath();
	}

	if (bPathGenerationSucceeded == false)
	{
		ResetTransientData();

		bPathGenerationSucceeded = GenerateSimplePath(DestLocation);
	}

	return bPathGenerationSucceeded;
}

bool UNavigationComponent::FindPathToLocation(const FVector& DestLocation, TSharedPtr<const FNavigationQueryFilter> QueryFilter)
{
	bool bPathGenerationSucceeded = false;

	if (bIsPaused == true && Path.IsValid() && Path->IsValid() 
		&& FVector::DistSquared(CurrentGoal, DestLocation) < SMALL_NUMBER)
	{
		// path to destination already generated. Use it.

		if (bDoAsyncPathfinding == true)
		{
			FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
				FSimpleDelegateGraphTask::FDelegate::CreateUObject(this, &UNavigationComponent::DeferredResumePath)
				, TEXT("Deferred resume path")
				, NULL
				, ENamedThreads::GameThread
				);
			bPathGenerationSucceeded = true;
		}
		else
		{
			bPathGenerationSucceeded = ResumePath();
		}
	}
	
	if (bPathGenerationSucceeded == false)
	{
		ResetTransientData();

		if (bDoAsyncPathfinding == true)
		{
			bPathGenerationSucceeded = AsyncGeneratePathTo(DestLocation, QueryFilter);
		}
		else
		{
			bPathGenerationSucceeded = GeneratePathTo(DestLocation, QueryFilter);
		}
	}

	return bPathGenerationSucceeded;
}

void UNavigationComponent::SetPath(const FNavPathSharedPtr& ResultPath)
{
	check(ResultPath.Get() != NULL);

	Path = ResultPath;
	Path->SetObserver(PathObserverDelegate);
	bIsPathPartial = Path->IsPartial();
}

void UNavigationComponent::ResetTransientData()
{
	GoalActor = NULL;
	RepathDistanceSq = 0.f;
	bIsPathPartial = false;
	bIsPaused = false;
	bUseSimplePath = false;
	Path = NULL;
	StoredQueryFilter = NULL;

	// stop ticking - this component should tick only when following an actor
	SetComponentTickEnabledAsync(false);
}

void UNavigationComponent::NotifyPathUpdate()
{
	UE_VLOG(GetOwner(), LogNavigation, Log, TEXT("NotifyPathUpdate points:%d valid:%s"),
		Path.IsValid() ? Path->PathPoints.Num() : 0,
		Path.IsValid() ? (Path->IsValid() ? TEXT("yes") : TEXT("no")) : TEXT("missing!"));

	if (MyPathObserver != NULL)
	{
		MyPathObserver->OnPathUpdated(this);
	}

	if (PathFollowComp && bNotifyPathFollowing)
	{
		PathFollowComp->UpdateMove(Path);
	}

	if (!Path.IsValid() || !Path->IsValid())
	{	// If pointer is valid, nav-path is not.  Print information so we'll know exactly what went wrong when we hit the check below.
		UE_CVLOG(Path.IsValid(), GetOwner(), LogNavigation, Warning,
		TEXT("NotifyPathUpdate fetched invalid NavPath!  NavPath has %d points, is%sready, and is%sup-to-date"),
		Path->PathPoints.Num(), Path->IsReady() ? TEXT(" ") : TEXT(" NOT "), Path->IsUpToDate() ? TEXT(" ") : TEXT(" NOT ")	);

		ResetTransientData();
	}
}

void UNavigationComponent::DeferredRepathToGoal()
{
	GoalActor = RepathData.GoalActor.Get();
	CurrentGoal = RepathData.GoalLocation;
	bUseSimplePath = RepathData.bSimplePath;
	FMemory::MemZero<FDeferredRepath>(RepathData);

	const FVector GoalLocation = GoalActor ? GetCurrentMoveGoal(GoalActor, GetOwner()) : CurrentGoal;
	if (RePathTo(GoalLocation))
	{
		OriginalGoalActorLocation = GoalLocation;
	}
	else
	{
		FAIMessage::Send(Cast<AController>(GetOwner()), FAIMessage(UBrainComponent::AIMessage_RepathFailed, this));
	}
}

void UNavigationComponent::SetPathFollowingUpdates(bool bEnabled)
{
	bNotifyPathFollowing = bEnabled;
}

void UNavigationComponent::SetRepathWhenInvalid(bool bEnabled)
{
	bRepathWhenInvalid = bEnabled;
}

void UNavigationComponent::SetComponentTickEnabledAsync(bool bEnabled)
{
	bShoudTick = bEnabled;
	Super::SetComponentTickEnabledAsync(bEnabled);
}

void UNavigationComponent::OnNavAgentChanged()
{
	MyNavAgent = NULL;
	PickNavData();
}

ANavigationData* UNavigationComponent::PickNavData() const
{
	// reset to default
	MyNavData = NULL;

	if (MyNavAgent == NULL)
	{
		MyNavAgent = InterfaceCast<INavAgentInterface>( GetOuter() );
	}

	if (MyPathObserver == NULL)
	{
		MyPathObserver = InterfaceCast<INavPathObserverInterface>( GetOuter() );
	}

	if (GetWorld() != NULL && GetWorld()->GetNavigationSystem() != NULL && MyNavAgent != NULL)
	{
		const FNavAgentProperties* MoveProps = MyNavAgent->GetNavAgentProperties();

		// Make sure any previous registration for the event is cleared.
		GetWorld()->GetNavigationSystem()->OnNavDataRegisteredEvent.RemoveDynamic(this, &UNavigationComponent::OnNavDataRegistered);
		
		if (MoveProps != NULL)
		{
			// cache it
			MyNavData = GetWorld()->GetNavigationSystem()->GetNavDataForProps(*MoveProps);
		}

		if (MyNavData == NULL)
		{	// Failed to pick it, so register to try again.
			// note that if this happens a lot there's a bigger problem like AIs being spawned before navmesh is ready
			GetWorld()->GetNavigationSystem()->OnNavDataRegisteredEvent.AddDynamic(this, &UNavigationComponent::OnNavDataRegistered);
		}
	}

	return MyNavData;
}

void UNavigationComponent::OnNavDataRegistered(class ANavigationData* NavData)
{
	// re-pick regardless of whether we have MyNavData set already - the new one can be better!
	PickNavData();
}

void UNavigationComponent::DrawDebugCurrentPath(UCanvas* Canvas, FColor PathColor, const uint32 NextPathPointIndex)
{
	if (MyNavData == NULL)
	{
		return;
	}

	const bool bPersistent = Canvas == NULL;

	if (bPersistent)
	{
		FlushPersistentDebugLines( GetWorld() );
	}

	DrawDebugBox(GetWorld(), CurrentGoal, FVector(10.f), PathColor, bPersistent);
	if (Path.IsValid())
	{
		MyNavData->DrawDebugPath(Path.Get(), PathColor, Canvas, bPersistent, NextPathPointIndex);
	}
}

void UNavigationComponent::SetReceiveSmartLinkUpdates(bool bEnabled)
{
	bUpdateForSmartLinks = bEnabled;
}

void UNavigationComponent::OnSmartLinkBroadcast(class USmartNavLinkComponent* NearbyLink)
{
	// update only when agent is actually moving
	if (NearbyLink == NULL || !Path.IsValid() ||
		PathFollowComp == NULL || PathFollowComp->GetStatus() == EPathFollowingStatus::Idle)
	{
		return;
	}

	UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(GetWorld());
	if (NavSys == NULL || MyNavAgent == NULL || GetNavData() == NULL)
	{
		return;
	}

	const bool bHasLink = Path->ContainsSmartLink(NearbyLink);
	const bool bIsEnabled = NearbyLink->IsEnabled();
	if (bIsEnabled == bHasLink)
	{
		// enabled link, path already use it - or - disabled link and path it's not using it
		return;
	}

	const FVector GoalLocation = Path->PathPoints.Last().Location;

	FPathFindingResult Result = NavSys->FindPathSync(*MyNavAgent->GetNavAgentProperties(),
		FPathFindingQuery(GetNavData(), MyNavAgent->GetNavAgentLocation(), GoalLocation, StoredQueryFilter),
		bDoHierarchicalPathfinding ? EPathFindingMode::Hierarchical : EPathFindingMode::Regular);

	if (Result.IsSuccessful() || Result.IsPartial())
	{
		const bool bNewHasLink = Result.Path->ContainsSmartLink(NearbyLink);

		if ((bIsEnabled && !bHasLink && bNewHasLink) ||
			(!bIsEnabled && bHasLink && !bNewHasLink))
		{			
			SetPath(Result.Path);
			NotifyPathUpdate();
		}
	}
}

#if ENABLE_VISUAL_LOG
//----------------------------------------------------------------------//
// debug
//----------------------------------------------------------------------//
void UNavigationComponent::DescribeSelfToVisLog(FVisLogEntry* Snapshot) const
{
	if (Path.IsValid() && Path->IsValid())
	{
		Path->DescribeSelfToVisLog(Snapshot);
	}
}

#endif // ENABLE_VISUAL_LOG
