// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#if WITH_RECAST
#	include "DetourNavMeshQuery.h"
#endif
#define USE_PHYSIC_FOR_VISIBILITY_TESTS 1 // Physic will be used for visibility tests if set or only raycasts on navmesh if not

DEFINE_LOG_CATEGORY(LogPathFollowing);

// debugging colors
#if ENABLE_VISUAL_LOG
namespace PathDebugColor
{
	static const FColor Segment = FColor::Yellow;
	static const FColor Portal = FColorList::LightGrey;
	static const FColor CurrentPathPoint = FColor::White;
	static const FColor Marker = FColor::Green;
}
#endif // ENABLE_VISUAL_LOG

//----------------------------------------------------------------------//
// Life cycle                                                        
//----------------------------------------------------------------------//
uint32 UPathFollowingComponent::NextRequestId = 0;
const float UPathFollowingComponent::DefaultAcceptanceRadius = -1.f;
UPathFollowingComponent::FRequestCompletedSignature UPathFollowingComponent::UnboundRequestDelegate;

UPathFollowingComponent::UPathFollowingComponent(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = true;

	MinAgentRadiusPct = 0.1f;
	MinAgentHeightPct = 0.05f;
	BlockDetectionDistance = 10.0f;
	BlockDetectionInterval = 0.5f;
	BlockDetectionSampleCount = 10;
	LastSampleTime = 0.0f;
	NextSampleIdx = 0;
	bUseBlockDetection = true;
	bLastMoveReachedGoal = false;
	bUseVisibilityTestsSimplification = false;
	bPendingPathStartUpdate = false;

	MoveSegmentStartIndex = 0;
	MoveSegmentEndIndex = 1;
	MoveSegmentStartRef = INVALID_NAVNODEREF;
	MoveSegmentEndRef = INVALID_NAVNODEREF;

	bStopOnOverlap = true;
	Status = EPathFollowingStatus::Idle;
}

void LogPathHelper(AActor* LogOwner, FNavPathSharedPtr Path, const AActor* GoalActor, const int32 CurrentPathPointGoal)
{
#if ENABLE_VISUAL_LOG
	if (Path.IsValid() && Path->IsValid())
	{
		FNavMeshPath* NavMeshPath = Path->CastPath<FNavMeshPath>();
		FVector SegmentEnd = Path->PathPoints[0].Location + NavigationDebugDrawing::PathOffeset;
#if WITH_RECAST
		if (NavMeshPath && !NavMeshPath->IsStringPulled())
		{
			ARecastNavMesh* NavMesh = Cast<ARecastNavMesh>(NavMeshPath->GetOwner());
			NavMesh->BeginBatchQuery();

			for (int32 i = 1; i < NavMeshPath->PathCorridor.Num() - 1; i++)
			{
				FVector SegmentStart = SegmentEnd;
				NavMesh->GetPolyCenter(NavMeshPath->PathCorridor[i], SegmentEnd);
				UE_VLOG_SEGMENT_THICK(LogOwner, SegmentStart, SegmentEnd, PathDebugColor::Segment, 2, TEXT_EMPTY);
			}

			NavMesh->FinishBatchQuery();

			if (Path->PathPoints.IsValidIndex(1))
			{
				UE_VLOG_SEGMENT_THICK(LogOwner, SegmentEnd, Path->PathPoints[1].Location, PathDebugColor::Segment, 2, TEXT_EMPTY);
				SegmentEnd = Path->PathPoints[1].Location;
			}
		}
		else
		{
#endif // WITH_RECAST
			for (int32 i = 1; i < Path->PathPoints.Num(); i++)
			{
				FVector SegmentStart = SegmentEnd;
				SegmentEnd = Path->PathPoints[i].Location + NavigationDebugDrawing::PathOffeset;
				UE_VLOG_SEGMENT_THICK(LogOwner, SegmentStart, SegmentEnd, PathDebugColor::Segment, 2, TEXT_EMPTY);
			}

			if (NavMeshPath)
			{
				const TArray<FNavigationPortalEdge>* Edges = NavMeshPath->GetPathCorridorEdges();
				for (auto Edge : *Edges)
				{
					UE_VLOG_SEGMENT_THICK(LogOwner, Edge.Left + NavigationDebugDrawing::PathOffeset, Edge.Right + NavigationDebugDrawing::PathOffeset, PathDebugColor::Portal, 2, TEXT_EMPTY);
				}
			}

			if (Path->PathPoints.IsValidIndex(CurrentPathPointGoal))
			{
				INavAgentInterface* NavAgent = InterfaceCast<INavAgentInterface>(LogOwner);
				UE_VLOG_SEGMENT(LogOwner, NavAgent->GetNavAgentLocation()
					, Path->PathPoints[CurrentPathPointGoal].Location
					, PathDebugColor::CurrentPathPoint, TEXT_EMPTY);
			}
#if WITH_RECAST
		}
#endif // WITH_RECAST

		if (GoalActor)
		{
			const FVector GoalLoc = GoalActor->GetActorLocation();
			if (FVector::DistSquared(GoalLoc, SegmentEnd) > 1.0f)
			{
				UE_VLOG_LOCATION(LogOwner, GoalLoc, 30, PathDebugColor::Marker, TEXT("GoalActor"));
				UE_VLOG_SEGMENT(LogOwner, GoalLoc, SegmentEnd, PathDebugColor::Marker, TEXT_EMPTY);
			}
		}

		UE_VLOG_BOX(LogOwner, FBox(SegmentEnd - FVector(30.0f), SegmentEnd + FVector(30.0f)), PathDebugColor::Marker, TEXT("PathEnd"));
	}
#endif // ENABLE_VISUAL_LOG
}

void LogBlockHelper(AActor* LogOwner, class UNavMovementComponent* MoveComp, float RadiusPct, float HeightPct, const FVector& SegmentStart, const FVector& SegmentEnd)
{
#if ENABLE_VISUAL_LOG
	if (MoveComp && LogOwner)
	{
		const FVector AgentLocation = MoveComp->GetActorFeetLocation();
		const FVector ToTarget = (SegmentEnd - AgentLocation);
		const float SegmentDot = FVector::DotProduct(ToTarget.SafeNormal(), (SegmentEnd - SegmentStart).SafeNormal());
		UE_VLOG(LogOwner, LogPathFollowing, Verbose, TEXT("[agent to segment end] dot [segment dir]: %f"), SegmentDot);
		
		float AgentRadius = 0.0f;
		float AgentHalfHeight = 0.0f;
		AActor* MovingAgent = MoveComp->GetOwner();
		MovingAgent->GetSimpleCollisionCylinder(AgentRadius, AgentHalfHeight);

		const float Dist2D = ToTarget.Size2D();
		UE_VLOG(LogOwner, LogPathFollowing, Verbose, TEXT("dist 2d: %f (agent radius: %f [%f])"), Dist2D, AgentRadius, AgentRadius * (1 + RadiusPct));

		const float ZDiff = FMath::Abs(ToTarget.Z);
		UE_VLOG(LogOwner, LogPathFollowing, Verbose, TEXT("Z diff: %f (agent halfZ: %f [%f])"), ZDiff, AgentHalfHeight, AgentHalfHeight * (1 + HeightPct));
	}
#endif // ENABLE_VISUAL_LOG
}

FString GetPathDescHelper(FNavPathSharedPtr Path)
{
	return !Path.IsValid() ? TEXT("missing") :
		!Path->IsValid() ? TEXT("invalid") :
		FString::Printf(TEXT("%s:%d"), Path->IsPartial() ? TEXT("partial") : TEXT("complete"), Path->PathPoints.Num());
}

