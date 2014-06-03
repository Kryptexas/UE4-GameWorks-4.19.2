// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Navigation/CrowdManager.h"
#include "Navigation/NavigationComponent.h"
#include "AI/Navigation/SmartNavLinkComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "VisualLog.h"

DEFINE_LOG_CATEGORY(LogCrowdFollowing);

UCrowdFollowingComponent::UCrowdFollowingComponent(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	bAffectFallingVelocity = false;
	bRotateToVelocity = true;
	bEnableCrowdSimulation = true;
	bSuspendCrowdSimulation = false;

	bEnableAnticipateTurns = false;
	bEnableObstacleAvoidance = true;
	bEnableSeparation = false;
	bEnableOptimizeVisibility = true;
	bEnableOptimizeTopology = true;

	SeparationWeight = 2.0f;
	CollisionQueryRange = 400.0f;		// approx: radius * 12.0f
	PathOptimizationRange = 1000.0f;	// approx: radius * 30.0f
	AvoidanceQuality = ECrowdAvoidanceQuality::Low;
}

FVector UCrowdFollowingComponent::GetCrowdAgentLocation() const
{
	return MovementComp ? MovementComp->GetActorFeetLocation() : FVector::ZeroVector;
}

FVector UCrowdFollowingComponent::GetCrowdAgentVelocity() const
{
	FVector Velocity(MovementComp ? MovementComp->Velocity : FVector::ZeroVector);
	Velocity *= (Status == EPathFollowingStatus::Moving) ? 1.0f : 0.25f;
	return Velocity;
}

void UCrowdFollowingComponent::GetCrowdAgentCollisions(float& CylinderRadius, float& CylinderHalfHeight) const
{
	if (MovementComp && MovementComp->UpdatedComponent)
	{
		MovementComp->UpdatedComponent->CalcBoundingCylinder(CylinderRadius, CylinderHalfHeight);
	}
}

float UCrowdFollowingComponent::GetCrowdAgentMaxSpeed() const
{
	return MovementComp ? MovementComp->GetMaxSpeed() : 0.0f;
}

void UCrowdFollowingComponent::SetCrowdAnticipateTurns(bool bEnable, bool bUpdateAgent)
{
	if (bEnableAnticipateTurns != bEnable)
	{
		bEnableAnticipateTurns = bEnable;
		
		if (bUpdateAgent)
		{
			UpdateCrowdAgentParams();
		}
	}
}

void UCrowdFollowingComponent::SetCrowdObstacleAvoidance(bool bEnable, bool bUpdateAgent)
{
	if (bEnableObstacleAvoidance != bEnable)
	{
		bEnableObstacleAvoidance = bEnable;

		if (bUpdateAgent)
		{
			UpdateCrowdAgentParams();
		}
	}
}

void UCrowdFollowingComponent::SetCrowdSeparation(bool bEnable, bool bUpdateAgent)
{
	if (bEnableSeparation != bEnable)
	{
		bEnableSeparation = bEnable;

		if (bUpdateAgent)
		{
			UpdateCrowdAgentParams();
		}
	}
}

void UCrowdFollowingComponent::SetCrowdOptimizeVisibility(bool bEnable, bool bUpdateAgent)
{
	if (bEnableOptimizeVisibility != bEnable)
	{
		bEnableOptimizeVisibility = bEnable;

		if (bUpdateAgent)
		{
			UpdateCrowdAgentParams();
		}
	}
}

void UCrowdFollowingComponent::SetCrowdOptimizeTopology(bool bEnable, bool bUpdateAgent)
{
	if (bEnableOptimizeTopology != bEnable)
	{
		bEnableOptimizeTopology = bEnable;

		if (bUpdateAgent)
		{
			UpdateCrowdAgentParams();
		}
	}
}

void UCrowdFollowingComponent::SetCrowdSeparationWeight(float Weight, bool bUpdateAgent)
{
	if (SeparationWeight != Weight)
	{
		SeparationWeight = Weight;

		if (bUpdateAgent)
		{
			UpdateCrowdAgentParams();
		}
	}
}

