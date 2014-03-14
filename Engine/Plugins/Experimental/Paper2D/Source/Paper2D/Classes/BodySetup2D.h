// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BodyInstance2D.h"
#include "BodySetup2D.generated.h"

UCLASS(DependsOn=UEngineTypes)
class PAPER2D_API UBodySetup2D : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Default properties of the body instance, copied into objects on instantiation */
	UPROPERTY(EditAnywhere, Category=Collision, meta=(FullyExpand = "true"))
	struct FBodyInstance2D DefaultInstance;


protected:
	/** Physics triangle mesh, created from cooked data in CreatePhysicsMeshes */
	//class PxTriangleMesh* TriMesh;

};
