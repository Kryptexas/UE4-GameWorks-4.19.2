// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BTTask_BlueprintBase.generated.h"

UCLASS(Abstract, Blueprintable)
class ENGINE_API UBTTask_BlueprintBase : public UBTTaskNode
{
	GENERATED_UCLASS_BODY()

	/** setup node name */
	virtual void PostInitProperties() OVERRIDE;

	virtual EBTNodeResult::Type ExecuteTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) OVERRIDE;
	virtual EBTNodeResult::Type AbortTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) OVERRIDE;

	virtual FString GetStaticDescription() const OVERRIDE;
	virtual void DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const OVERRIDE;

protected:

	/** temporary variable for ReceiveExecute(Abort)-FinishExecute(Abort) chain */
	mutable TEnumAsByte<EBTNodeResult::Type> CurrentCallResult;

	/** properties that should be copied */
	TArray<UProperty*> PropertyData;

	/** show detailed information about properties */
	UPROPERTY(EditInstanceOnly, Category=Description)
	uint32 bShowPropertyDetails : 1;

	/** set if ReceiveTick is implemented by blueprint */
	uint32 bImplementsReceiveTick : 1;

	/** set if ReceiveExecute is implemented by blueprint */
	uint32 bImplementsReceiveExecute : 1;

	/** set if ReceiveAbort is implemented by blueprint */
	uint32 bImplementsReceiveAbort : 1;

	/** if set, execution is inside blueprint's ReceiveExecute(Abort) event
	  * FinishExecute(Abort) function should store their result in CurrentCallResult variable */
	mutable uint32 bStoreFinishResult : 1;

	/** entry point, task will stay active until FinishExecute is called */
	UFUNCTION(BlueprintImplementableEvent)
	virtual void ReceiveExecute(AActor* OwnerActor);

	/** if blueprint graph contains this event, task will stay active until FinishAbort is called */
	UFUNCTION(BlueprintImplementableEvent)
	virtual void ReceiveAbort(AActor* OwnerActor);

	/** tick function */
	UFUNCTION(BlueprintImplementableEvent)
	virtual void ReceiveTick(AActor* OwnerActor, float DeltaSeconds);

	/** finishes task execution with Success or Fail result */
	UFUNCTION(BlueprintCallable, Category="AI|BehaviorTree")
	void FinishExecute(bool bSuccess);

	/** aborts task execution */
	UFUNCTION(BlueprintCallable, Category="AI|BehaviorTree")
	void FinishAbort();

	/** task execution will be finished (with result \'Success\') after receiving specified message */
	UFUNCTION(BlueprintCallable, Category="AI|BehaviorTree")
	void SetFinishOnMessage(FName MessageName);

	/** task execution will be finished (with result \'Success\') after receiving specified message with indicated ID */
	UFUNCTION(BlueprintCallable, Category="AI|BehaviorTree")
	void SetFinishOnMessageWithId(FName MessageName, int32 RequestID = -1);

	/** ticks this task */
	virtual void TickTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) OVERRIDE;

	friend class FBehaviorBlueprintDetails;
};
