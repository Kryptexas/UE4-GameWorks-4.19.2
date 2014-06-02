// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AggregateGeometry2D.h"

#include "BodySetup2D.generated.h"

UCLASS(DependsOn=UEngineTypes)
class ENGINE_API UBodySetup2D : public UBodySetup
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()

	FAggregateGeometry2D AggGeom2D;

	// UBodySetup interface
	virtual void CreatePhysicsMeshes() OVERRIDE;
	virtual float GetVolume(const FVector& Scale) const OVERRIDE;
	// End of UBodySetup interface

#if WITH_EDITOR
	// UBodySetup interface
	virtual void InvalidatePhysicsData() OVERRIDE;
	// End of UBodySetup interface
#endif
};
