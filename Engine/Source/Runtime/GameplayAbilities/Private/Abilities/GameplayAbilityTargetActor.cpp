// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "GameplayAbilityTargetActor.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	AGameplayAbilityTargetActor
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

AGameplayAbilityTargetActor::AGameplayAbilityTargetActor(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	StaticTargetFunction = false;
	ShouldProduceTargetDataOnServer = false;
	bDebug = false;
}

void AGameplayAbilityTargetActor::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGameplayAbilityTargetActor, StartLocation);
	DOREPLIFETIME(AGameplayAbilityTargetActor, SourceActor);
	DOREPLIFETIME(AGameplayAbilityTargetActor, bDebug);
}

void AGameplayAbilityTargetActor::StartTargeting(UGameplayAbility* Ability)
{
	OwningAbility = Ability;
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
	// return true if we are locally owned, or (we are the server and this is a gameplaytarget ability that can produce target data server side)
	return (MasterPC && (MasterPC->IsLocalController() || ShouldProduceTargetDataOnServer));
}

void AGameplayAbilityTargetActor::BindToConfirmCancelInputs()
{
	check(OwningAbility);

	UAbilitySystemComponent* ASC = OwningAbility->GetCurrentActorInfo()->AbilitySystemComponent.Get();
	if (ASC)
	{
		ASC->ConfirmCallbacks.AddDynamic(this, &AGameplayAbilityTargetActor::ConfirmTargeting);
		ASC->CancelCallbacks.AddDynamic(this, &AGameplayAbilityTargetActor::CancelTargeting);
	}
}