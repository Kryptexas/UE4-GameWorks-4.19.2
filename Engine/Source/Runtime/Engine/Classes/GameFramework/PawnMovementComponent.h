// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Movement component meant for use with Pawns.
 */

#pragma once
#include "GameFramework/NavMovementComponent.h"
#include "PawnMovementComponent.generated.h"

UCLASS(abstract)
class ENGINE_API UPawnMovementComponent : public UNavMovementComponent
{
	GENERATED_UCLASS_BODY()

	// Overridden to only allow registration with components owned by a Pawn.
	virtual void SetUpdatedComponent(class UPrimitiveComponent* NewUpdatedComponent) override;

	/**
	 * Adds the given vector to the accumulated input in world space. Input vectors are usually between 0 and 1 in magnitude. 
	 * They are accumulated during a frame then applied as acceleration during the movement update.
	 * @see Pawn.AddMovementInput()
	 *
	 * @param WorldDirection:	Direction in world space to apply input
	 * @param ScaleValue:		Scale to apply to input. This can be used for analog input, ie a value of 0.5 applies half the normal value.
	 * @param bForce:			If true always add the input, ignoring the result of IsMoveInputIgnored().
	 */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|PawnMovement")
	virtual void AddInputVector(FVector WorldVector, bool bForce = false);

	/** Return the input vector in world space. Note that the input should be consumed with ConsumeInputVector() at the end of an update, to prevent accumulation of control input between frames. */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|PawnMovement")
	FVector GetInputVector() const;

	/** Returns the input vector and resets it to zero. Should be used during an update to prevent accumulation of control input between frames. */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|PawnMovement")
	virtual FVector ConsumeInputVector();

	/** Helper to see if move input is ignored. If there is no Pawn or UpdatedComponent, returns true, otherwise defers to the Pawn's implementation of IsMoveInputIgnored(). */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|PawnMovement")
	virtual bool IsMoveInputIgnored() const;

	/** Return the Pawn that owns UpdatedComponent. */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|PawnMovement")
	class APawn* GetPawnOwner() const;

public:
	/** Notify of collision in case we want to react, such as waking up avoidance or pathing code. */
	virtual void NotifyBumpedPawn(APawn* BumpedPawn) {}

protected:

	/** Pawn which owns this component. */
	UPROPERTY()
	class APawn* PawnOwner;
};
