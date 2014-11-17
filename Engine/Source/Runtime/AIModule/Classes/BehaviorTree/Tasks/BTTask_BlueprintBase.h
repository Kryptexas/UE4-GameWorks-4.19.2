// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BTTask_BlueprintBase.generated.h"

/**
 *  Base class for blueprint based task nodes. Do NOT use it for creating native c++ classes!
 *
 *  When task receives Abort event, all latent actions associated this instance are being removed.
 *  This prevents from resuming activity started by Execute, but does not handle external events.
 *  Please use them safely (unregister at abort) and call IsTaskExecuting() when in doubt.
 */

UCLASS(Abstract, Blueprintable)
class AIMODULE_API UBTTask_BlueprintBase : public UBTTaskNode
{
	GENERATED_UCLASS_BODY()

	/** setup node name */
	virtual void PostInitProperties() override;

	virtual EBTNodeResult::Type ExecuteTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) override;

	virtual FString GetStaticDescription() const override;
	virtual void DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const override;

#if WITH_EDITOR
	virtual bool UsesBlueprint() const override;
#endif

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

	/** check if task is currently being executed */
	UFUNCTION(BlueprintCallable, Category="AI|BehaviorTree")
	bool IsTaskExecuting() const;

	/** ticks this task */
	virtual void TickTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	friend class FBehaviorBlueprintDetails;
};
