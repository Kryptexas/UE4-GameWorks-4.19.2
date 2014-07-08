// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "AbilitySystemEditorPrivatePCH.h"
#include "K2Node_GameplayEffectVariable.h"

#define LOCTEXT_NAMESPACE "K2Node_GameplayEffectVariable"

UK2Node_GameplayEffectVariable::UK2Node_GameplayEffectVariable(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_GameplayEffectVariable::FGameplayEffectDisplayInfo::Init(UGameplayEffect& GameplayEffect)
{
	this->GameplayEffect = &GameplayEffect;
	Duration = GameplayEffect.Duration.Value;
	Period = GameplayEffect.Period.Value;
	ChanceToApplyToTarget = GameplayEffect.ChanceToApplyToTarget.Value;
	ChanceToExecuteOnGameplayEffect = GameplayEffect.ChanceToExecuteOnGameplayEffect.Value;

	for (int Idx = 0; Idx < GameplayEffect.Modifiers.Num(); ++Idx)
	{
		if (GameplayEffect.Modifiers[Idx].ModifierType == EGameplayMod::Attribute)
		{
			ModifierDisplayInfo ModInfo;
			ModInfo.DisplayName = GameplayEffect.Modifiers[Idx].Attribute.GetName();
			ModInfo.Magnitude = GameplayEffect.Modifiers[Idx].Magnitude.Value;
			ModInfo.ModOp = GameplayEffect.Modifiers[Idx].ModifierOp;

			AttributeModifiers.Add(ModInfo);
		}
	}
}

#undef LOCTEXT_NAMESPACE