FAIRequestID UPathFollowingComponent::RequestMove(FNavPathSharedPtr InPath, FRequestCompletedSignature OnComplete,
	const AActor* InDestinationActor, float InAcceptanceRadius, bool bInStopOnOverlap, FCustomMoveSharedPtr InGameData)
{
	UE_VLOG(GetOwner(), LogPathFollowing, Log, TEXT("RequestMove: Path(%s), AcceptRadius(%.1f%s), DestinationActor(%s), GameData(%s)"),
		*GetPathDescHelper(InPath),
		InAcceptanceRadius, bInStopOnOverlap ? TEXT(" + agent") : TEXT(""),
		*GetNameSafe(InDestinationActor),
		!InGameData.IsValid() ? TEXT("missing") : TEXT("valid"));

	LogPathHelper(GetOwner(), InPath, InDestinationActor, GetNextPathIndex());

	if (InAcceptanceRadius == UPathFollowingComponent::DefaultAcceptanceRadius)
	{
		InAcceptanceRadius = MyDefaultAcceptanceRadius;
	}

	if (ResourceLock.IsLocked())
	{
		UE_VLOG(GetOwner(), LogPathFollowing, Log, TEXT("Rejecting move request due to resource lock by %s"), *ResourceLock.GetLockSourceName());
		return FAIRequestID::InvalidRequest;
	}

	if (!InPath.IsValid() || InAcceptanceRadius < 0.0f)
	{
		UE_VLOG(GetOwner(), LogPathFollowing, Log, TEXT("RequestMove: invalid request"));
		return FAIRequestID::InvalidRequest;
	}

	// try to grab movement component
	if (!UpdateMovementComponent())
	{
		UE_VLOG(GetOwner(), LogPathFollowing, Warning, TEXT("RequestMove: missing movement component"));
		return FAIRequestID::InvalidRequest;
	}

	// update ID first, so any observer notified by AbortMove() could detect new request
	const uint32 PrevMoveId = CurrentRequestId;
	
	// abort previous movement
	if (Status == EPathFollowingStatus::Paused && Path.IsValid() && InPath.Get() == Path.Get() && DestinationActor == InDestinationActor)
	{
		ResumeMove();
	}
	else
	{
		if (Status == EPathFollowingStatus::Moving)
		{
			const bool bResetVelocity = false;
			const bool bFinishAsSkipped = true;
			AbortMove(TEXT("new request"), PrevMoveId, bResetVelocity, bFinishAsSkipped);
		}
		
		Reset();

		StoreRequestId();

		// store new data
		Path = InPath;
		OnPathUpdated();

		AcceptanceRadius = InAcceptanceRadius;
		GameData = InGameData;
		OnRequestFinished = OnComplete;	
		bStopOnOverlap = bInStopOnOverlap;
		SetDestinationActor(InDestinationActor);

	#if ENABLE_VISUAL_LOG
		const FVector CurrentLocation = MovementComp ? MovementComp->GetActorFeetLocation() : FVector::ZeroVector;
		const FVector DestLocation = InPath->GetDestinationLocation();
		const FVector ToDest = DestLocation - CurrentLocation;
		UE_VLOG(GetOwner(), LogPathFollowing, Log, TEXT("RequestMove: accepted, ID(%u) dist2D(%.0f) distZ(%.0f)"),
			CurrentRequestId.GetID(), ToDest.Size2D(), FMath::Abs(ToDest.Z));
	#endif // ENABLE_VISUAL_LOG

		// with async pathfinding paths can be incomplete, movement will start after receiving UpdateMove 
		if (Path->IsValid())
		{
			Status = EPathFollowingStatus::Moving;

			// determine with path segment should be followed
			const uint32 CurrentSegment = DetermineStartingPathPoint(InPath.Get());
			bPendingPathStartUpdate = false;
			SetMoveSegment(CurrentSegment);
		}
		else
		{
			Status = EPathFollowingStatus::Waiting;
		}
	}

	return CurrentRequestId;
}

bool UPathFollowingComponent::UpdateMove(FNavPathSharedPtr InPath, FAIRequestID RequestID)
{
	UE_VLOG(GetOwner(), LogPathFollowing, Log, TEXT("UpdateMove: Path(%s) Status(%s) RequestID(%u)"),
		*GetPathDescHelper(InPath),
		*GetStatusDesc(), RequestID);

	LogPathHelper(GetOwner(), InPath, DestinationActor.Get(), GetNextPathIndex());

	if (!InPath.IsValid() || !InPath->IsValid() || Status == EPathFollowingStatus::Idle 
		|| RequestID.IsEquivalent(GetCurrentRequestId()) == false)
	{
		UE_VLOG(GetOwner(), LogPathFollowing, Log, TEXT("UpdateMove: invalid request"));
		return false;
	}

	Path = InPath;
	OnPathUpdated();

	if (Status == EPathFollowingStatus::Waiting || Status == EPathFollowingStatus::Moving)
	{
		Status = EPathFollowingStatus::Moving;

		const int32 CurrentSegment = DetermineStartingPathPoint(InPath.Get());
		bPendingPathStartUpdate = false;
		SetMoveSegment(CurrentSegment);
	}
	else if (Status == EPathFollowingStatus::Paused)
	{
		bPendingPathStartUpdate = true;
	}

	return true;
}

void UPathFollowingComponent::AbortMove(const FString& Reason, FAIRequestID RequestID, bool bResetVelocity, bool bSilent)
{
	UE_VLOG(GetOwner(), LogPathFollowing, Log, TEXT("AbortMove: Reason(%s) RequestID(%u)"), *Reason, RequestID);

	if ((Status != EPathFollowingStatus::Idle) && RequestID.IsEquivalent(GetCurrentRequestId()))
	{
		const EPathFollowingResult::Type FinishResult = bSilent ? EPathFollowingResult::Skipped : EPathFollowingResult::Aborted;

		if (CurrentSmartLink)
		{
			CurrentSmartLink->NotifyMoveAborted(this);
		}

		// save data required for observers before reseting temporary variables
		const uint32 AbortedMoveId = RequestID ? RequestID : CurrentRequestId;
		FRequestCompletedSignature SavedReqFinished = OnRequestFinished;

		Reset();
		bLastMoveReachedGoal = false;

		if (bResetVelocity && MovementComp && MovementComp->CanStopPathFollowing())
		{
			MovementComp->StopMovementImmediately();
		}

		if (NavComp && FinishResult != EPathFollowingResult::Skipped)
		{
			NavComp->ResetTransientData();
		}

		// notify observers after state was reset (they can request another move)
		SavedReqFinished.ExecuteIfBound(FinishResult);
		OnMoveFinished.Broadcast(AbortedMoveId, FinishResult);
		FAIMessage::Send(Cast<AController>(GetOwner()), FAIMessage(UBrainComponent::AIMessage_MoveFinished, this, AbortedMoveId, FAIMessage::Failure));
	}
	else
	{
		UE_VLOG(GetOwner(), LogPathFollowing, Log, TEXT("AbortMove FAILED due to RequestID(%u) != CurrentRequestId()%d")
			, RequestID, CurrentRequestId);
	}
}

