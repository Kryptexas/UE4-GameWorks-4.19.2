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
	check(ShouldProduceTargetData());

	TargetDataReadyDelegate.Broadcast(FGameplayAbilityTargetDataHandle());
	Destroy();
}

/** Outside code is saying 'stop everything and just forget about it' */
void AGameplayAbilityTargetActor::CancelTargeting()
{
	CanceledDelegate.Broadcast(FGameplayAbilityTargetDataHandle());
	Destroy();
}

bool AGameplayAbilityTargetActor::IsNetRelevantFor(class APlayerController* RealViewer, AActor* Viewer, const FVector& SrcLocation)
{
	//The player who created the ability doesn't need to be updated about it - there should be local prediction in place.
	if (RealViewer == MasterPC)
	{
		return false;
	}

	return Super::IsNetRelevantFor(RealViewer, Viewer, SrcLocation);
}

bool AGameplayAbilityTargetActor::GetReplicates() const
{
	return bReplicates;
}

FGameplayAbilityTargetDataHandle AGameplayAbilityTargetActor::StaticGetTargetData(UWorld * World, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const
{
	return FGameplayAbilityTargetDataHandle();
}

bool AGameplayAbilityTargetActor::OnReplicatedTargetDataReceived(FGameplayAbilityTargetDataHandle& Data) const
{
	return true;
}

bool AGameplayAbilityTargetActor::ShouldProduceTargetData() const
{
	return (MasterPC && MasterPC->IsLocalController());
}
