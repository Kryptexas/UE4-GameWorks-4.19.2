// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "BehaviorTree/BTService.h"

UBTService::UBTService(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	bNotifyBecomeRelevant = true;
	bNotifyTick = true;
	bTickIntervals = true;

	Interval = 0.5f;
	RandomDeviation = 0.1f;
}

void UBTService::TickNode(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	const float NextTickTime = FMath::FRandRange(FMath::Max(0.0f, Interval - RandomDeviation), (Interval + RandomDeviation));
	SetNextTickTime(NodeMemory, NextTickTime);
}

void UBTService::OnBecomeRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory)
{
	// force it, without checking delays in ConditionalTickNode
	TickNode(OwnerComp, NodeMemory, 0.0f);
}

FString UBTService::GetStaticDescription() const
{
	FString IntervalDesc = (RandomDeviation > 0.0f) ?
		FString::Printf(TEXT("%.2fs..%.2fs"), FMath::Max(0.0f, Interval - RandomDeviation), (Interval + RandomDeviation)) :
		FString::Printf(TEXT("%.2fs"), Interval);

	return FString::Printf(TEXT("%s: tick every %s"), *UBehaviorTreeTypes::GetShortTypeName(this), *IntervalDesc);
}

#if WITH_EDITOR

FName UBTService::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Service.Icon");
}

#endif // WITH_EDITOR
