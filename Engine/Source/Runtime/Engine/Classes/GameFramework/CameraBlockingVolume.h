// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CameraBlockingVolume.generated.h"

/**
 * A volume which blocks the Camera channel by default.
 */
UCLASS()
class ENGINE_API ACameraBlockingVolume : public AVolume
{
	GENERATED_BODY()
public:
	ACameraBlockingVolume(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};



