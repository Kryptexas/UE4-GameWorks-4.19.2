// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilityTargetActor_ActorPlacement.h"
#include "GameplayAbilityWorldReticle_ActorVisualization.h"

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	AGameplayAbilityTargetActor_ActorPlacement
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

AGameplayAbilityTargetActor_ActorPlacement::AGameplayAbilityTargetActor_ActorPlacement(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void AGameplayAbilityTargetActor_ActorPlacement::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TurretReticleActor.IsValid())
	{
		TurretReticleActor.Get()->Destroy();
	}

	Super::EndPlay(EndPlayReason);
}

void AGameplayAbilityTargetActor_ActorPlacement::StartTargeting(UGameplayAbility* InAbility)
{
	Super::StartTargeting(InAbility);
	if (AActor *TurretActor = GetWorld()->SpawnActor(TurretClass))
	{
		TurretReticleActor = GetWorld()->SpawnActor<AGameplayAbilityWorldReticle_ActorVisualization>();
		TurretReticleActor->InitializeReticleTurretInformation(TurretActor, TurretMaterial);
		GetWorld()->DestroyActor(TurretActor);
	}
	if (AGameplayAbilityWorldReticle* CachedReticleActor = ReticleActor.Get())
	{
		TurretReticleActor->AttachRootComponentToActor(CachedReticleActor);
	}
	else
	{
		ReticleActor = TurretReticleActor;
		TurretReticleActor = nullptr;
	}
}

//Might want to override this function to allow for a radius check against the ground, possibly including a height check. Or might want to do it in ground trace.
//FHitResult AGameplayAbilityTargetActor_ActorPlacement::PerformTrace(AActor* InSourceActor) const
