// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SkillSystemTestPawn.generated.h"

UCLASS(Blueprintable, BlueprintType)
class SKILLSYSTEM_API ASkillSystemTestPawn : public ADefaultPawn, public IGameplayCueInterface
{
	GENERATED_UCLASS_BODY()

	virtual void PostInitializeComponents() OVERRIDE;

	UFUNCTION()
	virtual void GameplayCueActivated(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude) OVERRIDE;

	UFUNCTION()
	virtual void GameplayCueExecuted(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude) OVERRIDE;

	UFUNCTION()
	virtual void GameplayCueAdded(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude) OVERRIDE;

	UFUNCTION()
	virtual void GameplayCueRemoved(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude) OVERRIDE;


	/** DefaultPawn collision component */
	UPROPERTY(Category = SkillSystem, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<class UAttributeComponent> AttributeComponent;

	UPROPERTY(EditDefaultsOnly, Category=GameplayEffects)
	FGameplayCueHandler	GameplayCueHandler;

	static FName AttributeComponentName;


	void OnDamageExecute(struct FGameplayModifierEvaluatedData & Data);

	void OnDidDamage(const struct FGameplayEffectSpec & Spec, struct FGameplayModifierEvaluatedData & Data);
};