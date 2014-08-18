// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilitySet.h"


// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	UGameplayAbilitySet
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------


UGameplayAbilitySet::UGameplayAbilitySet(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

}

void UGameplayAbilitySet::GiveAbilities(UAbilitySystemComponent* AbilitySystemComponent) const
{
	for (const FGameplayAbilityBindInfo& BindInfo : Abilities)
	{
		if (BindInfo.GameplayAbilityClass)
		{
			AbilitySystemComponent->GiveAbility(BindInfo.GameplayAbilityClass->GetDefaultObject<UGameplayAbility>(), (int32)BindInfo.Command);
		}
	}
}
