// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "NavigationPathBuilder.h"
#include "VisualLog.h"
#include "../Classes/Kismet/GameplayStatics.h"


bool AAIController::bAIIgnorePlayers = false;

DECLARE_CYCLE_STAT(TEXT("MoveToLocation"),STAT_MoveToLocation,STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("MoveToActor"),STAT_MoveToActor,STATGROUP_AI);


AAIController::AAIController(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

	// set up navigation component
	NavComponent = PCIP.CreateDefaultSubobject<UNavigationComponent>(this, TEXT("NavComponent"));

	PathFollowingComponent = PCIP.CreateDefaultSubobject<UPathFollowingComponent>(this, TEXT("PathFollowingComponent"));
	PathFollowingComponent->OnMoveFinished.AddUObject(this, &AAIController::OnMoveCompleted);

	bSkipExtraLOSChecks = true;
	bWantsPlayerState = false;
}

void AAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateControlRotation(DeltaTime);
}

void AAIController::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (bWantsPlayerState && !IsPendingKill() && (GetNetMode() != NM_Client))
	{
		InitPlayerState();
	}
}

void AAIController::Reset()
{
	Super::Reset();

	if (PathFollowingComponent)
	{
		PathFollowingComponent->AbortMove(TEXT("controller reset"));
	}
}

void AAIController::DisplayDebug(class UCanvas* Canvas, const TArray<FName>& DebugDisplay, float& YL, float& YPos)
{
	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

	static FName NAME_AI = FName(TEXT("AI"));
	if (DebugDisplay.Contains(NAME_AI))
	{
		if (PathFollowingComponent)
		{
			PathFollowingComponent->DisplayDebug(Canvas, DebugDisplay, YL, YPos);
		}
		
		if ( GetFocusActor() )
		{
			Canvas->DrawText(GEngine->GetSmallFont(), FString::Printf(TEXT("      Focus %s"), *GetFocusActor()->GetName()), 4.0f, YPos);
			YPos += YL;
		}
	}
}

#if ENABLE_VISUAL_LOG

void AAIController::GrabDebugSnapshot(FVisLogEntry* Snapshot) const
{
	Snapshot->StatusString += FString::Printf(TEXT("\
										Pawn: %s\n\
										Focus: %s\n")
		, *GetDebugName(GetPawn()), *GetDebugName(GetFocusActor()));

	if (GetPawn())
	{
		Snapshot->Location = GetPawn()->GetActorLocation();
	}

	if (NavComponent.IsValid())
	{
		NavComponent->DescribeSelfToVisLog(Snapshot);
	}

	if (PathFollowingComponent.IsValid())
	{
		PathFollowingComponent->DescribeSelfToVisLog(Snapshot);
	}

	if (BrainComponent != NULL)
	{
		BrainComponent->DescribeSelfToVisLog(Snapshot);
	}
}
#endif // ENABLE_VISUAL_LOG

void AAIController::GetPlayerViewPoint(FVector& out_Location, FRotator& out_Rotation) const
{
	// AI does things from the Pawn
	if (GetPawn() != NULL)
	{
		out_Location = GetPawn()->GetActorLocation();
		out_Rotation = GetPawn()->GetActorRotation();
	}
	else
	{
		Super::GetPlayerViewPoint(out_Location, out_Rotation);
	}
}

void AAIController::SetFocalPoint( FVector FP, bool bOffsetFromBase, uint8 InPriority)
{
	if (InPriority >= FocusInformation.Priorities.Num())
	{
		FocusInformation.Priorities.SetNum(InPriority+1);
	}
	FFocusKnowledge::FFocusItem& Focusitem = FocusInformation.Priorities[InPriority];

	{
		AActor* FocalActor = NULL;
		if (bOffsetFromBase)
		{
			APawn *Pawn = GetPawn();
			if (Pawn && Pawn->GetMovementBase())
			{
				FocalActor = Pawn->GetMovementBase()->GetOwner();
			}
		}
		Focusitem.Position.Set( FocalActor, FP );
	}

	Focusitem.Actor = NULL;
}

