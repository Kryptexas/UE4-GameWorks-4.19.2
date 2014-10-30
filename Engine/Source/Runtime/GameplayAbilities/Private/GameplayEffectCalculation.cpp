// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "GameplayEffectCalculation.h"

UGameplayEffectCalculation::UGameplayEffectCalculation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UGameplayEffectCalculation::Execute(FGameplayEffectSpec& Spec, int32 ExecutionIdx, UAbilitySystemComponent* TargetAbilitySystemComponent) const
{
	
}

void UGameplayEffectCalculation::GetCaptureDefinitions(OUT TArray<FGameplayEffectAttributeCaptureDefinition>& Definitions) const
{

}