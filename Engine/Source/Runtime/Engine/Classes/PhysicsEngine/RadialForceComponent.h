// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "RadialForceComponent.generated.h"

/**
 *	Used to emit a radial force or impulse that can affect physics objects and or destructible objects.
 */
UCLASS(hidecategories=(Object, Mobility), ClassGroup=Physics, showcategories=Trigger, HeaderGroup=Component, meta=(BlueprintSpawnableComponent), MinimalAPI)
class URadialForceComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

	/** The radius to apply the force or impulse in */
	UPROPERTY(interp, EditAnywhere, BlueprintReadWrite, Category=RadialForceComponent)
	float Radius;

	/** How the force or impulse should fall off as object are further away from the center */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=RadialForceComponent)
	TEnumAsByte<enum ERadialImpulseFalloff> Falloff;

	/** How strong the impulse should be */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Impulse)
	float ImpulseStrength;

	/** If true, the impulse will ignore mass of objects and will always result in a fixed velocity change */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Impulse)
	uint32 bImpulseVelChange:1;

	/** How strong the force should be */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Force)
	float ForceStrength;

	/** If > 0.f, will cause damage to destructible meshes as well  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Destructible)
	float DestructibleDamage;

	/** Is the force currently enabled */
	UPROPERTY()
	uint32 bForceEnabled_DEPRECATED:1;

	/** Fire a single impulse */
	UFUNCTION(BlueprintCallable, Category="Physics|Components|RadialForce")
	virtual void FireImpulse();

protected:
	// Begin UActorComponent interface.
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) OVERRIDE;
	// End UActorComponent interface.

	// Begin UObject interface.
	virtual void PostLoad() OVERRIDE;
	// End UObject interface.
};



