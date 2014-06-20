// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "GameplayCueInterface.h"
#include "GameplayCueView.h"
#include "AbilitySystemTestPawn.generated.h"

UCLASS(Blueprintable, BlueprintType)
class GAMEPLAYABILITIES_API AAbilitySystemTestPawn : public ADefaultPawn, public IGameplayCueInterface
{
	GENERATED_UCLASS_BODY()

	virtual void PostInitializeComponents() override;
	
	/** DefaultPawn collision component */
	UPROPERTY(Category = AbilitySystem, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<class UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(EditDefaultsOnly, Category=GameplayEffects)
	FGameplayCueHandler	GameplayCueHandler;

	//UPROPERTY(EditDefaultsOnly, Category=GameplayEffects)
	//UGameplayAbilitySet * DefaultAbilitySet;

	static FName AbilitySystemComponentName;
};