void UPathFollowingComponent::PauseMove(FAIRequestID RequestID, bool bResetVelocity)
{
	UE_VLOG(GetOwner(), LogPathFollowing, Log, TEXT("PauseMove: RequestID(%u)"), RequestID);
	if (Status == EPathFollowingStatus::Paused)
	{
		return;
	}

	if (RequestID.IsEquivalent(GetCurrentRequestId()))
	{
		if (bResetVelocity && MovementComp && MovementComp->CanStopPathFollowing())
		{
			MovementComp->StopMovementImmediately();
		}

		LocationWhenPaused = MovementComp ? MovementComp->GetActorFeetLocation() : FVector::ZeroVector;
		Status = EPathFollowingStatus::Paused;

		UpdateMoveFocus();
	}
}

void UPathFollowingComponent::ResumeMove(FAIRequestID RequestID)
{
	if (RequestID.IsEquivalent(CurrentRequestId) && RequestID.IsValid())
	{
		UE_VLOG(GetOwner(), LogPathFollowing, Log, TEXT("ResumeMove: RequestID(%u)"), RequestID);

		const bool bMovedDuringPause = ShouldCheckPathOnResume();
		const bool bIsOnPath = IsOnPath();
		if (bIsOnPath)
		{
			Status = EPathFollowingStatus::Moving;
			if (bMovedDuringPause || bPendingPathStartUpdate)
			{
				const int32 CurrentSegment = DetermineStartingPathPoint(Path.Get());
				bPendingPathStartUpdate = false;
				SetMoveSegment(CurrentSegment);
			}
			else
			{
				UpdateMoveFocus();
			}
		}
		else if (Path.IsValid() && Path->IsValid() && Path->GetOwner() == NULL)
		{
			// this means it's a scripted path, just resume
			Status = EPathFollowingStatus::Moving;
			UpdateMoveFocus();
		}
		else
		{
			OnPathFinished(EPathFollowingResult::OffPath);
		}
	}
	else
	{
		UE_VLOG(GetOwner(), LogPathFollowing, Log, TEXT("ResumeMove: RequestID(%u) is neither \'AnyRequest\' not CurrentRequestId(%u). Ignoring.")
			, RequestID, CurrentRequestId);
	}
}

bool UPathFollowingComponent::ShouldCheckPathOnResume() const
{
	bool bCheckPath = true;
	if (MovementComp != NULL)
	{
		float AgentRadius = 0.0f, AgentHalfHeight = 0.0f;
		MovementComp->GetOwner()->GetSimpleCollisionCylinder(AgentRadius, AgentHalfHeight);

		const FVector CurrentLocation = MovementComp->GetActorFeetLocation();
		const float DeltaMove2DSq = (CurrentLocation - LocationWhenPaused).SizeSquared2D();
		const float DeltaZ = FMath::Abs(CurrentLocation.Z - LocationWhenPaused.Z);
		if (DeltaMove2DSq < FMath::Square(AgentRadius) && DeltaZ < (AgentHalfHeight * 0.5f))
		{
			bCheckPath = false;
		}
	}

	return bCheckPath;
}

void UPathFollowingComponent::OnPathFinished(EPathFollowingResult::Type Result)
{
	UE_VLOG(GetOwner(), LogPathFollowing, Log, TEXT("OnPathFinished: %s"), *GetResultDesc(Result));
	
	// save move status
	bLastMoveReachedGoal = (Result == EPathFollowingResult::Success) && !HasPartialPath();

	// save data required for observers before reseting temporary variables
	const FAIRequestID FinishedMoveId = CurrentRequestId;
	FRequestCompletedSignature SavedReqFinished = OnRequestFinished;

	Reset();
	UpdateMoveFocus();

	if (MovementComp && MovementComp->CanStopPathFollowing())	
	{
		MovementComp->StopMovementImmediately();
	}

	if (NavComp && Result != EPathFollowingResult::Skipped)
	{
		NavComp->ResetTransientData();
	}

	// notify observers after state was reset (they can request another move)
	SavedReqFinished.ExecuteIfBound(Result);
	OnMoveFinished.Broadcast(FinishedMoveId, Result);

	FAIMessage::Send(Cast<AController>(GetOwner()), FAIMessage(UBrainComponent::AIMessage_MoveFinished, this, FinishedMoveId, (Result == EPathFollowingResult::Success)));
}

void UPathFollowingComponent::OnSegmentFinished()
{
	UE_VLOG(GetOwner(), LogPathFollowing, Verbose, TEXT("OnSegmentFinished"));
}

void UPathFollowingComponent::OnPathUpdated()
{
}

void UPathFollowingComponent::Initialize()
{
	UpdateCachedComponents();
}

void UPathFollowingComponent::Cleanup()
{
	// empty in base class
}

void UPathFollowingComponent::UpdateCachedComponents()
{
	AActor* MyOwner = GetOwner();
	if (MyOwner)
	{
		NavComp = MyOwner->FindComponentByClass<UNavigationComponent>();
	}

	UpdateMovementComponent(/*bForce=*/true);
}

void UPathFollowingComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Status == EPathFollowingStatus::Moving)
	{
		// check finish conditions, update current segment if needed
		// may result in changing current status!
		UpdatePathSegment();
	}

	if (Status == EPathFollowingStatus::Moving)
	{
		// follow current path segment
		FollowPathSegment(DeltaTime);
	}
};

void UPathFollowingComponent::SetMovementComponent(UNavMovementComponent* MoveComp)
{
	MovementComp = MoveComp;
	MyNavData = NULL;

	if (MoveComp != NULL)
	{
		const FNavAgentProperties* NavAgentProps = MoveComp->GetNavAgentProperties();
		check(NavAgentProps);
		MyDefaultAcceptanceRadius = NavAgentProps->AgentRadius;
		MoveComp->PathFollowingComp = this;

		if (GetWorld() && GetWorld()->GetNavigationSystem())
		{	
			MyNavData = GetWorld()->GetNavigationSystem()->GetNavDataForProps(*NavAgentProps);
		}
	}
}

FVector UPathFollowingComponent::GetMoveFocus(bool bAllowStrafe) const
{
	FVector MoveFocus = FVector::ZeroVector;
	if (bAllowStrafe && DestinationActor.IsValid())
	{
		MoveFocus = DestinationActor->GetActorLocation();
	}
	else
	{
		MoveFocus = *CurrentDestination + MoveSegmentDirection * 20.0f;
	}

	return MoveFocus;
}

void UPathFollowingComponent::Reset()
{
	MoveSegmentStartIndex = 0;
	MoveSegmentStartRef = INVALID_NAVNODEREF;
	MoveSegmentEndRef = INVALID_NAVNODEREF;

	LocationSamples.Reset();
	LastSampleTime = 0.0f;
	NextSampleIdx = 0;

	Path.Reset();
	GameData.Reset();
	DestinationActor.Reset();
	OnRequestFinished.Unbind();
	AcceptanceRadius = MyDefaultAcceptanceRadius;
	bStopOnOverlap = true;
	bCollidedWithGoal = false;

	CurrentRequestId = FAIRequestID::InvalidRequest;

	CurrentDestination.Clear();
	CurrentSmartLink = NULL;

	Status = EPathFollowingStatus::Idle;
}

