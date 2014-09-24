// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once 
#include "BehaviorTree/BTDecorator.h"
#include "TestBTDecorator_CantExecute.generated.h"

UCLASS()
class UTestBTDecorator_CantExecute : public UBTDecorator
{
	GENERATED_UCLASS_BODY()

	virtual bool CalculateRawConditionValue(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const override;
};
