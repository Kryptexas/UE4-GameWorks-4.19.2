// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BTTask_RunBehavior.generated.h"

UCLASS(MinimalAPI)
class UBTTask_RunBehavior : public UBTTaskNode
{
	GENERATED_UCLASS_BODY()

	/** behavior to run */
	UPROPERTY(Category=Node, EditAnywhere)
	class UBehaviorTree* BehaviorAsset;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
	virtual FString GetStaticDescription() const OVERRIDE;
};