int32 UPathFollowingComponent::DetermineStartingPathPoint(const FNavigationPath* ConsideredPath) const
{
	int32 PickedPathPoint = INDEX_NONE;

	if (ConsideredPath && ConsideredPath->IsValid())
	{
		// if we already have some info on where we were on previous path
		// we can find out if there's a segment on new path we're currently at
		if (MoveSegmentStartRef != INVALID_NAVNODEREF &&
			MoveSegmentEndRef != INVALID_NAVNODEREF &&
			ConsideredPath->GetOwner() != NULL)
		{
			// iterate every new path node and see if segment match
			for (int32 PathPoint = 0; PathPoint < ConsideredPath->PathPoints.Num() - 1; ++PathPoint)
			{
				if (ConsideredPath->PathPoints[PathPoint].NodeRef == MoveSegmentStartRef && 
					ConsideredPath->PathPoints[PathPoint+1].NodeRef == MoveSegmentEndRef)
				{
					PickedPathPoint = PathPoint;
					break;
				}
			}
		}

		if (MovementComp && PickedPathPoint == INDEX_NONE)
		{
			if (ConsideredPath->PathPoints.Num() > 2)
			{
				// check if is closer to first or second path point (don't assume AI's standing)
				const FVector CurrentLocation = MovementComp->GetActorFeetLocation();
				const float SqDistToFirstPoint = FVector::DistSquared(CurrentLocation, ConsideredPath->PathPoints[0].Location);
				const float SqDistToSecondPoint = FVector::DistSquared(CurrentLocation, ConsideredPath->PathPoints[1].Location);
				PickedPathPoint = (SqDistToFirstPoint < SqDistToSecondPoint) ? 0 : 1;
			}
			else
			{
				// If there are only two point we probably should start from the beginning
				PickedPathPoint = 0;
			}
		}
	}

	return PickedPathPoint;
}

void UPathFollowingComponent::SetDestinationActor(const AActor* InDestinationActor)
{
	DestinationActor = InDestinationActor;
	DestinationAgent = InterfaceCast<const INavAgentInterface>(InDestinationActor);
	MoveOffset = DestinationAgent ? DestinationAgent->GetMoveGoalOffset(GetOwner()) : FVector::ZeroVector;
}

void UPathFollowingComponent::SetMoveSegment(int32 SegmentStartIndex)
{
	const bool bResumedFromSmartLink = (CurrentSmartLink != NULL) && (MoveSegmentStartIndex == SegmentStartIndex);
	SetCurrentSmartLink(NULL, FVector::ZeroVector);

	int32 EndSegmentIndex = SegmentStartIndex + 1;
	if (Path.IsValid() && Path->PathPoints.IsValidIndex(SegmentStartIndex) && Path->PathPoints.IsValidIndex(EndSegmentIndex))
	{
		EndSegmentIndex = DetermineCurrentTargetPathPoint(SegmentStartIndex, bResumedFromSmartLink);

		MoveSegmentStartIndex = SegmentStartIndex;
		MoveSegmentEndIndex = EndSegmentIndex;
		const FNavPathPoint& PathPt0 = Path->PathPoints[MoveSegmentStartIndex];
		const FNavPathPoint& PathPt1 = Path->PathPoints[MoveSegmentEndIndex];

		MoveSegmentStartRef = PathPt0.NodeRef;
		MoveSegmentEndRef = PathPt1.NodeRef;
		
		const FVector SegmentStart = PathPt0.Location;
		const FVector SegmentEnd = PathPt1.Location;

		CurrentDestination.Set(Path->Base.Get(), SegmentEnd);
		CurrentAcceptanceRadius = (Path->PathPoints.Num() == (EndSegmentIndex + 1)) ? AcceptanceRadius : 0.0f;

		MoveSegmentDirection = (SegmentEnd - SegmentStart).SafeNormal();
		
		// make sure we have a non-zero direction if still following a valid path
		if (MoveSegmentDirection.IsZero() && int32(MoveSegmentEndIndex + 1) < Path->PathPoints.Num())
		{
			MoveSegmentDirection = (Path->PathPoints[MoveSegmentEndIndex + 1].Location - SegmentStart).SafeNormal();
		}

		// handle moving through smart links
		if (!bResumedFromSmartLink && PathPt0.SmartLink.IsValid())
		{
			SetCurrentSmartLink(PathPt0.SmartLink.Get(), SegmentEnd);
		}

		// update move focus in owning AI
		UpdateMoveFocus();
	}
}

int32 UPathFollowingComponent::DetermineCurrentTargetPathPoint(int32 StartIndex, bool bResumedFromSmartLink)
{
	if (!bUseVisibilityTestsSimplification ||
		bResumedFromSmartLink ||
		Path->PathPoints[StartIndex].SmartLink.IsValid() ||
		Path->PathPoints[StartIndex + 1].SmartLink.IsValid())
	{
		return StartIndex + 1;
	}


#if WITH_RECAST
	FNavMeshNodeFlags Flags0(Path->PathPoints[StartIndex].Flags);
	FNavMeshNodeFlags Flags1(Path->PathPoints[StartIndex + 1].Flags);

	const bool bOffMesh0 = (Flags0.PathFlags & RECAST_STRAIGHTPATH_OFFMESH_CONNECTION) != 0;
	const bool bOffMesh1 = (Flags1.PathFlags & RECAST_STRAIGHTPATH_OFFMESH_CONNECTION) != 0;

	if ((Flags0.Area != Flags1.Area) || (bOffMesh0 && bOffMesh1))
	{
		return StartIndex + 1;
	}
#endif

	return OptimizeSegmentVisibility(StartIndex);
}

