// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BTTask_MakeNoise.generated.h"

UCLASS()
class UBTTask_MakeNoise : public UBTTaskNode
{
	GENERATED_UCLASS_BODY()

	/** Loudnes of generated noise */
	UPROPERTY(Category=Node, EditAnywhere, meta=(ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1"))
	float Loudnes;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
};
