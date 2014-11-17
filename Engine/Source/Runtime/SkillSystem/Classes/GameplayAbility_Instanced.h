// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayAbility_Instanced.generated.h"

class UAnimInstance;
class UAttributeComponent;

/**
* UGameplayEffect
*	The GameplayEffect definition. This is the data asset defined in the editor that drives everything.
*/
UCLASS(Blueprintable)
class SKILLSYSTEM_API UGameplayAbility_Instanced : public UGameplayAbility
{
	GENERATED_UCLASS_BODY()

public:
	
	// -------------------------------------------------

	UFUNCTION(BlueprintImplementableEvent, Category = Ability)
	virtual bool K2_CanActivateAbility(const FGameplayAbilityActorInfo ActorInfo) const;

	virtual bool CanActivateAbility(const FGameplayAbilityActorInfo ActorInfo) const OVERRIDE;	

	// -------------------------------------------------

	UFUNCTION(BlueprintImplementableEvent, Category = Ability)
	virtual void K2_CommitExecute(const FGameplayAbilityActorInfo ActorInfo);

	virtual void CommitExecute(const FGameplayAbilityActorInfo ActorInfo) OVERRIDE;

	// -------------------------------------------------

	UFUNCTION(BlueprintImplementableEvent, Category = Ability)
	virtual void K2_ActivateAbility(const FGameplayAbilityActorInfo ActorInfo);

	virtual void ActivateAbility(const FGameplayAbilityActorInfo ActorInfo) OVERRIDE;

	// -------------------------------------------------

	virtual bool AllowInstancing() const OVERRIDE
	{
		return true;
	}

private:
	
};