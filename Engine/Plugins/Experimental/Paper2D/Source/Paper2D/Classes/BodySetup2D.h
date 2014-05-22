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

	/** Default properties of the body instance, copied into objects on instantiation */
	UPROPERTY(EditAnywhere, Category=Collision, meta=(FullyExpand = "true"))
	struct FBodyInstance2D DefaultInstance;

	/** Create Physics meshes (ConvexMeshes, TriMesh & TriMeshNegX) from cooked data */
	void CreatePhysicsMeshes();


#if WITH_EDITOR
	/** Release Physics meshes (ConvexMeshes, TriMesh & TriMeshNegX). Must be called before the BodySetup is destroyed */
	void InvalidatePhysicsData();
#endif

protected:
	/** Physics triangle mesh, created from cooked data in CreatePhysicsMeshes */
	//class PxTriangleMesh* TriMesh;
};