FVector AAIController::GetFocalPoint()
{
	FBasedPosition FinalFocus;
	// find focus with highest priority
	for( int32 Index = FocusInformation.Priorities.Num()-1; Index >= 0; --Index)
	{
		const FFocusKnowledge::FFocusItem& Focusitem = FocusInformation.Priorities[Index];
		if ( Focusitem.Actor.IsValid() )
		{
			const AActor* Focus = Focusitem.Actor.Get();
			UPrimitiveComponent* MyBase = GetPawn() ? GetPawn()->GetMovementBase() : NULL;
			const bool bRequestedFocusIsBased = MyBase && Cast<const APawn>(Focus) && (Cast<const APawn>(Focus)->GetMovementBase() == MyBase);
			FinalFocus.Set(bRequestedFocusIsBased && MyBase ? MyBase->GetOwner() : NULL, Focus->GetActorLocation());
			break;
		}
		else if( !(*Focusitem.Position).IsZero() )
		{
			FinalFocus = Focusitem.Position;
			break;
		}
	}

	return *FinalFocus;
}

AActor* AAIController::GetFocusActor() const
{
	AActor* FocusActor = NULL;
	for( int32 Index = FocusInformation.Priorities.Num()-1; Index >= 0; --Index)
	{
		const FFocusKnowledge::FFocusItem& Focusitem = FocusInformation.Priorities[Index];
		if ( Focusitem.Actor.IsValid() )
		{
			FocusActor = Focusitem.Actor.Get();
			break;
		}
		else if( !(*Focusitem.Position).IsZero() )
		{
			break;
		}
	}

	return FocusActor;
}

void AAIController::K2_SetFocus(AActor* NewFocus)
{ 
	SetFocus(NewFocus, EAIFocusPriority::Gameplay); 
}

void AAIController::K2_SetFocalPoint(FVector FP, bool bOffsetFromBase)
{
	SetFocalPoint(FP, bOffsetFromBase, EAIFocusPriority::Gameplay);
}

void AAIController::K2_ClearFocus()
{
	ClearFocus(EAIFocusPriority::Gameplay);
}

void AAIController::SetFocus(AActor* NewFocus, uint8 InPriority)
{
	if( NewFocus )
	{
		if (InPriority >= FocusInformation.Priorities.Num())
		{
			FocusInformation.Priorities.SetNum(InPriority+1);
		}
		FocusInformation.Priorities[InPriority].Actor = NewFocus;
	}
	else
	{
		ClearFocus(InPriority);
	}
}

void AAIController::ClearFocus(uint8 InPriority)
{
	if (InPriority < FocusInformation.Priorities.Num())
	{
		FocusInformation.Priorities[InPriority].Actor = NULL;
		FocusInformation.Priorities[InPriority].Position.Clear();
	}
}

