// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTreeTypes.h"
#include "BTAuxiliaryNode.generated.h"

struct FBTAuxiliaryMemory
{
	float NextTickRemainingTime;
};

UCLASS(Abstract)
class ENGINE_API UBTAuxiliaryNode : public UBTNode
{
	GENERATED_UCLASS_BODY()

	/** called when node becomes active */
	void ConditionalOnBecomeRelevant(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const;

	/** called when node becomes inactive */
	void ConditionalOnCeaseRelevant(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const;

	/** tick function */
	void ConditionalTickNode(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const;

	virtual void DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const OVERRIDE;

	/** additional memory for aux node, so any decorators/services (especially the game specific ones)
	  * don't have to inherit their memory structs from FBTAuxiliaryMemory */
	uint16 GetInstanceAuxMemorySize() const;

protected:

	/** if set, OnBecomeRelevant will be used */
	uint8 bNotifyBecomeRelevant:1;

	/** if set, OnCeaseRelevant will be used */
	uint8 bNotifyCeaseRelevant:1;

	/** if set, OnTick will be used */
	uint8 bNotifyTick : 1;

	/** if set, conditional tick will use remaining time form node's memory */
	uint8 bTickIntervals : 1;

	/** called when auxiliary node becomes active */
	virtual void OnBecomeRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const;

	/** called when auxiliary node becomes inactive */
	virtual void OnCeaseRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const;

	/** tick function */
	virtual void TickNode(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const;

	/** sets next tick time */
	void SetNextTickTime(uint8* NodeMemory, float RemainingTime) const;

	/** get aux node memory */
	struct FBTAuxiliaryMemory* GetAuxNodeMemory(uint8* NodeMemory) const;
};
