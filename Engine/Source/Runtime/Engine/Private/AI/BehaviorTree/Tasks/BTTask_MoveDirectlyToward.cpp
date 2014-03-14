// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTTask_MoveDirectlyToward::UBTTask_MoveDirectlyToward(const class FPostConstructInitializeProperties& PCIP) 
	: Super(PCIP)
	, AcceptableRadius(50.f)
	, bProjectVectorGoalToNavigation(true)
	, bAllowStrafe(true)
{
	NodeName = "MoveDirectlyToward";

	// accept only actors and vectors
	BlackboardKey.AddObjectFilter(this, AActor::StaticClass());
	BlackboardKey.AddVectorFilter(this);
}

EBTNodeResult::Type UBTTask_MoveDirectlyToward::ExecuteTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* MyBlackboard = OwnerComp->GetBlackboardComponent();
	FBTMoveDirectlyTowardMemory* MyMemory = (FBTMoveDirectlyTowardMemory*)NodeMemory;
	AAIController* MyController = OwnerComp ? Cast<AAIController>(OwnerComp->GetOwner()) : NULL;
	EBTNodeResult::Type NodeResult = EBTNodeResult::Failed;

	if (MyController && MyBlackboard)
	{
		EPathFollowingRequestResult::Type RequestResult = EPathFollowingRequestResult::Failed;

		if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
		{
			UObject* KeyValue = MyBlackboard->GetValueAsObject(BlackboardKey.SelectedKeyID);
			AActor* TargetActor = Cast<AActor>(KeyValue);
			if (TargetActor)
			{
				RequestResult = bForceMoveToLocation ?
					MyController->MoveToLocation(TargetActor->GetActorLocation(), AcceptableRadius, /*bStopOnOverlap=*/true, /*bUsePathfinding=*/false, /*bProjectDestinationToNavigation=*/bProjectVectorGoalToNavigation, bAllowStrafe) :
					MyController->MoveToActor(TargetActor, AcceptableRadius, /*bStopOnOverlap=*/true, /*bUsePathfinding=*/false, bAllowStrafe);
			}
		}
		else if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
		{
			const FVector TargetLocation = MyBlackboard->GetValueAsVector(BlackboardKey.SelectedKeyID);
			RequestResult = MyController->MoveToLocation(TargetLocation, AcceptableRadius, /*bStopOnOverlap=*/true, /*bUsePathfinding=*/false, /*bProjectDestinationToNavigation=*/bProjectVectorGoalToNavigation, bAllowStrafe);
		}

		if (RequestResult == EPathFollowingRequestResult::RequestSuccessful)
		{
			const uint32 RequestID = MyController->GetCurrentMoveRequestID();

			MyMemory->MoveRequestID = RequestID;
			WaitForMessage(OwnerComp, UBrainComponent::AIMessage_MoveFinished, RequestID);

			NodeResult = EBTNodeResult::InProgress;
		}
		else if (RequestResult == EPathFollowingRequestResult::AlreadyAtGoal)
		{
			NodeResult = EBTNodeResult::Succeeded;
		}
	}

	return NodeResult;
}

EBTNodeResult::Type UBTTask_MoveDirectlyToward::AbortTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	FBTMoveDirectlyTowardMemory* MyMemory = (FBTMoveDirectlyTowardMemory*)NodeMemory;
	AAIController* MyController = OwnerComp ? Cast<AAIController>(OwnerComp->GetOwner()) : NULL;

	if (MyController && MyController->PathFollowingComponent)
	{
		MyController->PathFollowingComponent->AbortMove(TEXT("BehaviorTree abort"), MyMemory->MoveRequestID);
	}

	return Super::AbortTask(OwnerComp, NodeMemory);
}

FString UBTTask_MoveDirectlyToward::GetStaticDescription() const
{
	FString KeyDesc("invalid");
	if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass() ||
		BlackboardKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		KeyDesc = BlackboardKey.SelectedKeyName.ToString();
	}

	return FString::Printf(TEXT("%s: %s"), *Super::GetStaticDescription(), *KeyDesc);
}

void UBTTask_MoveDirectlyToward::DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);

	const UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
	FBTMoveDirectlyTowardMemory* MyMemory = (FBTMoveDirectlyTowardMemory*)NodeMemory;

	if (MyMemory->MoveRequestID && BlackboardComp)
	{
		FString KeyValue = BlackboardComp->DescribeKeyValue(BlackboardKey.SelectedKeyID, EBlackboardDescription::OnlyValue);
		Values.Add(FString::Printf(TEXT("move target: %s"), *KeyValue));
	}
}

uint16 UBTTask_MoveDirectlyToward::GetInstanceMemorySize() const
{
	return sizeof(FBTMoveDirectlyTowardMemory);
}
