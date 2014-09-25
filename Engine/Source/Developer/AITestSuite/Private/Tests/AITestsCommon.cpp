// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AITestSuitePrivatePCH.h"
#include "EngineGlobals.h"
#include "Engine.h"

bool FAITestCommand_WaitSeconds::Update()
{
	float NewTime = FPlatformTime::Seconds();
	if (NewTime - StartTime >= Duration)
	{
		return true;
	}
	return false;
}

bool FAITestCommand_WaitOneTick::Update()
{
	if (bAlreadyRun == false)
	{
		bAlreadyRun = true;
		return true;
	}
	return false;
}

bool FAITestCommand_SetUpTest::Update()
{
	if (AITest)
	{
		AITest->SetUp();
	}
	return true;
}

bool FAITestCommand_PerformTest::Update()
{
	return AITest != nullptr && AITest->Update();
}

bool FAITestCommand_TearDownTest::Update()
{
	if (AITest)
	{
		AITest->TearDown();
		delete AITest;
		AITest = nullptr;
	}
	return true;
}

namespace FAITestHelpers
{
	UWorld* GetWorld()
	{
#if WITH_EDITOR
		if (GIsEditor)
		{
			return GWorld;
		}
#endif // WITH_EDITOR
		return GEngine->GetWorldContexts()[0].World();
	}
}

//----------------------------------------------------------------------//
// FAITest_SimpleBT
//----------------------------------------------------------------------//
FAITest_SimpleBT::FAITest_SimpleBT()
{
	AIBTUser = NewObject<UMockAI_BT>();
	AIBTUser->AddToRoot();
	BTAsset = &FBTBuilder::CreateBehaviorTree();

	UMockAI_BT::ExecutionLog.Reset();
}

FAITest_SimpleBT::~FAITest_SimpleBT()
{
	if (AIBTUser)
	{
		AIBTUser->RemoveFromRoot();
		AIBTUser = nullptr;
	}
}

void FAITest_SimpleBT::SetUp()
{
	if (AIBTUser)
	{
		AIBTUser->RunBT(BTAsset, EBTExecutionMode::SingleRun);
		AIBTUser->SetEnableTicking(true);
	}
}

bool FAITest_SimpleBT::Update()
{
	if (AIBTUser != NULL && AIBTUser->IsRunning())
	{
		return false;
	}

	VerifyResults();
	return true;
}

void FAITest_SimpleBT::VerifyResults()
{
	const bool bMatch = (ExpectedResult == UMockAI_BT::ExecutionLog);
	if (!bMatch)
	{
		FString Desc;
		for (int32 Idx = 0; Idx < UMockAI_BT::ExecutionLog.Num(); Idx++)
		{
			Desc += TTypeToString<int32>::ToString(UMockAI_BT::ExecutionLog[Idx]);
			if (Idx < (UMockAI_BT::ExecutionLog.Num() - 1))
			{
				Desc += TEXT(", ");
			}
		}

		UE_LOG(LogBehaviorTreeTest, Error, TEXT("Results are not matching!\nExecution log: %s"), *Desc);
	}
}