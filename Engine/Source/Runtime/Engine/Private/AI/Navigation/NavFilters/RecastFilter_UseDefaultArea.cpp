// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#if WITH_RECAST
#include "DetourNavMeshQuery.h"
#endif // WITH_RECAST

URecastFilter_UseDefaultArea::URecastFilter_UseDefaultArea(const class FPostConstructInitializeProperties& PCIP) 
	: Super(PCIP)
{
#if WITH_RECAST
	QueryFilter->SetFilterImplementation((const INavigationQueryFilterInterface*)ARecastNavMesh::GetNamedFilter(ERecastNamedFilter::FilterOutAreas));
#endif // WITH_RECAST
}
