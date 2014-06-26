
#include "AbilitySystemPrivatePCH.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEffectRemoved.h"

#include "AbilitySystemComponent.h"

#include "Abilities/GameplayAbility.h"

UAbilityTask_WaitGameplayEffectRemoved::UAbilityTask_WaitGameplayEffectRemoved(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

UAbilityTask_WaitGameplayEffectRemoved* UAbilityTask_WaitGameplayEffectRemoved::WaitForGameplayEffectRemoved(UObject* WorldContextObject, FActiveGameplayEffectHandle InHandle)
{
	check(WorldContextObject);
	UGameplayAbility* ThisAbility = CastChecked<UGameplayAbility>(WorldContextObject);
	if (ThisAbility)
	{
		UAbilityTask_WaitGameplayEffectRemoved * MyObj = NULL;
		MyObj = NewObject<UAbilityTask_WaitGameplayEffectRemoved>();
		MyObj->InitTask(ThisAbility);
		MyObj->Handle = InHandle;

		return MyObj;
	}
	return NULL;
}

void UAbilityTask_WaitGameplayEffectRemoved::Activate()
{
	if (AbilitySystemComponent.IsValid())	
	{		
		FOnActiveGameplayEffectRemoved* DelPtr = AbilitySystemComponent->OnGameplayEffectRemovedDelegate(Handle);
		if (DelPtr)
		{
			DelPtr->AddUObject(this, &UAbilityTask_WaitGameplayEffectRemoved::OnGameplayEffectRemoved);
		}
		else
		{
			// GameplayEffect was already removed, treat this as a warning? Could be cases of immunity or chained gameplay rules that would instant remove something
			OnGameplayEffectRemoved();
		}
	}
}

void UAbilityTask_WaitGameplayEffectRemoved::OnGameplayEffectRemoved()
{
	OnRemoved.Broadcast();
	MarkPendingKill();
}