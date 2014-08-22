// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
#include "AbilitySystemInterface.generated.h"

class UAbilitySystemComponent;

/** Interface for actors that wish to handle GameplayCue events from GameplayEffects */
UINTERFACE(MinimalAPI)
class UAbilitySystemInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class GAMEPLAYABILITIES_API IAbilitySystemInterface
{
	GENERATED_IINTERFACE_BODY()

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const = 0;
};