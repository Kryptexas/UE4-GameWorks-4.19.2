
#include "AbilitySystemPrivatePCH.h"
#include "Abilities/Tasks/AbilityTask_WaitConfirm.h"

UAbilityTask_WaitConfirm::UAbilityTask_WaitConfirm(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

}

void UAbilityTask_WaitConfirm::OnConfirmCallback(UGameplayAbility* InAbility)
{
	OnConfirm.Broadcast();

	// We are done. Kill us so we don't keep getting broadcast messages
	MarkPendingKill();
}

UAbilityTask_WaitConfirm* UAbilityTask_WaitConfirm::WaitConfirm(class UObject* WorldContextObject)
{
	check(WorldContextObject);

	UAbilityTask_WaitConfirm * MyObj = NewObject<UAbilityTask_WaitConfirm>();
	UGameplayAbility* ThisAbility = CastChecked<UGameplayAbility>(WorldContextObject);
	MyObj->InitTask(ThisAbility);	

	return MyObj;
}

void UAbilityTask_WaitConfirm::Activate()
{
	if (Ability.IsValid())
	{
		if (Ability->GetCurrentActivationInfo().ActivationMode == EGameplayAbilityActivationMode::Predicting)
		{
			// Register delegate so that when we are confirmed by the server, we call OnConfirmCallback
			Ability->OnConfirmDelegate.AddUObject(this, &UAbilityTask_WaitConfirm::OnConfirmCallback);
		}
		else
		{
			// We have already been confirmed, just call the OnConfirm callback
			OnConfirmCallback(Ability.Get());
		}
	}
}