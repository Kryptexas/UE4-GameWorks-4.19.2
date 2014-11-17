
#include "AbilitySystemPrivatePCH.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"

UAbilityTask_WaitInputRelease::UAbilityTask_WaitInputRelease(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	StartTime = 0.f;
	RegisteredCallback= false;
}

void UAbilityTask_WaitInputRelease::OnReleaseCallback(UGameplayAbility* InAbility)
{
	float ElapsedTime = GetWorld()->GetTimeSeconds() - StartTime;

	if (Ability->GetCurrentActivationInfo().ActivationMode != EGameplayAbilityActivationMode::Authority)
	{
		// Tell the server we released.
		AbilitySystemComponent->ServerInputRelease(Ability->GetCurrentAbilitySpecHandle());
	}

	// We are done. Kill us so we don't keep getting broadcast messages
	OnRelease.Broadcast(ElapsedTime);
	EndTask();
}

UAbilityTask_WaitInputRelease* UAbilityTask_WaitInputRelease::WaitInputRelease(class UObject* WorldContextObject)
{
	return NewTask<UAbilityTask_WaitInputRelease>(WorldContextObject);
}

void UAbilityTask_WaitInputRelease::Activate()
{
	if (Ability.IsValid())
	{
		FGameplayAbilitySpec* Spec = Ability->GetCurrentAbilitySpec();
		if (Spec)
		{
			if (!Spec->InputPressed)
			{
				// Input was already released before we got here, we are done.
				OnRelease.Broadcast(0.f);
				EndTask();
			}
			else
			{
				StartTime = GetWorld()->GetTimeSeconds();
				Ability->OnInputRelease.AddUObject(this, &UAbilityTask_WaitInputRelease::OnReleaseCallback);
				RegisteredCallback = true;
			}
		}
	}
}

void UAbilityTask_WaitInputRelease::OnDestroy(bool AbilityEnded)
{
	if (RegisteredCallback && Ability.IsValid())
	{
		Ability->OnInputRelease.RemoveUObject(this, &UAbilityTask_WaitInputRelease::OnReleaseCallback);
	}

	Super::OnDestroy(AbilityEnded);
}
