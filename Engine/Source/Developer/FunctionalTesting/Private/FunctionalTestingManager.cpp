// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FunctionalTestingPrivatePCH.h"

#define LOCTEXT_NAMESPACE "FunctionalTesting"

UFunctionalTestingManager::UFunctionalTestingManager( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{
	
}

void UFunctionalTestingManager::SetUpTests()
{
	OnSetupTests.Broadcast();
}

void UFunctionalTestingManager::RunAllTestsOnMap(bool bNewLog, bool bRunLooped)
{
	if (bIsRunning)
	{
		UE_LOG(LogFunctionalTest, Warning, TEXT("Functional tests are already running, aborting."));
		return;
	}
	
	FMessageLog FunctionalTestingLog("FunctionalTestingLog");
	FunctionalTestingLog.Open();
	if (bNewLog)
	{
		FunctionalTestingLog.NewPage(LOCTEXT("NewLogLabel", "Functional Test"));
	}
	bLooped = bRunLooped;

	CurrentIteration = 0;

	for ( FActorIterator It(GWorld); It; ++It )
	{
		AFunctionalTest* Test = Cast<AFunctionalTest>(*It);
		if (Test != NULL)
		{
			AllTests.Add(Test);
		}
	}

	if (AllTests.Num() > 0)
	{
		SetUpTests();

		TestsLeft = AllTests;
		
		bIsRunning = RunFirstValidTest();
	}
	
	if (bIsRunning == false)
	{
		FunctionalTestingLog.Info(LOCTEXT("NoTestsDefined", "No tests defined on map or all disabled. DONE."));
	}
}

UFunctionalTestingManager* UFunctionalTestingManager::CreateFTestManager(UObject* WorldContext)
{
	UObject* Outer = WorldContext ? WorldContext : (UObject*)GetTransientPackage();
	UFunctionalTestingManager* Script = NewObject<UFunctionalTestingManager>(Outer);
	FFunctionalTestingModule::Get()->SetScript(Script);

	return Script;
}

void UFunctionalTestingManager::OnTestDone(AFunctionalTest* FTest)
{
	// add a delay
	FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
		FSimpleDelegateGraphTask::FDelegate::CreateUObject(this, &UFunctionalTestingManager::NotifyTestDone, FTest)
		, TEXT("Requesting to build next tile if necessary")
		, NULL
		, ENamedThreads::GameThread
		);
}

void UFunctionalTestingManager::NotifyTestDone(AFunctionalTest* FTest)
{
	FMessageLog FunctionalTestingLog("FunctionalTestingLog");

	if (TestsLeft.RemoveSingle(FTest) && TestsLeft.Num() > 0)
	{
		bIsRunning = RunFirstValidTest();
	}
	else
	{
		bIsRunning = false;
	}

	if (bIsRunning == false)
	{
		if (bLooped == true)
		{
			++CurrentIteration;
			TestsLeft = AllTests;
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("IterationIndex"), CurrentIteration);
			FunctionalTestingLog.Info( FText::Format(LOCTEXT("StartingIteration", "----- Starting iteration {IterationIndex} -----"), Arguments) );
			bIsRunning = RunFirstValidTest();
		}
		else
		{
			FunctionalTestingLog.Info( LOCTEXT("TestDone", "DONE.") );
		}
	}
}

bool UFunctionalTestingManager::RunFirstValidTest()
{
	for (int32 Index = 0; Index < TestsLeft.Num(); ++Index)
	{
		if (TestsLeft[Index] != NULL && TestsLeft[Index]->bIsTestDisabled == false)
		{
			TestsLeft[Index]->TestFinishedObserver = FFunctionalTestDoneSignature::CreateUObject(this, &UFunctionalTestingManager::OnTestDone);
			TestsLeft[Index]->StartTest(); 

			return true;
		}
		else
		{
			TestsLeft.RemoveAtSwap(Index, 1, false);
		}
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