int32 UPathFollowingComponent::OptimizeSegmentVisibility(int32 StartIndex)
{
	SCOPE_CYCLE_COUNTER(STAT_Navigation_PathVisibilityOptimisation);

	const AAIController* MyAI = Cast<AAIController>(GetOwner());
	const ANavigationData* NavData = MyAI && MyAI->NavComponent ? MyAI->NavComponent->GetNavData() : NULL;
	if (Path.IsValid())
	{
		Path->ShortcutNodeRefs.Reset();
	}

	if (NavData == NULL)
	{
		return StartIndex + 1;
	}

	const APawn* MyPawn = MyAI->GetPawn();
	const float PawnHalfHeight = (MyAI->GetCharacter() && MyAI->GetCharacter()->CapsuleComponent) ? MyAI->GetCharacter()->CapsuleComponent->GetScaledCapsuleHalfHeight() : 0.0f;
	const FVector PawnLocation = MyPawn->GetActorLocation();

#if USE_PHYSIC_FOR_VISIBILITY_TESTS
	static const FName NAME_NavAreaTrace = TEXT("NavmeshVisibilityTrace");
	float Radius, HalfHeight;
	MyPawn->GetSimpleCollisionCylinder(Radius, HalfHeight);
	const FCollisionShape CollisionShape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
	FCollisionQueryParams CapsuleParams(NAME_NavAreaTrace, false, MyAI->GetCharacter());
	if (MyPawn->GetRootPrimitiveComponent())
	{
		CapsuleParams.AddIgnoredActors(MyPawn->GetRootPrimitiveComponent()->MoveIgnoreActors);
	}

#endif
	TSharedPtr<FNavigationQueryFilter> QueryFilter = (NavComp && NavComp->GetStoredQueryFilter().IsValid()) ? NavComp->GetStoredQueryFilter()->GetCopy() : NavData->GetDefaultQueryFilter()->GetCopy();
#if WITH_RECAST
	const uint8 StartArea = FNavMeshNodeFlags(Path->PathPoints[StartIndex].Flags).Area;
	TArray<float> CostArray;
	CostArray.Init(DT_UNWALKABLE_POLY_COST, DT_MAX_AREAS);
	CostArray[StartArea] = 0;
	QueryFilter->SetAllAreaCosts(CostArray);
#endif

	const int32 MaxPoints = Path->PathPoints.Num();
	int32 Index = StartIndex + 2;

	for (; Index <= MaxPoints; ++Index)
	{
		if (!Path->PathPoints.IsValidIndex(Index))
		{
			break;
		}

		const FNavPathPoint& PathPt1 = Path->PathPoints[Index];
		const FVector AdjustedDestination = PathPt1.Location + FVector(0.0f, 0.0f, PawnHalfHeight);

		FVector HitLocation;
#if WITH_RECAST
		ARecastNavMesh::FRaycastResult RaycastResult;
		const bool RaycastHitResult = ARecastNavMesh::NavMeshRaycast(NavData, PawnLocation, AdjustedDestination, HitLocation, QueryFilter, MyAI, RaycastResult);
		if (Path.IsValid())
		{
			Path->ShortcutNodeRefs.Reserve(RaycastResult.CorridorPolysCount);
			Path->ShortcutNodeRefs.SetNumUninitialized(RaycastResult.CorridorPolysCount);
		}
		FPlatformMemory::Memcmp(Path->ShortcutNodeRefs.GetTypedData(), RaycastResult.CorridorPolys, RaycastResult.CorridorPolysCount * sizeof(NavNodeRef));
#else
		const bool RaycastHitResult = NavData->Raycast(PawnLocation, AdjustedDestination, HitLocation, QueryFilter);
#endif
		if (RaycastHitResult)
		{
			break;
		}
#if USE_PHYSIC_FOR_VISIBILITY_TESTS
		else
		{
			FHitResult Result;
			if (GetWorld()->SweepSingle(Result, PawnLocation, AdjustedDestination, FQuat::Identity, ECC_WorldStatic, CollisionShape, CapsuleParams))
			{
				break;
			}
		}
#endif

#if WITH_RECAST
		/** don't move to next point if we are changing area, we are at off mesh connection or it's a smart link*/
		if (FNavMeshNodeFlags(Path->PathPoints[StartIndex].Flags).Area != FNavMeshNodeFlags(PathPt1.Flags).Area ||
			FNavMeshNodeFlags(PathPt1.Flags).PathFlags & RECAST_STRAIGHTPATH_OFFMESH_CONNECTION || 
			PathPt1.SmartLink.IsValid())
		{
			return Index;
		}
#endif
	}

	return Index-1;
}

void UPathFollowingComponent::UpdatePathSegment()
{
	if (!Path.IsValid() || MovementComp == NULL)
	{
		AbortMove(TEXT("no path"));
		return;
	}

	if (!Path->IsValid())
	{
		if (NavComp == NULL || !NavComp->IsWaitingForRepath())
		{
			AbortMove(TEXT("no path"));
		}
		return;
	}

	// if agent has control over its movement, check finish conditions
	const bool bCanReachTarget = MovementComp->CanStopPathFollowing();
	if (bCanReachTarget && Status == EPathFollowingStatus::Moving)
	{
		const FVector CurrentLocation = MovementComp->GetActorFeetLocation();
		const int32 LastSegmentEndIndex = Path->PathPoints.Num() - 1;
		const bool bFollowingLastSegment = (MoveSegmentEndIndex >= LastSegmentEndIndex);

		if (bCollidedWithGoal)
		{
			// check if collided with goal actor
			OnSegmentFinished();
			OnPathFinished(EPathFollowingResult::Success);
		}
		else if (bFollowingLastSegment)
		{
			// use goal actor for end of last path segment
			// UNLESS it's partial path (can't reach goal)
			if (DestinationActor.IsValid() && !Path->IsPartial())
			{
				const FVector AgentLocation = DestinationAgent ? DestinationAgent->GetNavAgentLocation() : DestinationActor->GetActorLocation();
				const FVector GoalLocation = FRotationTranslationMatrix(DestinationActor->GetActorRotation(), AgentLocation).TransformPosition(MoveOffset);
				CurrentDestination.Set(NULL, GoalLocation);
			}

			// include goal actor in reach test only for last segment 
			if (HasReachedDestination(CurrentLocation))
			{
				OnSegmentFinished();
				OnPathFinished(EPathFollowingResult::Success);
			}
			else
			{
				UpdateMoveFocus();
			}
		}
		else
		{
			// check if current move segment is finished
			if (HasReachedCurrentTarget(CurrentLocation))
			{
				OnSegmentFinished();
				SetNextMoveSegment();
			}
		}
	}

	// gather location samples to detect if moving agent is blocked
	if (bCanReachTarget && Status == EPathFollowingStatus::Moving)
	{
		const bool bHasNewSample = UpdateBlockDetection();
		if (bHasNewSample && IsBlocked())
		{
			if (Path->PathPoints.IsValidIndex(MoveSegmentEndIndex) && Path->PathPoints.IsValidIndex(MoveSegmentStartIndex))
			{
				LogBlockHelper(GetOwner(), MovementComp, MinAgentRadiusPct, MinAgentHeightPct,
					Path->PathPoints[MoveSegmentStartIndex].Location, Path->PathPoints[MoveSegmentEndIndex].Location);
			}
			else
			{
				if ((GetOwner() != NULL) && (MovementComp != NULL))
				{
					UE_VLOG(GetOwner(), LogPathFollowing, Verbose, TEXT("Path blocked, but move segment indices are not valid: start %d, end %d of %d"), MoveSegmentStartIndex, MoveSegmentEndIndex, Path->PathPoints.Num());
				}
			}
			OnPathFinished(EPathFollowingResult::Blocked);
		}
	}
}

void UPathFollowingComponent::FollowPathSegment(float DeltaTime)
{
	if (MovementComp == NULL || !Path.IsValid())
	{
		return;
	}

	const FVector CurrentLocation = MovementComp->GetActorFeetLocation();

	// check if already at acceptance radius
	if (HasReachedDestination(CurrentLocation))
	{
		OnSegmentFinished();
		OnPathFinished(EPathFollowingResult::Success);
	}
	else
	{
		const FVector CurrentTarget = GetCurrentTargetLocation();
		FVector MoveVelocity = (CurrentTarget - CurrentLocation) / DeltaTime;

		const int32 LastSegmentStartIndex = Path->PathPoints.Num() - 2;
		const bool bNotFollowingLastSegment = (MoveSegmentStartIndex < LastSegmentStartIndex);

		PostProcessMove.ExecuteIfBound(this, MoveVelocity);
		MovementComp->RequestDirectMove(MoveVelocity, bNotFollowingLastSegment);
	}
}

bool UPathFollowingComponent::HasReached(const FVector& TestPoint, float InAcceptanceRadius, bool bExactSpot) const
{
	// simple test for stationary agent, used as early finish condition
	const FVector CurrentLocation = MovementComp ? MovementComp->GetActorFeetLocation() : FVector::ZeroVector;
	const float GoalRadius = 0.0f;
	const float GoalHalfHeight = 0.0f;
	if (InAcceptanceRadius == UPathFollowingComponent::DefaultAcceptanceRadius)
	{
		InAcceptanceRadius = MyDefaultAcceptanceRadius;
	}
	return HasReachedInternal(TestPoint, GoalRadius, GoalHalfHeight, CurrentLocation, InAcceptanceRadius, !bExactSpot);
}

