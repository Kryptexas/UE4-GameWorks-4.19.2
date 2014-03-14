// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

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
	
	UFUNCTION(BlueprintCallable, Category="Development")	
	/** Triggers in sequence all functional tests found on the level.*/
	void RunAllTestsOnMap(bool bNewLog, bool bRunLooped);

	UFUNCTION(BlueprintPure, Category="Development", meta=(HidePin="WorldContext", DefaultToSelf="WorldContext") )
	static UFunctionalTestingManager* CreateFTestManager(UObject* WorldContext);
	
	bool IsRunning() const { return bIsRunning; }
	bool IsLooped() const { return bLooped; }
	void SetLooped(const bool bNewLooped) { bLooped = bNewLooped; }

private:
	void LogMessage(const FString& MessageString, TSharedPtr<IMessageLogListing> LogListing = NULL);

protected:
	void SetUpTests();

	void OnTestDone(class AFunctionalTest* FTest);

	bool RunFirstValidTest();

	void NotifyTestDone(class AFunctionalTest* FTest);
	
	bool bIsRunning;
	bool bLooped;
	uint32 CurrentIteration;
};
