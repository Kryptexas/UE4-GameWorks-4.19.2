// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
#include "AbilitySystemInterface.generated.h"

class UAbilitySystemComponent;

/** Interface for actors that expose access to an ability system component */
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UAbilitySystemInterface : public UInterface
{
	GENERATED_BODY()
public:
	GAMEPLAYABILITIES_API UAbilitySystemInterface(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

class GAMEPLAYABILITIES_API IAbilitySystemInterface
{
	GENERATED_BODY()
public:

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const = 0;
};
