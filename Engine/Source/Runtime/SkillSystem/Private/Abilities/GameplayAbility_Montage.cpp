// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"
#include "Abilities/GameplayAbility_Montage.h"

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

void UGameplayAbility_Montage::ActivateAbility(const FGameplayAbilityActorInfo OwnerInfo)
{
	if (!CommitAbility(OwnerInfo))
	{
		return;
	}

	if (MontageToPlay != NULL && OwnerInfo.AnimInstance != NULL && OwnerInfo.AnimInstance->GetActiveMontageInstance() == NULL)
	{
		TArray<FActiveGameplayEffectHandle>	AppliedEffects;

		// Apply GameplayEffects
		for (const UGameplayEffect * Effect : GameplayEffectsWhileAnimating)
		{
			FActiveGameplayEffectHandle Handle = OwnerInfo.AttributeComponent->ApplyGameplayEffectToSelf(Effect, 1.f, OwnerInfo.Actor.Get());
			if (Handle.IsValid())
			{
				AppliedEffects.Add(Handle);
			}
		}

		float const Duration = OwnerInfo.AnimInstance->Montage_Play(MontageToPlay, PlayRate);

		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UGameplayAbility_Montage::OnMontageEnded, OwnerInfo, AppliedEffects);
		OwnerInfo.AnimInstance->Montage_SetEndDelegate(EndDelegate);

		if (SectionName != NAME_None)
		{
			OwnerInfo.AnimInstance->Montage_JumpToSection(SectionName);
		}
	}
}

void UGameplayAbility_Montage::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted, const FGameplayAbilityActorInfo OwnerInfo, TArray<FActiveGameplayEffectHandle> AppliedEffects)
{
	// Remove any GameplayEffects that we applied
	UAttributeComponent *OwnerComponent = OwnerInfo.AttributeComponent.Get();
	if (OwnerComponent)
	{
		for (FActiveGameplayEffectHandle Handle : AppliedEffects)
		{
			OwnerComponent->RemoveActiveGameplayEffect(Handle);
		}
	}
}