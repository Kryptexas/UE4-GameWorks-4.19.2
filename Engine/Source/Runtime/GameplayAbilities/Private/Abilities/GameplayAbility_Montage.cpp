// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "Abilities/GameplayAbility_Montage.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	UGameplayAbility_Montage
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

UGameplayAbility_Montage::UGameplayAbility_Montage(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PlayRate = 1.f;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::Server;
}

void UGameplayAbility_Montage::ActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (!CommitAbility(ActorInfo, ActivationInfo))
	{
		return;
	}

	if (MontageToPlay != NULL && ActorInfo->AnimInstance != NULL && ActorInfo->AnimInstance->GetActiveMontageInstance() == NULL)
	{
		TArray<FActiveGameplayEffectHandle>	AppliedEffects;

		// Apply GameplayEffects
		for (const UGameplayEffect * Effect : GameplayEffectsWhileAnimating)
		{
			FActiveGameplayEffectHandle Handle = ActorInfo->AbilitySystemComponent->ApplyGameplayEffectToSelf(Effect, 1.f, ActorInfo->Actor.Get());
			if (Handle.IsValid())
			{
				AppliedEffects.Add(Handle);
			}
		}

		float const Duration = ActorInfo->AnimInstance->Montage_Play(MontageToPlay, PlayRate);

		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UGameplayAbility_Montage::OnMontageEnded, ActorInfo->AbilitySystemComponent, AppliedEffects);
		ActorInfo->AnimInstance->Montage_SetEndDelegate(EndDelegate);

		if (SectionName != NAME_None)
		{
			ActorInfo->AnimInstance->Montage_JumpToSection(SectionName);
		}
	}
}

void UGameplayAbility_Montage::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted, TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent, TArray<FActiveGameplayEffectHandle> AppliedEffects)
{
	// Remove any GameplayEffects that we applied
	if (AbilitySystemComponent.IsValid())
	{
		for (FActiveGameplayEffectHandle Handle : AppliedEffects)
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(Handle);
		}
	}
}