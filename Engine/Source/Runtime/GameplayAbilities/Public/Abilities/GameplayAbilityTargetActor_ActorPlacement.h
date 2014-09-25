// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayAbilityTargetActor_GroundTrace.h"
#include "GameplayAbilityWorldReticle_ActorVisualization.h"
#include "GameplayAbilityTargetActor_ActorPlacement.generated.h"

UCLASS(Blueprintable)
class AGameplayAbilityTargetActor_ActorPlacement : public AGameplayAbilityTargetActor_GroundTrace
{
	GENERATED_UCLASS_BODY()

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void StartTargeting(UGameplayAbility* InAbility) override;

	/** Actor we intend to place. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Targeting)
	UClass* TurretClass;		//Using a special class for replication purposes. (Not implemented yet)
	
	/** Override Material 0 on our turret's meshes with this material for visualization. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Projectile)
	UMaterialInterface *TurretMaterial;

	/** Visualization for the turret. */
	TWeakObjectPtr<AGameplayAbilityWorldReticle_ActorVisualization> TurretReticleActor;

protected:
	//Consider whether we want size-checking here, or in GroundTrace
	//virtual FHitResult PerformTrace(AActor* InSourceActor) const override;
};