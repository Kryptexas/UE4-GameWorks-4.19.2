// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "LandscapePrivatePCH.h"
#include "LandscapeComponent.h"

FName FWeightmapLayerAllocationInfo::GetLayerName() const
{
	if (LayerInfo)
	{
		return LayerInfo->LayerName;
	}
	return NAME_None;
}
