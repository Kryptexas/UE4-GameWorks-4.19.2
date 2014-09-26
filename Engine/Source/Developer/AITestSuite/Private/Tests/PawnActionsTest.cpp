// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AITestSuitePrivatePCH.h"
#include "MockAI_BT.h"
#include "BehaviorTree/TestBTDecorator_CantExecute.h"

#define LOCTEXT_NAMESPACE "AITestSuite_PawnActionTest"

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
struct FAITest_PawnActions_Push : public FAITest_SimpleActionsTest
{	
	UTestPawnAction_Log* Action;

	void SetUp()
	{
		FAITest_SimpleActionsTest::SetUp();

		UWorld& World = GetWorld();
		Action = UTestPawnAction_Log::CreateAction(World, Logger);
		ActionsComponent->PushAction(Action, EAIRequestPriority::Logic);

		Test(TEXT("No action should be active at this point"), ActionsComponent->GetCurrentAction() == nullptr);
	}

	bool Update()
	{
		ActionsComponent->TickComponent(FAITestHelpers::TickInterval, ELevelTick::LEVELTICK_All, nullptr);
		Test(TEXT("After one tick created action should be the active one"), ActionsComponent->GetCurrentAction() == Action);
		Test(TEXT("After one tick created action should have been started"), Logger.LoggedValues.Top() == ETestPawnActionMessage::Started);
		return true;
	}
};
IMPLEMENT_AI_TEST(FAITest_PawnActions_Push, "Engine.AI.Pawn Actions.Pushing Single Action")

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
struct FAITest_PawnActions_PushingSameActionWithDelay : public FAITest_SimpleActionsTest
{
	UTestPawnAction_Log* Action;

	void SetUp()
	{
		FAITest_SimpleActionsTest::SetUp();

		UWorld& World = GetWorld();
		Action = UTestPawnAction_Log::CreateAction(World, Logger);
		ActionsComponent->PushAction(Action, EAIRequestPriority::Logic);
	}

	bool Update()
	{
		ActionsComponent->TickComponent(FAITestHelpers::TickInterval, ELevelTick::LEVELTICK_All, nullptr);

		Test(TEXT("Addind an action for a second time should fail"), ActionsComponent->PushAction(Action, EAIRequestPriority::Logic) == false);
		Test(TEXT("Addind an action for a second time, but with different priority, should fail"), ActionsComponent->PushAction(Action, EAIRequestPriority::Ultimate) == false);
		
		return true;
	}
};
IMPLEMENT_AI_TEST(FAITest_PawnActions_PushingSameActionWithDelay, "Engine.AI.Pawn Actions.Pusihng action that has already been pushed should fail")


//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
struct FAITest_PawnActions_Pause : public FAITest_SimpleActionsTest
{
	UTestPawnAction_Log* Action;

	void SetUp()
	{
		FAITest_SimpleActionsTest::SetUp();

		Logger.ExpectedValues.Add(ETestPawnActionMessage::Started);
		Logger.ExpectedValues.Add(ETestPawnActionMessage::Paused);
		Logger.ExpectedValues.Add(ETestPawnActionMessage::Started);

		UWorld& World = GetWorld();
		Action = UTestPawnAction_Log::CreateAction(World, Logger);
		ActionsComponent->PushAction(Action, EAIRequestPriority::Logic);
	}
		
	bool Update()
	{
		UWorld& World = GetWorld();

		ActionsComponent->TickComponent(FAITestHelpers::TickInterval, ELevelTick::LEVELTICK_All, nullptr);
		UTestPawnAction_Log* AnotherAction = UTestPawnAction_Log::CreateAction(World, Logger);
		ActionsComponent->PushAction(AnotherAction, EAIRequestPriority::Logic);

		ActionsComponent->TickComponent(FAITestHelpers::TickInterval, ELevelTick::LEVELTICK_All, nullptr);

		Test(TEXT("Second pushed action should be the active one"), ActionsComponent->GetCurrentAction() == AnotherAction);
		Test(TEXT("First actionshould be paused"), Action->IsPaused() == true);

		return true; 
	}
};
IMPLEMENT_AI_TEST(FAITest_PawnActions_Pause, "Engine.AI.Pawn Actions.Pausing Action by younger Action of same priority")

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
struct FAITest_PawnActions_SamePriorityOrder : public FAITest_SimpleActionsTest
{
	UTestPawnAction_Log* FirstAction;

