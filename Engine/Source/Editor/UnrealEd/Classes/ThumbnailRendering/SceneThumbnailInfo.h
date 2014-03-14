// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Thumbnail information for assets that need a scene
 */

#pragma once
#include "SceneThumbnailInfo.generated.h"


UCLASS(MinimalAPI)
class USceneThumbnailInfo : public UThumbnailInfo
{
	GENERATED_UCLASS_BODY()

	/** The pitch of the orbit camera around the asset */
	UPROPERTY()
	float OrbitPitch;

	/** The yaw of the orbit camera around the asset */
	UPROPERTY()
	float OrbitYaw;

	/** The offset from the bounds sphere distance from the asset */
	UPROPERTY()
	float OrbitZoom;
};
