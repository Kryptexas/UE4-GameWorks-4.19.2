// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayTagsModule.h"
#include "GameplayEffectTypes.h"

//----------------------------------------------------------------------

void FGameplayAbilityActorInfo::InitFromActor(AActor *InActor, UAbilitySystemComponent* InAbilitySystemComponent)
{
	check(InActor);
	check(InAbilitySystemComponent);

	Actor = InActor;
	AbilitySystemComponent = InAbilitySystemComponent;

	// Look for a player controller or pawn in the owner chain.
	AActor *TestActor = InActor;
	while (TestActor)
	{
		if (APlayerController * CastPC = Cast<APlayerController>(TestActor))
		{
			PlayerController = CastPC;
			break;
		}

		if (APawn * Pawn = Cast<APawn>(TestActor))
		{
			PlayerController = Cast<APlayerController>(Pawn->GetController());
			break;
		}

		TestActor = TestActor->GetOwner();
	}

	// Grab Components that we care about
	USkeletalMeshComponent * SkelMeshComponent = InActor->FindComponentByClass<USkeletalMeshComponent>();
	if (SkelMeshComponent)
	{
		this->AnimInstance = SkelMeshComponent->GetAnimInstance();
	}

	MovementComponent = InActor->FindComponentByClass<UMovementComponent>();
}

void FGameplayAbilityActorInfo::ClearActorInfo()
{
	Actor = NULL;
	PlayerController = NULL;
	AnimInstance = NULL;
	MovementComponent = NULL;
}

bool FGameplayAbilityActorInfo::IsLocallyControlled() const
{
	if (PlayerController.IsValid())
	{
		return PlayerController->IsLocalController();
	}

	return false;
}

bool FGameplayAbilityActorInfo::IsNetAuthority() const
{
	if (Actor.IsValid())
	{
		return (Actor->Role == ROLE_Authority);
	}

	// If we encounter issues with this being called before or after the owning actor is destroyed,
	// we may need to cache off the authority (or look for it on some global/world state).

	ensure(false);
	return false;
}

void FGameplayAbilityActivationInfo::GenerateNewPredictionKey() const
{	
	PredictionKey.GenerateDependantPredictionKey();
	ActivationMode = EGameplayAbilityActivationMode::Predicting;
}

void FGameplayAbilityActivationInfo::SetActivationConfirmed()
{
	ActivationMode = EGameplayAbilityActivationMode::Confirmed;
}

void FGameplayAbilityActivationInfo::SetPredictionStale()
{
	PredictionKey.bIsStale = true;
}