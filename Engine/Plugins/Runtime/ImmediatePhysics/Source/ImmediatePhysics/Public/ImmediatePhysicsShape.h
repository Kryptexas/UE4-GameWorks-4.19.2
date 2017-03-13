// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#if WITH_PHYSX
#include "PhysXPublic.h"
#endif

namespace ImmediatePhysics
{
/** Holds shape data*/
struct FShape
{
#if WITH_PHYSX
	const PxTransform LocalTM;
	const PxGeometry* Geometry;
	const float BoundsMagnitude;

	FShape(const PxTransform& InLocalTM, const PxGeometry* InGeometry, const float InBoundsMagnitude)
		: LocalTM(InLocalTM)
		, Geometry(InGeometry)
		, BoundsMagnitude(InBoundsMagnitude)
	{
	}
#endif
};

}