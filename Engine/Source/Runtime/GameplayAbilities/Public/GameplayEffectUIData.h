// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayEffectUIData.generated.h"

/**
 * UGameplayEffectUIData
 * Base class to provide game-specific data about how to describe a Gameplay Effect in the UI. Subclass with data to use in your game.
 */
UCLASS(Blueprintable, Abstract, EditInlineNew, CollapseCategories)
class GAMEPLAYABILITIES_API UGameplayEffectUIData : public UObject
{
	GENERATED_BODY()
public:
	UGameplayEffectUIData(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool NeedsLoadForServer() const override
	{
		return false;
	}
};
