// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayEffectCalculation.h"
#include "GameplayEffect.h"
#include "GameplayModMagnitudeCalculation.generated.h"

// Forward declarations
struct FGameplayEffectSpec;
class UAbilitySystemComponent;

/** Class used to perform custom gameplay effect modifier calculations, either via blueprint or native code */ 
UCLASS(BlueprintType, Blueprintable, Abstract)
class GAMEPLAYABILITIES_API UGameplayModMagnitudeCalculation : public UGameplayEffectCalculation
{

public:
	GENERATED_UCLASS_BODY()

	/**
	 * Calculate the magnitude of the gameplay effect modifier, given the specified spec
	 * 
	 * @param Spec	Gameplay effect spec to use to calculate the magnitude with
	 * 
	 * @return Computed magnitude of the modifier
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Calculation")
	float CalculateMagnitude(const FGameplayEffectSpec& Spec) const;
};