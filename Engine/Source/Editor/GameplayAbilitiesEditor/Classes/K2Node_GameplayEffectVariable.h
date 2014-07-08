// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "K2Node.h"
#include "K2Node_VariableGet.h"
#include "GameplayEffect.h"

#include "K2Node_GameplayEffectVariable.generated.h"

UCLASS(MinimalAPI)
class UK2Node_GameplayEffectVariable : public UK2Node_VariableGet // only need a custom node for the getter
{
	GENERATED_UCLASS_BODY()

public:
	class FGameplayEffectDisplayInfo
	{
	public:
		struct ModifierDisplayInfo
		{
			FString DisplayName;
			float Magnitude;
			EGameplayModOp::Type ModOp;
		};

		FGameplayEffectDisplayInfo() : GameplayEffect(NULL)
		{}

		void Init(UGameplayEffect& GameplayEffect);

		float Duration;
		float Period;
		float ChanceToApplyToTarget;
		float ChanceToExecuteOnGameplayEffect;
		UGameplayEffect* GameplayEffect;

		TArray<ModifierDisplayInfo> AttributeModifiers;
	};

	FGameplayEffectDisplayInfo GameplayEffectInfo;
};
