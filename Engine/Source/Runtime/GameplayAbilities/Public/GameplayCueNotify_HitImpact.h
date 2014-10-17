// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayCueNotify_HitImpact.generated.h"

/**
 *	Non instanced GAmeplayCueNotify for spawning particle and sound FX.
 *	Still WIP - needs to be fleshed out more.
 */

UCLASS()
class GAMEPLAYABILITIES_API UGameplayCueNotify_HitImpact : public UGameplayCueNotify
{
	GENERATED_UCLASS_BODY()

	/** Does this GameplayCueNotify handle this type of GameplayCueEvent? */
	virtual bool HandlesEvent(EGameplayCueEvent::Type EventType) const;

	/** Does this GameplayCueNotify need a per source actor instance for this event? */
	virtual bool NeedsInstanceForEvent(EGameplayCueEvent::Type EventType) const;
	
	virtual void HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, FGameplayCueParameters Parameters) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayCue)
	USoundBase* Sound;

	/** Effects to play for weapon attacks against specific surfaces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayCue)
	UParticleSystem* ParticleSystem;
};
