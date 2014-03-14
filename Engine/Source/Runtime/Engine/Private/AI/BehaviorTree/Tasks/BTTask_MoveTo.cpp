// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTTask_MoveTo::UBTTask_MoveTo(const class FPostConstructInitializeProperties& PCIP) 
	: Super(PCIP)
	, AcceptableRadius(50.f)
	, bAllowStrafe(false)
{
	NodeName = "Move To";

	// accept only actors and vectors
	BlackboardKey.AddObjectFilter(this, AActor::StaticClass());
	BlackboardKey.AddVectorFilter(this);
}

EBTNodeResult::Type UBTTask_MoveTo::ExecuteTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* MyBlackboard = OwnerComp->GetBlackboardComponent();
	FBTMoveToTaskMemory* MyMemory = (FBTMoveToTaskMemory*)NodeMemory;
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
				RequestResult = MyController->MoveToActor(TargetActor, AcceptableRadius, true, true, bAllowStrafe, FilterClass);
			}
			else
			{
				UE_VLOG(MyController, LogBehaviorTree, Warning, TEXT("UBTTask_MoveTo::ExecuteTask tried to go to actor while BB %s entry was empty"), *BlackboardKey.SelectedKeyName.ToString());
			}
		}
		else if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
		{
			const FVector TargetLocation = MyBlackboard->GetValueAsVector(BlackboardKey.SelectedKeyID);
			RequestResult = MyController->MoveToLocation(TargetLocation, AcceptableRadius, true, true, false, bAllowStrafe, FilterClass);
		}

		if (RequestResult == EPathFollowingRequestResult::RequestSuccessful)
		{
			const uint32 RequestID = MyController->GetCurrentMoveRequestID();

			MyMemory->MoveRequestID = RequestID;
			WaitForMessage(OwnerComp, UBrainComponent::AIMessage_MoveFinished, RequestID);
			WaitForMessage(OwnerComp, UBrainComponent::AIMessage_RepathFailed);

			NodeResult = EBTNodeResult::InProgress;
		}
		else if (RequestResult == EPathFollowingRequestResult::AlreadyAtGoal)
		{
			NodeResult = EBTNodeResult::Succeeded;
		}
	}

	return NodeResult;
}

EBTNodeResult::Type UBTTask_MoveTo::AbortTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	FBTMoveToTaskMemory* MyMemory = (FBTMoveToTaskMemory*)NodeMemory;
	AAIController* MyController = OwnerComp ? Cast<AAIController>(OwnerComp->GetOwner()) : NULL;

	if (MyController && MyController->PathFollowingComponent)
	{
		MyController->PathFollowingComponent->AbortMove(TEXT("BehaviorTree abort"), MyMemory->MoveRequestID);
	}

	return Super::AbortTask(OwnerComp, NodeMemory);
}

void UBTTask_MoveTo::OnMessage(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, FName Message, int32 SenderID, bool bSuccess) const
{
	// AIMessage_RepathFailed means task has failed
	bSuccess &= (Message != UBrainComponent::AIMessage_RepathFailed);
	Super::OnMessage(OwnerComp, NodeMemory, Message, SenderID, bSuccess);
}

FString UBTTask_MoveTo::GetStaticDescription() const
{
	FString KeyDesc("invalid");
	if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass() ||
		BlackboardKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		KeyDesc = BlackboardKey.SelectedKeyName.ToString();
	}

	return FString::Printf(TEXT("%s: %s"), *Super::GetStaticDescription(), *KeyDesc);
}

void UBTTask_MoveTo::DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);

	const UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
	FBTMoveToTaskMemory* MyMemory = (FBTMoveToTaskMemory*)NodeMemory;

	if (MyMemory->MoveRequestID && BlackboardComp)
	{
		FString KeyValue = BlackboardComp->DescribeKeyValue(BlackboardKey.SelectedKeyID, EBlackboardDescription::OnlyValue);
		Values.Add(FString::Printf(TEXT("move target: %s"), *KeyValue));
	}
}

uint16 UBTTask_MoveTo::GetInstanceMemorySize() const
{
	return sizeof(FBTMoveToTaskMemory);
}
