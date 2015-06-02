// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AITestSuitePrivatePCH.h"
#include "GameplayTasksComponent.h"
#include "MockGameplayTasks.h"
//#include "MockAI_BT.h"
//#include "BehaviorTree/TestBTDecorator_CantExecute.h"
//#include "Actions/TestPawnAction_CallFunction.h"

#define LOCTEXT_NAMESPACE "AITestSuite_GameplayTasksTest"

typedef FAITest_SimpleComponentBasedTest<UGameplayTasksComponent> FAITest_GameplayTasksTest;


struct FAITest_GameplayTask_ComponentState : public FAITest_GameplayTasksTest
{
	void SetUp()
	{
		FAITest_GameplayTasksTest::SetUp();

		Test(TEXT("Initially UGameplayTasksComponent should not want to tick"), Component->GetShouldTick() == false);

		/*UWorld& World = GetWorld();
		Task = UMockTask_Log::CreateTask(*Component, Logger);

		Test(TEXT("Task should be \'uninitialized\' before Activate is called on it"), Task->GetState() == EGameplayTaskState::AwaitingActivation);

		Component->

			Task->ReadyForActivation();
		Test(TEXT("Task should be \'uninitialized\' before Activate is called on it"), Task->GetState() == EGameplayTaskState::Active);


		Test(TEXT("Task should be \'uninitialized\' before Activate is called on it"), Task->GetState() == EGameplayTaskState::Active);*/
	}
};
IMPLEMENT_AI_LATENT_TEST(FAITest_GameplayTask_ComponentState, "Engine.AI.Gameplay Tasks.Component\'s basic behavior")

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
struct FAITest_GameplayTask_ExternalCancelWithTick : public FAITest_GameplayTasksTest
{	
	UMockTask_Log* Task;

	void SetUp()
	{
		FAITest_GameplayTasksTest::SetUp();

		Logger.ExpectedValues.Add(ETestTaskMessage::Activate);
		Logger.ExpectedValues.Add(ETestTaskMessage::Tick);
		Logger.ExpectedValues.Add(ETestTaskMessage::ExternalCancel);
		Logger.ExpectedValues.Add(ETestTaskMessage::Ended);

		UWorld& World = GetWorld();
		Task = UMockTask_Log::CreateTask(*Component, Logger);
		Task->EnableTick();
		
		Test(TEXT("Task should be \'uninitialized\' before Activate is called on it"), Task->GetState() == EGameplayTaskState::AwaitingActivation);
		
		Task->ReadyForActivation();
		Test(TEXT("Task should be \'Active\' after basic call to ReadyForActivation"), Task->GetState() == EGameplayTaskState::Active);
		Test(TEXT("Component should want to tick in this scenario"), Component->GetShouldTick() == true);
	}

	bool Update()
	{
		TickComponent();

		Task->ExternalCancel();
		
		return true;
	}
};
IMPLEMENT_AI_LATENT_TEST(FAITest_GameplayTask_ExternalCancelWithTick, "Engine.AI.Gameplay Tasks.External Cancel with Tick")

//----------------------------------------------------------------------//
// In this test the task should get properly created, acticated and end 
// during update without any ticking
//----------------------------------------------------------------------//
struct FAITest_GameplayTask_SelfEnd : public FAITest_GameplayTasksTest
{
	UMockTask_Log* Task;

	void SetUp()
	{
		FAITest_GameplayTasksTest::SetUp();

		Logger.ExpectedValues.Add(ETestTaskMessage::Activate);
		Logger.ExpectedValues.Add(ETestTaskMessage::Ended);

		UWorld& World = GetWorld();
		Task = UMockTask_Log::CreateTask(*Component, Logger);
		Task->EnableTick();
		Task->ReadyForActivation();
	}

	bool Update()
	{
		Task->EndTask();

		return true;
	}
};
IMPLEMENT_AI_LATENT_TEST(FAITest_GameplayTask_SelfEnd, "Engine.AI.Gameplay Tasks.Self End")

#undef LOCTEXT_NAMESPACE