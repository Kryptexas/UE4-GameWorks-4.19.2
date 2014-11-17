// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/Decorators/BTDecorator_ConditionalLoop.h"

UBTDecorator_ConditionalLoop::UBTDecorator_ConditionalLoop(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "Conditional Loop";
	bNotifyDeactivation = true;

	bAllowAbortNone = false;
	bAllowAbortLowerPri = false;
	bAllowAbortChildNodes = false;
}

bool UBTDecorator_ConditionalLoop::CalculateRawConditionValue(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	// always allows execution
	return true;
}

void UBTDecorator_ConditionalLoop::OnBlackboardChange(const class UBlackboardComponent* Blackboard, FBlackboard::FKey ChangedKeyID)
{
	// empty, don't react to blackboard value changes
}

void UBTDecorator_ConditionalLoop::OnNodeDeactivation(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type NodeResult)
{
	if (NodeResult != EBTNodeResult::Aborted && SearchData.OwnerComp)
	{
		const bool bEvalResult = EvaluateOnBlackboard(SearchData.OwnerComp->GetBlackboardComponent());
		UE_VLOG(SearchData.OwnerComp->GetOwner(), LogBehaviorTree, Verbose, TEXT("Loop condition: %s -> %s"),
			bEvalResult ? TEXT("true") : TEXT("false"), (bEvalResult != IsInversed()) ? TEXT("run again!") : TEXT("break"));

		if (bEvalResult != IsInversed())
		{
			GetParentNode()->SetChildOverride(SearchData, GetChildIndex());
		}
	}
}

#if WITH_EDITOR

FName UBTDecorator_ConditionalLoop::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Decorator.Loop.Icon");
}

#endif	// WITH_EDITOR
