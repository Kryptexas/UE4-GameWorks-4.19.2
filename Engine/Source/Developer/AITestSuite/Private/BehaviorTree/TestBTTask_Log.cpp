// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AITestSuitePrivatePCH.h"
#include "BehaviorTree/TestBTTask_Log.h"
#include "MockAI_BT.h"

UTestBTTask_Log::UTestBTTask_Log(const FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "Log";
	ExecutionTicks = 0;
	LogIndex = 0;
	LogFinished = -1;
	LogResult = EBTNodeResult::Succeeded;

	bNotifyTick = true;
}

EBTNodeResult::Type UTestBTTask_Log::ExecuteTask(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory)
{
	FBTLogTaskMemory* MyMemory = (FBTLogTaskMemory*)NodeMemory;
	MyMemory->EndFrameIdx = ExecutionTicks + GFrameCounter;

	LogExecution(OwnerComp, LogIndex);
	if (ExecutionTicks == 0)
	{
		return LogResult;
	}

	return EBTNodeResult::InProgress;
}

void UTestBTTask_Log::TickTask(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FBTLogTaskMemory* MyMemory = (FBTLogTaskMemory*)NodeMemory;

	if (GFrameCounter >= MyMemory->EndFrameIdx)
	{
		LogExecution(OwnerComp, LogFinished);
		FinishLatentTask(OwnerComp, LogResult);
	}
}

uint16 UTestBTTask_Log::GetInstanceMemorySize() const
{
	return sizeof(FBTLogTaskMemory);
}

void UTestBTTask_Log::LogExecution(UBehaviorTreeComponent* OwnerComp, int32 LogNumber)
{
	if (LogNumber >= 0)
	{
		UMockAI_BT::ExecutionLog.Add(LogNumber);
	}
}
