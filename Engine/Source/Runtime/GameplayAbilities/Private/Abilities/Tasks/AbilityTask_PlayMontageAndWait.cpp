
#include "AbilitySystemPrivatePCH.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UAbilityTask_PlayMontageAndWait::UAbilityTask_PlayMontageAndWait(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

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

UAbilityTask_PlayMontageAndWait* UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(UObject* WorldContextObject, UAnimMontage *MontageToPlay)
{
	UAbilityTask_PlayMontageAndWait* MyObj = NewTask<UAbilityTask_PlayMontageAndWait>(WorldContextObject);
	MyObj->MontageToPlay = MontageToPlay;

	return MyObj;
}

void UAbilityTask_PlayMontageAndWait::Activate()
{
	if (Ability.IsValid())
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		if (ActorInfo->AnimInstance.IsValid())
		{
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &UAbilityTask_PlayMontageAndWait::OnMontageEnded);

			ActorInfo->AnimInstance->Montage_Play(MontageToPlay, 1.f);
			ActorInfo->AnimInstance->Montage_SetEndDelegate(EndDelegate);
		}
	}
}

void UAbilityTask_PlayMontageAndWait::OnDestroy(bool AbilityEnded)
{
	// Note: Clearing montage end delegate isn't necessary since its not a multicast and will be cleared when the next montage plays.
	// (If we are destroyed, it will detect this and not do anything)

	Super::OnDestroy(AbilityEnded);
}