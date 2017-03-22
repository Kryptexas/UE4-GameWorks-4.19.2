//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#pragma once

#include "Components/SceneComponent.h"
#include "PhononSourceComponent.generated.h"

/**
 * Phonon Source Components can be placed alongside statically positioned Audio Components in order to bake impulse response data
 * to be applied at runtime.
 */
UCLASS(ClassGroup = (Audio), meta = (BlueprintSpawnableComponent), HideCategories = (Activation, Collision, Tags, Rendering, Physics, LOD))
class PHONON_API UPhononSourceComponent : public USceneComponent
{
	GENERATED_BODY()

public:

	// Any acoustic probes that lie within the bake radius will be used to produce baked impulse response data for this source location.
	UPROPERTY(EditAnywhere, Category = Baking)
	float BakeRadius = 1.0f;
};