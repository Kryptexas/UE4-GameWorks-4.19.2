// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once 
#include "BehaviorTree/BTDecorator.h"
#include "TestBTDecorator_CantExecute.generated.h"

UCLASS(meta=(HiddenNode))
class UTestBTDecorator_CantExecute : public UBTDecorator
{
	GENERATED_BODY()
public:
	UTestBTDecorator_CantExecute(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
