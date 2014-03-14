// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "BTDecorator_TimeLimit.generated.h"

UCLASS(HideCategories=(Condition))
class ENGINE_API UBTDecorator_TimeLimit : public UBTDecorator
{
	GENERATED_UCLASS_BODY()
		
	/** max allowed time for execution of underlying node */
	UPROPERTY(Category=Decorator, EditAnywhere)
	float TimeLimit;

	virtual void DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const OVERRIDE;
	virtual FString GetStaticDescription() const OVERRIDE;

protected:

	virtual void OnBecomeRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
	virtual void TickNode(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const OVERRIDE;
};