bool UPathFollowingComponent::HasReached(const AActor* TestGoal, float InAcceptanceRadius, bool bExactSpot) const
{
	if (TestGoal == NULL)
	{
		// we might just as well say "we're there" if "there" is NULL
		return true;
	}

	// simple test for stationary agent, used as early finish condition
	float GoalRadius = 0.0f;
	float GoalHalfHeight = 0.0f;
	FVector GoalOffset = FVector::ZeroVector;
	FVector TestPoint = TestGoal->GetActorLocation();
	if (InAcceptanceRadius == UPathFollowingComponent::DefaultAcceptanceRadius)
	{
		InAcceptanceRadius = MyDefaultAcceptanceRadius;
	}

	const INavAgentInterface* NavAgent = InterfaceCast<const INavAgentInterface>(TestGoal);
	if (NavAgent)
	{
		const FVector GoalMoveOffset = NavAgent->GetMoveGoalOffset(GetOwner());
		NavAgent->GetMoveGoalReachTest(GetOwner(), GoalMoveOffset, GoalOffset, GoalRadius, GoalHalfHeight);
		TestPoint = FRotationTranslationMatrix(TestGoal->GetActorRotation(), NavAgent->GetNavAgentLocation()).TransformPosition(GoalOffset);
	}

	const FVector CurrentLocation = MovementComp ? MovementComp->GetActorFeetLocation() : FVector::ZeroVector;
	return HasReachedInternal(TestPoint, GoalRadius, GoalHalfHeight, CurrentLocation, InAcceptanceRadius, !bExactSpot);
}

bool UPathFollowingComponent::HasReachedDestination(const FVector& CurrentLocation) const
{
	// get cylinder at goal location
	FVector GoalLocation = Path->PathPoints[Path->PathPoints.Num() - 1].Location;
	float GoalRadius = 0.0f;
	float GoalHalfHeight = 0.0f;
	
	// take goal's current location, unless path is partial
	if (DestinationActor.IsValid() && !Path->IsPartial())
	{
		if (DestinationAgent)
		{
			FVector GoalOffset;
			DestinationAgent->GetMoveGoalReachTest(GetOwner(), MoveOffset, GoalOffset, GoalRadius, GoalHalfHeight);

			GoalLocation = FRotationTranslationMatrix(DestinationActor->GetActorRotation(), DestinationAgent->GetNavAgentLocation()).TransformPosition(GoalOffset);
		}
		else
		{
			GoalLocation = DestinationActor->GetActorLocation();
		}
	}

	return HasReachedInternal(GoalLocation, GoalRadius, GoalHalfHeight, CurrentLocation, AcceptanceRadius, bStopOnOverlap);
}

bool UPathFollowingComponent::HasReachedCurrentTarget(const FVector& CurrentLocation) const
{
	if (MovementComp == NULL)
	{
		return false;
	}

	const FVector CurrentTarget = GetCurrentTargetLocation();

	// check if moved too far
	const FVector ToTarget = (CurrentTarget - MovementComp->GetActorFeetLocation());
	const float SegmentDot = FVector::DotProduct(ToTarget, MoveSegmentDirection);
	if (SegmentDot < 0.0)
	{
		return true;
	}

	// or standing at target position
	// don't use acceptance radius here, it has to be exact for moving near corners
	const float GoalRadius = 0.0f;
	const float GoalHalfHeight = 0.0f;
	return HasReachedInternal(CurrentTarget, GoalRadius, GoalHalfHeight, CurrentLocation, CurrentAcceptanceRadius, false);
}

bool UPathFollowingComponent::HasReachedInternal(const FVector& Goal, float GoalRadius, float GoalHalfHeight, const FVector& AgentLocation, float RadiusThreshold, bool bUseAgentRadius) const
{
	if (MovementComp == NULL)
	{
		return false;
	}

	// get cylinder of moving agent
	float AgentRadius = 0.0f;
	float AgentHalfHeight = 0.0f;
	AActor* MovingAgent = MovementComp->GetOwner();
	MovingAgent->GetSimpleCollisionCylinder(AgentRadius, AgentHalfHeight);

	// check if they overlap (with added AcceptanceRadius)
	const FVector ToGoal = Goal - AgentLocation;

	const float Dist2DSq = ToGoal.SizeSquared2D();
	const float UseRadius = GoalRadius + RadiusThreshold + (bUseAgentRadius ? AgentRadius : 0.0f) + (AgentRadius * MinAgentRadiusPct);
	if (Dist2DSq > FMath::Square(UseRadius))
	{
		return false;
	}

	const float ZDiff = FMath::Abs(ToGoal.Z);
	const float UseHeight = GoalHalfHeight + AgentHalfHeight + (AgentHalfHeight * MinAgentHeightPct);
	if (ZDiff > UseHeight)
	{
		return false;
	}

	return true;
}

void UPathFollowingComponent::DebugReachTest(float& CurrentDot, float& CurrentDistance, float& CurrentHeight, uint8& bDotFailed, uint8& bDistanceFailed, uint8& bHeightFailed) const
{
	if (!Path.IsValid() || MovementComp == NULL)
	{
		return;
	}

	const int32 LastSegmentEndIndex = Path->PathPoints.Num() - 1;
	const bool bFollowingLastSegment = (MoveSegmentEndIndex >= LastSegmentEndIndex);

	float GoalRadius = 0.0f;
	float GoalHalfHeight = 0.0f;
	bool bUseAgentRadius = false;
	float RadiusThreshold = 0.0f;

	FVector AgentLocation = MovementComp->GetActorFeetLocation();
	FVector GoalLocation = GetCurrentTargetLocation();
	RadiusThreshold = CurrentAcceptanceRadius;

	if (bFollowingLastSegment)
	{
		GoalLocation = Path->PathPoints[Path->PathPoints.Num() - 1].Location;
		bUseAgentRadius = bStopOnOverlap;

		// take goal's current location, unless path is partial
		if (DestinationActor.IsValid() && !Path->IsPartial())
		{
			if (DestinationAgent)
			{
				FVector GoalOffset;
				DestinationAgent->GetMoveGoalReachTest(GetOwner(), MoveOffset, GoalOffset, GoalRadius, GoalHalfHeight);

				GoalLocation = FRotationTranslationMatrix(DestinationActor->GetActorRotation(), DestinationAgent->GetNavAgentLocation()).TransformPosition(GoalOffset);
			}
			else
			{
				GoalLocation = DestinationActor->GetActorLocation();
			}
		}
	}

	const FVector ToGoal = (GoalLocation - AgentLocation);
	CurrentDot = FVector::DotProduct(ToGoal.SafeNormal(), MoveSegmentDirection);
	bDotFailed = (CurrentDot < 0.0f) ? 1 : 0;

	// get cylinder of moving agent
	float AgentRadius = 0.0f;
	float AgentHalfHeight = 0.0f;
	AActor* MovingAgent = MovementComp->GetOwner();
	MovingAgent->GetSimpleCollisionCylinder(AgentRadius, AgentHalfHeight);

	CurrentDistance = ToGoal.Size2D();
	const float UseRadius = GoalRadius + RadiusThreshold + (bUseAgentRadius ? AgentRadius : 0.0f) + (AgentRadius * MinAgentRadiusPct);
	bDistanceFailed = (CurrentDistance > UseRadius) ? 1 : 0;

	CurrentHeight = FMath::Abs(ToGoal.Z);
	const float UseHeight = GoalHalfHeight + AgentHalfHeight + (AgentHalfHeight * MinAgentHeightPct);
	bHeightFailed = (CurrentHeight > UseHeight) ? 1 : 0;
}

