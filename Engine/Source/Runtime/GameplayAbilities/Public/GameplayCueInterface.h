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

	UFUNCTION(BlueprintImplementableEvent, Category = "GameplayCue")
	void HandleGameplayCue(FGameplayTag Tag, EGameplayCueEvent::Type CueType);

	// EVENT -----------------------

	/** EGameplayCueEvent::OnActive - Called when GameplayCue is really activated (not called again in relevancy, Join In Progress situations) */
	UFUNCTION()
	virtual void GameplayCueActivated(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude, const FGameplayEffectInstigatorContext InstigatorContext) = 0;

	/** EGameplayCueEvent::Executed - Called when GameplayCue is executed (instant effect or periodic effect ticks) */
	UFUNCTION()
	virtual void GameplayCueExecuted(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude, const FGameplayEffectInstigatorContext InstigatorContext) = 0;

	/** EGameplayCueEvent::Removed - Called when GameplayCue is removed */
	UFUNCTION()
	virtual void GameplayCueRemoved(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude, const FGameplayEffectInstigatorContext InstigatorContext) = 0;


	// STATE -----------------------

	/** EGameplayCueEvent::WhileActive - Called when GameplayCue is added locally (e.g., casted for the first time or in relevancy / Join in Progress situations) */
	UFUNCTION()
	virtual void GameplayCueAdded(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude, const FGameplayEffectInstigatorContext InstigatorContext) = 0;
};