// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "GameplayModCalculation.h"

UGameplayModCalculation::UGameplayModCalculation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

float UGameplayModCalculation::CalculateMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	return 0.f;
}