void UPathFollowingComponent::SetCurrentSmartLink(USmartNavLinkComponent* InLink, const FVector& DestPoint)
{
	if (CurrentSmartLink)
	{
		CurrentSmartLink->NotifyLinkFinished(this);
		CurrentSmartLink = NULL;
	}

	if (InLink)
	{
		PauseMove();

		CurrentSmartLink = InLink;
		CurrentSmartLink->NotifyLinkReached(this, DestPoint);
	}
}

bool UPathFollowingComponent::UpdateMovementComponent(bool bForce)
{
	if (MovementComp == NULL || bForce == true)
	{
		APawn* MyPawn = Cast<APawn>(GetOwner());
		if (MyPawn == NULL)
		{
			AController* MyController = Cast<AController>(GetOwner());
			if (MyController)
			{
				MyPawn = MyController->GetPawn();
			}
		}

		if (MyPawn)
		{
			FScriptDelegate Delegate;
			Delegate.BindUFunction(this, "OnActorBump");
			MyPawn->OnActorHit.AddUnique(Delegate);

			SetMovementComponent(MyPawn->FindComponentByClass<UNavMovementComponent>());
		}
	}

	return (MovementComp != NULL);
}

void UPathFollowingComponent::OnActorBump(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	if (Path.IsValid() && DestinationActor.IsValid() && DestinationActor.Get() == OtherActor)
	{
		UE_VLOG(GetOwner(), LogPathFollowing, Verbose, TEXT("Collided with goal actor"));
		bCollidedWithGoal = true;
	}
}

bool UPathFollowingComponent::IsOnPath() const
{
	bool bOnPath = false;
	if (Path.IsValid() && Path->IsValid() && Path->GetOwner() != NULL)
	{
		const bool bHasNavigationCorridor = (Path->CastPath<FNavMeshPath>() != NULL);
		if (bHasNavigationCorridor)
		{
			FNavLocation NavLoc = GetCurrentNavLocation();
			bOnPath = Path->ContainsNode(NavLoc.NodeRef);
		}
		else
		{
			bOnPath = true;
		}
	}

	return bOnPath;
}

bool UPathFollowingComponent::IsBlocked() const
{
	bool bBlocked = false;

	if (LocationSamples.Num() == BlockDetectionSampleCount && BlockDetectionSampleCount > 0)
	{
		FVector Center = FVector::ZeroVector;
		for (int32 i = 0; i < LocationSamples.Num(); i++)
		{
			Center += *LocationSamples[i];
		}

		Center /= LocationSamples.Num();
		bBlocked = true;

		for (int32 i = 0; i < LocationSamples.Num(); i++)
		{
			const float TestDistance = FVector::DistSquared(*LocationSamples[i], Center);
			if (TestDistance > BlockDetectionDistance)
			{
				bBlocked = false;
				break;
			}
		}
	}

	return bBlocked;
}

bool UPathFollowingComponent::UpdateBlockDetection()
{
	const float GameTime = GetWorld()->GetTimeSeconds();
	if (bUseBlockDetection &&
		MovementComp && 
		GameTime > (LastSampleTime + BlockDetectionInterval) &&
		BlockDetectionSampleCount > 0)
	{
		LastSampleTime = GameTime;

		if (LocationSamples.Num() == NextSampleIdx)
		{
			LocationSamples.AddZeroed(1);
		}

		LocationSamples[NextSampleIdx] = MovementComp->GetActorFeetLocationBased();
		NextSampleIdx = (NextSampleIdx + 1) % BlockDetectionSampleCount;
		return true;
	}

	return false;
}

void UPathFollowingComponent::SetBlockDetection(float DistanceThreshold, float Interval, int32 NumSamples)
{
	BlockDetectionDistance = DistanceThreshold;
	BlockDetectionInterval = Interval;
	BlockDetectionSampleCount = NumSamples;
	ResetBlockDetectionData();
}

void UPathFollowingComponent::SetBlockDetectionState(bool bEnable)
{
	bUseBlockDetection = bEnable;
	ResetBlockDetectionData();
}

void UPathFollowingComponent::ResetBlockDetectionData()
{
	LastSampleTime = 0.0f;
	NextSampleIdx = 0;
	LocationSamples.Reset();
}

void UPathFollowingComponent::ForceBlockDetectionUpdate()
{
	LastSampleTime = 0.0f;
}

void UPathFollowingComponent::UpdateMoveFocus()
{
	AAIController* AIOwner = Cast<AAIController>(GetOwner());
	if (AIOwner != NULL)
	{
		if (Status == EPathFollowingStatus::Moving)
		{
			const FVector MoveFocus = GetMoveFocus(AIOwner->bAllowStrafe);
			AIOwner->SetFocalPoint(MoveFocus, false, EAIFocusPriority::Move);
		}
		else
		{
			AIOwner->ClearFocus(EAIFocusPriority::Move);
		}
	}
}

void UPathFollowingComponent::SetPreciseReachThreshold(float AgentRadiusMultiplier, float AgentHeightMultiplier)
{
	MinAgentRadiusPct = AgentRadiusMultiplier;
	MinAgentHeightPct = AgentHeightMultiplier;
}

float UPathFollowingComponent::GetRemainingPathCost() const
{
	float Cost = 0.f;

	if (Path.IsValid() && Path->IsValid() && Status == EPathFollowingStatus::Moving)
	{
		const FNavLocation& NavLocation = GetCurrentNavLocation();
		Cost = Path->GetCostFromNode(NavLocation.NodeRef);
	}

	return Cost;
}	

FNavLocation UPathFollowingComponent::GetCurrentNavLocation() const
{
	// get navigation location of moved actor
	if (MovementComp == NULL || GetWorld() == NULL || GetWorld()->GetNavigationSystem() == NULL)
	{
		return FVector::ZeroVector;
	}

	const FVector OwnerLoc = MovementComp->GetActorNavLocation();
	const float Tolerance = 1.0f;
	
	// Using !Equals rather than != because exact equivalence isn't required.  (Equals allows a small tolerance.)
	if (!OwnerLoc.Equals(CurrentNavLocation.Location, Tolerance))
	{
		const AActor* OwnerActor = MovementComp->GetOwner();
		const FVector OwnerExtent = OwnerActor ? OwnerActor->GetSimpleCollisionCylinderExtent() : FVector::ZeroVector;

		GetWorld()->GetNavigationSystem()->ProjectPointToNavigation(OwnerLoc, CurrentNavLocation, OwnerExtent, MyNavData);
	}

	return CurrentNavLocation;
}

// will be deprecated soon, please don't use it!
EPathFollowingAction::Type UPathFollowingComponent::GetPathActionType() const
{
	switch (Status)
	{
		case EPathFollowingStatus::Idle:
			return EPathFollowingAction::NoMove;

		case EPathFollowingStatus::Waiting:
		case EPathFollowingStatus::Paused:	
		case EPathFollowingStatus::Moving:
			return !Path.IsValid() ? EPathFollowingAction::Error :
				(Path->GetOwner() == NULL) ? EPathFollowingAction::DirectMove :
				Path->IsPartial() ? EPathFollowingAction::PartialPath : EPathFollowingAction::PathToGoal;

		default:
			break;
	}

	return EPathFollowingAction::Error;
}

// will be deprecated soon, please don't use it!
FVector UPathFollowingComponent::GetPathDestination() const
{
	return Path.IsValid() ? Path->GetDestinationLocation() : FVector::ZeroVector;
}

FString UPathFollowingComponent::GetStatusDesc() const
{
	const static UEnum* StatusEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EPathFollowingStatus"));
	if (StatusEnum)
	{
		return StatusEnum->GetEnumName(Status);
	}

	return TEXT("Unknown");
}

