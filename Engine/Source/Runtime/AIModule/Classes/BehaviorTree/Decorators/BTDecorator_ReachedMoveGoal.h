// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_ReachedMoveGoal.generated.h"

UCLASS()
class AIMODULE_API UBTDecorator_ReachedMoveGoal : public UBTDecorator
{
	GENERATED_UCLASS_BODY()

	virtual bool CalculateRawConditionValue(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
};
