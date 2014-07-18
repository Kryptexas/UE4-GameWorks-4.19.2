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

bool AGameplayAbilityTargetActor::IsNetRelevantFor(class APlayerController* RealViewer, AActor* Viewer, const FVector& SrcLocation)
{
	//The player who created the ability doesn't need to be updated about it - there should be local prediction in place.
	return (RealViewer == MasterPC) ? false : Super::IsNetRelevantFor(RealViewer, Viewer, SrcLocation);
}
