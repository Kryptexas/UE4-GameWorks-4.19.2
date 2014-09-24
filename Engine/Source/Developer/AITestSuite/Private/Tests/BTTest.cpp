// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AITestSuitePrivatePCH.h"

#define LOCTEXT_NAMESPACE "AITestSuite_BTTest"

// create test base class
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FAITestSuite_BehaviorTree, "Engine.AI.Behavior Trees", (EAutomationTestFlags::ATF_Game | EAutomationTestFlags::ATF_Editor))

/** 
 * Requests a enumeration of all maps to be loaded
 */
void FAITestSuite_BehaviorTree::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	OutBeautifiedNames.Add(TEXT("Simple Parallel aborting"));
	OutTestCommands.Add(TEXT("BTTestID"));
}

/** 
 * Execute the loading of each map and performance captures
 *
 * @param Parameters - Should specify which map name to load
 * @return	TRUE if the test was successful, FALSE otherwise
 */
bool FAITestSuite_BehaviorTree::RunTest(const FString& Parameters)
{	
	return true;
}

#undef LOCTEXT_NAMESPACE