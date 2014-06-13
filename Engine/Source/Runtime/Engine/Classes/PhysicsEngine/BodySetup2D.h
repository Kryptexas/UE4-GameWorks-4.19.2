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
	virtual void CreatePhysicsMeshes() override;
	virtual float GetVolume(const FVector& Scale) const override;
	// End of UBodySetup interface

#if WITH_EDITOR
	// UBodySetup interface
	virtual void InvalidatePhysicsData() override;
	// End of UBodySetup interface
#endif
};
