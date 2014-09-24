// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Misc/AutomationTest.h"

class UBehaviorTree;
class UMockAI_BT;

DECLARE_LOG_CATEGORY_EXTERN(LogAITestSuite, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogBehaviorTreeTest, Log, All);

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FAITestCommand_WaitSeconds, float, Duration);

class FAITestCommand_WaitOneTick : public IAutomationLatentCommand
{
public: 
	FAITestCommand_WaitOneTick()
		: bAlreadyRun(false)
	{} 
	virtual bool Update() override;
private: 
	bool bAlreadyRun;
};


namespace FAITestHelpers
{
	UWorld* GetWorld();
}

struct FAITestBase
{
	virtual ~FAITestBase(){}
	virtual void SetUp() {}
	virtual bool Update() { return true; }
	virtual void TearDown() {}
};

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FAITestCommand_SetUpTest, FAITestBase*, AITest);
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FAITestCommand_PerformTest, FAITestBase*, AITest);
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FAITestCommand_TearDownTest, FAITestBase*, AITest);

#define IMPLEMENT_AI_TEST(TestClass, PrettyName) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(TestClass##Runner, PrettyName, (EAutomationTestFlags::ATF_Game | EAutomationTestFlags::ATF_Editor)) \
	bool TestClass##Runner::RunTest(const FString& Parameters) \
	{ \
		/* spawn test instance. Setup should be done in test's constructor */ \
		TestClass* TestInstance = new TestClass(); \
		/* set up */ \
		ADD_LATENT_AUTOMATION_COMMAND(FAITestCommand_SetUpTest(TestInstance)); \
		/* run latent command to update */ \
		ADD_LATENT_AUTOMATION_COMMAND(FAITestCommand_PerformTest(TestInstance)); \
		/* run latent command to tear down */ \
		ADD_LATENT_AUTOMATION_COMMAND(FAITestCommand_TearDownTest(TestInstance)); \
		return true; \
	} 

//----------------------------------------------------------------------//
// Specific test types
//----------------------------------------------------------------------//

struct FAITest_SimpleBT : public FAITestBase
{
	TArray<int32> ExpectedResult;
	UBehaviorTree* BTAsset;
	UMockAI_BT* AIBTUser;

	FAITest_SimpleBT();	
	virtual ~FAITest_SimpleBT();
	virtual void SetUp() override;
	virtual bool Update() override;
	
	virtual void VerifyResults();
};