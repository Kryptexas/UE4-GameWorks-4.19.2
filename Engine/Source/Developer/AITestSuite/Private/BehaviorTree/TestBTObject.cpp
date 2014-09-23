// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AITestSuitePrivatePCH.h"
#include "BehaviorTree/TestBTObject.h"

DEFINE_LOG_CATEGORY_STATIC(LogBehaviorTreeTest, Log, All);

void FTestTickHelper::Tick(float DeltaTime)
{
	if (Owner.IsValid())
	{
		Owner->TickMe(DeltaTime);
	}
}

TStatId FTestTickHelper::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FTestTickHelper, STATGROUP_Tickables);
}

TArray<int32> UTestBTObject::ExecutionLog;

UTestBTObject::UTestBTObject(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
}

void UTestBTObject::TickMe(float DeltaTime)
{
	if (BBComp)
	{
		BBComp->TickComponent(DeltaTime, ELevelTick::LEVELTICK_All, NULL);
	}

	if (BTComp)
	{
		BTComp->TickComponent(DeltaTime, ELevelTick::LEVELTICK_All, NULL);
	}
}

bool UTestBTObject::RunTest(UBehaviorTree* TreeOb)
{
	const TIndirectArray<FWorldContext>& Contexts = GEngine->GetWorldContexts();
	UWorld* MyWorld = NULL;
	for (int32 Idx = 0; Idx < Contexts.Num(); Idx++)
	{
		if (Contexts[Idx].World())
		{
			MyWorld = Contexts[Idx].World();
			break;
		}
	}

	if (MyWorld == NULL)
	{
		return false;
	}

	BTComp = NewObject<UBehaviorTreeComponent>(MyWorld);
	BBComp = NewObject<UBlackboardComponent>(MyWorld);

	BBComp->InitializeBlackboard(TreeOb->BlackboardAsset);
	BTComp->CacheBlackboardComponent(BBComp);

	BBComp->RegisterComponentWithWorld(MyWorld);
	BTComp->RegisterComponentWithWorld(MyWorld);

	ExecutionLog.Reset();
	BTComp->StartTree(TreeOb, EBTExecutionMode::SingleRun);
	
	TickHelper.Owner = this;
	return true;
}

bool UTestBTObject::IsRunning() const
{
	return BTComp && BTComp->IsRunning() && BTComp->GetRootTree();
}

void UTestBTObject::VerifyResults()
{
	const bool bMatch = (ExpectedResult == ExecutionLog);
	if (!bMatch)
	{
		FString Desc;
		for (int32 Idx = 0; Idx < ExecutionLog.Num(); Idx++)
		{
			Desc += TTypeToString<int32>::ToString(ExecutionLog[Idx]);
			if (Idx < (ExecutionLog.Num() - 1))
			{
				Desc += TEXT(", ");
			}
		}

		UE_LOG(LogBehaviorTreeTest, Error, TEXT("Results are not matching!\nExecution log: %s"), *Desc);
	}

	TickHelper.Owner.Reset();
}