void UCrowdFollowingComponent::SetCrowdCollisionQueryRange(float Range, bool bUpdateAgent)
{
	if (CollisionQueryRange != Range)
	{
		CollisionQueryRange = Range;

		if (bUpdateAgent)
		{
			UpdateCrowdAgentParams();
		}
	}
}

void UCrowdFollowingComponent::SetCrowdPathOptimizationRange(float Range, bool bUpdateAgent)
{
	if (PathOptimizationRange != Range)
	{
		PathOptimizationRange = Range;

		if (bUpdateAgent)
		{
			UpdateCrowdAgentParams();
		}
	}
}

void UCrowdFollowingComponent::SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Type Quality, bool bUpdateAgent)
{
	if (AvoidanceQuality != Quality)
	{
		AvoidanceQuality = Quality;

		if (bUpdateAgent)
		{
			UpdateCrowdAgentParams();
		}
	}
}

void UCrowdFollowingComponent::SuspendCrowdSteering(bool bSuspend)
{
	if (bSuspendCrowdSimulation != bSuspend)
	{
		bSuspendCrowdSimulation = bSuspend;
		UpdateCrowdAgentParams();
	}
}

void UCrowdFollowingComponent::UpdateCrowdAgentParams() const
{
	UCrowdManager* CrowdManager = UCrowdManager::GetCurrent(GetWorld());
	if (CrowdManager)
	{
		const ICrowdAgentInterface* IAgent = InterfaceCast<ICrowdAgentInterface>(this);
		CrowdManager->UpdateAgentParams(IAgent);
	}
}

void UCrowdFollowingComponent::UpdateCachedDirections(const FVector& NewVelocity, const FVector& NextPathCorner, bool bTraversingLink)
{
	// MoveSegmentDirection = direction on string pulled path
	const FVector AgentLoc = GetCrowdAgentLocation();
	const FVector ToCorner = NextPathCorner - AgentLoc;
	if (ToCorner.SizeSquared() > FMath::Square(10.0f))
	{
		MoveSegmentDirection = ToCorner.SafeNormal();
	}

	// CrowdAgentMoveDirection either direction on path or aligned with current velocity
	if (!bTraversingLink)
	{
		CrowdAgentMoveDirection = bRotateToVelocity && (NewVelocity.SizeSquared() > KINDA_SMALL_NUMBER) ? NewVelocity.SafeNormal() : MoveSegmentDirection;
	}
}

void UCrowdFollowingComponent::ApplyCrowdAgentVelocity(const FVector& NewVelocity, const FVector& DestPathCorner, bool bTraversingLink)
{
	if (bEnableCrowdSimulation)
	{
		if (Status != EPathFollowingStatus::Paused &&
			(bAffectFallingVelocity || CharacterMovement == NULL || CharacterMovement->MovementMode != MOVE_Falling))
		{
			MovementComp->RequestDirectMove(NewVelocity, false);

			UpdateCachedDirections(NewVelocity, DestPathCorner, bTraversingLink);
		}
	}
}

void UCrowdFollowingComponent::SetCrowdSimulation(bool bEnable)
{
	if (bEnableCrowdSimulation == bEnable)
	{
		return;
	}

	if (GetStatus() != EPathFollowingStatus::Idle)
	{
		UE_VLOG(GetOwner(), LogCrowdFollowing, Warning, TEXT("SetCrowdSimulation failed, agent is not in Idle state!"));
		return;
	}

	UE_VLOG(GetOwner(), LogCrowdFollowing, Log, TEXT("SetCrowdSimulation: %s"), bEnable ? TEXT("enabled") : TEXT("disabled"));
	bEnableCrowdSimulation = bEnable;

	if (NavComp)
	{
		if (bEnableCrowdSimulation)
		{
			// disable path post processing (string pulling), crowd simulation needs to handle 
			// large paths by splitting into smaller parts and optimization gets in the way
			NavComp->SetNavDataFlag(ERecastPathFlags::SkipStringPulling);
		}
		else
		{
			NavComp->ClearNavDataFlag(ERecastPathFlags::SkipStringPulling);
		}
	}
}

