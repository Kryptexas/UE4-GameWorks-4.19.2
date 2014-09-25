// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "GameplayAbilityWorldReticle_ActorVisualization.generated.h"

class AGameplayAbilityTargetActor;

/** This is a dummy reticle for internal use by turret placement tasks. It builds a custom visual model of the turret being placed. */
UCLASS(notplaceable)
class AGameplayAbilityWorldReticle_ActorVisualization : public AGameplayAbilityWorldReticle
{
	GENERATED_UCLASS_BODY()

public:

	void InitializeReticleTurretInformation(AActor* TurretActor, UMaterialInterface *TurretMaterial);

	/** Hardcoded collision component, so other objects don't think they can collide with the visualization turret */
	UPROPERTY()
	TSubobjectPtr<class UCapsuleComponent> CollisionComponent;

	UPROPERTY()
	TArray<UActorComponent*> TurretComponents;

	/** Overridable function called whenever this actor is being removed from a level */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);

protected:
};
