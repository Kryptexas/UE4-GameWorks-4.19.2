// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Movement component that is compatible with the navigation system's PathFollowingComponent
 */

#pragma once
#include "AI/Navigation/NavAgentInterface.h"
#include "NavMovementComponent.generated.h"

UCLASS(HeaderGroup=Component, abstract, dependson=(UNavigationSystem, UPrimitiveComponent, INavAgentInterface))
class ENGINE_API UNavMovementComponent : public UMovementComponent
{
	GENERATED_UCLASS_BODY()

	/** Properties that define how navigation can interact with this component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MovementComponent)
	FNavAgentProperties NavAgentProps;

	/** Expresses runtime state of character's movement. Put all temporal changes to movement properties here */
	UPROPERTY()
	FMovementProperties MovementState;

	/** associated path following component */
	TWeakObjectPtr<class UPathFollowingComponent> PathFollowingComp;

	/** @returns location of controlled actor (on ground level) */
	FORCEINLINE FVector GetActorLocation() const { return UpdatedComponent ? (UpdatedComponent->GetComponentLocation() - FVector(0,0,UpdatedComponent->Bounds.BoxExtent.Z)) : FVector::ZeroVector; }
	/** @returns based location of controlled actor */
	virtual FBasedPosition GetActorLocationBased() const;
	/** @returns navigation location of controlled actor */
	FORCEINLINE FVector GetActorNavLocation() const { INavAgentInterface* MyOwner = InterfaceCast<INavAgentInterface>(GetOwner()); return MyOwner ? MyOwner->GetNavAgentLocation() : FVector::ZeroVector; }

	/** request direct movement */
	virtual void RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed);

	/** check if current move target can be reached right now if positions are matching
	 *  (e.g. performing scripted move and can't stop) */
	virtual bool CanStopPathFollowing() const;

	/** @returns the NavAgentProps(const) */
	FORCEINLINE const FNavAgentProperties* GetNavAgentProperties() const { return &NavAgentProps; }
	/** @returns the NavAgentProps */
	FORCEINLINE FNavAgentProperties* GetNavAgentProperties() { return &NavAgentProps; }

	/** Resets runtime movement state to character's movement capabilities */
	void ResetMoveState() { MovementState = NavAgentProps; }


	/** @return true if component can crouch */
	FORCEINLINE bool CanEverCrouch() const { return NavAgentProps.bCanCrouch; }

	/** @return true if component can jump */
	FORCEINLINE bool CanEverJump() const { return NavAgentProps.bCanJump; }

	/** @return true if component can move along the ground (walk, drive, etc) */
	FORCEINLINE bool CanEverMoveOnGround() const { return NavAgentProps.bCanWalk; }

	/** @return true if component can swim */
	FORCEINLINE bool CanEverSwim() const { return NavAgentProps.bCanSwim; }

	/** @return true if component can fly */
	FORCEINLINE bool CanEverFly() const { return NavAgentProps.bCanFly; }

	/** @return true if component is allowed to jump */
	FORCEINLINE bool IsJumpAllowed() const { return NavAgentProps.bCanJump && MovementState.bCanJump; }

	/** @param bAllowed true if component is allowed to jump */
	FORCEINLINE void SetJumpAllowed(bool bAllowed) { MovementState.bCanJump = bAllowed; }


	/** @return true if currently crouching */ 
	UFUNCTION(BlueprintCallable, Category="AI|Components|NavMovement")
	virtual bool IsCrouching() const;

	/** @return true if currently falling (not flying, in a non-fluid volume, and not on the ground) */ 
	UFUNCTION(BlueprintCallable, Category="AI|Components|NavMovement")
	virtual bool IsFalling() const;

	/** @return true if currently moving on the ground (e.g. walking or driving) */ 
	UFUNCTION(BlueprintCallable, Category="AI|Components|NavMovement")
	virtual bool IsMovingOnGround() const;
	
	/** @return true if currently swimming (moving through a fluid volume) */
	UFUNCTION(BlueprintCallable, Category="AI|Components|NavMovement")
	virtual bool IsSwimming() const;

	/** @return true if currently flying (moving through a non-fluid volume without resting on the ground) */
	UFUNCTION(BlueprintCallable, Category="AI|Components|NavMovement")
	virtual bool IsFlying() const;
};


inline bool UNavMovementComponent::IsCrouching() const
{
	return false;
}

inline bool UNavMovementComponent::IsFalling() const
{
	return false;
}

inline bool UNavMovementComponent::IsMovingOnGround() const
{
	return false;
}

inline bool UNavMovementComponent::IsSwimming() const
{
	return false;
}

inline bool UNavMovementComponent::IsFlying() const
{
	return false;
}

