// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FunctionalTestingPrivatePCH.h"

#define LOCTEXT_NAMESPACE "FunctionalTesting"

//struct FFuncTestingTickHelper : FTickableGameObject
//{
//	class UFunctionalTestingManager* Manager;
//
//	FFuncTestingTickHelper() : Manager(NULL) {}
//	virtual void Tick(float DeltaTime);
//	virtual bool IsTickable() const { return Owner && !((AActor*)Owner)->IsPendingKillPending(); }
//	virtual bool IsTickableInEditor() const { return true; }
//	virtual TStatId GetStatId() const ;
//};
//
////----------------------------------------------------------------------//
//// 
////----------------------------------------------------------------------//
//void FFuncTestingTickHelper::Tick(float DeltaTime)
//{
//	if (Manager->IsPendingKill() == false)
//	{
//		Manager->TickMe(DeltaTime);
//	}
//}
//
//TStatId FFuncTestingTickHelper::GetStatId() const 
//{
//	RETURN_QUICK_DECLARE_CYCLE_STAT(FRecastTickHelper, STATGROUP_Tickables);
//}

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//

FAutomationTestExecutionInfo* UFunctionalTestingManager::ExecutionInfo = NULL;

UFunctionalTestingManager::UFunctionalTestingManager( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
	, bIsRunning(false)
	, bLooped(false)
	, bWaitForNavigationBuildFinish(false)
	, bInitialDelayApplied(false)
	, CurrentIteration(INDEX_NONE)
{
}

void UFunctionalTestingManager::SetUpTests()
{
	OnSetupTests.Broadcast();
}

bool UFunctionalTestingManager::RunAllFunctionalTests(UObject* WorldContext, bool bNewLog, bool bRunLooped, bool bInWaitForNavigationBuildFinish, bool bLoopOnlyFailedTests)
{
	UFunctionalTestingManager* Manager = GetManager(WorldContext);

	if (Manager->bIsRunning)
	{
		UE_LOG(LogFunctionalTest, Warning, TEXT("Functional tests are already running, aborting."));
		return true;
	}
	
	FMessageLog FunctionalTestingLog("FunctionalTestingLog");
	FunctionalTestingLog.Open();
	if (bNewLog)
	{
		FunctionalTestingLog.NewPage(LOCTEXT("NewLogLabel", "Functional Test"));
	}

	Manager->bLooped = bRunLooped;
	Manager->bWaitForNavigationBuildFinish = bInWaitForNavigationBuildFinish;
	Manager->CurrentIteration = 0;
	Manager->bDiscardSuccessfulTests = bLoopOnlyFailedTests;

	for (TActorIterator<AFunctionalTest> It(GWorld); It; ++It)
	{
		AFunctionalTest* Test = (*It);
		if (Test != NULL && Test->bIsEnabled == true)
		{
			Manager->AllTests.Add(Test);
		}
	}

	if (Manager->AllTests.Num() > 0)
	{
		Manager->SetUpTests();
		Manager->TestsLeft = Manager->AllTests;

		Manager->TriggerFirstValidTest();
	}

	if (Manager->bIsRunning == false)
	{
		AddLogItem(LOCTEXT("NoTestsDefined", "No tests defined on map or . DONE.")); 
		return false;
	}

#if WITH_EDITOR
	FEditorDelegates::EndPIE.AddUObject(Manager, &UFunctionalTestingManager::OnEndPIE);
#endif // WITH_EDITOR
	Manager->AddToRoot();

	return true;
}

void UFunctionalTestingManager::OnEndPIE(const bool bIsSimulating)
{
	RemoveFromRoot();
}

void UFunctionalTestingManager::TriggerFirstValidTest()
{
	UWorld* World = GEngine->GetWorldFromContextObject(GetOuter()); 
	bIsRunning = World != NULL && World->GetNavigationSystem() != NULL;

	if (bInitialDelayApplied == true && (bWaitForNavigationBuildFinish == false || UNavigationSystem::IsNavigationBeingBuilt(World) == false))
	{
		bIsRunning = RunFirstValidTest();
	}
	else
	{
		bInitialDelayApplied = true;
		static const float WaitingTime = 0.25f;
		World->GetTimerManager().SetTimer(this, &UFunctionalTestingManager::TriggerFirstValidTest, WaitingTime);
	}
}

UFunctionalTestingManager* UFunctionalTestingManager::GetManager(UObject* WorldContext)
{
	UFunctionalTestingManager* Manager = FFunctionalTestingModule::Get()->GetCurrentScript();

	if (Manager == NULL)
	{
		UObject* Outer = WorldContext ? WorldContext : (UObject*)GetTransientPackage();
		Manager = NewObject<UFunctionalTestingManager>(Outer);
		FFunctionalTestingModule::Get()->SetScript(Manager);
	}

	return Manager;
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
	if (FTest->WantsToRunAgain() == false)
	{
		TestsLeft.RemoveSingle(FTest);
		/*if (bDiscardSuccessfulTests && FTest->IsSuccessful())
		{
			AllTests.RemoveSingle(FTest);
		}*/
		FTest->CleanUp();
	}

	if (TestsLeft.Num() > 0)
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
			AddLogItem(FText::Format(LOCTEXT("StartingIteration", "----- Starting iteration {IterationIndex} -----"), Arguments));
			bIsRunning = RunFirstValidTest();
		}
		else
		{
			AddLogItem(LOCTEXT("TestDone", "DONE."));
			RemoveFromRoot();
		}
	}
}

bool UFunctionalTestingManager::RunFirstValidTest()
{
	for (int32 Index = 0; Index < TestsLeft.Num(); ++Index)
	{
		bool bRemove = TestsLeft[Index] == NULL;
		if (TestsLeft[Index] != NULL)
		{
			ensure(TestsLeft[Index]->bIsEnabled);
			TestsLeft[Index]->TestFinishedObserver = FFunctionalTestDoneSignature::CreateUObject(this, &UFunctionalTestingManager::OnTestDone);
			if (TestsLeft[Index]->StartTest())
			{
				return true;
			}
			else
			{
				bRemove = true;
			}
		}
		
		if (bRemove)
		{
			TestsLeft.RemoveAtSwap(Index, 1, false);
		}
	}

	return false;
}

void UFunctionalTestingManager::TickMe(float DeltaTime)
{

	
}

//----------------------------------------------------------------------//
// logging
//----------------------------------------------------------------------//
void UFunctionalTestingManager::AddError(const FText& InError)
{
	FMessageLog FunctionalTestingLog("FunctionalTestingLog");
	FunctionalTestingLog.Error(InError);
	if (ExecutionInfo)
	{
		ExecutionInfo->Errors.Add(InError.ToString());
	}
}


void UFunctionalTestingManager::AddWarning(const FText& InWarning)
{
	FMessageLog FunctionalTestingLog("FunctionalTestingLog");
	FunctionalTestingLog.Warning(InWarning);
	if (ExecutionInfo)
	{
		ExecutionInfo->Warnings.Add(InWarning.ToString());
	}
}


void UFunctionalTestingManager::AddLogItem(const FText& InLogItem)
{
	FMessageLog FunctionalTestingLog("FunctionalTestingLog");
	FunctionalTestingLog.Info(InLogItem);
	if (ExecutionInfo)
	{
		ExecutionInfo->LogItems.Add(InLogItem.ToString());
	}
}
#undef LOCTEXT_NAMESPACE
