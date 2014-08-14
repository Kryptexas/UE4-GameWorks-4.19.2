// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "GameplayAbility_CharacterJump.generated.h"

/**
 *	Ability that jumps with a character.
 */
UCLASS()
class GAMEPLAYABILITIES_API UGameplayAbility_CharacterJump : public UGameplayAbility
{
	GENERATED_UCLASS_BODY()

public:

	virtual bool CanActivateAbility(const FGameplayAbilityActorInfo* ActorInfo) const override;
	
	virtual void ActivateAbility(const FGameplayAbilityActorInfo* OwnerInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void InputReleased(int32 InputID, const FGameplayAbilityActorInfo* ActorInfo) override;

	virtual void CancelAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
};