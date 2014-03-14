// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "GameplayTagAssetInterface.h"
#include "AttributeSet.h"
#include "GameplayEffectExtension.generated.h"

struct FGameplayEffectModCallbackData
{
	FGameplayEffectModCallbackData(const FGameplayEffectSpec & InEffectSpec, const FModifierSpec & InModifierSpec, FGameplayModifierEvaluatedData & InEvaluatedData, UAttributeComponent & InTarget)
		: EffectSpec(InEffectSpec)
		, ModifierSpec(InModifierSpec)
		, EvaluatedData(InEvaluatedData)
		, Target(InTarget)
	{

	}

	const struct FGameplayEffectSpec &		EffectSpec;		// The spec that the mod came from
	const struct FModifierSpec &			ModifierSpec;	// The mod we are going to apply
	struct FGameplayModifierEvaluatedData & EvaluatedData;	// The 'flat'/computed data to be applied to the target

	class UAttributeComponent &Target;		// Target we intend to apply to
};

UCLASS(BlueprintType)
class SKILLSYSTEM_API UGameplayEffectExtension: public UObject
{
	GENERATED_UCLASS_BODY()

public:

	virtual void PreGameplayEffectExecute(const FGameplayModifierEvaluatedData &SelfData, FGameplayEffectModCallbackData &Data) const
	{

	}

	virtual void PostGameplayEffectExecute(const FGameplayModifierEvaluatedData &SelfData, const FGameplayEffectModCallbackData &Data) const
	{

	}
};