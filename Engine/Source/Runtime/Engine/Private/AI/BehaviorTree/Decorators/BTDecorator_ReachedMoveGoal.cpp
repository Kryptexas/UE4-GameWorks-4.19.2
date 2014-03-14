// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTDecorator_ReachedMoveGoal::UBTDecorator_ReachedMoveGoal(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "Reached move goal";

	// can't abort, it's not observing anything
	bAllowAbortLowerPri = false;
	bAllowAbortNone = false;
	bAllowAbortChildNodes = false;
	FlowAbortMode = EBTFlowAbortMode::None;
}

bool UBTDecorator_ReachedMoveGoal::CalculateRawConditionValue(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const 
{
	AAIController* AIOwner = Cast<AAIController>(OwnerComp->GetOwner());
	const bool bReachedGoal = AIOwner && AIOwner->PathFollowingComponent && AIOwner->PathFollowingComponent->DidMoveReachGoal();
	return bReachedGoal;
}
