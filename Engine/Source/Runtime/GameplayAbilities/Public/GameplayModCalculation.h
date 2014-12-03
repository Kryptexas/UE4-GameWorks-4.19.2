// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayModCalculation.generated.h"

// Forward declarations
struct FGameplayEffectSpec;
class UAbilitySystemComponent;

UCLASS(BlueprintType, Blueprintable, Abstract)
class GAMEPLAYABILITIES_API UGameplayModCalculation : public UObject
{

public:
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintNativeEvent, Category="Calculation")
	float CalculateMagnitude(const FGameplayEffectSpec& Spec) const;
};