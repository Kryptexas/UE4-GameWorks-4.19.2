// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "Actions/PawnAction_Move.h"

UPawnAction_Move::UPawnAction_Move(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, GoalLocation(FAISystem::InvalidLocation)
	, AcceptableRadius(30.f)
	, bUsePathfinding(true)
	, bProjectGoalToNavigation(false)
	, bUpdatePathToGoal(true)
	, bAbortChildActionOnPathChange(false)
{
}

UPawnAction_Move* UPawnAction_Move::CreateAction(UWorld* World, class AActor* GoalActor, EPawnActionMoveMode::Type Mode)
{
	if (GoalActor == NULL)
	{
		return NULL;
	}

	UPawnAction_Move* Action = UPawnAction::CreateActionInstance<UPawnAction_Move>(World);
	if (Action)
	{
		Action->GoalActor = GoalActor;
		Action->bUsePathfinding = (Mode == EPawnActionMoveMode::UsePathfinding);
	}

	return Action;
}

UPawnAction_Move* UPawnAction_Move::CreateAction(UWorld* World, const FVector& GoalLocation, EPawnActionMoveMode::Type Mode)
{
	if (FAISystem::IsValidLocation(GoalLocation) == false)
	{
		return NULL;
	}

	UPawnAction_Move* Action = UPawnAction::CreateActionInstance<UPawnAction_Move>(World);
	if (Action)
	{
		Action->GoalLocation = GoalLocation;
		Action->bUsePathfinding = (Mode == EPawnActionMoveMode::UsePathfinding);
	}

	return Action;
}

bool UPawnAction_Move::Start()
{
	bool bResult = Super::Start();
	if (bResult)
	{
		bResult = PerformMoveAction();
	}

	return bResult;
}

EPathFollowingRequestResult::Type UPawnAction_Move::RequestMove(AAIController* Controller)
{
	EPathFollowingRequestResult::Type RequestResult = EPathFollowingRequestResult::Failed;

	if (GoalActor != NULL)
	{
		const bool bAtGoal = CheckAlreadyAtGoal(Controller, GoalActor, AcceptableRadius);
		RequestResult = bAtGoal ? EPathFollowingRequestResult::AlreadyAtGoal : 
			bUpdatePathToGoal ? Controller->MoveToActor(GoalActor, AcceptableRadius, true, bUsePathfinding, bAllowStrafe, FilterClass) :
			Controller->MoveToLocation(GoalActor->GetActorLocation(), AcceptableRadius, true, bUsePathfinding, bProjectGoalToNavigation, bAllowStrafe);
	}
	else if (FAISystem::IsValidLocation(GoalLocation))
	{
		const bool bAtGoal = CheckAlreadyAtGoal(Controller, GoalLocation, AcceptableRadius);
		RequestResult = bAtGoal ? EPathFollowingRequestResult::AlreadyAtGoal :
			Controller->MoveToLocation(GoalLocation, AcceptableRadius, true, bUsePathfinding, bProjectGoalToNavigation, bAllowStrafe, FilterClass);
	}
	else
	{
		UE_VLOG(Controller, LogPawnAction, Warning, TEXT("UPawnAction_Move::Start: no valid move goal set"));
	}

	return RequestResult;
}

bool UPawnAction_Move::PerformMoveAction()
{
	AAIController* MyController = Cast<AAIController>(GetController());
	if (MyController == NULL)
	{
		return false;
	}

	if (MyController->ShouldPostponePathUpdates())
	{
		UE_VLOG(MyController, LogPawnAction, Log, TEXT("Can't path right now, waiting..."));
		MyController->GetWorldTimerManager().SetTimer(this, &UPawnAction_Move::DeferredPerformMoveAction, 0.1f);
		return true;
	}

	const EPathFollowingRequestResult::Type RequestResult = RequestMove(MyController);
	bool bResult = true;

	if (RequestResult == EPathFollowingRequestResult::RequestSuccessful)
	{
		RequestID = MyController->GetCurrentMoveRequestID();
		WaitForMessage(UBrainComponent::AIMessage_MoveFinished, RequestID);
		WaitForMessage(UBrainComponent::AIMessage_RepathFailed);
	}
	else if (RequestResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		// note that this will result in latently notifying actions component about finishing
		// so it's no problem we run it from withing Start function
		Finish(EPawnActionResult::Success);
	}
	else
	{
		bResult = false;
	}

	return bResult;
}

void UPawnAction_Move::DeferredPerformMoveAction()
{
	const bool bResult = PerformMoveAction();
	if (!bResult)
	{
		Finish(EPawnActionResult::Failed);
	}
}

bool UPawnAction_Move::Pause()
{
	bool bResult = Super::Pause();
	if (bResult)
	{
		AAIController* MyController = Cast<AAIController>(GetController());
		if (MyController)
		{
			bResult = MyController->PauseMove(RequestID);
		}
	}
	return bResult;
}

bool UPawnAction_Move::Resume()
{
	bool bResult = Super::Resume();
	if (bResult)
	{
		bResult = false;
		AAIController* MyController = Cast<AAIController>(GetController());
		if (MyController)
		{
			if (MyController->ResumeMove(RequestID) == false)
			{
				// try requesting a new move
				StopWaitingForMessages();
				
				bResult = PerformMoveAction();
			}
			else
			{
				bResult = true;
			}
		}
	}
	return bResult;
}

EPawnActionAbortState::Type UPawnAction_Move::PerformAbort(EAIForceParam::Type ShouldForce)
{
	AAIController* MyController = Cast<AAIController>(GetController());

	if (MyController && MyController->PathFollowingComponent)
	{
		MyController->PathFollowingComponent->AbortMove(TEXT("BehaviorTree abort"), RequestID);
	}

	return Super::PerformAbort(ShouldForce);
}

void UPawnAction_Move::HandleAIMessage(UBrainComponent*, const FAIMessage& Message)
{
	if (Message.MessageName == UBrainComponent::AIMessage_MoveFinished && Message.HasFlag(EPathFollowingMessage::OtherRequest))
	{
		// move was aborted by another request from different action, don't finish yet
		return;
	}

	const bool bFail = Message.MessageName == UBrainComponent::AIMessage_RepathFailed
		|| Message.Status == FAIMessage::Failure;

	Finish(bFail ? EPawnActionResult::Failed : EPawnActionResult::Success);
}

void UPawnAction_Move::SetPath(FNavPathSharedPtr InPath)
{
	Path = InPath;
	Path->GetObserver(PathObserver);
	Path->SetObserver(FNavigationPath::FPathObserverDelegate::CreateUObject(this, &UPawnAction_Move::OnPathUpdated));
}

void UPawnAction_Move::OnPathUpdated(FNavigationPath* UpdatedPath)
{
	UE_VLOG(GetController(), LogPawnAction, Log, TEXT("%s> Path updated!"), *GetName());
	if (PathObserver.IsBound() && PathObserver.GetUObject() != this)
	{
		PathObserver.ExecuteIfBound(UpdatedPath);
	}

	if (bAbortChildActionOnPathChange && GetChildAction())
	{
		UE_VLOG(GetController(), LogPawnAction, Log, TEXT(">> aborting child action: %s"), *GetNameSafe(GetChildAction()));
		GetChildAction()->Abort(EAIForceParam::Force);
	}
}

bool UPawnAction_Move::CheckAlreadyAtGoal(class AAIController* Controller, const FVector& TestLocation, float Radius)
{
	const bool bAlreadyAtGoal = Controller->PathFollowingComponent->HasReached(TestLocation, Radius);
	if (bAlreadyAtGoal)
	{
		Controller->PathFollowingComponent->AbortMove(TEXT("Aborting move due to new move request finishing with AlreadyAtGoal"), FAIRequestID::AnyRequest, true, false, EPathFollowingMessage::OtherRequest);
		Controller->PathFollowingComponent->SetLastMoveAtGoal(true);
	}

	return bAlreadyAtGoal;
}

bool UPawnAction_Move::CheckAlreadyAtGoal(class AAIController* Controller, const AActor* TestGoal, float Radius)
{
	const bool bAlreadyAtGoal = Controller->PathFollowingComponent->HasReached(TestGoal, Radius);
	if (bAlreadyAtGoal)
	{
		Controller->PathFollowingComponent->AbortMove(TEXT("Aborting move due to new move request finishing with AlreadyAtGoal"), FAIRequestID::AnyRequest, true, false, EPathFollowingMessage::OtherRequest);
		Controller->PathFollowingComponent->SetLastMoveAtGoal(true);
	}

	return bAlreadyAtGoal;
}
