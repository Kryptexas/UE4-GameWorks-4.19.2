// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Landscape/LandscapeSplineProxies.h"
#include "Landscape/LandscapeSplineSegment.h"

ENGINE_API void HLandscapeSplineProxy_Tangent::Serialize(FArchive& Ar)
{
	Ar << SplineSegment;
}