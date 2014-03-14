// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Movement component meant for use with Pawns.
 */

#pragma once
#include "PawnMovementComponent.generated.h"

UCLASS(HeaderGroup=Component, abstract)
class ENGINE_API UPawnMovementComponent : public UNavMovementComponent
{
	GENERATED_UCLASS_BODY()

	// Overridden to only allow registration with components owned by a Pawn.
	virtual void SetUpdatedComponent(class UPrimitiveComponent* NewUpdatedComponent) OVERRIDE;

	// Overridden to also call StopActiveMovement().
	virtual void StopMovementImmediately() OVERRIDE;

	/** Stops applying further movement (usually zeros acceleration). */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|PawnMovement")
	virtual void StopActiveMovement();

	/** Adds the given vector to the accumulated input in world space. Input vectors are usually between 0 and 1 in magnitude. 
	 * They are accumulated during a frame then applied as acceleration during the movement update.
	 * if bForce is true, it bypasses the IsMoveInputIgnored() check. */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|PawnMovement")
	virtual void AddInputVector(FVector WorldVector, bool bForce = false);

	/** Return the input vector in world space. Note that the input should be consumed with ConsumeInputVector() at the end of an update, to prevent accumulation of control input between frames. */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|PawnMovement")
	FVector GetInputVector() const;

	/** Returns the input vector and resets it to zero. Should be used during an update to prevent accumulation of control input between frames. */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|PawnMovement")
	virtual FVector ConsumeInputVector();

	/** Helper to see if move input is ignored. If possessed by a player-controlled Pawn, defers to the PlayerController's input ignore flags. */
	virtual bool IsMoveInputIgnored() const;

	/** Return the Pawn that owns UpdatedComponent. */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|PawnMovement")
	class APawn* GetPawnOwner() const;

public:
	/** Notify of collision in case we want to react, such as waking up avoidance or pathing code. */
	virtual void NotifyBumpedPawn(APawn* BumpedPawn) {}

protected:
	/** Accumulated control input vector, stored in world space.  */
	UPROPERTY(Transient)
	FVector ControlInputVector;

	/** Pawn which owns this component. */
	UPROPERTY()
	class APawn* PawnOwner;
};


//////////////////////////////////////////////////////////////////////////
// Inlines

inline void UPawnMovementComponent::StopMovementImmediately()
{
	Super::StopMovementImmediately();
	StopActiveMovement();
}

inline void UPawnMovementComponent::StopActiveMovement()
{
}

inline FVector UPawnMovementComponent::GetInputVector() const
{
	return ControlInputVector;
}

inline class APawn* UPawnMovementComponent::GetPawnOwner() const
{
	return PawnOwner;
}
