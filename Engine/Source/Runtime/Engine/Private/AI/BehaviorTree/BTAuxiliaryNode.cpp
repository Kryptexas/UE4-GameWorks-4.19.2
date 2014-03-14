// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTAuxiliaryNode::UBTAuxiliaryNode(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	bNotifyBecomeRelevant = false;
	bNotifyCeaseRelevant = false;
	bNotifyTick = false;
	bTickIntervals = false;
}

void UBTAuxiliaryNode::ConditionalOnBecomeRelevant(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	if (bNotifyBecomeRelevant)
	{
		OnBecomeRelevant(OwnerComp, NodeMemory);
	}
}

void UBTAuxiliaryNode::ConditionalOnCeaseRelevant(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	if (bNotifyCeaseRelevant)
	{
		OnCeaseRelevant(OwnerComp, NodeMemory);
	}
}

void UBTAuxiliaryNode::ConditionalTickNode(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const
{
	if (bNotifyTick)
	{
		if (bTickIntervals)
		{
			FBTAuxiliaryMemory* AuxMemory = GetAuxNodeMemory(NodeMemory);
			AuxMemory->NextTickRemainingTime -= DeltaSeconds;
			if (AuxMemory->NextTickRemainingTime > 0.0f)
			{
				return;
			}	
		}

		TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	}
}

void UBTAuxiliaryNode::SetNextTickTime(uint8* NodeMemory, float RemainingTime) const
{
	if (bTickIntervals)
	{
		FBTAuxiliaryMemory* AuxMemory = GetAuxNodeMemory(NodeMemory);
		AuxMemory->NextTickRemainingTime = RemainingTime;
	}
}

void UBTAuxiliaryNode::DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);

	if (Verbosity == EBTDescriptionVerbosity::Detailed && bTickIntervals)
	{
		FBTAuxiliaryMemory* AuxMemory = GetAuxNodeMemory(NodeMemory);
		Values.Add(FString::Printf(TEXT("next tick: %ss"), *FString::SanitizeFloat(AuxMemory->NextTickRemainingTime)));
	}
}

void UBTAuxiliaryNode::OnBecomeRelevant(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	// empty in base class
}

void UBTAuxiliaryNode::OnCeaseRelevant(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	// empty in base class
}

void UBTAuxiliaryNode::TickNode(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const
{
	// empty in base class
}

uint16 UBTAuxiliaryNode::GetInstanceAuxMemorySize() const
{
	return bTickIntervals ? sizeof(FBTAuxiliaryMemory) : 0;
}

FBTAuxiliaryMemory* UBTAuxiliaryNode::GetAuxNodeMemory(uint8* NodeMemory) const
{
	if (bTickIntervals)
	{
		const int32 AlignedAuxMemory = ((sizeof(FBTAuxiliaryMemory) + 3) & ~3);
		return (FBTAuxiliaryMemory*)(NodeMemory - AlignedAuxMemory);
	}

	return NULL;
}
