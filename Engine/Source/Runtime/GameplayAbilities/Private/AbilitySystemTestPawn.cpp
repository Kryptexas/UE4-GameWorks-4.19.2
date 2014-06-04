// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemTestPawn.h"
#include "AbilitySystemTestAttributeSet.h"
#include "AbilitySystemComponent.h"

FName  AAbilitySystemTestPawn::AbilitySystemComponentName(TEXT("AbilitySystemComponent0"));

AAbilitySystemTestPawn::AAbilitySystemTestPawn(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	AbilitySystemComponent = PCIP.CreateDefaultSubobject<UAbilitySystemComponent>(this, AAbilitySystemTestPawn::AbilitySystemComponentName);
	AbilitySystemComponent->SetIsReplicated(true);

	//DefaultAbilitySet = NULL;
}

void AAbilitySystemTestPawn::PostInitializeComponents()
{	
	static UProperty *DamageProperty = FindFieldChecked<UProperty>(UAbilitySystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(UAbilitySystemTestAttributeSet, Damage));

	Super::PostInitializeComponents();
	GameplayCueHandler.Owner = this;
	AbilitySystemComponent->InitStats(UAbilitySystemTestAttributeSet::StaticClass(), NULL);

	/*
	if (DefaultAbilitySet != NULL)
	{
		AbilitySystemComponent->InitializeAbilities(DefaultAbilitySet);
	}
	*/
}

void AAbilitySystemTestPawn::GameplayCueActivated(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude, const FGameplayEffectInstigatorContext InstigatorContext)
{
	ABILITY_LOG(Log, TEXT("GameplayCueExecuted: %s. %.2f"), *GetName(), NormalizedMagnitude);
	GameplayCueHandler.GameplayCueActivated(GameplayCueTags, NormalizedMagnitude, InstigatorContext);
}

void AAbilitySystemTestPawn::GameplayCueExecuted(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude, const FGameplayEffectInstigatorContext InstigatorContext)
{
	ABILITY_LOG(Log, TEXT("GameplayCueExecuted: %s. %.2f"), *GetName(), NormalizedMagnitude);
	GameplayCueHandler.GameplayCueExecuted(GameplayCueTags, NormalizedMagnitude, InstigatorContext);
}

void AAbilitySystemTestPawn::GameplayCueAdded(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude, const FGameplayEffectInstigatorContext InstigatorContext)
{
	ABILITY_LOG(Log, TEXT("GameplayCueAdded: %s. %.2f"), *GetName(), NormalizedMagnitude);
	GameplayCueHandler.GameplayCueAdded(GameplayCueTags, NormalizedMagnitude, InstigatorContext);
}

void AAbilitySystemTestPawn::GameplayCueRemoved(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude, const FGameplayEffectInstigatorContext InstigatorContext)
{
	ABILITY_LOG(Log, TEXT("GameplayCueRemoved: %s. %.2f"), *GetName(), NormalizedMagnitude);
	GameplayCueHandler.GameplayCueRemoved(GameplayCueTags, NormalizedMagnitude, InstigatorContext);
}