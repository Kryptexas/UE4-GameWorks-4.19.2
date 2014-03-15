// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"

USkillSystemTestAttributeSet::USkillSystemTestAttributeSet(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	Health = 100.f;
	Damage = 0.f;
	CritChance = 1.f;
}


void USkillSystemTestAttributeSet::PreAttributeModify(struct FGameplayEffectModCallbackData &Data)
{

}

void USkillSystemTestAttributeSet::PostAttributeModify(const struct FGameplayEffectModCallbackData &Data)
{
	static UProperty *HealthProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Health));
	static UProperty *DamageProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Damage));

	UProperty *ModifiedProperty = Data.ModifierSpec.Info.Attribute.GetUProperty();

	if ( Data.EvaluatedData.Tags.HasTag(FName(TEXT("FireDamage")) ))
	{
	}

	// What property was modified?
	if (DamageProperty == ModifiedProperty)
	{
		// Treat damage as minus health
		Health -= Damage;
		Damage = 0.f;
	}
}
