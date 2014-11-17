// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// ActorComponent.cpp: Actor component implementation.

#include "SkillSystemModulePrivatePCH.h"

#include "Net/UnrealNetwork.h"
#include "MessageLog.h"
#include "UObjectToken.h"
#include "MapErrors.h"

#define LOCTEXT_NAMESPACE "AttributeComponent"

/** Enable to log out all render state create, destroy and updatetransform events */
#define LOG_RENDER_STATE 0

void UAttributeComponent::InitializeComponent()
{
	Super::InitializeComponent();
}
void UAttributeComponent::InitializeAbilities(class UGameplayAbilitySet *AbilitySet)
{
	AActor * OwnerActor = GetOwner();
	check(OwnerActor);

	FGameplayAbilityActorInfo	ActorInfo;
	ActorInfo.InitFromActor(OwnerActor);

	for (const FGameplayAbilityBindInfo &BindInfo : AbilitySet->Abilities)
	{
		UGameplayAbility * Ability = NULL;
		if (BindInfo.GameplayAbilityClass)
		{
			Ability = BindInfo.GameplayAbilityClass->GetDefaultObject<UGameplayAbility>();
			if (Ability->AllowInstancing())
			{
				Ability = ConstructObject<UGameplayAbility>(BindInfo.GameplayAbilityClass, OwnerActor);
				InstancedAbilities.Add(Ability);
			}
			AllAbilities.Add(Ability);
		}
	}
}

/** This is a convenience/helper function to bind input directly to abilities. Some games may need to do things differently, if for example you want weapons to fire abilities and the pawn to swap weapons. */
void UAttributeComponent::BindInputComponentToAbilities(UInputComponent *InputComponent, UGameplayAbilitySet *AbilitySet)
{
	check(AbilitySet->Abilities.Num() == AllAbilities.Num());

	AActor * OwnerActor = GetOwner();
	check(OwnerActor);

	FGameplayAbilityActorInfo	ActorInfo;
	ActorInfo.InitFromActor(OwnerActor);

	for (int32 idx = 0; idx < AbilitySet->Abilities.Num(); ++idx)
	{
		FGameplayAbilityBindInfo & BindInfo = AbilitySet->Abilities[idx];
		UGameplayAbility * Ability = AllAbilities[idx];
		
		check(Ability->GetClass() == AbilitySet->Abilities[idx].GameplayAbilityClass);
		
		for (const FGameplayAbilityBindInfoCommandIDPair &CommandPair : BindInfo.CommandBindings)
		{
			// Pressed event
			{
				FInputActionBinding AB(FName(*CommandPair.CommandString), IE_Pressed);
				AB.ActionDelegate.GetDelegateForManualSet().BindUObject(Ability, &UGameplayAbility::InputPressed, CommandPair.InputID, ActorInfo);
				InputComponent->AddActionBinding(AB);
			}

			// Released Event
			{
				FInputActionBinding AB(FName(*CommandPair.CommandString), IE_Released);
				AB.ActionDelegate.GetDelegateForManualSet().BindUObject(Ability, &UGameplayAbility::InputReleased, CommandPair.InputID, ActorInfo);
				InputComponent->AddActionBinding(AB);
			}
		}
	}
}

void UAttributeComponent::MontageBranchPoint_AbilityDecisionStop()
{
	if (AnimatingAbility)
	{
		AActor * OwnerActor = GetOwner();
		check(OwnerActor);

		FGameplayAbilityActorInfo	ActorInfo;
		ActorInfo.InitFromActor(OwnerActor);

		AnimatingAbility->MontageBranchPoint_AbilityDecisionStop(ActorInfo);
	}

}

void UAttributeComponent::MontageBranchPoint_AbilityDecisionStart()
{
	if (AnimatingAbility)
	{
		AActor * OwnerActor = GetOwner();
		check(OwnerActor);

		FGameplayAbilityActorInfo	ActorInfo;
		ActorInfo.InitFromActor(OwnerActor);

		AnimatingAbility->MontageBranchPoint_AbilityDecisionStart(ActorInfo);
	}
}

bool UAttributeComponent::ActivateAbility(int32 AbilityIdx)
{
	AActor * OwnerActor = GetOwner();
	check(OwnerActor);

	FGameplayAbilityActorInfo	ActorInfo;
	ActorInfo.InitFromActor(OwnerActor);

	if (AllAbilities.IsValidIndex(AbilityIdx))
	{
		// Fixme
		AllAbilities[AbilityIdx]->InputPressed(0, ActorInfo);
	}

	return false;
}

void UAttributeComponent::ServerTryActivateAbility_Implementation(class UGameplayAbility * AbilityToActivate)
{
	ensure(AbilityToActivate);

	AActor * OwnerActor = GetOwner();
	check(OwnerActor);

	FGameplayAbilityActorInfo	ActorInfo;
	ActorInfo.InitFromActor(OwnerActor);
	
	if (AbilityToActivate->CanActivateAbility(ActorInfo))
	{
		AbilityToActivate->ActivateAbility(ActorInfo);
		ClientActivateAbilitySucceed(AbilityToActivate);
	}
	else
	{
		ClientActivateAbilityFailed(AbilityToActivate);
	}
}

bool UAttributeComponent::ServerTryActivateAbility_Validate(class UGameplayAbility * AbilityToActivate)
{
	return true;
}

void UAttributeComponent::ClientActivateAbilityFailed_Implementation(class UGameplayAbility * AbilityToActivate)
{

}

void UAttributeComponent::ClientActivateAbilitySucceed_Implementation(class UGameplayAbility * AbilityToActivate)
{
	ensure(AbilityToActivate);

	AActor * OwnerActor = GetOwner();
	check(OwnerActor);

	FGameplayAbilityActorInfo	ActorInfo;
	ActorInfo.InitFromActor(OwnerActor);

	AbilityToActivate->ActivateAbility(ActorInfo);
}


#undef LOCTEXT_NAMESPACE

