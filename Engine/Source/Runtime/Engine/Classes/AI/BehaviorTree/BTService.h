// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTreeTypes.h"
#include "BTService.generated.h"

/** 
 *	Behavior Tree service nodes is designed to perform "background" tasks
 *	that update AI's knowledge.
 *
 *  First tick will always happen on composite's activation,
 *  just before running child node.
 *	
 *	BT service should never modify the parent BT's state or flow. From BT point
 *	of view services are fire-and-forget calls and no service-state-checking
 *	is being performed. If you need this data create a BTDecorator to check this
 *  information.
 */
UCLASS(Abstract)
class ENGINE_API UBTService : public UBTAuxiliaryNode
{
	GENERATED_UCLASS_BODY()

	virtual FString GetStaticDescription() const OVERRIDE;

protected:

	/** defines time span between subsequent ticks of the service */
	UPROPERTY(Category=Service, EditAnywhere, meta=(ClampMin="0.001"))
	float Interval;

	/** allows adding random time to service's Interval */
	UPROPERTY(Category=Service, EditAnywhere, meta=(ClampMin="0.0"))
	float RandomDeviation;

	/** update next tick interval */
	virtual void TickNode(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const OVERRIDE;

	/** called when auxiliary node becomes active */
	virtual void OnBecomeRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
};
