// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameplayAbility_Instanced.h"
#include "AbilityTask.Generated.h"

UCLASS(MinimalAPI)
class UAbilityTask : public UObject
{
	GENERATED_UCLASS_BODY()

	// Called to trigger the actual task once the delegates have been set up
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = "Abilities")
	virtual void Activate();
	
	TWeakObjectPtr<UGameplayAbility> Ability;
};