// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BTService_BlueprintBase.generated.h"

UCLASS(Abstract, Blueprintable)
class ENGINE_API UBTService_BlueprintBase : public UBTService
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

	friend class FBehaviorBlueprintDetails;
};
