//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#pragma once

#include "Components/ActorComponent.h"
#include "PhononGeometryComponent.generated.h"

/**
 * Phonon Geometry components are used to tag an actor as containing geometry relevant to acoustics calculations.
 * Usually placed on StaticMesh or Landscape actors.
 */
UCLASS(ClassGroup = (Audio), HideCategories = (Activation, Collision), meta = (BlueprintSpawnableComponent))
class UPhononGeometryComponent : public UActorComponent
{
	GENERATED_BODY()
}; 