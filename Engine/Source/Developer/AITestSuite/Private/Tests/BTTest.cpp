// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AITestSuitePrivatePCH.h"
#include "MockAI_BT.h"
#include "BehaviorTree/TestBTDecorator_CantExecute.h"

#define LOCTEXT_NAMESPACE "AITestSuite_BTTest"

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
struct FAITest_BasicSelector : public FAITest_SimpleBT
{
	FAITest_BasicSelector()
	{
		UBTCompositeNode& CompNode = FBTBuilder::AddSelector(*BTAsset);
		{
			FBTBuilder::AddTask(CompNode, 0, EBTNodeResult::Failed);

			FBTBuilder::AddTask(CompNode, 1, EBTNodeResult::Succeeded);
			{
				FBTBuilder::WithDecorator<UTestBTDecorator_CantExecute>(CompNode);
			}

			FBTBuilder::AddTask(CompNode, 2, EBTNodeResult::Succeeded, 0.5f);

			FBTBuilder::AddTask(CompNode, 3, EBTNodeResult::Failed);
		}

		ExpectedResult.Add(0);
		ExpectedResult.Add(2);
	}
};
IMPLEMENT_AI_TEST(FAITest_BasicSelector, "Engine.AI.Behavior Trees.Composite node: selector")

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
struct FAITest_BasicSequence : public FAITest_SimpleBT
{
	FAITest_BasicSequence()
	{
		UBTCompositeNode& CompNode = FBTBuilder::AddSequence(*BTAsset);
		{
			FBTBuilder::AddTask(CompNode, 0, EBTNodeResult::Succeeded);

			FBTBuilder::AddTask(CompNode, 1, EBTNodeResult::Failed);
			{
				FBTBuilder::WithDecorator<UTestBTDecorator_CantExecute>(CompNode);
				FBTBuilder::WithDecorator<UBTDecorator_ForceSuccess>(CompNode);
			}

			FBTBuilder::AddTask(CompNode, 2, EBTNodeResult::Failed, 0.5f);

			FBTBuilder::AddTask(CompNode, 3, EBTNodeResult::Succeeded);
		}

		ExpectedResult.Add(0);
		ExpectedResult.Add(2);
	}
};
IMPLEMENT_AI_TEST(FAITest_BasicSequence, "Engine.AI.Behavior Trees.Composite node: sequence")

#undef LOCTEXT_NAMESPACE
