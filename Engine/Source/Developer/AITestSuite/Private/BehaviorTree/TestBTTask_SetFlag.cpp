// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AITestSuitePrivatePCH.h"
#include "BehaviorTree/TestBTTask_SetFlag.h"
#include "BehaviorTree/BlackboardComponent.h"

UTestBTTask_SetFlag::UTestBTTask_SetFlag(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "Log";
	TaskResult = EBTNodeResult::Succeeded;
	bValue = true;
}

EBTNodeResult::Type UTestBTTask_SetFlag::ExecuteTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory)
{
	OwnerComp->GetBlackboardComponent()->SetValueAsBool(TEXT("Bool1"), bValue);
	return TaskResult;
}
