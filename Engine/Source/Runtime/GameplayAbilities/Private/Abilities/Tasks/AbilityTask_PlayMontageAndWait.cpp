
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
}

UAbilityTask_PlayMontageAndWait* UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(UObject* WorldContextObject, UAnimMontage *MontageToPlay)
{
	check(WorldContextObject);

	UAbilityTask_PlayMontageAndWait* MyObj = NewObject<UAbilityTask_PlayMontageAndWait>();
	UGameplayAbility* ThisAbility = CastChecked<UGameplayAbility>(WorldContextObject);
	MyObj->Ability = ThisAbility;
	MyObj->MontageToPlay = MontageToPlay;

	return MyObj;
}

void UAbilityTask_PlayMontageAndWait::Activate()
{
	if (Ability.IsValid())
	{
		AActor * ActorOwner = Cast<AActor>(Ability->GetOuter());
				
		TSharedPtr<FGameplayAbilityActorInfo> ActorInfo = TSharedPtr<FGameplayAbilityActorInfo>(UAbilitySystemGlobals::Get().AllocAbilityActorInfo(ActorOwner));
		
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UAbilityTask_PlayMontageAndWait::OnMontageEnded);

		ActorInfo->AnimInstance->Montage_Play(MontageToPlay, 1.f);
		ActorInfo->AnimInstance->Montage_SetEndDelegate(EndDelegate);
	}
}