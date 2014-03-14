// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BTTask_BlueprintBase.generated.h"

/**
 *  Blueprint implementable task
 *
 *  Properties defined in blueprint class marked as "Use Behavior Tree Instance Memory"
 *  will be saved in context's memory, so they can be safely used from Receive* events.
 */

UCLASS(Abstract, Blueprintable)
class ENGINE_API UBTTask_BlueprintBase : public UBTTaskNode
{
	GENERATED_UCLASS_BODY()

	/** initialize data about blueprint defined properties */
	void DelayedInitialize();

	/** setup node name */
	virtual void PostInitProperties() OVERRIDE;

	virtual EBTNodeResult::Type ExecuteTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
	virtual EBTNodeResult::Type AbortTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;

	virtual uint16 GetInstanceMemorySize() const OVERRIDE;
	virtual FString GetStaticDescription() const OVERRIDE;
	virtual void DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const OVERRIDE;
	virtual class UBehaviorTreeComponent* GetCurrentCallOwner() const OVERRIDE { return CurrentCallOwner; }
	virtual void StartUsingExternalEvent(AActor* OwningActor) OVERRIDE;
	virtual void StopUsingExternalEvent() OVERRIDE;

protected:

	/** temporary variable for Receive* event chain */
	UPROPERTY(transient)
	mutable class UBehaviorTreeComponent* CurrentCallOwner;

	/** temporary variable for ReceiveExecute(Abort)-FinishExecute(Abort) chain */
	mutable TEnumAsByte<EBTNodeResult::Type> CurrentCallResult;

	/** properties that should be copied */
	TArray<UProperty*> PropertyData;

	/** cached memory size (PropertyData is accessible AFTER a tick, so it could read meta data) */
	int32 PropertyMemorySize;

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
	UFUNCTION(BlueprintCallable, Category="AI|BehaviorTree", Meta=(HidePin="NodeOwner", DefaultToSelf="NodeOwner"))
	static void FinishExecute(UBTTask_BlueprintBase* NodeOwner, bool bSuccess);

	/** aborts task execution */
	UFUNCTION(BlueprintCallable, Category="AI|BehaviorTree", Meta=(HidePin="NodeOwner", DefaultToSelf="NodeOwner"))
	static void FinishAbort(UBTTask_BlueprintBase* NodeOwner);

	/** task execution will be finished (with result \'Success\') after receiving specified message */
	UFUNCTION(BlueprintCallable, Category="AI|BehaviorTree", Meta=(HidePin="NodeOwner", DefaultToSelf="NodeOwner"))
	static void SetFinishOnMessage(UBTTask_BlueprintBase* NodeOwner, FName MessageName);

	/** task execution will be finished (with result \'Success\') after receiving specified message with indicated ID */
	UFUNCTION(BlueprintCallable, Category="AI|BehaviorTree", Meta=(HidePin="NodeOwner", DefaultToSelf="NodeOwner"))
	static void SetFinishOnMessageWithId(UBTTask_BlueprintBase* NodeOwner, FName MessageName, int32 RequestID = -1);

	/** copy all property data to context memory */
	void CopyPropertiesToMemory(uint8* NodeMemory) const;

	/** copy all property data from context memory */
	void CopyPropertiesFromMemory(const uint8* NodeMemory);

	/** ticks this task */
	virtual void TickTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const OVERRIDE;

	friend class FBehaviorBlueprintDetails;
};
