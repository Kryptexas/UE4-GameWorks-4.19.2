// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "GameplayAbilityWorldReticle.h"
#include "GameplayAbilityTargetActor.h"

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	AGameplayAbilityWorldReticle
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

AGameplayAbilityWorldReticle::AGameplayAbilityWorldReticle(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
}

void AGameplayAbilityWorldReticle::InitializeReticle(AGameplayAbilityTargetActor* InTargetingActor, FWorldReticleParameters InParameters)
{
	check(InTargetingActor);
	TargetingActor = InTargetingActor;
	MasterPC = InTargetingActor->MasterPC;
	AddTickPrerequisiteActor(TargetingActor);		//We want the reticle to tick after the targeting actor so that designers have the final say on the position
	Parameters = InParameters;
	OnParametersInitialized();
}

bool AGameplayAbilityWorldReticle::IsNetRelevantFor(const APlayerController* RealViewer, const AActor* Viewer, const FVector& SrcLocation) const
{
	//The player who created the ability doesn't need to be updated about it - there should be local prediction in place.
	if (RealViewer == MasterPC)
	{
		return false;
	}

	return Super::IsNetRelevantFor(RealViewer, Viewer, SrcLocation);
}

bool AGameplayAbilityWorldReticle::GetReplicates() const
{
	return bReplicates;
}