void UCrowdFollowingComponent::Initialize()
{
	Super::Initialize();

	UCrowdManager* CrowdManager = UCrowdManager::GetCurrent(GetWorld());
	if (CrowdManager)
	{
		const ICrowdAgentInterface* IAgent = InterfaceCast<ICrowdAgentInterface>(this);
		// if CrowdManager->RegisterAgent fails we disable the crowd simulation for this guy
		// @todo might consider re-trying in some time
		bEnableCrowdSimulation = CrowdManager->RegisterAgent(IAgent);
	}

	if (bEnableCrowdSimulation && NavComp)
	{
		// disable path post processing (string pulling), crowd simulation needs to handle 
		// large paths by splitting into smaller parts and optimization gets in the way
		NavComp->SetNavDataFlag(ERecastPathFlags::SkipStringPulling);
	}
}

void UCrowdFollowingComponent::Cleanup()
{
	Super::Cleanup();

	UCrowdManager* CrowdManager = UCrowdManager::GetCurrent(GetWorld());
	if (CrowdManager)
	{
		const ICrowdAgentInterface* IAgent = InterfaceCast<ICrowdAgentInterface>(this);
		CrowdManager->UnregisterAgent(IAgent);
	}
}

void UCrowdFollowingComponent::AbortMove(const FString& Reason, FAIRequestID RequestID, bool bResetVelocity, bool bSilent)
{
	if (bEnableCrowdSimulation && (Status != EPathFollowingStatus::Idle) && RequestID.IsEquivalent(GetCurrentRequestId()))
	{
		UCrowdManager* CrowdManager = UCrowdManager::GetCurrent(GetWorld());
		if (CrowdManager)
		{
			CrowdManager->ClearAgentMoveTarget(this);
		}
	}

	Super::AbortMove(Reason, RequestID, bResetVelocity, bSilent);
}

void UCrowdFollowingComponent::PauseMove(FAIRequestID RequestID, bool bResetVelocity)
{
	if (bEnableCrowdSimulation && (Status != EPathFollowingStatus::Paused) && RequestID.IsEquivalent(GetCurrentRequestId()))
	{
		UCrowdManager* CrowdManager = UCrowdManager::GetCurrent(GetWorld());
		if (CrowdManager)
		{
			CrowdManager->PauseAgent(this);
		}
	}

	Super::PauseMove(RequestID, bResetVelocity);
}

void UCrowdFollowingComponent::ResumeMove(FAIRequestID RequestID)
{
	if (bEnableCrowdSimulation && (Status == EPathFollowingStatus::Paused) && RequestID.IsEquivalent(GetCurrentRequestId()))
	{
		UCrowdManager* CrowdManager = UCrowdManager::GetCurrent(GetWorld());
		if (CrowdManager)
		{
			const bool bHasMoved = HasMovedDuringPause();
			CrowdManager->ResumeAgent(this, bHasMoved);
		}
	}

	Super::ResumeMove(RequestID);
}

void UCrowdFollowingComponent::Reset()
{
	Super::Reset();
	PathStartIndex = 0;

	bFinalPathPart = false;
	bCheckMovementAngle = false;
	bUpdateDirectMoveVelocity = false;
}

bool UCrowdFollowingComponent::UpdateMovementComponent(bool bForce)
{
	bool bRet = Super::UpdateMovementComponent(bForce);
	CharacterMovement = Cast<UCharacterMovementComponent>(MovementComp);

	return bRet;
}

void UCrowdFollowingComponent::OnLanded()
{
	UCrowdManager* CrowdManager = UCrowdManager::GetCurrent(GetWorld());
	if (bEnableCrowdSimulation && CrowdManager)
	{
		const ICrowdAgentInterface* IAgent = InterfaceCast<ICrowdAgentInterface>(this);
		CrowdManager->UpdateAgentState(IAgent);
	}
}

void UCrowdFollowingComponent::OnPathFinished(EPathFollowingResult::Type Result)
{
	UCrowdManager* CrowdManager = UCrowdManager::GetCurrent(GetWorld());
	if (bEnableCrowdSimulation && CrowdManager)
	{
		CrowdManager->ClearAgentMoveTarget(this);
	}

	Super::OnPathFinished(Result);
}

