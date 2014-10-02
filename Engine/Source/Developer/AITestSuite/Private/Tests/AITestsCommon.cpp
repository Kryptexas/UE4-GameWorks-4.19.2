// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AITestSuitePrivatePCH.h"

namespace FAITestHelpers
{
	uint64 UpdatesCounter = 0;
	
	void UpdateFrameCounter()
	{
		static uint64 PreviousFramesCounter = GFrameCounter;
		if (PreviousFramesCounter != GFrameCounter)
		{
			++UpdatesCounter;
			PreviousFramesCounter = GFrameCounter;
		}
	}

	uint64 FramesCounter()
	{
		return UpdatesCounter;
	}
}

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
// FAITestBase
//----------------------------------------------------------------------//
FAITestBase::~FAITestBase()
{
	for (auto AutoDestroyedObject : SpawnedObjects)
	{
		AutoDestroyedObject->RemoveFromRoot();
	}
	SpawnedObjects.Reset();
}

void FAITestBase::Test(const FString& Description, bool bValue)
{
	if (TestRunner)
	{
		TestRunner->TestTrue(Description, bValue);
	}
#if ENSURE_FAILED_TESTS
	ensure(bValue);
#endif // ENSURE_FAILED_TESTS
}

//----------------------------------------------------------------------//
// FAITest_SimpleBT
//----------------------------------------------------------------------//
FAITest_SimpleBT::FAITest_SimpleBT()
{
	AIBTUser = NewAutoDestroyObject<UMockAI_BT>();
	BTAsset = &FBTBuilder::CreateBehaviorTree();

	UMockAI_BT::ExecutionLog.Reset();
}

void FAITest_SimpleBT::SetUp()
{
	if (AIBTUser && BTAsset)
	{
		AIBTUser->RunBT(*BTAsset, EBTExecutionMode::SingleRun);
		AIBTUser->SetEnableTicking(true);
	}
}

bool FAITest_SimpleBT::Update()
{
	FAITestHelpers::UpdateFrameCounter();

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

		for (int32 Idx = 0; Idx < ExpectedResult.Num(); Idx++)
		{
			Desc += TTypeToString<int32>::ToString(ExpectedResult[Idx]);
			if (Idx < (ExpectedResult.Num() - 1))
			{
				Desc += TEXT(", ");
			}
		}

		UE_LOG(LogBehaviorTreeTest, Error, TEXT("Test scenario failed to produce expected results!\nExecution log: %s\nExpected values: %s"), *Desc);
	}
}

//----------------------------------------------------------------------//
// FAITest_SimpleActionsTest
//----------------------------------------------------------------------//
FAITest_SimpleActionsTest::FAITest_SimpleActionsTest()
{
	ActionsComponent = NewAutoDestroyObject<UPawnActionsComponent>();
}

FAITest_SimpleActionsTest::~FAITest_SimpleActionsTest()
{
	Test(TEXT("Not all expected values has been logged"), Logger.ExpectedValues.Num() == 0 || Logger.ExpectedValues.Num() == Logger.LoggedValues.Num());
}

void FAITest_SimpleActionsTest::SetUp()
{
	UWorld* World = FAITestHelpers::GetWorld();
	ActionsComponent->RegisterComponentWithWorld(World);	
}
