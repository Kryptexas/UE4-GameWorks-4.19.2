// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTTask_Wait::UBTTask_Wait(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "Wait";
	WaitTime = 5.0f;
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_Wait::ExecuteTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	FBTWaitTaskMemory* MyMemory = (FBTWaitTaskMemory*)NodeMemory;
	MyMemory->RemainingWaitTime = WaitTime;
	
	return EBTNodeResult::InProgress;
}

void UBTTask_Wait::TickTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const
{
	FBTWaitTaskMemory* MyMemory = (FBTWaitTaskMemory*)NodeMemory;
	MyMemory->RemainingWaitTime -= DeltaSeconds;

	if (MyMemory->RemainingWaitTime <= 0.0f)
	{
		// continue execution from this node
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

FString UBTTask_Wait::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %.1fs"), *Super::GetStaticDescription(), WaitTime);
}

void UBTTask_Wait::DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);

	FBTWaitTaskMemory* MyMemory = (FBTWaitTaskMemory*)NodeMemory;
	if (MyMemory->RemainingWaitTime)
	{
		Values.Add(FString::Printf(TEXT("remaining: %ss"), *FString::SanitizeFloat(MyMemory->RemainingWaitTime)));
	}
}

uint16 UBTTask_Wait::GetInstanceMemorySize() const
{
	return sizeof(FBTWaitTaskMemory);
}
