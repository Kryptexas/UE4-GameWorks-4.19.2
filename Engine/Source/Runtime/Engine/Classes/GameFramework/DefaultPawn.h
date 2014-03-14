// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// DefaultPawns are simple pawns that can fly around the world.
//
//=============================================================================

#pragma once
#include "DefaultPawn.generated.h"

UCLASS(HeaderGroup=Pawn, config=Game, Blueprintable, BlueprintType)
class ENGINE_API ADefaultPawn : public APawn
{
	GENERATED_UCLASS_BODY()

	// Begin Pawn overrides
	virtual class UPawnMovementComponent* GetMovementComponent() const OVERRIDE { return MovementComponent; }
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) OVERRIDE;
	// End Pawn overrides

	/** Input callback to move forward. */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	virtual void MoveForward(float Val);

	/** Input callback to strafe right. */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	virtual void MoveRight(float Val);

	/** Input callback to move up in world space. */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	virtual void MoveUp_World(float Val);

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	void TurnAtRate(float Rate);

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pawn")
	float BaseTurnRate;

private:
	/** Input callback on pitch (look up) input. */
	UFUNCTION(BlueprintCallable, Category="Pawn", meta=(DeprecatedFunction, DeprecationMessage="Use Pawn.AddControllerPitchInput instead."))
	virtual void LookUp(float Val);

	/** Input callback on yaw (turn) input. */
	UFUNCTION(BlueprintCallable, Category="Pawn", meta=(DeprecatedFunction, DeprecationMessage="Use Pawn.AddControllerYawInput instead."))
	virtual void Turn(float Val);

public:
	/** Name of the MovementComponent.  Use this name if you want to use a different class (with PCIP.SetDefaultSubobjectClass). */
	static FName MovementComponentName;

	/** DefaultPawn movement component */
	UPROPERTY(Category=Pawn, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<class UFloatingPawnMovement> MovementComponent;

	/** Name of the CollisionComponent. */
	static FName CollisionComponentName;

	/** DefaultPawn collision component */
	UPROPERTY(Category=Pawn, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<class USphereComponent> CollisionComponent;

	/** Name of the MeshComponent. Use this name if you want to prevent creation of the component (with PCIP.DoNotCreateDefaultSubobject). */
	static FName MeshComponentName;

	/** The mesh associated with this Pawn. */
	UPROPERTY(Category=Pawn, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<class UStaticMeshComponent> MeshComponent;

	/** If true, adds default bindings for movement and camera look. */
	UPROPERTY(Category=Pawn, EditAnywhere, BlueprintReadOnly)
	uint32 bAddDefaultMovementBindings:1;
};

