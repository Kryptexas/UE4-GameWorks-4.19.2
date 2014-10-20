// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "GameplayCueInterface.h"
#include "AbilitySystemInterface.h"
#include "GameplayCueView.h"
#include "AbilitySystemTestPawn.generated.h"

UCLASS(Blueprintable, BlueprintType, notplaceable)
class GAMEPLAYABILITIES_API AAbilitySystemTestPawn : public ADefaultPawn, public IGameplayCueInterface, public IAbilitySystemInterface
{
	GENERATED_UCLASS_BODY()

	virtual void PostInitializeComponents() override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
private:
	/** DefaultPawn collision component */
	UPROPERTY(Category = AbilitySystem, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UAbilitySystemComponent* AbilitySystemComponent;
public:

	UPROPERTY(EditDefaultsOnly, Category=GameplayEffects)
	FGameplayCueHandler	GameplayCueHandler;

	//UPROPERTY(EditDefaultsOnly, Category=GameplayEffects)
	//UGameplayAbilitySet * DefaultAbilitySet;

	static FName AbilitySystemComponentName;

	/** Returns AbilitySystemComponent subobject **/
	FORCEINLINE class UAbilitySystemComponent* GetAbilitySystemComponent() { return AbilitySystemComponent; }
};
