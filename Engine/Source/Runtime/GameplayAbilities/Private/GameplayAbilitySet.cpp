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

void InputPressed(int32 AbilityIdx, int32 InputID, TWeakObjectPtr<AActor> ActorOwner)
{
	FGameplayAbilityActorInfo	ActorInfo;
	ActorInfo.InitFromActor(ActorOwner.Get());

	UAbilitySystemComponent * AbilitySystemComponent = ActorInfo.AbilitySystemComponent.Get();
	if (AbilitySystemComponent)
	{
		if (AbilitySystemComponent->ActivatableAbilities.IsValidIndex(AbilityIdx))
		{
			UGameplayAbility * Ability = AbilitySystemComponent->ActivatableAbilities[AbilityIdx];
			if (Ability)
			{
				Ability->InputPressed(InputID, AbilitySystemComponent->AbilityActorInfo.Get());
			}
		}
	}
}

void InputReleased(int32 AbilityIdx, int32 InputID, TWeakObjectPtr<AActor> ActorOwner)
{
	FGameplayAbilityActorInfo	ActorInfo;
	ActorInfo.InitFromActor(ActorOwner.Get());

	UAbilitySystemComponent * AbilitySystemComponent = ActorInfo.AbilitySystemComponent.Get();
	if (AbilitySystemComponent)
	{
		if (AbilitySystemComponent->ActivatableAbilities.IsValidIndex(AbilityIdx))
		{
			UGameplayAbility * Ability = AbilitySystemComponent->ActivatableAbilities[AbilityIdx];
			if (Ability)
			{
				Ability->InputReleased(InputID, AbilitySystemComponent->AbilityActorInfo.Get());
			}
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	UGameplayAbilitySet
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------


UGameplayAbilitySet::UGameplayAbilitySet(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

}

void UGameplayAbilitySet::InitializeAbilities(UAbilitySystemComponent *AbilitySystemComponent) const
{
	for (const FGameplayAbilityBindInfo &BindInfo : this->Abilities)
	{
		UGameplayAbility * Ability = BindInfo.GameplayAbilityClass ? BindInfo.GameplayAbilityClass->GetDefaultObject<UGameplayAbility>() : BindInfo.GameplayAbilityInstance;
		if (Ability)
		{
			if (Ability->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerActor)
			{
				Ability = AbilitySystemComponent->CreateNewInstanceOfAbility(Ability);
			}

			AbilitySystemComponent->ActivatableAbilities.Add(Ability);
		}
	}
}

void UGameplayAbilitySet::BindInputComponentToAbilities(UInputComponent *InputComponent, UAbilitySystemComponent *AbilitySystemComponent) const
{
	AActor * OwnerActor = AbilitySystemComponent->GetOwner();
	check(OwnerActor);

	for (int32 idx = 0; idx < this->Abilities.Num(); ++idx)
	{
		const FGameplayAbilityBindInfo & BindInfo = this->Abilities[idx];
		UGameplayAbility * Ability = AbilitySystemComponent->ActivatableAbilities[idx];	
		
		for (const FGameplayAbilityBindInfoCommandIDPair &CommandPair : BindInfo.CommandBindings)
		{
			// Pressed event
			{
				FInputActionBinding AB(FName(*CommandPair.CommandString), IE_Pressed);
				AB.ActionDelegate.GetDelegateForManualSet().BindStatic(&InputPressed, idx, CommandPair.InputID, TWeakObjectPtr<AActor>(OwnerActor));
				InputComponent->AddActionBinding(AB);
			}

			// Released event
			{
				FInputActionBinding AB(FName(*CommandPair.CommandString), IE_Released);
				AB.ActionDelegate.GetDelegateForManualSet().BindStatic(&InputReleased, idx, CommandPair.InputID, TWeakObjectPtr<AActor>(OwnerActor));
				InputComponent->AddActionBinding(AB);
			}
		}
	}
}