void UCrowdFollowingComponent::OnPathUpdated()
{
	PathStartIndex = 0;
}

bool UCrowdFollowingComponent::ShouldCheckPathOnResume() const
{
	if (bEnableCrowdSimulation)
	{
		// never call SetMoveSegment on resuming
		return false;
	}

	return HasMovedDuringPause();
}

bool UCrowdFollowingComponent::HasMovedDuringPause() const
{
	return Super::ShouldCheckPathOnResume();
}

bool UCrowdFollowingComponent::IsOnPath() const
{
	if (bEnableCrowdSimulation)
	{
		// agent can move off path for steering/avoidance purposes
		// just pretend it's always on path to avoid problems when movement is being resumed
		return true;
	}

	return Super::IsOnPath();
}

int32 UCrowdFollowingComponent::DetermineStartingPathPoint(const FNavigationPath* ConsideredPath) const
{
	int32 StartIdx = 0;

	if (bEnableCrowdSimulation)
	{
		StartIdx = PathStartIndex;

		// no path = called from SwitchToNextPathPart, check if agent is already further on path
		if (ConsideredPath == NULL && Path.IsValid())
		{
			UCrowdManager* Manager = UCrowdManager::GetCurrent(GetWorld());
			FNavMeshPath* NavPath = Path->CastPath<FNavMeshPath>();
			if (Manager && NavPath)
			{
				int32 AdjustedStartIdx = PathStartIndex;
				Manager->AdjustAgentPathStart(this, NavPath, AdjustedStartIdx);

				StartIdx = AdjustedStartIdx;
			}
		}
	}
	else
	{
		StartIdx = Super::DetermineStartingPathPoint(ConsideredPath);
	}

	return StartIdx;
}

void LogPathPartHelper(AActor* LogOwner, FNavMeshPath* NavMeshPath, int32 StartIdx, int32 EndIdx)
{
#if ENABLE_VISUAL_LOG && WITH_RECAST
	ARecastNavMesh* NavMesh = Cast<ARecastNavMesh>(NavMeshPath->GetOwner());
	NavMesh->BeginBatchQuery();
	
	FVector SegmentStart = NavMeshPath->PathPoints[0].Location;
	FVector SegmentEnd(FVector::ZeroVector);

	if (StartIdx > 0)
	{
		NavMesh->GetPolyCenter(NavMeshPath->PathCorridor[StartIdx], SegmentStart);
	}

	for (int32 Idx = StartIdx + 1; Idx < EndIdx; Idx++)
	{
		NavMesh->GetPolyCenter(NavMeshPath->PathCorridor[Idx], SegmentEnd);
		UE_VLOG_SEGMENT_THICK(LogOwner, LogCrowdFollowing, Log, SegmentStart, SegmentEnd, FColor::Yellow, 2, TEXT_EMPTY);

		SegmentStart = SegmentEnd;
	}

	if (EndIdx == (NavMeshPath->PathCorridor.Num() - 1))
	{
		SegmentEnd = NavMeshPath->PathPoints[1].Location;
	}
	else
	{
		NavMesh->GetPolyCenter(NavMeshPath->PathCorridor[EndIdx], SegmentEnd);
	}

	UE_VLOG_SEGMENT_THICK(LogOwner, LogCrowdFollowing, Log, SegmentStart, SegmentEnd, FColor::Yellow, 2, TEXT_EMPTY);
	NavMesh->FinishBatchQuery();
#endif // ENABLE_VISUAL_LOG && WITH_RECAST
}

