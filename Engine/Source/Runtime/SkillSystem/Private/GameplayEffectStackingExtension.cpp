// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"

UGameplayEffectStackingExtension::UGameplayEffectStackingExtension(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	
}

FGameplayEffectSpec* UGameplayEffectStackingExtension::StackCustomEffects(TArray<FGameplayEffectSpec*>& CustomGameplayEffects)
{
	return NULL;
}