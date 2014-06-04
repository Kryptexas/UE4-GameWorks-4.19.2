// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"
#include "Abilities/GameplayAbility_CharacterJump.h"

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	UGameplayAbility_CharacterJump
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

UGameplayAbility_CharacterJump::UGameplayAbility_CharacterJump(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::Predictive;
}

void UGameplayAbility_CharacterJump::ActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (ActivationInfo.ActivationMode == EGameplayAbilityActivationMode::Authority ||
		ActivationInfo.ActivationMode == EGameplayAbilityActivationMode::Predicting)
	{
		if (!CommitAbility(ActorInfo, ActivationInfo))
		{
			return;
		}

		ACharacter * Character = CastChecked<ACharacter>(ActorInfo->Actor.Get());
		Character->Jump();
	}
}

void UGameplayAbility_CharacterJump::PredictiveActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	CallActivateAbility(ActorInfo, ActivationInfo);
}

void UGameplayAbility_CharacterJump::InputReleased(int32 InputID, const FGameplayAbilityActorInfo* ActorInfo)
{
	// This is the 'top' of the ability execution code path. We don't have an ActivationInfo yet.
	// Normally for InputPressed, we do a bunch of stuff (for clients): predict, tell the server, wait for confirm, rollback, etc.
	// For jump, we are just going to call CancelAbility with an authority ActivationInfo. This is all jump needs, but more complex
	// abilities may need more work.

	FGameplayAbilityActivationInfo	ActivationInfo(EGameplayAbilityActivationMode::Authority, 0);

	CancelAbility(ActorInfo, ActivationInfo);
}

bool UGameplayAbility_CharacterJump::CanActivateAbility(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!Super::CanActivateAbility(ActorInfo))
	{
		return false;
	}

	const UCharacterMovementComponent *CharMovement = CastChecked<UCharacterMovementComponent>(ActorInfo->MovementComponent.Get());
	return (CharMovement->MovementMode == EMovementMode::MOVE_Walking);
}

/**
 *	Canceling an non instanced ability is tricky. Right now this works for Jump since there is nothing that can go wrong by calling
 *	StopJumping() if you aren't already jumping. If we had a montage playing non instanced ability, it would need to make sure the
 *	Montage that *it* played was still playing, and if so, to cancel it. If this is something we need to support, we may need some
 *	light weight data structure to represent 'non intanced abilities in action' with a way to cancel/end them.
 */
void UGameplayAbility_CharacterJump::CancelAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (ActivationInfo.ActivationMode == EGameplayAbilityActivationMode::Authority ||
		ActivationInfo.ActivationMode == EGameplayAbilityActivationMode::Predicting)
	{
		ACharacter * Character = CastChecked<ACharacter>(ActorInfo->Actor.Get());
		Character->StopJumping();
	}
}