void UCrowdFollowingComponent::SetMoveSegment(int32 SegmentStartIndex)
{
	if (!bEnableCrowdSimulation)
	{
		Super::SetMoveSegment(SegmentStartIndex);
		return;
	}

	PathStartIndex = SegmentStartIndex;
	if (!Path.IsValid())
	{
		return;
	}

	FVector CurrentTargetPt = Path->PathPoints[1].Location;

	FNavMeshPath* NavMeshPath = Path->CastPath<FNavMeshPath>();
	if (NavMeshPath)
	{
#if WITH_RECAST
		// cut paths into parts to avoid problems with crowds getting into local minimum
		// due to using only first 10 steps of A*

		// do NOT use PathPoints here, crowd simulation disables path post processing
		// which means, that PathPoints contains only start and end position 
		// full path is available through PathCorridor array (poly refs)

		ARecastNavMesh* RecastNavData = Cast<ARecastNavMesh>(MyNavData);

		const int32 PathPartSize = 15;
		const int32 LastPolyIdx = NavMeshPath->PathCorridor.Num() - 1;
		const int32 PathPartEndIdx = FMath::Min(PathStartIndex + PathPartSize, LastPolyIdx);

		bFinalPathPart = (PathPartEndIdx == LastPolyIdx);
		if (!bFinalPathPart)
		{
			RecastNavData->GetPolyCenter(NavMeshPath->PathCorridor[PathPartEndIdx], CurrentTargetPt);
		}

		// not safe to read those directions yet, you have to wait until crowd manager gives you next corner of string pulled path
		CrowdAgentMoveDirection = FVector::ZeroVector;
		MoveSegmentDirection = FVector::ZeroVector;

		CurrentDestination.Set(Path->Base.Get(), CurrentTargetPt);
		SuspendCrowdSteering(false);

		UE_VLOG(GetOwner(), LogCrowdFollowing, Log, TEXT("SetMoveSegment, from:%d segments:%d%s"), PathStartIndex, PathPartSize, bFinalPathPart ? TEXT(" (final)") : TEXT(""));
		UE_VLOG_SEGMENT(GetOwner(), LogCrowdFollowing, Log, MovementComp->GetActorFeetLocation(), CurrentTargetPt, FColor::Red, TEXT("path part"));
		LogPathPartHelper(GetOwner(), NavMeshPath, PathStartIndex, PathPartEndIdx);

		UCrowdManager* CrowdManager = UCrowdManager::GetCurrent(GetWorld());
		if (CrowdManager)
		{
			CrowdManager->SetAgentMovePath(this, NavMeshPath, PathStartIndex, PathPartEndIdx);
		}
#endif
	}
	else
	{
		// direct paths are not using any steering or avoidance
		// pathfinding is replaced with simple velocity request 

		const FVector AgentLoc = MovementComp->GetActorFeetLocation();

		bFinalPathPart = true;
		bCheckMovementAngle = true;
		bUpdateDirectMoveVelocity = DestinationActor.IsValid();
		CurrentDestination.Set(Path->Base.Get(), CurrentTargetPt);
		CrowdAgentMoveDirection = (CurrentTargetPt - AgentLoc).SafeNormal();
		MoveSegmentDirection = CrowdAgentMoveDirection;
		SuspendCrowdSteering(true);

		UE_VLOG(GetOwner(), LogCrowdFollowing, Log, TEXT("SetMoveSegment, direct move"));
		UE_VLOG_SEGMENT(GetOwner(), LogCrowdFollowing, Log, AgentLoc, CurrentTargetPt, FColor::Red, TEXT("path"));

		UCrowdManager* CrowdManager = UCrowdManager::GetCurrent(GetWorld());
		if (CrowdManager)
		{
			CrowdManager->SetAgentMoveDirection(this, CrowdAgentMoveDirection);
		}
	}
}

void UCrowdFollowingComponent::UpdatePathSegment()
{
	if (!bEnableCrowdSimulation)
	{
		Super::UpdatePathSegment();
		return;
	}

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
		const FVector GoalLocation = GetCurrentTargetLocation();

		if (bFinalPathPart)
		{
			const FVector ToTarget = (GoalLocation - MovementComp->GetActorFeetLocation());
			const float SegmentDot = FVector::DotProduct(ToTarget, Path->IsDirect() ? MovementComp->Velocity : CrowdAgentMoveDirection);
			const bool bMovedTooFar = bCheckMovementAngle && (SegmentDot < 0.0);

			if (bMovedTooFar || HasReachedDestination(CurrentLocation))
			{
				UE_VLOG(GetOwner(), LogCrowdFollowing, Log, TEXT("Last path segment finished due to \'%s\'"), bMovedTooFar ? TEXT("Missing Last Point") : TEXT("Reaching Destination"));
				OnPathFinished(EPathFollowingResult::Success);
			}
		}
		else
		{
			// override radius multiplier and switch to next path part when closer than 4x agent radius
			const float SavedAgentRadiusPct = MinAgentRadiusPct;
			MinAgentRadiusPct = 4.0f;
			
			const bool bHasReached = HasReachedInternal(GoalLocation, 0.0f, 0.0f, CurrentLocation, 0.0f, false);
			
			MinAgentRadiusPct = SavedAgentRadiusPct;

			if (bHasReached)
			{
				SwitchToNextPathPart();
			}
		}
	}

	// gather location samples to detect if moving agent is blocked
	if (bCanReachTarget && Status == EPathFollowingStatus::Moving)
	{
		const bool bHasNewSample = UpdateBlockDetection();
		if (bHasNewSample && IsBlocked())
		{
			OnPathFinished(EPathFollowingResult::Blocked);
		}
	}
}

