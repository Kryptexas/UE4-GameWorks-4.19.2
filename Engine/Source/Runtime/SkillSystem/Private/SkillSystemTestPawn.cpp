// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"

FName  ASkillSystemTestPawn::AttributeComponentName(TEXT("AttributeComponent0"));

ASkillSystemTestPawn::ASkillSystemTestPawn(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	AttributeComponent = PCIP.CreateDefaultSubobject<UAttributeComponent>(this, ASkillSystemTestPawn::AttributeComponentName);
	AttributeComponent->SetIsReplicated(true);
}

void ASkillSystemTestPawn::PostInitializeComponents()
{	
	static UProperty *DamageProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Damage));

	Super::PostInitializeComponents();
	GameplayCueHandler.Owner = this;
	AttributeComponent->InitStats(USkillSystemTestAttributeSet::StaticClass(), NULL);
	AttributeComponent->RegisterAttributeModifyCallback(FGameplayAttribute(DamageProperty), FOnGameplayAttributeEffectExecuted::CreateUObject(this, &ASkillSystemTestPawn::OnDamageExecute));
}

void ASkillSystemTestPawn::GameplayCueActivated(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude)
{
	SKILL_LOG(Log, TEXT("GameplayCueExecuted: %s. %.2f"), *GetName(), NormalizedMagnitude);
	GameplayCueHandler.GameplayCueActivated(GameplayCueTags, NormalizedMagnitude);
}

void ASkillSystemTestPawn::GameplayCueExecuted(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude)
{
	SKILL_LOG(Log, TEXT("GameplayCueExecuted: %s. %.2f"), *GetName(), NormalizedMagnitude);
	GameplayCueHandler.GameplayCueExecuted(GameplayCueTags, NormalizedMagnitude);
}

void ASkillSystemTestPawn::GameplayCueAdded(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude)
{
	SKILL_LOG(Log, TEXT("GameplayCueAdded: %s. %.2f"), *GetName(), NormalizedMagnitude);
	GameplayCueHandler.GameplayCueAdded(GameplayCueTags, NormalizedMagnitude);
}

void ASkillSystemTestPawn::GameplayCueRemoved(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude)
{
	SKILL_LOG(Log, TEXT("GameplayCueRemoved: %s. %.2f"), *GetName(), NormalizedMagnitude);
	GameplayCueHandler.GameplayCueRemoved(GameplayCueTags, NormalizedMagnitude);
}

void ASkillSystemTestPawn::OnDamageExecute(FGameplayModifierEvaluatedData & Data)
{
	auto AttributeSet = AttributeComponent->GetSet<USkillSystemTestAttributeSet>();

	// A very basic example of how you could do a critical chance roll at the actor level
	if (Data.Tags.HasTag(FName(TEXT("CanCrit"))))
	{
		if (AttributeSet->CritChance >= FMath::FRand())
		{
			// Crit success - modify Data
			// (this example is simple - we will
			Data.Magnitude *= AttributeSet->CritMultiplier;
		}
	}
}

void ASkillSystemTestPawn::OnDidDamage(const struct FGameplayEffectSpec & Spec, struct FGameplayModifierEvaluatedData & Data)
{
	auto AttributeSet = AttributeComponent->GetSet<USkillSystemTestAttributeSet>();

	// A very basic example of how you could do a critical chance roll at the actor level
	if (Data.Tags.HasTag(FName(TEXT("LifeSteal"))))
	{
		if (AttributeSet->LifeSteal >= 0.f)
		{
			float LifeStealAmount = AttributeSet->LifeSteal * Data.Magnitude;

			// We should apply a gameplay effect to ourself instead
			AttributeSet->Health += LifeStealAmount;
		}
	}
}