// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BodyInstance2D.h"
#include "AggregateGeometry2D.h"

#include "BodySetup2D.generated.h"


UCLASS(DependsOn=UEngineTypes)
class PAPER2D_API UBodySetup2D : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	FAggregateGeometry2D AggGeom2D;

	/** Physical material to use for simple collision on this body. Encodes information about density, friction etc. */
	UPROPERTY(EditAnywhere, Category = Physics, meta = (DisplayName = "Simple Collision Physical Material"))
	class UPhysicalMaterial* PhysMaterial;

	/** Default properties of the body instance, copied into objects on instantiation */
	UPROPERTY(EditAnywhere, Category=Collision, meta=(FullyExpand = "true"))
	struct FBodyInstance2D DefaultInstance;

	/** Create Physics meshes (ConvexMeshes, TriMesh & TriMeshNegX) from cooked data */
	void CreatePhysicsMeshes();

	/** Calculates the mass. You can pass in the component where additional information is pulled from ( Scale, PhysMaterialOverride ) */
	float CalculateMass(const class UPrimitiveComponent2D* Component = nullptr) const;

	/** Returns the physics material used for this body. If none, specified, returns the default engine material. */
	class UPhysicalMaterial* GetPhysMaterial() const;

#if WITH_EDITOR
	/** Release Physics meshes (ConvexMeshes, TriMesh & TriMeshNegX). Must be called before the BodySetup is destroyed */
	void InvalidatePhysicsData();
#endif

protected:
	/** Physics triangle mesh, created from cooked data in CreatePhysicsMeshes */
	//class PxTriangleMesh* TriMesh;
};
