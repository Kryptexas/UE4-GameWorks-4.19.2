// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FunctionalTest.generated.h"

UENUM()
namespace EFunctionalTestResult
{
	enum Type
	{
		Invalid,
		Error,
		Running,		
		Failed,
		Succeeded,
	};
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFunctionalTestEventSignature);
DECLARE_DELEGATE_OneParam(FFunctionalTestDoneSignature, class AFunctionalTest*);

UCLASS(Blueprintable, MinimalAPI)
class AFunctionalTest : public AActor
{
	GENERATED_UCLASS_BODY()

	static const uint32 DefaultTimeLimit = 60;	// seconds

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	class UBillboardComponent* SpriteComponent;
#endif // WITH_EDITORONLY_DATA

	UPROPERTY(BlueprintReadWrite, Category=FunctionalTesting)
	TEnumAsByte<EFunctionalTestResult::Type> Result;

	/** If test is limited by time this is the result that will be returned when time runs out */
	UPROPERTY(EditAnywhere, Category=FunctionalTesting)
	TEnumAsByte<EFunctionalTestResult::Type> TimesUpResult;

	/** Test's time limit. '0' means no limit */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=FunctionalTesting)
	float TimeLimit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=FunctionalTesting)
	FText TimesUpMessage;

	/** Called when the test is started */
	UPROPERTY(BlueprintAssignable)
	FFunctionalTestEventSignature OnTestStart;

	/** Called when the test is finished. Use it to clean up */
	UPROPERTY(BlueprintAssignable)
	FFunctionalTestEventSignature OnTestFinished;

	UPROPERTY(Transient)
	TArray<AActor*> AutoDestroyActors;
	
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	class UFuncTestRenderingComponent* RenderComp;
#endif // WITH_EDITORONLY_DATA

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=FunctionalTesting)
	uint32 bIsTestDisabled : 1;

	UFUNCTION(BlueprintCallable, Category="Development")
	virtual void StartTest();

	UFUNCTION(BlueprintCallable, Category="Development")
	virtual void FinishTest(TEnumAsByte<EFunctionalTestResult::Type> TestResult, const FString& Message);

	UFUNCTION(BlueprintCallable, Category="Development")
	virtual void LogMessage(const FString& Message);

	UFUNCTION(BlueprintCallable, Category="Development")
	virtual void SetTimeLimit(float NewTimeLimit, TEnumAsByte<EFunctionalTestResult::Type> ResultWhenTimeRunsOut);

	/** ACtors registered this way will be automatically destroyed (by limiting their lifespan)
	 *	on test finish */
	UFUNCTION(BlueprintCallable, Category="Development", meta=(Keywords = "Delete"))
	virtual void RegisterAutoDestroyActor(AActor* ActorToAutoDestroy);

#if WITH_EDITOR
	void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR

	// AActor interface begin
	virtual void Tick(float DeltaSeconds) OVERRIDE;
	// AActor interface end

	FFunctionalTestDoneSignature TestFinishedObserver;

protected:
	uint32 bIsRunning;
	float TimeLeft;
};