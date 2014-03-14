// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SpringArmComponent.generated.h"

/**
 * This component tried to maintain its children at a fixed distance from the parent,
 * but will retract the children if there is a collision, and spring back when there is no collision.
 *
 * Example: Use as a 'camera boom' to keep the follow camera for a player from colliding into the world.
 */

UCLASS(HeaderGroup=Component, ClassGroup=Camera, meta=(BlueprintSpawnableComponent), hideCategories=(Mobility))
class ENGINE_API USpringArmComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

	/** Natural length of the spring arm when there are no collisions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Camera)
	float TargetArmLength;

	/** offset at end of spring arm; use this instead of the relative offset of the attached component to ensure the line trace works as desired */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Camera)
	FVector SocketOffset;

	/** How big should the query probe sphere be (in unreal units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraCollision, meta=(editcondition="bDoCollisionTest"))
	float ProbeSize;

	/** Collision channel of the query probe (defaults to ECC_Camera) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraCollision, meta=(editcondition="bDoCollisionTest"))
	TEnumAsByte<ECollisionChannel> ProbeChannel;

	/** If true, do a collision test using ProbeChannel and ProbeSize to prevent camera clipping into level.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraCollision)
	uint32 bDoCollisionTest:1;

	/** If this component is placed on a pawn, should it use the view rotation of the pawn where possible? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraSettings)
	uint32 bUseControllerViewRotation:1;

	// UActorComponent interface
	virtual void OnRegister() OVERRIDE;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) OVERRIDE;
	// End of UActorComponent interface

	// USceneComponent interface
	virtual bool HasAnySockets() const OVERRIDE;
	virtual FTransform GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace = RTS_World) const OVERRIDE;
	virtual void QuerySupportedSockets(TArray<FComponentSocketDescription>& OutSockets) const OVERRIDE;
	// End of USceneComponent interface

	/** The name of the socket at the end of the spring arm (looking back towards the spring arm origin) */
	static const FName SocketName;
protected:
	/** Cached component-space socket location */
	FVector RelativeSocketLocation;

protected:
	/** Updates the desired arm location, calling BlendLocations to do the actual blending if a trace is done */
	virtual void UpdateDesiredArmLocation(const FVector& Origin, const FRotator& Direction, bool bDoTrace, float DeltaTime);

	/**
	 * This function allows subclasses to blend the trace hit location with the desired arm location;
	 * by default it returns bHitSomething ? TraceHitLocation : DesiredArmLocation
	 */
	virtual FVector BlendLocations(const FVector& DesiredArmLocation, const FVector& TraceHitLocation, bool bHitSomething, float DeltaTime);
};
