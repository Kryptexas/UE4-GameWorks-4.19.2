// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "NavigationObjectBase.generated.h"

UCLASS(hidecategories=(Lighting, LightColor, Force), ClassGroup=Navigation, NotBlueprintable, abstract)
class ENGINE_API ANavigationObjectBase : public AActor, public INavAgentInterface
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
	virtual void GetSimpleCollisionCylinder(float& CollisionRadius, float& CollisionHalfHeight) const override;

#if	WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
	virtual void PostEditUndo() override;
#endif	// WITH_EDITOR
	// End AActor Interface

	/** @todo document */
	virtual bool ShouldBeBased();
	/** @todo document */
	virtual void FindBase();

	/** 
	 * Check to make sure the navigation is at a valid point. 
	 * Detect when path building is going to move a pathnode around
	 * This may be undesirable for LDs (ie b/c cover links define slots by offsets)
	 */
	virtual void Validate();

	/** Return Physics Volume for this Actor. **/
	class APhysicsVolume* GetNavPhysicsVolume();

	// INavAgentInterface start
	virtual FVector GetNavAgentLocation() const { return GetActorLocation(); }
	virtual void GetMoveGoalReachTest(class AActor* MovingActor, const FVector& MoveOffset, FVector& GoalOffset, float& GoalRadius, float& GoalHalfHeight) const;
	// INavAgentInterface end
};



