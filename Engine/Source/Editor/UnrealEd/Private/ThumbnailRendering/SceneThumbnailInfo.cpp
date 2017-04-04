// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ThumbnailRendering/SceneThumbnailInfo.h"


USceneThumbnailInfo::USceneThumbnailInfo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OrbitPitch = -11.25;
	OrbitYaw = -157.5;
	OrbitZoom = 0;
}

void USceneThumbnailInfo::ResetToDefault()
{
	const USceneThumbnailInfo* Default = GetDefault<USceneThumbnailInfo>();
	OrbitPitch = Default->OrbitPitch;
	OrbitYaw = Default->OrbitYaw;
	OrbitZoom = Default->OrbitZoom;
}

bool USceneThumbnailInfo::DiffersFromDefault() const
{
	const USceneThumbnailInfo* Default = GetDefault<USceneThumbnailInfo>();
	return OrbitPitch != Default->OrbitPitch ||
		OrbitYaw != Default->OrbitYaw ||
		OrbitZoom != Default->OrbitZoom;
}
