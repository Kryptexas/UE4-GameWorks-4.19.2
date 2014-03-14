// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Movement component updates position of associated PrimitiveComponent during its tick
 */

#pragma once
#include "SpectatorPawnMovement.generated.h"

UCLASS(HeaderGroup=Component, DependsOn=UFloatingPawnMovement)
class ENGINE_API USpectatorPawnMovement : public UFloatingPawnMovement
{
	GENERATED_UCLASS_BODY()

	/** If true, component moves at full speed no matter the time dilation. Default is false. */
	UPROPERTY()
	uint32 bIgnoreTimeDilation:1;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) OVERRIDE;
};

