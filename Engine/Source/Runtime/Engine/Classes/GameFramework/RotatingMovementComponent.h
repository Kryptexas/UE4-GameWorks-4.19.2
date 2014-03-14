// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "RotatingMovementComponent.generated.h"

/**
 * Movement component updates position of associated PrimitiveComponent during its tick.
 */
UCLASS(HeaderGroup=Component, ClassGroup=Movement, meta=(BlueprintSpawnableComponent), HideCategories=(PlanarMovement, "Components|Movement|Planar", Velocity))
class ENGINE_API URotatingMovementComponent : public UMovementComponent
{
	GENERATED_UCLASS_BODY()

	/** How fast to update roll/pitch/yaw of UpdatedComponent */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=RotatingComponent)
	FRotator RotationRate;

	/** Translation of pivot point. Always relative to current rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=RotatingComponent)
	FVector PivotTranslation;

	/** Whether rotation is applied in local or world space. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=RotatingComponent)
	uint32 bRotationInLocalSpace:1;

	//Begin UActorComponent Interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) OVERRIDE;
	//End UActorComponent Interface
};



