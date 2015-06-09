// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "GameplayTasksComponent.h"
#include "Tasks/AITask_MoveTo.h"

UAITask_MoveTo::UAITask_MoveTo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsPausable = true;
}

UAITask_MoveTo* UAITask_MoveTo::PawnMoveTo(APawn* Pawn, FVector Destination, AActor* TargetActor, float AcceptanceRadius, bool bStopOnOverlap, bool bAcceptPartialPath)
{
	return nullptr;
}

UAITask_MoveTo* UAITask_MoveTo::AIMoveTo(AAIController* Controller, FVector TargetLocation, AActor* TargetActor, float AcceptanceRadius, bool bStopOnOverlap, bool bAcceptPartialPath)
{
	if (Controller == nullptr)
	{
		return nullptr; 
	}

	UAITask_MoveTo* MyTask = NewTask<UAITask_MoveTo>(*static_cast<IGameplayTaskOwnerInterface*>(Controller));
	if (MyTask)
	{
		MyTask->OwnerController = Controller;

		if (TargetActor != nullptr)
		{
			MyTask->MoveRequest = FAIMoveRequest(TargetActor);
		}
		else
		{
			MyTask->MoveRequest = FAIMoveRequest(TargetLocation);
		}

		MyTask->MoveRequest.SetAllowPartialPath(bAcceptPartialPath)
			.SetAcceptanceRadius(AcceptanceRadius)
			.SetStopOnOverlap(bStopOnOverlap);
		//MyTask->MoveReq.SetUsePathfinding(bUsePathfinding);
		//MyTask->MoveRequest.SetNavigationFilter(FilterClass);
		//MyTask->MoveRequest.SetCanStrafe(bCanStrafe);

		MyTask->RequireResource(UAIResource_Movement::StaticClass());
	}

	return MyTask;
}

void UAITask_MoveTo::HandleMoveFinished(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	if (RequestID == MoveRequestID)
	{
		if (Result == EPathFollowingResult::Success)
		{
			EndTask();
			OnMoveFinished.Broadcast(Result);
		}
		else
		{
			Pause();
		}
	}
	else
	{
		// @todo report issue to the owner component
		UE_VLOG(GetGameplayTasksComponent(), LogGameplayTasks, Warning, TEXT("%s got movement-finished-notification but RequestID doesn't match. Possible task leak"), *GetName());
	}
}

void UAITask_MoveTo::Activate()
{
	const EPathFollowingRequestResult::Type RequestResult = OwnerController->MoveTo(MoveRequest);

	switch (RequestResult)
	{
	case EPathFollowingRequestResult::Failed:
		{
			EndTask();
			OnRequestFailed.Broadcast();
		}
		break;
	case EPathFollowingRequestResult::AlreadyAtGoal:
		{
			EndTask();
			OnMoveFinished.Broadcast(EPathFollowingResult::Success);
		}
		break;
	case EPathFollowingRequestResult::RequestSuccessful:
		if (OwnerController->GetPathFollowingComponent())
		{
			MoveRequestID = OwnerController->GetPathFollowingComponent()->GetCurrentRequestId();
			OwnerController->GetPathFollowingComponent()->OnMoveFinished.AddUObject(this, &UAITask_MoveTo::HandleMoveFinished);
		}
		break;
	default:
		checkNoEntry();
		break;
	}
}

void UAITask_MoveTo::Pause()
{
	if (OwnerController)
	{
		OwnerController->GetPathFollowingComponent()->OnMoveFinished.RemoveAll(this);
	}
	Super::Pause();
}

void UAITask_MoveTo::Resume()
{
	Activate();
	Super::Resume();
}
