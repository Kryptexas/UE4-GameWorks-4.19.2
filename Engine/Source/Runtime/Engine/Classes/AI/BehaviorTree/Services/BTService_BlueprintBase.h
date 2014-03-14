// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BTService_BlueprintBase.generated.h"

/**
 *  Blueprint implementable service
 *
 *  Properties defined in blueprint class marked as "Use Behavior Tree Instance Memory"
 *  will be saved in context's memory, so they can be safely used from Receive* events.
 */

UCLASS(Abstract, Blueprintable)
class ENGINE_API UBTService_BlueprintBase : public UBTService
{
	GENERATED_UCLASS_BODY()

	/** initialize data about blueprint defined properties */
	void DelayedInitialize();

	virtual void PostInitProperties() OVERRIDE;

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

	/** properties that should be copied */
	TArray<UProperty*> PropertyData;

	/** cached memory size (PropertyData is accessible AFTER a tick, so it could read meta data) */
	int32 PropertyMemorySize;

	/** show detailed information about properties */
	UPROPERTY(EditInstanceOnly, Category=Description)
	uint32 bShowPropertyDetails : 1;

	/** set if ReceiveTick is implemented by blueprint */
	uint32 bImplementsReceiveTick : 1;
	
	/** set if ReceiveActivation is implemented by blueprint */
	uint32 bImplementsReceiveActivation : 1;

	/** set if ReceiveDeactivation is implemented by blueprint */
	uint32 bImplementsReceiveDeactivation : 1;

	virtual void OnBecomeRelevant(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
	virtual void OnCeaseRelevant(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
	virtual void TickNode(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const OVERRIDE;

	/** tick function */
	UFUNCTION(BlueprintImplementableEvent)
	virtual void ReceiveTick(AActor* OwnerActor, float DeltaSeconds);

	/** service became active */
	UFUNCTION(BlueprintImplementableEvent)
	virtual void ReceiveActivation(AActor* OwnerActor);

	/** service became inactive */
	UFUNCTION(BlueprintImplementableEvent)
	virtual void ReceiveDeactivation(AActor* OwnerActor);

	/** copy all property data to context memory */
	void CopyPropertiesToMemory(uint8* NodeMemory) const;

	/** copy all property data from context memory */
	void CopyPropertiesFromMemory(const uint8* NodeMemory);

	friend class FBehaviorBlueprintDetails;
};
