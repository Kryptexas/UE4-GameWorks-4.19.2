// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BTDecorator_BlueprintBase.generated.h"

/**
 *  Blueprint implementable decorator
 *
 *  Properties defined in blueprint class marked as "Use Behavior Tree Instance Memory"
 *  will be saved in context's memory, so they can be safely used from Receive* events.
 */

UCLASS(Abstract, Blueprintable)
class ENGINE_API UBTDecorator_BlueprintBase : public UBTDecorator
{
	GENERATED_UCLASS_BODY()

	/** initialize data about blueprint defined properties */
	void DelayedInitialize();

	/** setup node name */
	virtual void PostInitProperties() OVERRIDE;

	/** notify about changes in blackboard */
	void OnBlackboardChange(const class UBlackboardComponent* Blackboard, uint8 ChangedKeyID);

	virtual FString GetStaticDescription() const OVERRIDE;
	virtual void DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const OVERRIDE;
	virtual class UBehaviorTreeComponent* GetCurrentCallOwner() const OVERRIDE { return CurrentCallOwner; }
	virtual void StartUsingExternalEvent(AActor* OwningActor) OVERRIDE;
	virtual void StopUsingExternalEvent() OVERRIDE;

	virtual bool CalculateRawConditionValue(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
	virtual uint16 GetInstanceMemorySize() const OVERRIDE;

protected:

	/** temporary variable for Receive* event chain */
	UPROPERTY(transient)
	mutable class UBehaviorTreeComponent* CurrentCallOwner;

	/** blackboard key names that should be observed */
	TArray<FName> ObservedKeyNames;

	/** properties that should be copied */
	TArray<UProperty*> PropertyData;

	/** cached memory size */
	int32 PropertyMemorySize;

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

	virtual void OnBecomeRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
	virtual void OnNodeActivation(struct FBehaviorTreeSearchData& SearchData) const OVERRIDE;
	virtual void OnNodeDeactivation(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type NodeResult) const OVERRIDE;
	virtual void TickNode(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const OVERRIDE;

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
	UFUNCTION(BlueprintCallable, Category="AI|BehaviorTree", Meta=(HidePin="NodeOwner", DefaultToSelf="NodeOwner"))
	static void FinishConditionCheck(UBTDecorator_BlueprintBase* NodeOwner, bool bAllowExecution);
		
	/** copy all property data to context memory */
	void CopyPropertiesToMemory(uint8* NodeMemory) const;
	void CopyPropertiesToMemory(struct FBehaviorTreeSearchData& SearchData) const;

	/** copy all property data from context memory */
	void CopyPropertiesFromMemory(const uint8* NodeMemory);
	void CopyPropertiesFromMemory(const struct FBehaviorTreeSearchData& SearchData);

	friend class FBehaviorBlueprintDetails;
};
