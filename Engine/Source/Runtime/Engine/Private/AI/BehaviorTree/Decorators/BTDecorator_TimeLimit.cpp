// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTDecorator_TimeLimit::UBTDecorator_TimeLimit(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "TimeLimit";
	TimeLimit = 5.0f;
	bNotifyBecomeRelevant = true;
	bNotifyTick = true;
	bTickIntervals = true;

	// time limit always abort current branch
	bAllowAbortLowerPri = false;
	bAllowAbortNone = false;
	FlowAbortMode = EBTFlowAbortMode::Self;
}

void UBTDecorator_TimeLimit::OnBecomeRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	FBTAuxiliaryMemory* DecoratorMemory = GetAuxNodeMemory(NodeMemory);
	DecoratorMemory->NextTickRemainingTime = TimeLimit;
}

void UBTDecorator_TimeLimit::TickNode(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const
{
	OwnerComp->RequestExecution(this);
}

FString UBTDecorator_TimeLimit::GetStaticDescription() const
{
	// basic info: result after time
	return FString::Printf(TEXT("%s: %s after %.1fs"), *Super::GetStaticDescription(),
		*UBehaviorTreeTypes::DescribeNodeResult(EBTNodeResult::Failed), TimeLimit);
}

void UBTDecorator_TimeLimit::DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);

	FBTAuxiliaryMemory* DecoratorMemory = GetAuxNodeMemory(NodeMemory);
	if (OwnerComp && OwnerComp->GetWorld())
	{
		const float TimeLeft = DecoratorMemory->NextTickRemainingTime > 0 ? DecoratorMemory->NextTickRemainingTime : 0;
		Values.Add(FString::Printf(TEXT("%s in %ss"),
			*UBehaviorTreeTypes::DescribeNodeResult(EBTNodeResult::Failed),
			*FString::SanitizeFloat(TimeLeft)));
	}
}
