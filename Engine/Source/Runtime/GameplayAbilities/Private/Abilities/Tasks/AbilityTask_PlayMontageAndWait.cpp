
#include "AbilitySystemPrivatePCH.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"

UAbilityTask_PlayMontageAndWait::UAbilityTask_PlayMontageAndWait(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Rate = 1.f;
}

void UAbilityTask_PlayMontageAndWait::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
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

UAbilityTask_PlayMontageAndWait* UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(UObject* WorldContextObject, UAnimMontage *MontageToPlay, float Rate)
{
	UAbilityTask_PlayMontageAndWait* MyObj = NewTask<UAbilityTask_PlayMontageAndWait>(WorldContextObject);
	MyObj->MontageToPlay = MontageToPlay;
	MyObj->Rate = Rate;

	return MyObj;
}

void UAbilityTask_PlayMontageAndWait::Activate()
{
	if (AbilitySystemComponent.IsValid() && Ability.IsValid())
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		if (ActorInfo->AnimInstance.IsValid())
		{
			if (AbilitySystemComponent->PlayMontage(Ability.Get(), Ability->GetCurrentActivationInfo(), MontageToPlay, Rate	) > 0.f)
			{
				FOnMontageEnded EndDelegate;
				EndDelegate.BindUObject(this, &UAbilityTask_PlayMontageAndWait::OnMontageEnded);
				ActorInfo->AnimInstance->Montage_SetEndDelegate(EndDelegate);
			}
			else
			{
				ABILITY_LOG(Warning, TEXT("UAbilityTask_PlayMontageAndWait called in Ability %s with null montage."), *Ability->GetName());
			}
		}
	}
}

void UAbilityTask_PlayMontageAndWait::OnDestroy(bool AbilityEnded)
{
	// Note: Clearing montage end delegate isn't necessary since its not a multicast and will be cleared when the next montage plays.
	// (If we are destroyed, it will detect this and not do anything)

	Super::OnDestroy(AbilityEnded);
}

FString UAbilityTask_PlayMontageAndWait::GetDebugString() const
{
	UAnimMontage* PlayingMontage = nullptr;
	if (Ability.IsValid())
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		if (ActorInfo->AnimInstance.IsValid())
		{
			PlayingMontage = ActorInfo->AnimInstance->GetCurrentActiveMontage();
		}
	}

	return FString::Printf(TEXT("PlayMontageAndWait. MontageToPlay: %s  (Currently Playing): %s"), *GetNameSafe(MontageToPlay), *GetNameSafe(PlayingMontage));
}