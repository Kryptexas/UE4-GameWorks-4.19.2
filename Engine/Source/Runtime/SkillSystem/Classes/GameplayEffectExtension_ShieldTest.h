// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "GameplayTagAssetInterface.h"
#include "AttributeSet.h"
#include "GameplayEffectExtension_ShieldTest.generated.h"

UCLASS(BlueprintType)
class SKILLSYSTEM_API UGameplayEffectExtension_ShieldTest : public UGameplayEffectExtension
{
	GENERATED_UCLASS_BODY()

public:

	void PreGameplayEffectExecute(const FGameplayModifierEvaluatedData &SelfData, FGameplayEffectModCallbackData &Data) const OVERRIDE;
	void PostGameplayEffectExecute(const FGameplayModifierEvaluatedData &SelfData, const FGameplayEffectModCallbackData &Data) const OVERRIDE;

	UPROPERTY(EditDefaultsOnly, Category = Lifesteal)
	UGameplayEffect * ShieldRemoveGameplayEffect;
};