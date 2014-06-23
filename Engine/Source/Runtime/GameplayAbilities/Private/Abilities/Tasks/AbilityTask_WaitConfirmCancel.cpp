
#include "AbilitySystemPrivatePCH.h"
#include "Abilities/Tasks/AbilityTask_WaitConfirmCancel.h"

#include "AbilitySystemComponent.h"

#include "Abilities/GameplayAbility.h"

UAbilityTask_WaitConfirmCancel::UAbilityTask_WaitConfirmCancel(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

}

void UAbilityTask_WaitConfirmCancel::OnConfirmCallback()
{
	OnConfirm.Broadcast();
	if (AbilitySystemComponent.IsValid())
	{
		AbilitySystemComponent->ConsumeAbilityConfirmCancel();
		MarkPendingKill();
	}
}

void UAbilityTask_WaitConfirmCancel::OnCancelCallback()
{
	OnCancel.Broadcast();
	if (AbilitySystemComponent.IsValid())
	{
		AbilitySystemComponent->ConsumeAbilityConfirmCancel();
		MarkPendingKill();
	}
}

UAbilityTask_WaitConfirmCancel* UAbilityTask_WaitConfirmCancel::WaitConfirmCancel(UObject* WorldContextObject)
{
	check(WorldContextObject);
	UGameplayAbility* ThisAbility = CastChecked<UGameplayAbility>(WorldContextObject);
	if (ThisAbility)
	{
		UAbilityTask_WaitConfirmCancel * MyObj = NULL;
		MyObj = NewObject<UAbilityTask_WaitConfirmCancel>();
		MyObj->InitTask(ThisAbility);

		return MyObj;
	}
	return NULL;
}

void UAbilityTask_WaitConfirmCancel::Activate()
{
	if (AbilitySystemComponent.IsValid())
	{
		const FGameplayAbilityActorInfo* Info = Ability.Get()->GetCurrentActorInfo();

		// If not locally controlled (E.g., we are the server and this is an ability running on the client), check to see if they already sent us
		// the Confirm/Cancel. If so, immediately end this task.
		if (!Info->IsLocallyControlled())
		{
			if (AbilitySystemComponent->ReplicatedConfirmAbility)
			{
				OnConfirmCallback();
				return;
			}
			else if (AbilitySystemComponent->ReplicatedCancelAbility)
			{
				OnCancelCallback();
				return;
			}
		}
		
		// We have to wait for the callback from the AbilitySystemComponent. Which may be instigated locally or through network replication

		AbilitySystemComponent->ConfirmCallbacks.AddDynamic(this, &UAbilityTask_WaitConfirmCancel::OnConfirmCallback);	// Tell me if the confirm input is pressed
		AbilitySystemComponent->CancelCallbacks.AddDynamic(this, &UAbilityTask_WaitConfirmCancel::OnCancelCallback);	// Tell me if the cancel input is pressed
		
		// Tell me if something else tells me to 'force' my target data (an ability may allow targeting during animation/limited time)
		// Tell me if something else tells me to cancel my targeting
	}
}