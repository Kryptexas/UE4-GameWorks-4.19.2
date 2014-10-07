// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "GameplayModCalculation.h"

FGameplayModCalculationSpecHandle FGameplayModCalculationSpecHandle::GenerateNewHandle()
{
	static int32 GHandleID = 0;
	FGameplayModCalculationSpecHandle NewHandle(GHandleID++);

	return NewHandle;
}

UGameplayModCalculation::UGameplayModCalculation(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{

}

FGameplayModCalculationSpecHandle UGameplayModCalculation::GetContextForSource(const FGameplayEffectSpec& Spec)
{
	return FGameplayModCalculationSpecHandle::GenerateNewHandle();
}

FGameplayModCalculationSpecHandle UGameplayModCalculation::GetContextForTarget(const FGameplayEffectSpec& Spec)
{
	return FGameplayModCalculationSpecHandle::GenerateNewHandle();
}