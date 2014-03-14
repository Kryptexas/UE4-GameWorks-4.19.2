// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NavAgentInterface.generated.h"

UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UNavAgentInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class INavAgentInterface
{
	GENERATED_IINTERFACE_BODY()

	/**
	 *	Retrieves FNavAgentProperties expressing navigation props and caps of represented agent
	 */
	virtual const struct FNavAgentProperties* GetNavAgentProperties() const { return NULL;} //PURE_VIRTUAL(INavAgentInterface::GetNavAgentProperties,return NULL;);

	/**
	 *	Retrieves Agent's location
	 */
	virtual FVector GetNavAgentLocation() const PURE_VIRTUAL(INavAgentInterface::GetNavAgentLocation,return FVector::ZeroVector;);
	
	/** Allow actor to specify additional offset (relative to NavLocation) when it's used as move goal */
	virtual FVector GetMoveGoalOffset(class AActor* MovingActor) const { return FVector::ZeroVector; }
	
	/** Get cylinder for testing if actor has been reached
	 * @param MoveOffset - destination (relative to actor's nav location)
	 * @param GoalOffset - cylinder center (relative to actor's nav location)
	 * @param GoalRadius - cylinder radius
	 * @param GoalHalfHeight - cylinder half height
	 */
	virtual void GetMoveGoalReachTest(class AActor* MovingActor, const FVector& MoveOffset, FVector& GoalOffset, float& GoalRadius, float& GoalHalfHeight) const {}
};
