// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "NavigationObjectBase.generated.h"

UCLASS(hidecategories=(Lighting, LightColor, Force), ClassGroup=Navigation,MinimalAPI, NotBlueprintable, abstract)
class ANavigationObjectBase : public AActor, public INavAgentInterface
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TSubobjectPtr<class UCapsuleComponent> CapsuleComponent;

	/** Normal editor sprite. */
	UPROPERTY()
	TSubobjectPtr<class UBillboardComponent> GoodSprite;

	/** Used to draw bad collision intersection in editor. */
	UPROPERTY()
	TSubobjectPtr<class UBillboardComponent> BadSprite;

	/** True if this nav point was spawned to be a PIE player start. */
	UPROPERTY()
	uint32 bIsPIEPlayerStart:1;


	// Begin AActor Interface
	ENGINE_API virtual void GetSimpleCollisionCylinder(float& CollisionRadius, float& CollisionHalfHeight) const OVERRIDE;

#if	WITH_EDITOR
	ENGINE_API virtual void PostEditMove(bool bFinished) OVERRIDE;
	ENGINE_API virtual void PostEditUndo() OVERRIDE;
#endif	// WITH_EDITOR
	// End AActor Interface

	/** @todo document */
	ENGINE_API virtual bool ShouldBeBased();
	/** @todo document */
	ENGINE_API virtual void FindBase();

	/** 
	 * Check to make sure the navigation is at a valid point. 
	 * Detect when path building is going to move a pathnode around
	 * This may be undesirable for LDs (ie b/c cover links define slots by offsets)
	 */
	ENGINE_API virtual void Validate();

	/** Return Physics Volume for this Actor. **/
	ENGINE_API class APhysicsVolume* GetNavPhysicsVolume();

	// INavAgentInterface start
	ENGINE_API virtual FVector GetNavAgentLocation() const { return GetActorLocation(); }
	ENGINE_API virtual void GetMoveGoalReachTest(class AActor* MovingActor, const FVector& MoveOffset, FVector& GoalOffset, float& GoalRadius, float& GoalHalfHeight) const;
	// INavAgentInterface end
};