bool AAIController::LineOfSightTo(const AActor* Other, FVector ViewPoint, bool bAlternateChecks)
{
	if( !Other )
	{
		return false;
	}
		
	if ( ViewPoint.IsZero() )
	{
		AActor*	ViewTarg = GetViewTarget();
		ViewPoint = ViewTarg->GetActorLocation();
		if( ViewTarg == GetPawn() )
		{
			ViewPoint.Z += GetPawn()->BaseEyeHeight; //look from eyes
		}
	}

	static FName NAME_LineOfSight = FName(TEXT("LineOfSight"));
	FVector TargetLocation = Other->GetTargetLocation(GetPawn());

	FCollisionQueryParams CollisionParams(NAME_LineOfSight, true, this->GetPawn());
	CollisionParams.AddIgnoredActor(Other);

	bool bHit = GetWorld()->LineTraceTest(ViewPoint, TargetLocation, ECC_Visibility, CollisionParams);
	if( !bHit )
	{
		return true;
	}

	// if other isn't using a cylinder for collision and isn't a Pawn (which already requires an accurate cylinder for AI)
	// then don't go any further as it likely will not be tracing to the correct location
	const APawn * OtherPawn = Cast<const APawn>(Other);
	if (!OtherPawn && Cast<UCapsuleComponent>(Other->GetRootComponent()) == NULL)
	{
		return false;
	}

	const FVector OtherActorLocation = Other->GetActorLocation();
	float distSq = (OtherActorLocation - ViewPoint).SizeSquared();
	if ( distSq > FARSIGHTTHRESHOLDSQUARED )
	{
		return false;
	}

	if ( !OtherPawn && (distSq > NEARSIGHTTHRESHOLDSQUARED) ) 
	{
		return false;
	}

	float OtherRadius, OtherHeight;
	Other->GetSimpleCollisionCylinder(OtherRadius, OtherHeight);

	if ( !bAlternateChecks || !bLOSflag )
	{
		//try viewpoint to head
		bHit = GetWorld()->LineTraceTest(ViewPoint,  OtherActorLocation + FVector(0.f,0.f,OtherHeight), ECC_Visibility, CollisionParams);
		if ( !bHit )
		{
			return true;
		}
	}

	if( !bSkipExtraLOSChecks && (!bAlternateChecks || bLOSflag) )
	{
		// only check sides if width of other is significant compared to distance
		if( OtherRadius * OtherRadius/(OtherActorLocation - ViewPoint).SizeSquared() < 0.0001f )
		{
			return false;
		}
		//try checking sides - look at dist to four side points, and cull furthest and closest
		FVector Points[4];
		Points[0] = OtherActorLocation - FVector(OtherRadius, -1 * OtherRadius, 0);
		Points[1] = OtherActorLocation + FVector(OtherRadius, OtherRadius, 0);
		Points[2] = OtherActorLocation - FVector(OtherRadius, OtherRadius, 0);
		Points[3] = OtherActorLocation + FVector(OtherRadius, -1 * OtherRadius, 0);
		int32 imin = 0;
		int32 imax = 0;
		float currentmin = (Points[0] - ViewPoint).SizeSquared();
		float currentmax = currentmin;
		for ( int32 i=1; i<4; i++ )
		{
			float nextsize = (Points[i] - ViewPoint).SizeSquared();
			if (nextsize > currentmax)
			{
				currentmax = nextsize;
				imax = i;
			}
			else if (nextsize < currentmin)
			{
				currentmin = nextsize;
				imin = i;
			}
		}

		for ( int32 i=0; i<4; i++ )
		{
			if	( (i != imin) && (i != imax) )
			{
				bHit = GetWorld()->LineTraceTest(ViewPoint,  Points[i], ECC_Visibility, CollisionParams);
				if ( !bHit )
				{
					return true;
				}
			}
		}
	}
	return false;
}

DEFINE_LOG_CATEGORY_STATIC(LogTestAI, All, All);

void AAIController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
	// Look toward focus
	FVector FocalPoint = GetFocalPoint();
	if( !FocalPoint.IsZero() && GetPawn())
	{
		FVector Direction = FocalPoint - GetPawn()->GetActorLocation();
		FRotator NewControlRotation = Direction.Rotation();

		// Don't pitch view of walking pawns unless looking at another pawn
		if ( GetPawn()->GetMovementComponent() && GetPawn()->GetMovementComponent()->IsMovingOnGround() &&
			PathFollowingComponent && (!PathFollowingComponent->GetMoveGoal() || !Cast<APawn>(PathFollowingComponent->GetMoveGoal()) ) )
		{
			NewControlRotation.Pitch = 0.f;
		}
		NewControlRotation.Yaw = FRotator::ClampAxis(NewControlRotation.Yaw);

		SetControlRotation(NewControlRotation);

		APawn* const P = GetPawn();
		if (P && bUpdatePawn)
		{
			P->FaceRotation(NewControlRotation, DeltaTime);
		}
	}
}


void AAIController::Possess(APawn* InPawn)
{
	Super::Possess(InPawn);

	if (!GetPawn())
	{
		return;
	}
	
	UpdateNavigationComponents();

	if (bWantsPlayerState)
	{
		ChangeState(NAME_Playing);
	}
}

void AAIController::InitNavigationControl(UNavigationComponent*& PathFindingComp, UPathFollowingComponent*& PathFollowingComp)
{
	PathFindingComp = NavComponent;
	PathFollowingComp = PathFollowingComponent;
}

