// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "Engine/Blueprint.h"
#include "Blueprint/AIAsyncTaskBlueprintProxy.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"

//----------------------------------------------------------------------//
// UAIAsyncTaskBlueprintProxy
//----------------------------------------------------------------------//

UAIAsyncTaskBlueprintProxy::UAIAsyncTaskBlueprintProxy(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UAIAsyncTaskBlueprintProxy::OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type MovementResult)
{
	if (RequestID.IsEquivalent(MoveRequestId) && AIController.IsValid())
	{
		if (!AIController->IsPendingKill() && AIController->ReceiveMoveCompleted.IsBound())
		{
			AIController->ReceiveMoveCompleted.RemoveDynamic(this, &UAIAsyncTaskBlueprintProxy::OnMoveCompleted);
		}

		if (MovementResult == EPathFollowingResult::Success)
		{
			OnSuccess.Broadcast(MovementResult);
		}
		else
		{
			OnFail.Broadcast(MovementResult);
		}
	}
}

void UAIAsyncTaskBlueprintProxy::OnNoPath()
{
	OnFail.Broadcast(EPathFollowingResult::Aborted);
}

void UAIAsyncTaskBlueprintProxy::BeginDestroy()
{
	if (AIController.IsValid() && !AIController->IsPendingKill() && AIController->ReceiveMoveCompleted.IsBound())
	{
		AIController->ReceiveMoveCompleted.RemoveDynamic(this, &UAIAsyncTaskBlueprintProxy::OnMoveCompleted);
	}

	Super::BeginDestroy();
}

//----------------------------------------------------------------------//
// UAIAsyncTaskBlueprintProxy
//----------------------------------------------------------------------//

UAIBlueprintHelperLibrary::UAIBlueprintHelperLibrary(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

class UAIAsyncTaskBlueprintProxy* UAIBlueprintHelperLibrary::CreateMoveToProxyObject(class UObject* WorldContextObject, APawn* Pawn, FVector Destination, AActor* TargetActor, float AcceptanceRadius, bool bStopOnOverlap)
{
	check(WorldContextObject);
	if (!Pawn)
	{
		return NULL;
	}
	UAIAsyncTaskBlueprintProxy* MyObj = NULL;
	AAIController* AIController = Cast<AAIController>(Pawn->GetController());
	if (AIController)
	{
		MyObj = NewObject<UAIAsyncTaskBlueprintProxy>();
		FNavPathSharedPtr Path = TargetActor ? AIController->FindPath(TargetActor, true) : AIController->FindPath(Destination, true);
		if (Path.IsValid())
		{
			MyObj->AIController = AIController;
			MyObj->AIController->ReceiveMoveCompleted.AddDynamic(MyObj, &UAIAsyncTaskBlueprintProxy::OnMoveCompleted);
			MyObj->MoveRequestId = MyObj->AIController->RequestMove(Path, TargetActor, AcceptanceRadius, bStopOnOverlap);
		}
		else
		{
			UWorld* World = GEngine->GetWorldFromContextObject( WorldContextObject );
			World->GetTimerManager().SetTimer(MyObj, &UAIAsyncTaskBlueprintProxy::OnNoPath, 0.1, false);
		}
	}
	return MyObj;
}

void UAIBlueprintHelperLibrary::SendAIMessage(APawn* Target, FName Message, UObject* MessageSource, bool bSuccess)
{
	FAIMessage::Send(Target, FAIMessage(Message, MessageSource, bSuccess));
}

APawn* UAIBlueprintHelperLibrary::SpawnAIFromClass(class UObject* WorldContextObject, TSubclassOf<APawn> PawnClass, class UBehaviorTree* BehaviorTree, FVector Location, FRotator Rotation, bool bNoCollisionFail)
{
	APawn* NewPawn = NULL;

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
	if (World && *PawnClass)
	{
		FActorSpawnParameters ActorSpawnParams;
		ActorSpawnParams.bNoCollisionFail = bNoCollisionFail;
		NewPawn = World->SpawnActor<APawn>(*PawnClass, Location, Rotation, ActorSpawnParams);

		if (NewPawn != NULL)
		{
			if (NewPawn->Controller == NULL)
			{	// NOTE: SpawnDefaultController ALSO calls Possess() to possess the pawn (if a controller is successfully spawned).
				NewPawn->SpawnDefaultController();
			}

			if (BehaviorTree != NULL)
			{
				AAIController* AIController = Cast<AAIController>(NewPawn->Controller);

				if (AIController != NULL)
				{
					AIController->RunBehaviorTree(BehaviorTree);
				}
			}
		}
	}

	return NewPawn;
}

APawn* UAIBlueprintHelperLibrary::SpawnAI(class UObject* WorldContextObject, UBlueprint* Pawn, class UBehaviorTree* BehaviorTree, FVector Location, FRotator Rotation, bool bNoCollisionFail)
{
	APawn* NewPawn = NULL;

	const bool bGoodBPGeneratedClass = Pawn && Pawn->GeneratedClass && Pawn->GeneratedClass->IsChildOf(APawn::StaticClass());
	if (bGoodBPGeneratedClass)
	{
		NewPawn = SpawnAIFromClass(WorldContextObject, *(Pawn->GeneratedClass), BehaviorTree, Location, Rotation, bNoCollisionFail);
	}

	return NewPawn;
}

UBlackboardComponent* UAIBlueprintHelperLibrary::GetBlackboard(AActor* Target)
{
	UBlackboardComponent* BlackboardComp = NULL;

	APawn* TargetPawn = Cast<APawn>(Target);
	if (TargetPawn && TargetPawn->GetController())
	{
		BlackboardComp = TargetPawn->GetController()->FindComponentByClass<UBlackboardComponent>();
	}

	if (BlackboardComp == NULL && Target)
	{
		BlackboardComp = Target->FindComponentByClass<UBlackboardComponent>();
	}

	return BlackboardComp;
}

void UAIBlueprintHelperLibrary::LockAIResourcesWithAnimation(class UAnimInstance* AnimInstance, bool bLockMovement, bool LockAILogic)
{
	if (AnimInstance == NULL)
	{
		return;
	}

	APawn* PawnOwner = AnimInstance->TryGetPawnOwner();
	if (PawnOwner)
	{
		AAIController* OwningAI = Cast<AAIController>(PawnOwner->Controller);
		if (OwningAI)
		{
			if (bLockMovement && OwningAI->PathFollowingComponent)
			{
				OwningAI->PathFollowingComponent->LockResource(EAILockSource::Animation);
			}
			if (LockAILogic && OwningAI->BrainComponent)
			{
				OwningAI->BrainComponent->LockResource(EAILockSource::Animation);
			}
		}
	}
}

void UAIBlueprintHelperLibrary::UnlockAIResourcesWithAnimation(class UAnimInstance* AnimInstance, bool bUnlockMovement, bool UnlockAILogic)
{
	if (AnimInstance == NULL)
	{
		return;
	}

	APawn* PawnOwner = AnimInstance->TryGetPawnOwner();
	if (PawnOwner)
	{
		AAIController* OwningAI = Cast<AAIController>(PawnOwner->Controller);
		if (OwningAI)
		{
			if (bUnlockMovement && OwningAI->PathFollowingComponent)
			{
				OwningAI->PathFollowingComponent->ClearResourceLock(EAILockSource::Animation);
			}
			if (UnlockAILogic && OwningAI->BrainComponent)
			{
				OwningAI->BrainComponent->ClearResourceLock(EAILockSource::Animation);
			}
		}
	}
}