// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_BlueprintBase.generated.h"

/**
 *  Base class for blueprint based decorator nodes. Do NOT use it for creating native c++ classes!
 *
 *  Unlike task and services, decorator have two execution chains: 
 *   ExecutionStart-ExecutionFinish and ObserverActivated-ObserverDeactivated
 *  which makes automatic latent action cleanup impossible. Keep in mind, that
 *  you HAVE TO verify is given chain is still active after resuming from any
 *  latent action (like Delay, Timelines, etc).
 *
 *  Helper functions:
 *  - IsDecoratorExecutionActive (true after ExecutionStart, until ExecutionFinish)
 *  - IsDecoratorObserverActive (true after ObserverActivated, until ObserverDeactivated)
 */

UCLASS(Abstract, Blueprintable)
class AIMODULE_API UBTDecorator_BlueprintBase : public UBTDecorator
{
	GENERATED_UCLASS_BODY()

	/** initialize data about blueprint defined properties */
	void InitializeProperties();

	/** setup node name */
	virtual void PostInitProperties() override;

	/** notify about changes in blackboard */
	void OnBlackboardChange(const class UBlackboardComponent* Blackboard, uint8 ChangedKeyID);

	virtual FString GetStaticDescription() const override;
	virtual void DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const override;
	virtual bool CalculateRawConditionValue(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const override;

#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
	virtual bool UsesBlueprint() const override;
#endif

protected:

	/** blackboard key names that should be observed */
	TArray<FName> ObservedKeyNames;

	/** properties with runtime values, stored only in class default object */
	TArray<UProperty*> PropertyData;

	/** show detailed information about properties */
	UPROPERTY(EditInstanceOnly, Category=Description)
	uint32 bShowPropertyDetails : 1;

	/** temporary variable for ReceiveConditionCheck-FinishConditionCheck chain */
	mutable uint32 CurrentCallResult : 1;

	/** set if ReceiveTick is implemented by blueprint */
	uint32 bImplementsReceiveTick : 1;

	/** set if ReceiveExecutionStart is implemented by blueprint */
	uint32 bImplementsReceiveExecutionStart : 1;

	/** set if ReceiveExecutionFinish is implemented by blueprint */
	uint32 bImplementsReceiveExecutionFinish : 1;

	/** set if ReceiveObserverActivated is implemented by blueprint */
	uint32 bImplementsReceiveObserverActivated : 1;

	/** set if ReceiveObserverDeactivated is implemented by blueprint */
	uint32 bImplementsReceiveObserverDeactivated : 1;

	/** set if ReceiveConditionCheck is implemented by blueprint */
	uint32 bImplementsReceiveConditionCheck : 1;

	FOnBlackboardChange BBKeyObserver;

	virtual void OnBecomeRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) override;
	virtual void OnNodeActivation(struct FBehaviorTreeSearchData& SearchData) override;
	virtual void OnNodeDeactivation(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type NodeResult) override;
	virtual void TickNode(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** tick function */
	UFUNCTION(BlueprintImplementableEvent)
	virtual void ReceiveTick(AActor* OwnerActor, float DeltaSeconds);

	/** called on execution of underlying node */
	UFUNCTION(BlueprintImplementableEvent)
	virtual void ReceiveExecutionStart(AActor* OwnerActor);

	/** called when execution of underlying node is finished */
	UFUNCTION(BlueprintImplementableEvent)
	virtual void ReceiveExecutionFinish(AActor* OwnerActor, enum EBTNodeResult::Type NodeResult);

	/** called when observer is activated (flow controller) */
	UFUNCTION(BlueprintImplementableEvent)
	virtual void ReceiveObserverActivated(AActor* OwnerActor);

	/** called when observer is deactivated (flow controller) */
	UFUNCTION(BlueprintImplementableEvent)
	virtual void ReceiveObserverDeactivated(AActor* OwnerActor);

	/** called when testing if underlying node can be executed, must call FinishConditionCheck */
	UFUNCTION(BlueprintImplementableEvent)
	virtual void ReceiveConditionCheck(AActor* OwnerActor);

	/** finishes condition check */
	UFUNCTION(BlueprintCallable, Category="AI|BehaviorTree")
	void FinishConditionCheck(bool bAllowExecution);
		
	/** check if decorator is part of currently active branch */
	UFUNCTION(BlueprintCallable, Category="AI|BehaviorTree")
	bool IsDecoratorExecutionActive() const;

	/** check if decorator's observer is currently active */
	UFUNCTION(BlueprintCallable, Category="AI|BehaviorTree")
	bool IsDecoratorObserverActive() const;

	friend class FBehaviorBlueprintDetails;
};
