// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "PhysicsThrusterComponent.generated.h"

/** 
 *	Used with objects that have physics to apply a force down the negative-X direction
 *	ie. point X in the direction you want the thrust in.
 */
UCLASS(hidecategories=(Object, Mobility, LOD), ClassGroup=Physics, showcategories=Trigger, HeaderGroup=Component, meta=(BlueprintSpawnableComponent))
class UPhysicsThrusterComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

	/** If thrust should be applied at the moment. */
	UPROPERTY()
	uint32 bThrustEnabled_DEPRECATED:1;

	/** Strength of thrust force applied to the base object. */
	UPROPERTY(BlueprintReadWrite, interp, Category=Physics)
	float ThrustStrength;


	// Begin UActorComponent Interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) OVERRIDE;
	// End UActorComponent Interface

	// Begin UObject interface.
	virtual void PostLoad() OVERRIDE;
	// End UObject interface.
};



