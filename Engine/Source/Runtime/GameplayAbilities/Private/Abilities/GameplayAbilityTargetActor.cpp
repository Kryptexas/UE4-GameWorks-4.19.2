// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "GameplayAbilityTargetActor.h"

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	AGameplayAbilityTargetActor
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

AGameplayAbilityTargetActor::AGameplayAbilityTargetActor(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	StaticTargetFunction = false;
}

void AGameplayAbilityTargetActor::StartTargeting(UGameplayAbility* Ability)
{
}

void AGameplayAbilityTargetActor::ConfirmTargeting()
{
	TargetDataReadyDelegate.Broadcast(FGameplayAbilityTargetDataHandle());
}

/** Outside code is saying 'stop everything and just forget about it' */
void AGameplayAbilityTargetActor::CancelTargeting()
{
	CanceledDelegate.Broadcast(FGameplayAbilityTargetDataHandle());
}