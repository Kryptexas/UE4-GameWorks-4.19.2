// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "BTTask_Wait.generated.h"

struct FBTWaitTaskMemory
{
	/** time left */
	float RemainingWaitTime;
};

UCLASS()
class UBTTask_Wait : public UBTTaskNode
{
	GENERATED_UCLASS_BODY()

	/** wait time in seconds */
	UPROPERTY(Category=Node, EditAnywhere)
	float WaitTime;

	virtual EBTNodeResult::Type ExecuteTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
	virtual uint16 GetInstanceMemorySize() const OVERRIDE;
	virtual void DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const OVERRIDE;
	virtual FString GetStaticDescription() const OVERRIDE;

protected:

	virtual void TickTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const OVERRIDE;
};
