// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameplayAbility.h"
#include "AbilityTask.generated.h"

UCLASS(MinimalAPI)
class UAbilityTask : public UObject
{
	GENERATED_UCLASS_BODY()

	// Called to trigger the actual task once the delegates have been set up
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = "Abilities")
	virtual void Activate();

	virtual void InitTask(UGameplayAbility* InAbility);
	
	/** GameplayAbility that created us */
	TWeakObjectPtr<UGameplayAbility> Ability;

	TWeakObjectPtr<UAbilitySystemComponent>	AbilitySystemComponent;
};