// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"

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

void UGameplayAbility_CharacterJump::ActivateAbility(const FGameplayAbilityActorInfo ActorInfo)
{
	if (ActorInfo.ActivationMode == EGameplayAbilityActivationMode::Authority ||
		ActorInfo.ActivationMode == EGameplayAbilityActivationMode::Predicting)
	{
		if (!CommitAbility(ActorInfo))
		{
			return;
		}

		SKILL_LOG(Warning, TEXT("ActivateAbility Jump. %s. ActivationMode: %d"), *GetPathName(), (int32)ActorInfo.ActivationMode);

		ACharacter * Character = CastChecked<ACharacter>(ActorInfo.Actor.Get());
		Character->Jump();
	}
}

void UGameplayAbility_CharacterJump::PredictiveActivateAbility(const FGameplayAbilityActorInfo ActorInfo)
{
	CallActivateAbility(ActorInfo);
}

void UGameplayAbility_CharacterJump::InputReleased(int32 InputID, const FGameplayAbilityActorInfo ActorInfo)
{
	CancelAbility(ActorInfo);
}

bool UGameplayAbility_CharacterJump::CanActivateAbility(const FGameplayAbilityActorInfo ActorInfo) const
{
	if (!Super::CanActivateAbility(ActorInfo))
	{
		return false;
	}

	const UCharacterMovementComponent *CharMovement = CastChecked<UCharacterMovementComponent>(ActorInfo.MovementComponent.Get());
	return (CharMovement->MovementMode == EMovementMode::MOVE_Walking);
}

/**
 *	Canceling an non instanced ability is tricky. Right now this works for Jump since there is nothing that can go wrong by calling
 *	StopJumping() if you aren't already jumping. If we had a montage playing non instanced ability, it would need to make sure the
 *	Montage that *it* played was still playing, and if so, to cancel it. If this is something we need to support, we may need some
 *	light weight data structure to represent 'non intanced abilities in action' with a way to cancel/end them.
 */
void UGameplayAbility_CharacterJump::CancelAbility(const FGameplayAbilityActorInfo ActorInfo)
{
	if (ActorInfo.ActivationMode == EGameplayAbilityActivationMode::Authority ||
		ActorInfo.ActivationMode == EGameplayAbilityActivationMode::Predicting)
	{
		SKILL_LOG(Warning, TEXT("CancelAbility Jump. %s. ActivationMode: %d"), *GetPathName(), (int32)ActorInfo.ActivationMode);

		ACharacter * Character = CastChecked<ACharacter>(ActorInfo.Actor.Get());
		Character->StopJumping();
	}
}