// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AttributeSet.h"
#include "AbilitySystemTestAttributeSet.generated.h"

UCLASS(Blueprintable, BlueprintType)
class GAMEPLAYABILITIES_API UAbilitySystemTestAttributeSet : public UAttributeSet
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "AttributeTest", meta = (HideFromModifiers))			// You can't make a GameplayEffect modify Health Directly (go through)
	float	MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "AttributeTest", meta = (HideFromModifiers))			// You can't make a GameplayEffect modify Health Directly (go through)
	float	Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "AttributeTest")
	float	Mana;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "AttributeTest")
	float	MaxMana;

	/** This Damage is just used for applying negative health mods. Its not a 'persistent' attribute. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttributeTest", meta = (HideFromLevelInfos))		// You can't make a GameplayEffect 'powered' by Damage (Its transient)
	float	Damage;

	/** This Attribute is the actual spell damage for this actor. It will power spell based GameplayEffects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "AttributeTest")
	float	SpellDamage;

	/** This Attribute is the actual physical damage for this actor. It will power physical based GameplayEffects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "AttributeTest")
	float	PhysicalDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "AttributeTest")
	float	CritChance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "AttributeTest")
	float	CritMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "AttributeTest")
	float	ArmorDamageReduction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "AttributeTest")
	float	DodgeChance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "AttributeTest")
	float	LifeSteal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "AttributeTest")
	float	Strength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttributeTest")
	float	StackingAttribute1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttributeTest")
	float	StackingAttribute2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttributeTest")
	float	NoStackAttribute;

	virtual void PreAttributeModify(struct FGameplayEffectModCallbackData &Data) override;
	virtual void PostAttributeModify(const struct FGameplayEffectModCallbackData &Data) override;
};
