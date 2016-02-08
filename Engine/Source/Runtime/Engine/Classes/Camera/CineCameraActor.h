// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CineCameraActor.generated.h"

/** 
 * A CineCameraActor is a CameraActor specialized to work like a cinematic camera.
 */
UCLASS(ClassGroup = Common, hideCategories = (Input, Rendering, AutoPlayerActivation), showcategories = ("Input|MouseInput", "Input|TouchInput"), Blueprintable)
class ENGINE_API ACineCameraActor : public ACameraActor
{
	GENERATED_BODY()

public:
	// Ctor
	ACineCameraActor(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
