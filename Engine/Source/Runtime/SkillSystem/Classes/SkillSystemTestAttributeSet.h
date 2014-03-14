// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SkillSystemTestAttributeSet.generated.h"

UCLASS(Blueprintable, BlueprintType)
class USkillSystemTestAttributeSet : public UAttributeSet
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttributeTest")
	float	Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttributeTest")
	float	Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttributeTest")
	float	CritChance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttributeTest")
	float	CritMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttributeTest")
	float	LifeSteal;
};