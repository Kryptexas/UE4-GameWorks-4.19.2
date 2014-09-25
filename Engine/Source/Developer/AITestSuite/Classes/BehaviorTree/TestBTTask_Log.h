// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "BehaviorTree/BTTaskNode.h"
#include "TestBTTask_Log.generated.h"

struct FBTLogTaskMemory
{
	/** time left */
	float RemainingWaitTime;
};

UCLASS(meta=(HiddenNode))
class UTestBTTask_Log : public UBTTaskNode
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	int32 LogIndex;

	UPROPERTY()
	int32 LogFinished;

	UPROPERTY()
	float ExecutionTime;

	UPROPERTY()
	TEnumAsByte<EBTNodeResult::Type> LogResult;

	virtual EBTNodeResult::Type ExecuteTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override;

	void LogExecution(class UBehaviorTreeComponent* OwnerComp, int32 LogNumber);

protected:
	virtual void TickTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
