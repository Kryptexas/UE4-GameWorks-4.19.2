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

	virtual EBTNodeResult::Type ExecuteTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const override;
	virtual FString GetStaticDescription() const override;

#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
#endif // WITH_EDITOR

protected:

	virtual void TickTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
