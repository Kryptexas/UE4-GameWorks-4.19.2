// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

UWorldThumbnailInfo::UWorldThumbnailInfo(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	CameraMode = ECameraProjectionMode::Orthographic;
	OrthoDirection = EOrthoThumbnailDirection::Top;
}