	void SetUp()
	{
		FAITest_SimpleActionsTest::SetUp();

		Logger.ExpectedValues.Add(ETestPawnActionMessage::Started);

		UWorld& World = GetWorld();
		FirstAction = UTestPawnAction_Log::CreateAction(World, Logger);
		ActionsComponent->PushAction(FirstAction, EAIRequestPriority::Logic);
		UTestPawnAction_Log* SecondAction = UTestPawnAction_Log::CreateAction(World, Logger);
		ActionsComponent->PushAction(SecondAction, EAIRequestPriority::Logic);
	}

	bool Update()
	{
		ActionsComponent->TickComponent(FAITestHelpers::TickInterval, ELevelTick::LEVELTICK_All, nullptr);

		Test(TEXT("Second pushed action should be the active one"), ActionsComponent->GetCurrentAction() != FirstAction);
		
		return true;
	}
};
IMPLEMENT_AI_TEST(FAITest_PawnActions_SamePriorityOrder, "Engine.AI.Pawn Actions.Respecting push order")

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
struct FAITest_PawnActions_DoublePushingAction : public FAITest_SimpleActionsTest
{
	bool Update()
	{
		UWorld& World = GetWorld();
		UTestPawnAction_Log* Action = UTestPawnAction_Log::CreateAction(World, Logger);
		ActionsComponent->PushAction(Action, EAIRequestPriority::Logic);
		Test(TEXT("Pushing same action for the second time should fail"), ActionsComponent->PushAction(Action, EAIRequestPriority::Logic) == false);
		Test(TEXT("There should be exactly one ActionEvent awaiting processing"), ActionsComponent->GetActionEventsQueueSize() == 1);
	
		ActionsComponent->TickComponent(FAITestHelpers::TickInterval, ELevelTick::LEVELTICK_All, nullptr);

		Test(TEXT("There should be only one action on stack now."), ActionsComponent->GetActionStackSize(EAIRequestPriority::Logic) == 1);
		Test(TEXT("Action queue should be empty."), ActionsComponent->GetActionEventsQueueSize() == 0);

		Test(TEXT("Pushing already active action should"), ActionsComponent->PushAction(Action, EAIRequestPriority::Logic) == false);
		Test(TEXT("Action queue should be empty."), ActionsComponent->GetActionEventsQueueSize() == 0);

		return true;
	}
};
IMPLEMENT_AI_TEST(FAITest_PawnActions_DoublePushingAction, "Engine.AI.Pawn Actions.Pushing same action twice")

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
struct FAITest_PawnActions_SimplePriority : public FAITest_SimpleActionsTest
{
	bool Update()
	{
		Logger.ExpectedValues.Add(ETestPawnActionMessage::Started);
		Logger.ExpectedValues.Add(ETestPawnActionMessage::Paused);
		Logger.ExpectedValues.Add(ETestPawnActionMessage::Started);

		UWorld& World = GetWorld();

		UTestPawnAction_Log* LowPriorityAction = UTestPawnAction_Log::CreateAction(World, Logger);
		ActionsComponent->PushAction(LowPriorityAction, EAIRequestPriority::Logic);
		ActionsComponent->TickComponent(FAITestHelpers::TickInterval, ELevelTick::LEVELTICK_All, nullptr);

		UTestPawnAction_Log* HighPriorityAction = UTestPawnAction_Log::CreateAction(World, Logger);
		ActionsComponent->PushAction(HighPriorityAction, EAIRequestPriority::Reaction);
		ActionsComponent->TickComponent(FAITestHelpers::TickInterval, ELevelTick::LEVELTICK_All, nullptr);

		Test(TEXT("There should be exactly one action on Logic stack now."), ActionsComponent->GetActionStackSize(EAIRequestPriority::Logic) == 1);
		Test(TEXT("There should be exactly one action on Reaction stack now."), ActionsComponent->GetActionStackSize(EAIRequestPriority::Reaction) == 1);
		Test(TEXT("The higher priority action should be the active one"), ActionsComponent->GetCurrentAction() == HighPriorityAction);

		return true;
	}
};
IMPLEMENT_AI_TEST(FAITest_PawnActions_SimplePriority, "Engine.AI.Pawn Actions.Pushing different priority actions")

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
struct FAITest_PawnActions_HighPriorityKeepRunning : public FAITest_SimpleActionsTest
{
	bool Update()
	{
		// only this one event should get logged
		Logger.ExpectedValues.Add(ETestPawnActionMessage::Started);

		UWorld& World = GetWorld();

		UTestPawnAction_Log* HighPriorityAction = UTestPawnAction_Log::CreateAction(World, Logger);
		ActionsComponent->PushAction(HighPriorityAction, EAIRequestPriority::Reaction);
		ActionsComponent->TickComponent(FAITestHelpers::TickInterval, ELevelTick::LEVELTICK_All, nullptr);

		UTestPawnAction_Log* LowPriorityAction = UTestPawnAction_Log::CreateAction(World, Logger);
		ActionsComponent->PushAction(LowPriorityAction, EAIRequestPriority::Logic);
		ActionsComponent->TickComponent(FAITestHelpers::TickInterval, ELevelTick::LEVELTICK_All, nullptr);

		Test(TEXT("There should be exactly one action on Logic stack now."), ActionsComponent->GetActionStackSize(EAIRequestPriority::Logic) == 1);
		Test(TEXT("There should be exactly one action on Reaction stack now."), ActionsComponent->GetActionStackSize(EAIRequestPriority::Reaction) == 1);
		Test(TEXT("The higher priority action should still be the active"), ActionsComponent->GetCurrentAction() == HighPriorityAction);

		return true;
	}
};
IMPLEMENT_AI_TEST(FAITest_PawnActions_HighPriorityKeepRunning, "Engine.AI.Pawn Actions.High priority action still running after pushing lower priority action")

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
struct FAITest_PawnActions_SamePriorityActionsPushing : public FAITest_SimpleActionsTest
{
	bool Update()
	{
		Logger.ExpectedValues.Add(ETestPawnActionMessage::Started);
		UWorld& World = GetWorld();
		
		ActionsComponent->PushAction(UTestPawnAction_Log::CreateAction(World, Logger), EAIRequestPriority::Logic);
		ActionsComponent->PushAction(UTestPawnAction_Log::CreateAction(World, Logger), EAIRequestPriority::Logic);
		ActionsComponent->PushAction(UTestPawnAction_Log::CreateAction(World, Logger), EAIRequestPriority::Logic);
		UPawnAction* LastAction = UTestPawnAction_Log::CreateAction(World, Logger);
		ActionsComponent->PushAction(LastAction, EAIRequestPriority::Logic);
		
		Test(TEXT("Action queue should be empty."), ActionsComponent->GetActionEventsQueueSize() == 4);

		ActionsComponent->TickComponent(FAITestHelpers::TickInterval, ELevelTick::LEVELTICK_All, nullptr);

		Test(TEXT("Last action pushed should the one active"), ActionsComponent->GetCurrentAction() == LastAction);

		return true;
	}
};
IMPLEMENT_AI_TEST(FAITest_PawnActions_SamePriorityActionsPushing, "Engine.AI.Pawn Actions.Pushing multiple actions of same priority")

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
struct FAITest_PawnActions_Aborting : public FAITest_SimpleActionsTest
{
	bool Update()
	{
		Logger.ExpectedValues.Add(ETestPawnActionMessage::Started);
		Logger.ExpectedValues.Add(ETestPawnActionMessage::Finished);

		UWorld& World = GetWorld();
		UTestPawnAction_Log* Action = UTestPawnAction_Log::CreateAction(World, Logger);
		ActionsComponent->PushAction(Action, EAIRequestPriority::Logic);
		ActionsComponent->TickComponent(FAITestHelpers::TickInterval, ELevelTick::LEVELTICK_All, nullptr);

		ActionsComponent->AbortAction(Action);
		ActionsComponent->TickComponent(FAITestHelpers::TickInterval, ELevelTick::LEVELTICK_All, nullptr);

		Test(TEXT("There should be no actions on the stack."), ActionsComponent->GetActionStackSize(EAIRequestPriority::Logic) == 0);
		Test(TEXT("There should be no current action"), ActionsComponent->GetCurrentAction() == nullptr);

		return true;
	}
};
IMPLEMENT_AI_TEST(FAITest_PawnActions_Aborting, "Engine.AI.Pawn Actions.Basic aborting mechanics")

//struct FAITest_PawnActions_InstantFinish : public FAITest_SimpleActionsTest