EPathFollowingRequestResult::Type AAIController::MoveToActor(class AActor* Goal, float AcceptanceRadius, bool bStopOnOverlap, bool bUsePathfinding, bool bCanStrafe, TSubclassOf<UNavigationQueryFilter> FilterClass)
{
	SCOPE_CYCLE_COUNTER(STAT_MoveToActor);
	EPathFollowingRequestResult::Type Result = EPathFollowingRequestResult::Failed;

	UE_VLOG(this, LogNavigation, Log, TEXT("MoveToActor: Goal(%s) AcceptRadius(%.1f%s) bUsePathfinding(%d) bCanStrafe(%d)"),
		*GetNameSafe(Goal), AcceptanceRadius, bStopOnOverlap ? TEXT(" + agent") : TEXT(""), bUsePathfinding, bCanStrafe);

	if (Goal)
	{
		if (PathFollowingComponent && PathFollowingComponent->HasReached(Goal, AcceptanceRadius, !bStopOnOverlap))
		{
			UE_VLOG(this, LogNavigation, Log, TEXT("MoveToActor: already at goal!"));
			PathFollowingComponent->SetLastMoveAtGoal(true);

			OnMoveCompleted(0, EPathFollowingResult::Success);
			Result = EPathFollowingRequestResult::AlreadyAtGoal;
		}
		else 
		{
			FNavPathSharedPtr Path = FindPath(Goal, bUsePathfinding, UNavigationQueryFilter::GetQueryFilter(FilterClass));
			if (Path.IsValid())
			{
				bAllowStrafe = bCanStrafe;

				const uint32 RequestID = RequestMove(Path, Goal, AcceptanceRadius, bStopOnOverlap);
				Result = (RequestID != INVALID_MOVEREQUESTID) ? EPathFollowingRequestResult::RequestSuccessful : EPathFollowingRequestResult::Failed;
			}
		}
	}

	if (Result == EPathFollowingRequestResult::Failed)
	{
		if (PathFollowingComponent)
		{
			PathFollowingComponent->SetLastMoveAtGoal(false);
		}

		OnMoveCompleted(INVALID_MOVEREQUESTID, EPathFollowingResult::Invalid);
	}

	return Result;
}

EPathFollowingRequestResult::Type AAIController::MoveToLocation(const FVector& Dest, float AcceptanceRadius, bool bStopOnOverlap, bool bUsePathfinding, bool bProjectDestinationToNavigation, bool bCanStrafe, TSubclassOf<UNavigationQueryFilter> FilterClass)
{
	SCOPE_CYCLE_COUNTER(STAT_MoveToLocation);

	EPathFollowingRequestResult::Type Result = EPathFollowingRequestResult::Failed;
	bool bCanRequestMove = true;

	UE_VLOG(this, LogNavigation, Log, TEXT("MoveToLocation: Goal(%s) AcceptRadius(%.1f%s) bUsePathfinding(%d) bCanStrafe(%d)"),
		*Dest.ToString(), AcceptanceRadius, bStopOnOverlap ? TEXT(" + agent") : TEXT(""), bUsePathfinding, bCanStrafe);

	// Check input is valid
	if( Dest.ContainsNaN() )
	{
		UE_VLOG(this, LogNavigation, Error, TEXT("AAIController::MoveToLocation: Destination is not valid! Goal(%s) AcceptRadius(%.1f%s) bUsePathfinding(%d) bCanStrafe(%d)"),
			*Dest.ToString(), AcceptanceRadius, bStopOnOverlap ? TEXT(" + agent") : TEXT(""), bUsePathfinding, bCanStrafe);
		
		ensure( !Dest.ContainsNaN() );
		bCanRequestMove = false;
	}

	FVector GoalLocation = Dest;

	// fail if projection to navigation is required but it either failed or there's not navigation component
	if (bCanRequestMove && bProjectDestinationToNavigation && (NavComponent == NULL || !NavComponent->ProjectPointToNavigation(Dest, GoalLocation)))
	{
		UE_VLOG_LOCATION(this, Dest, 30.f, FLinearColor::Red, TEXT("AAIController::MoveToLocation failed to project destination location to navmesh"));
		bCanRequestMove = false;
	}

	if (bCanRequestMove && PathFollowingComponent && PathFollowingComponent->HasReached(GoalLocation, AcceptanceRadius, !bStopOnOverlap))
	{
		UE_VLOG(this, LogNavigation, Log, TEXT("MoveToLocation: already at goal!"));
		PathFollowingComponent->SetLastMoveAtGoal(true);

		OnMoveCompleted(0, EPathFollowingResult::Success);
		Result = EPathFollowingRequestResult::AlreadyAtGoal;
		bCanRequestMove = false;
	}

	if (bCanRequestMove)
	{
		FNavPathSharedPtr Path = FindPath(GoalLocation, bUsePathfinding, UNavigationQueryFilter::GetQueryFilter(FilterClass));
	if (Path.IsValid())
	{
		bAllowStrafe = bCanStrafe;

		const uint32 RequestID = RequestMove(Path, NULL, AcceptanceRadius, bStopOnOverlap);
			Result = (RequestID != INVALID_MOVEREQUESTID) ? EPathFollowingRequestResult::RequestSuccessful : EPathFollowingRequestResult::Failed;
		}
	}

	if (Result == EPathFollowingRequestResult::Failed)
	{
		if (PathFollowingComponent)
		{
			PathFollowingComponent->SetLastMoveAtGoal(false);
		}

		OnMoveCompleted(INVALID_MOVEREQUESTID, EPathFollowingResult::Invalid);
	}

	return Result;
}

