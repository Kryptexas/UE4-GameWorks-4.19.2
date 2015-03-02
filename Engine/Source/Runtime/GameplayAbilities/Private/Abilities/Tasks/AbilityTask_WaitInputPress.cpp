// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "GameplayPrediction.h"

UAbilityTask_WaitInputPress::UAbilityTask_WaitInputPress(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	StartTime = 0.f;
}

void UAbilityTask_WaitInputPress::OnPressCallback()
{
	float ElapsedTime = GetWorld()->GetTimeSeconds() - StartTime;

	if (!Ability.IsValid() || !AbilitySystemComponent.IsValid())
	{
		return;
	}

	AbilitySystemComponent->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::InputPressed, GetAbilitySpecHandle(), GetActivationPredictionKey()).Remove(DelegateHandle);

	if (IsPredictingClient())
	{
		// Tell the server about this
		AbilitySystemComponent->ServerSetReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, GetAbilitySpecHandle(), GetActivationPredictionKey(), AbilitySystemComponent->ScopedPredictionKey);
	}
	else
	{
		AbilitySystemComponent->ConsumeGenericReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, GetAbilitySpecHandle(), GetActivationPredictionKey());
	}

	// We are done. Kill us so we don't keep getting broadcast messages
	OnPress.Broadcast(ElapsedTime);
	EndTask();
}

UAbilityTask_WaitInputPress* UAbilityTask_WaitInputPress::WaitInputPress(class UObject* WorldContextObject)
{
	return NewTask<UAbilityTask_WaitInputPress>(WorldContextObject);
}

void UAbilityTask_WaitInputPress::Activate()
{
	StartTime = GetWorld()->GetTimeSeconds();
	if (Ability.IsValid())
	{
		DelegateHandle = AbilitySystemComponent->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::InputPressed, GetAbilitySpecHandle(), GetActivationPredictionKey()).AddUObject(this, &UAbilityTask_WaitInputPress::OnPressCallback);
		if (IsForRemoteClient())
		{
			AbilitySystemComponent->CallReplicatedEventDelegateIfSet(EAbilityGenericReplicatedEvent::InputPressed, GetAbilitySpecHandle(), GetActivationPredictionKey());
		}
	}
}

void UAbilityTask_WaitInputPress::OnDestroy(bool AbilityEnded)
{
	Super::OnDestroy(AbilityEnded);
}