void UCrowdFollowingComponent::FollowPathSegment(float DeltaTime)
{
	if (!bEnableCrowdSimulation)
	{
		Super::FollowPathSegment(DeltaTime);
		return;
	}

	if (bUpdateDirectMoveVelocity && DestinationActor.IsValid())
	{
		const FVector CurrentTargetPt = DestinationActor->GetActorLocation();
		const float DistSq = (CurrentTargetPt - GetCurrentTargetLocation()).SizeSquared();
		if (DistSq > FMath::Square(10.0f))
		{
			UCrowdManager* Manager = UCrowdManager::GetCurrent(GetWorld());
			const FVector AgentLoc = GetCrowdAgentLocation();

			CurrentDestination.Set(Path->Base.Get(), CurrentTargetPt);
			CrowdAgentMoveDirection = (CurrentTargetPt - AgentLoc).SafeNormal();
			MoveSegmentDirection = CrowdAgentMoveDirection;

			Manager->SetAgentMoveDirection(this, MoveSegmentDirection);
			UE_VLOG(GetOwner(), LogCrowdFollowing, Log, TEXT("Updated direct move direction for crowd agent."));
		}
	}

	UpdateMoveFocus();
}

FVector UCrowdFollowingComponent::GetMoveFocus(bool bAllowStrafe) const
{
	// can't really use CurrentDestination here, as it's pointing at end of path part
	// fallback to looking at point in front of agent

	if (!bAllowStrafe && MovementComp && bEnableCrowdSimulation)
	{
		const FVector AgentLoc = MovementComp->GetActorLocation();
		const FVector ForwardDir = CrowdAgentMoveDirection.IsNearlyZero() && MovementComp && MovementComp->GetOwner() ?
			MovementComp->GetOwner()->GetActorRotation().Vector() :
			CrowdAgentMoveDirection;

		return AgentLoc + ForwardDir * 100.0f;
	}

	return Super::GetMoveFocus(bAllowStrafe);
}

void UCrowdFollowingComponent::OnNavNodeChanged(NavNodeRef NewPolyRef, NavNodeRef PrevPolyRef, int32 CorridorSize)
{
	if (bEnableCrowdSimulation)
	{
		UE_VLOG(GetOwner(), LogCrowdFollowing, Verbose, TEXT("OnNavNodeChanged, CorridorSize:%d"), CorridorSize);

		const bool bSwitchPart = ShouldSwitchPathPart(CorridorSize);
		if (bSwitchPart && !bFinalPathPart)
		{
			SwitchToNextPathPart();
		}
	}
}

bool UCrowdFollowingComponent::ShouldSwitchPathPart(int32 CorridorSize) const
{
	return CorridorSize <= 2;
}

void UCrowdFollowingComponent::SwitchToNextPathPart()
{
	const int32 NewPartStart = DetermineStartingPathPoint(NULL);
	SetMoveSegment(NewPartStart);
}

