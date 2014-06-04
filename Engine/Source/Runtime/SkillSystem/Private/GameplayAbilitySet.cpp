// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"
#include "AttributeComponent.h"
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

	UAttributeComponent * AttributeComponent = ActorInfo.AttributeComponent.Get();
	if (AttributeComponent)
	{
		if (AttributeComponent->ActivatableAbilities.IsValidIndex(AbilityIdx))
		{
			UGameplayAbility * Ability = AttributeComponent->ActivatableAbilities[AbilityIdx];
			if (Ability)
			{
				Ability->InputPressed(InputID, AttributeComponent->AbilityActorInfo.Get());
			}
		}
	}
}

void InputReleased(int32 AbilityIdx, int32 InputID, TWeakObjectPtr<AActor> ActorOwner)
{
	FGameplayAbilityActorInfo	ActorInfo;
	ActorInfo.InitFromActor(ActorOwner.Get());

	UAttributeComponent * AttributeComponent = ActorInfo.AttributeComponent.Get();
	if (AttributeComponent)
	{
		if (AttributeComponent->ActivatableAbilities.IsValidIndex(AbilityIdx))
		{
			UGameplayAbility * Ability = AttributeComponent->ActivatableAbilities[AbilityIdx];
			if (Ability)
			{
				Ability->InputReleased(InputID, AttributeComponent->AbilityActorInfo.Get());
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

void UGameplayAbilitySet::InitializeAbilities(UAttributeComponent *AttributeComponent) const
{
	for (const FGameplayAbilityBindInfo &BindInfo : this->Abilities)
	{
		UGameplayAbility * Ability = BindInfo.GameplayAbilityClass ? BindInfo.GameplayAbilityClass->GetDefaultObject<UGameplayAbility>() : BindInfo.GameplayAbilityInstance;
		if (Ability)
		{
			if (Ability->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerActor)
			{
				Ability = AttributeComponent->CreateNewInstanceOfAbility(Ability);
			}

			AttributeComponent->ActivatableAbilities.Add(Ability);
		}
	}
}

void UGameplayAbilitySet::BindInputComponentToAbilities(UInputComponent *InputComponent, UAttributeComponent *AttributeComponent) const
{
	AActor * OwnerActor = AttributeComponent->GetOwner();
	check(OwnerActor);

	for (int32 idx = 0; idx < this->Abilities.Num(); ++idx)
	{
		const FGameplayAbilityBindInfo & BindInfo = this->Abilities[idx];
		UGameplayAbility * Ability = AttributeComponent->ActivatableAbilities[idx];	
		
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

