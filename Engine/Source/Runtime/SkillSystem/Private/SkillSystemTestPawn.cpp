// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"

FName  ASkillSystemTestPawn::AttributeComponentName(TEXT("AttributeComponent0"));

ASkillSystemTestPawn::ASkillSystemTestPawn(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	AttributeComponent = PCIP.CreateDefaultSubobject<UAttributeComponent>(this, ASkillSystemTestPawn::AttributeComponentName);
	AttributeComponent->SetIsReplicated(true);

	//DefaultAbilitySet = NULL;
}

void ASkillSystemTestPawn::PostInitializeComponents()
{	
	static UProperty *DamageProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Damage));

	Super::PostInitializeComponents();
	GameplayCueHandler.Owner = this;
	AttributeComponent->InitStats(USkillSystemTestAttributeSet::StaticClass(), NULL);

	/*
	if (DefaultAbilitySet != NULL)
	{
		AttributeComponent->InitializeAbilities(DefaultAbilitySet);
	}
	*/
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