void UCrowdFollowingComponent::GetDebugStringTokens(TArray<FString>& Tokens, TArray<EPathFollowingDebugTokens::Type>& Flags) const
{
	if (!bEnableCrowdSimulation)
	{
		Super::GetDebugStringTokens(Tokens, Flags);
		return;
	}

	Tokens.Add(GetStatusDesc());
	Flags.Add(EPathFollowingDebugTokens::Description);

	if (Status != EPathFollowingStatus::Moving)
	{
		return;
	}

	FString& StatusDesc = Tokens[0];
	if (Path.IsValid())
	{
		FNavMeshPath* NavMeshPath = Path->CastPath<FNavMeshPath>();
		if (NavMeshPath)
		{
			StatusDesc += FString::Printf(TEXT(" (path:%d)"), PathStartIndex);
		}
		else
		{
			StatusDesc += TEXT(" (direct)");
		}
	}

	// get cylinder of moving agent
	float AgentRadius = 0.0f;
	float AgentHalfHeight = 0.0f;
	AActor* MovingAgent = MovementComp->GetOwner();
	MovingAgent->GetSimpleCollisionCylinder(AgentRadius, AgentHalfHeight);

	if (bFinalPathPart)
	{
		float CurrentDot = 0.0f, CurrentDistance = 0.0f, CurrentHeight = 0.0f;
		uint8 bFailedDot = 0, bFailedDistance = 0, bFailedHeight = 0;
		DebugReachTest(CurrentDot, CurrentDistance, CurrentHeight, bFailedHeight, bFailedDistance, bFailedHeight);

		Tokens.Add(TEXT("dist2D"));
		Flags.Add(EPathFollowingDebugTokens::ParamName);
		Tokens.Add(FString::Printf(TEXT("%.0f"), CurrentDistance));
		Flags.Add(bFailedDistance ? EPathFollowingDebugTokens::FailedValue : EPathFollowingDebugTokens::PassedValue);

		Tokens.Add(TEXT("distZ"));
		Flags.Add(EPathFollowingDebugTokens::ParamName);
		Tokens.Add(FString::Printf(TEXT("%.0f"), CurrentHeight));
		Flags.Add(bFailedHeight ? EPathFollowingDebugTokens::FailedValue : EPathFollowingDebugTokens::PassedValue);
	}
	else
	{
		const FVector CurrentLocation = MovementComp->GetActorFeetLocation();

		// make sure we're not too close to end of path part (poly count can always fail when AI goes off path)
		const float DistSq = (GetCurrentTargetLocation() - CurrentLocation).SizeSquared();
		const float PathSwitchThresSq = FMath::Square(AgentRadius * 5.0f);

		Tokens.Add(TEXT("dist2D"));
		Flags.Add(EPathFollowingDebugTokens::ParamName);
		Tokens.Add(FString::Printf(TEXT("%.0f"), FMath::Sqrt(DistSq)));
		Flags.Add((DistSq < PathSwitchThresSq) ? EPathFollowingDebugTokens::PassedValue : EPathFollowingDebugTokens::FailedValue);
	}
}

#if ENABLE_VISUAL_LOG

void UCrowdFollowingComponent::DescribeSelfToVisLog(struct FVisLogEntry* Snapshot) const
{
	if (!bEnableCrowdSimulation)
	{
		Super::DescribeSelfToVisLog(Snapshot);
		return;
	}

	FVisLogEntry::FStatusCategory Category;
	Category.Category = TEXT("Path following");

	if (DestinationActor.IsValid())
	{
		Category.Add(TEXT("Goal"), GetNameSafe(DestinationActor.Get()));
	}

	FString StatusDesc = GetStatusDesc();

	FNavMeshPath* NavMeshPath = Path.IsValid() ? Path->CastPath<FNavMeshPath>() : NULL;
	if (Status == EPathFollowingStatus::Moving)
	{
		StatusDesc += FString::Printf(TEXT(" [path:%d]"), PathStartIndex);
	}

	Category.Add(TEXT("Status"), StatusDesc);
	Category.Add(TEXT("Path"), !Path.IsValid() ? TEXT("none") : NavMeshPath ? TEXT("navmesh") : TEXT("direct"));

	if (CurrentSmartLink)
	{
		Category.Add(TEXT("SmartLink"), CurrentSmartLink->GetName());
	}

	Snapshot->Status.Add(Category);
}

#endif // ENABLE_VISUAL_LOG
