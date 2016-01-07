// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"

UAbilityTask_PlayMontageAndWait::UAbilityTask_PlayMontageAndWait(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Rate = 1.f;
	bStopWhenAbilityEnds = true;
}

void UAbilityTask_PlayMontageAndWait::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (Ability && Ability->GetCurrentMontage() == MontageToPlay)
	{
		if (Montage == MontageToPlay)
		{
			AbilitySystemComponent->ClearAnimatingAbility(Ability);
		}
	}

	if (bInterrupted)
	{
		OnInterrupted.Broadcast();
	}
	else
	{
		OnComplete.Broadcast();
	}

	EndTask();
}

void UAbilityTask_PlayMontageAndWait::OnMontageInterrupted()
{
	if (StopPlayingMontage())
	{
		// Let the BP handle the interrupt as well
		OnInterrupted.Broadcast();
	}
}

UAbilityTask_PlayMontageAndWait* UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(UObject* WorldContextObject,
	FName TaskInstanceName, UAnimMontage *MontageToPlay, float Rate, FName StartSection, bool bStopWhenAbilityEnds)
{
	UAbilityTask_PlayMontageAndWait* MyObj = NewAbilityTask<UAbilityTask_PlayMontageAndWait>(WorldContextObject, TaskInstanceName);
	MyObj->MontageToPlay = MontageToPlay;
	MyObj->Rate = Rate;
	MyObj->StartSection = StartSection;
	MyObj->bStopWhenAbilityEnds = bStopWhenAbilityEnds;

	return MyObj;
}

void UAbilityTask_PlayMontageAndWait::Activate()
{
	if (Ability == nullptr)
	{
		return;
	}

	bool bPlayedMontage = false;

	if (AbilitySystemComponent)
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		if (ActorInfo->AnimInstance.IsValid())
		{
			if (AbilitySystemComponent->PlayMontage(Ability, Ability->GetCurrentActivationInfo(), MontageToPlay, Rate, StartSection) > 0.f)
			{
				// Playing a montage could potentially fire off a callback into game code which could kill this ability! Early out if we are  pending kill.
				if (IsPendingKill())
				{
					OnCancelled.Broadcast();
					return;
				}

				InterruptedHandle = Ability->OnGameplayAbilityCancelled.AddUObject(this, &UAbilityTask_PlayMontageAndWait::OnMontageInterrupted);

				BlendingOutDelegate.BindUObject(this, &UAbilityTask_PlayMontageAndWait::OnMontageBlendingOut);
				ActorInfo->AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate);

				bPlayedMontage = true;
			}
		}
	}

	if (!bPlayedMontage)
	{
		ABILITY_LOG(Warning, TEXT("UAbilityTask_PlayMontageAndWait called in Ability %s failed to play montage; Task Instance Name %s."), *Ability->GetName(), *InstanceName.ToString());
		OnCancelled.Broadcast();
	}
}

void UAbilityTask_PlayMontageAndWait::ExternalCancel()
{
	check(AbilitySystemComponent);


	OnCancelled.Broadcast();
	Super::ExternalCancel();
}

void UAbilityTask_PlayMontageAndWait::OnDestroy(bool AbilityEnded)
{
	// Note: Clearing montage end delegate isn't necessary since its not a multicast and will be cleared when the next montage plays.
	// (If we are destroyed, it will detect this and not do anything)

	// This delegate, however, should be cleared as it is a multicast
	if (Ability)
	{
		Ability->OnGameplayAbilityCancelled.Remove(InterruptedHandle);
		if (AbilityEnded && bStopWhenAbilityEnds)
		{
			StopPlayingMontage();
		}
	}

	Super::OnDestroy(AbilityEnded);

}

bool UAbilityTask_PlayMontageAndWait::StopPlayingMontage()
{
	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	if (!ActorInfo || !ActorInfo->AnimInstance.IsValid())
	{
		return false;
	}

	// Check if the montage is still playing
	// The ability would have been interrupted, in which case we should automatically stop the montage
	if (AbilitySystemComponent && Ability)
	{
		if (AbilitySystemComponent->GetAnimatingAbility() == Ability
			&& AbilitySystemComponent->GetCurrentMontage() == MontageToPlay)
		{
			// Unbind delegates so they don't get called as well
			if (FOnMontageBlendingOutStarted* BoundDelegate = ActorInfo->AnimInstance->Montage_GetBlendingOutDelegate(MontageToPlay))
			{
				BoundDelegate->Unbind();
			}

			AbilitySystemComponent->CurrentMontageStop();
			return true;
		}
	}

	return false;
}

FString UAbilityTask_PlayMontageAndWait::GetDebugString() const
{
	UAnimMontage* PlayingMontage = nullptr;
	if (Ability)
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		if (ActorInfo->AnimInstance.IsValid())
		{
			PlayingMontage = ActorInfo->AnimInstance->Montage_IsActive(MontageToPlay) ? MontageToPlay : ActorInfo->AnimInstance->GetCurrentActiveMontage();
		}
	}

	return FString::Printf(TEXT("PlayMontageAndWait. MontageToPlay: %s  (Currently Playing): %s"), *GetNameSafe(MontageToPlay), *GetNameSafe(PlayingMontage));
}