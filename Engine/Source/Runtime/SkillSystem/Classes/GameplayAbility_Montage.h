// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayAbility_Montage.generated.h"

/**
 *	A gameplay ability that plays a single montage and applies a GameplayEffect
 */
UCLASS()
class SKILLSYSTEM_API UGameplayAbility_Montage : public UGameplayAbility
{
	GENERATED_UCLASS_BODY()

public:
	
	virtual void ActivateAbility(const FGameplayAbilityActorInfo OwnerInfo) OVERRIDE;

	UPROPERTY(EditDefaultsOnly, Category = MontageAbility)
	UAnimMontage *	MontageToPlay;

	UPROPERTY(EditDefaultsOnly, Category = MontageAbility)
	float	PlayRate;

	UPROPERTY(EditDefaultsOnly, Category = MontageAbility)
	FName	SectionName;

	/** GameplayEffects to apply and then remove while the animation is playing */
	UPROPERTY(EditDefaultsOnly, Category = MontageAbility)
	TArray<const UGameplayEffect*>	GameplayEffectsWhileAnimating;

	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted, const FGameplayAbilityActorInfo OwnerInfo, TArray<struct FActiveGameplayEffectHandle>	AppliedEffects);
};