uint32 AAIController::RequestMove(FNavPathSharedPtr Path, class AActor* Goal, float AcceptanceRadius, bool bStopOnOverlap, FCustomMoveSharedPtr CustomData)
{
	uint32 RequestID = INVALID_MOVEREQUESTID;
	if (PathFollowingComponent)
	{
		RequestID = PathFollowingComponent->RequestMove(Path, Goal, AcceptanceRadius, bStopOnOverlap, CustomData);
	}

	return RequestID;
}

void AAIController::StopMovement()
{
	UE_VLOG(this, LogNavigation, Log, TEXT("%s STOP MOVEMENT"), *GetNameSafe(GetPawn()) );
	PathFollowingComponent->AbortMove(TEXT("StopMovement"));
}

FNavPathSharedPtr AAIController::FindPath(class AActor* Goal, bool bUsePathfinding, TSharedPtr<const FNavigationQueryFilter> QueryFilter)
{
	if (NavComponent)
	{
		const bool bFoundPath = bUsePathfinding ? NavComponent->FindPathToActor(Goal, QueryFilter) : NavComponent->FindSimplePathToActor(Goal);
		if (bFoundPath)
		{
			return NavComponent->GetPath();
		}
	}

	return NULL;
}

FNavPathSharedPtr AAIController::FindPath(const FVector& Dest, bool bUsePathfinding, TSharedPtr<const FNavigationQueryFilter> QueryFilter)
{
	if (NavComponent)
	{
		const bool bFoundPath = bUsePathfinding ? NavComponent->FindPathToLocation(Dest, QueryFilter) : NavComponent->FindSimplePathToLocation(Dest);
		if (bFoundPath) 
		{
			return NavComponent->GetPath();
		}
	}

	return NULL;
}

EPathFollowingStatus::Type AAIController::GetMoveStatus() const
{
	return (PathFollowingComponent) ? PathFollowingComponent->GetStatus() : EPathFollowingStatus::Idle;
}

bool AAIController::HasPartialPath() const
{
	return (PathFollowingComponent != NULL) && (PathFollowingComponent->HasPartialPath());
}

FVector AAIController::GetImmediateMoveDestination() const
{
	return (PathFollowingComponent) ? PathFollowingComponent->GetCurrentTargetLocation() : FVector::ZeroVector;
}

void AAIController::SetMoveBlockDetection(bool bEnable)
{
	if (PathFollowingComponent)
	{
		PathFollowingComponent->SetBlockDetectionState(bEnable);
	}
}

void AAIController::OnMoveCompleted(uint32 RequestID, EPathFollowingResult::Type Result)
{
	ReceiveMoveCompleted.Broadcast(RequestID, Result);
}

