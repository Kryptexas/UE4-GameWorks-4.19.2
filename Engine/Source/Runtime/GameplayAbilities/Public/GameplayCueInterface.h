// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
#include "GameplayCueInterface.generated.h"

/** Interface for actors that wish to handle GameplayCue events from GameplayEffects */
UINTERFACE(MinimalAPI)
class UGameplayCueInterface: public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class GAMEPLAYABILITIES_API IGameplayCueInterface
{
	GENERATED_IINTERFACE_BODY()

	// EVENT
	UFUNCTION()
	virtual void GameplayCueActivated(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude, const FGameplayEffectInstigatorContext InstigatorContext) = 0;

	UFUNCTION()
	virtual void GameplayCueExecuted(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude, const FGameplayEffectInstigatorContext InstigatorContext) = 0;


	// STATE
	UFUNCTION()
	virtual void GameplayCueAdded(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude, const FGameplayEffectInstigatorContext InstigatorContext) = 0;

	UFUNCTION()
	virtual void GameplayCueRemoved(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude, const FGameplayEffectInstigatorContext InstigatorContext) = 0;
};