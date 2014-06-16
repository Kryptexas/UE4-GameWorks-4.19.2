// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Misc/AutomationTest.h"
#include "FunctionalTestingManager.generated.h"

UCLASS(BlueprintType, MinimalAPI)
class UFunctionalTestingManager : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()
	
	UPROPERTY(Transient)
	TArray<class AFunctionalTest*> TestsLeft;

	UPROPERTY(Transient)
	TArray<class AFunctionalTest*> AllTests;

	UPROPERTY(BlueprintAssignable)
	FFunctionalTestEventSignature OnSetupTests;
	
	UFUNCTION(BlueprintCallable, Category="FunctionalTesting", meta=(HidePin="WorldContext", DefaultToSelf="WorldContext" ) )
	/** Triggers in sequence all functional tests found on the level.
	 *	@return true if any tests have been triggered */
	static bool RunAllFunctionalTests(UObject* WorldContext, bool bNewLog=true, bool bRunLooped=false, bool bWaitForNavigationBuildFinish=true, bool bLoopOnlyFailedTests = true);
		
	bool IsRunning() const { return bIsRunning; }
	bool IsLooped() const { return bLooped; }
	void SetLooped(const bool bNewLooped) { bLooped = bNewLooped; }

	void TickMe(float DeltaTime);

	//----------------------------------------------------------------------//
	// Automation logging
	//----------------------------------------------------------------------//
	static void SetAutomationExecutionInfo(FAutomationTestExecutionInfo* InExecutionInfo) { ExecutionInfo = InExecutionInfo; }
	static void AddError(const FText& InError);
	static void AddWarning(const FText& InWarning);
	static void AddLogItem(const FText& InLogItem);
	
private:
	void LogMessage(const FString& MessageString, TSharedPtr<IMessageLogListing> LogListing = NULL);
	
protected:
	static UFunctionalTestingManager* GetManager(UObject* WorldContext);

	void TriggerFirstValidTest();
	void SetUpTests();

	void OnTestDone(class AFunctionalTest* FTest);
	void OnEndPIE(const bool bIsSimulating);

	bool RunFirstValidTest();

	void NotifyTestDone(class AFunctionalTest* FTest);
	
	bool bIsRunning;
	bool bLooped;
	bool bWaitForNavigationBuildFinish;
	bool bInitialDelayApplied;
	bool bDiscardSuccessfulTests;
	uint32 CurrentIteration;

	static FAutomationTestExecutionInfo* ExecutionInfo;
};