bool AAIController::RunBehaviorTree(UBehaviorTree* BTAsset)
{
	// @todo: find BrainComponent and see if it's BehaviorTreeComponent
	// Also check if BTAsset requires BlackBoardComponent, and if so 
	// check if BB type is accepted by BTAsset.
	// Spawn BehaviorTreeComponent if none present. 
	// Spawn BlackBoardComponent if none present, but fail if one is present but is not of compatible class
	if (BTAsset == NULL)
	{
		UE_VLOG(this, LogBehaviorTree, Warning, TEXT("RunBehaviorTree: Unable to run NULL behavior tree"));
		return false;
	}

	bool bSuccess = true;
	bool bShouldInitializeBlackboard = false;

	// see if need a blackboard component at all
	UBlackboardComponent* BlackboardComp = NULL;
	if (BTAsset->BlackboardAsset)
	{
		BlackboardComp = FindComponentByClass<UBlackboardComponent>();
		if (BlackboardComp == NULL)
		{
			BlackboardComp = ConstructObject<UBlackboardComponent>(UBlackboardComponent::StaticClass(), this, TEXT("BlackboardComponent"));
			if (BlackboardComp != NULL)
			{
				BlackboardComp->InitializeBlackboard(BTAsset->BlackboardAsset);
				
				BlackboardComp->RegisterComponent();
				bShouldInitializeBlackboard = true;
			}
		}
		else if (BlackboardComp->GetBlackboardAsset() == NULL)
		{
			BlackboardComp->InitializeBlackboard(BTAsset->BlackboardAsset);
		}
		else if (BlackboardComp->GetBlackboardAsset() != BTAsset->BlackboardAsset)
		{
			bSuccess = false;
			UE_VLOG(this, LogBehaviorTree, Log, TEXT("RunBehaviorTree: BTAsset %s requires blackboard %s while already has %s instantiated"),
				*GetNameSafe(BTAsset), *GetNameSafe(BTAsset->BlackboardAsset), *GetNameSafe(BlackboardComp->GetBlackboardAsset()) );
		}
	}
	
	if (bSuccess)
	{
		UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComponent);
		if (BTComp == NULL)
		{
			UE_VLOG(this, LogBehaviorTree, Log, TEXT("RunBehaviorTree: spawning BehaviorTreeComponent.."));

			BrainComponent = BTComp = ConstructObject<UBehaviorTreeComponent>(UBehaviorTreeComponent::StaticClass(), this, TEXT("BTComponent"));
			BrainComponent->RegisterComponent();

			if (BrainComponent->bWantsInitializeComponent)
			{
				// make sure that newly created component is initialized before running BT
				// both blackboard and BT to must exist before calling it!
				BrainComponent->InitializeComponent();
			}
		}

		if (bShouldInitializeBlackboard && BlackboardComp && BlackboardComp->bWantsInitializeComponent)
		{
			// make sure that newly created component is initialized before running BT
			// both blackboard and BT to must exist before calling it!

			BlackboardComp->InitializeComponent();
		}

		check(BTComp != NULL);
		BTComp->StartTree(BTAsset, EBTExecutionMode::Looped);
	}

	return bSuccess;
}


bool AAIController::SuggestTossVelocity(FVector& OutTossVelocity, FVector Start, FVector End, float TossSpeed, bool bPreferHighArc, float CollisionRadius, bool bOnlyTraceUp)
{
	// pawn's physics volume gets 2nd priority
	APhysicsVolume const* const PhysicsVolume = GetPawn() ? GetPawn()->GetPawnPhysicsVolume() : NULL;
	float const GravityOverride = PhysicsVolume ? PhysicsVolume->GetGravityZ() : 0.f;
	ESuggestProjVelocityTraceOption::Type const TraceOption = bOnlyTraceUp ? ESuggestProjVelocityTraceOption::OnlyTraceWhileAsceding : ESuggestProjVelocityTraceOption::TraceFullPath;

	return UGameplayStatics::SuggestProjectileVelocity(this, OutTossVelocity, Start, End, TossSpeed, bPreferHighArc, CollisionRadius, GravityOverride, TraceOption);
}