FString UPathFollowingComponent::GetResultDesc(EPathFollowingResult::Type Result) const
{
	const static UEnum* ResultEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EPathFollowingResult"));
	if (ResultEnum)
	{
		return ResultEnum->GetEnumName(Result);
	}

	return TEXT("Unknown");
}

void UPathFollowingComponent::DisplayDebug(class UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) const
{
	Canvas->SetDrawColor(FColor::Blue);
	UFont* RenderFont = GEngine->GetSmallFont();
	FString StatusDesc = FString::Printf(TEXT("  Move status: %s"), *GetStatusDesc());
	Canvas->DrawText(RenderFont, StatusDesc, 4.0f, YPos);
	YPos += YL;

	if (Status == EPathFollowingStatus::Moving)
	{
		const int32 NumMoveSegments = (Path.IsValid() && Path->IsValid()) ? Path->PathPoints.Num() : -1;
		FString TargetDesc = FString::Printf(TEXT("  Move target [%d/%d]: %s (%s)"),
			MoveSegmentEndIndex, NumMoveSegments, *GetCurrentTargetLocation().ToString(), *GetNameSafe(DestinationActor.Get()));
		
		Canvas->DrawText(RenderFont, TargetDesc, 4.0f, YPos);
		YPos += YL;
	}
}

#if ENABLE_VISUAL_LOG
void UPathFollowingComponent::DescribeSelfToVisLog(struct FVisLogEntry* Snapshot) const
{
	FVisLogEntry::FStatusCategory Category;
	Category.Category = TEXT("Path following");

	if (DestinationActor.IsValid())
	{
		Category.Add(TEXT("Goal"), GetNameSafe(DestinationActor.Get()));
	}
	
	FString StatusDesc = GetStatusDesc();
	if (Status == EPathFollowingStatus::Moving)
	{
		const int32 NumMoveSegments = (Path.IsValid() && Path->IsValid()) ? Path->PathPoints.Num() : -1;
		StatusDesc += FString::Printf(TEXT(" [%d..%d/%d]"), MoveSegmentStartIndex + 1, MoveSegmentEndIndex + 1, NumMoveSegments);
	}

	Category.Add(TEXT("Status"), StatusDesc);
	Category.Add(TEXT("Path"), !Path.IsValid() ? TEXT("none") : (Path->CastPath<FNavMeshPath>() != NULL) ? TEXT("navmesh") : TEXT("direct"));
	
	if (CurrentSmartLink)
	{
		Category.Add(TEXT("SmartLink"), CurrentSmartLink->GetName());
	}

	Snapshot->Status.Add(Category);
}
#endif // ENABLE_VISUAL_LOG

void UPathFollowingComponent::GetDebugStringTokens(TArray<FString>& Tokens, TArray<EPathFollowingDebugTokens::Type>& Flags) const
{
	Tokens.Add(GetStatusDesc());
	Flags.Add(EPathFollowingDebugTokens::Description);

	if (Status != EPathFollowingStatus::Moving)
	{
		return;
	}

	FString& StatusDesc = Tokens[0];
	if (Path.IsValid())
	{
		const int32 NumMoveSegments = (Path.IsValid() && Path->IsValid()) ? Path->PathPoints.Num() : -1;
		const bool bIsDirect = (Path->CastPath<FNavMeshPath>() == NULL);
		const bool bIsSmartLink = (CurrentSmartLink != NULL);

		if (!bIsDirect)
		{
			StatusDesc += FString::Printf(TEXT(" (%d..%d/%d)%s"), MoveSegmentStartIndex + 1, MoveSegmentEndIndex + 1, NumMoveSegments,
				bIsSmartLink ? TEXT(" (SmartLink)") : TEXT(""));
		}
		else
		{
			StatusDesc += TEXT(" (direct)");
		}
	}
	else
	{
		StatusDesc += TEXT(" (invalid path)");
	}

	// add debug params
	float CurrentDot = 0.0f, CurrentDistance = 0.0f, CurrentHeight = 0.0f;
	uint8 bFailedDot = 0, bFailedDistance = 0, bFailedHeight = 0;
	DebugReachTest(CurrentDot, CurrentDistance, CurrentHeight, bFailedHeight, bFailedDistance, bFailedHeight);

	Tokens.Add(TEXT("dot"));
	Flags.Add(EPathFollowingDebugTokens::ParamName);
	Tokens.Add(FString::Printf(TEXT("%.2f"), CurrentDot));
	Flags.Add(bFailedDot ? EPathFollowingDebugTokens::FailedValue : EPathFollowingDebugTokens::PassedValue);

	Tokens.Add(TEXT("dist2D"));
	Flags.Add(EPathFollowingDebugTokens::ParamName);
	Tokens.Add(FString::Printf(TEXT("%.0f"), CurrentDistance));
	Flags.Add(bFailedDistance ? EPathFollowingDebugTokens::FailedValue : EPathFollowingDebugTokens::PassedValue);

	Tokens.Add(TEXT("distZ"));
	Flags.Add(EPathFollowingDebugTokens::ParamName);
	Tokens.Add(FString::Printf(TEXT("%.0f"), CurrentHeight));
	Flags.Add(bFailedHeight ? EPathFollowingDebugTokens::FailedValue : EPathFollowingDebugTokens::PassedValue);
}

FString UPathFollowingComponent::GetDebugString() const
{
	TArray<FString> Tokens;
	TArray<EPathFollowingDebugTokens::Type> Flags;

	GetDebugStringTokens(Tokens, Flags);

	FString Desc;
	for (int32 i = 0; i < Tokens.Num(); i++)
	{
		Desc += Tokens[i];
		Desc += (Flags.IsValidIndex(i) && Flags[i] == EPathFollowingDebugTokens::ParamName) ? TEXT(": ") : TEXT(", ");
	}

	return Desc;
}

void UPathFollowingComponent::LockResource(EAILockSource::Type LockSource)
{
	const static UEnum* SourceEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EAILockSource"));
	const bool bWasLocked = ResourceLock.IsLocked();

	ResourceLock.SetLock(LockSource);
	if (bWasLocked == false)
	{
		UE_VLOG(GetOwner(), LogPathFollowing, Log, TEXT("Locking Move by source %s"), SourceEnum ? *SourceEnum->GetEnumName(Status) : TEXT("invalid"));
		PauseMove();
	}
}

void UPathFollowingComponent::ClearResourceLock(EAILockSource::Type LockSource)
{
	const bool bWasLocked = ResourceLock.IsLocked();
	ResourceLock.ClearLock(LockSource);

	if (bWasLocked && !ResourceLock.IsLocked())
	{
		UE_VLOG(GetOwner(), LogPathFollowing, Log, TEXT("Unlocking Move"));
		ResumeMove();
	}
}

void UPathFollowingComponent::ForceUnlockResource()
{
	const bool bWasLocked = ResourceLock.IsLocked();
	ResourceLock.ForceClearAllLocks();

	if (bWasLocked)
	{
		UE_VLOG(GetOwner(), LogPathFollowing, Log, TEXT("Unlocking Move - forced"));
		ResumeMove();
	}
}

bool UPathFollowingComponent::IsResourceLocked() const
{
	return ResourceLock.IsLocked();
}

void UPathFollowingComponent::SetLastMoveAtGoal(bool bFinishedAtGoal)
{
	bLastMoveReachedGoal = bFinishedAtGoal;
}
