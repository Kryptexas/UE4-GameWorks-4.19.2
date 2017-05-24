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
	const PxVec3 BoundsOffset;
	const float BoundsMagnitude;

	FShape(const PxTransform& InLocalTM, const PxVec3& InBoundsOffset, const float InBoundsMagnitude, const PxGeometry* InGeometry)
		: LocalTM(InLocalTM)
		, Geometry(InGeometry)
		, BoundsOffset(InBoundsOffset)
		, BoundsMagnitude(InBoundsMagnitude)
	{
	}
#endif
};

}