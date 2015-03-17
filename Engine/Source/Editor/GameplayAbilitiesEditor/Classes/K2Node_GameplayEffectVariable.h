// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "K2Node.h"
#include "K2Node_VariableGet.h"
#include "GameplayEffect.h"

#include "K2Node_GameplayEffectVariable.generated.h"

UCLASS()
class GAMEPLAYABILITIESEDITOR_API UK2Node_GameplayEffectVariable : public UK2Node_VariableGet // only need a custom node for the getter
{
	GENERATED_BODY()
public:
	UK2Node_GameplayEffectVariable(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	UPROPERTY()
	UGameplayEffect* GameplayEffect;
};
