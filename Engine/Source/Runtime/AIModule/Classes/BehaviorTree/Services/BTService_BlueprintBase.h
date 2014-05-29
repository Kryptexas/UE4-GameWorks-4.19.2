// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTree/BTService.h"
#include "BTService_BlueprintBase.generated.h"

/**
 *  Base class for blueprint based service nodes. Do NOT use it for creating native c++ classes!
 *
 *  When service receives Deactivation event, all latent actions associated this instance are being removed.
 *  This prevents from resuming activity started by Activation, but does not handle external events.
 *  Please use them safely (unregister at abort) and call IsServiceActive() when in doubt.
 */

UCLASS(Abstract, Blueprintable)
class AIMODULE_API UBTService_BlueprintBase : public UBTService
{
	GENERATED_UCLASS_BODY()

	virtual void PostInitProperties() OVERRIDE;

	virtual FString GetStaticDescription() const OVERRIDE;
	virtual void DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const OVERRIDE;

protected:

	/** properties with runtime values, stored only in class default object */
	TArray<UProperty*> PropertyData;

	/** show detailed information about properties */
	UPROPERTY(EditInstanceOnly, Category=Description)
	uint32 bShowPropertyDetails : 1;

	/** set if ReceiveTick is implemented by blueprint */
	uint32 bImplementsReceiveTick : 1;
	
	/** set if ReceiveActivation is implemented by blueprint */
	uint32 bImplementsReceiveActivation : 1;

	/** set if ReceiveDeactivation is implemented by blueprint */
	uint32 bImplementsReceiveDeactivation : 1;

	virtual void OnBecomeRelevant(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) OVERRIDE;
	virtual void OnCeaseRelevant(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) OVERRIDE;
	virtual void TickNode(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) OVERRIDE;

	/** tick function */
	UFUNCTION(BlueprintImplementableEvent)
	virtual void ReceiveTick(AActor* OwnerActor, float DeltaSeconds);

	/** service became active */
	UFUNCTION(BlueprintImplementableEvent)
	virtual void ReceiveActivation(AActor* OwnerActor);

	/** service became inactive */
	UFUNCTION(BlueprintImplementableEvent)
	virtual void ReceiveDeactivation(AActor* OwnerActor);

	/** check if service is currently being active */
	UFUNCTION(BlueprintCallable, Category="AI|BehaviorTree")
	bool IsServiceActive() const;

	friend class FBehaviorBlueprintDetails;
};
