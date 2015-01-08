// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEffectApplied_Self.h"

#include "AbilitySystemComponent.h"

#include "Abilities/GameplayAbility.h"
#include "GameplayEffectExtension.h"

UAbilityTask_WaitGameplayEffectApplied_Self::UAbilityTask_WaitGameplayEffectApplied_Self(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UAbilityTask_WaitGameplayEffectApplied_Self* UAbilityTask_WaitGameplayEffectApplied_Self::WaitGameplayEffectAppliedToSelf(UObject* WorldContextObject, const FGameplayTargetDataFilterHandle InFilter, FGameplayTagRequirements InSourceTagRequirements, FGameplayTagRequirements InTargetTagRequirements, bool InTriggerOnce)
{
	auto MyObj = NewTask<UAbilityTask_WaitGameplayEffectApplied_Self>(WorldContextObject);
	MyObj->Filter = InFilter;
	MyObj->SourceTagRequirements = InSourceTagRequirements;
	MyObj->TargetTagRequirements = InTargetTagRequirements;
	MyObj->TriggerOnce = InTriggerOnce;
	return MyObj;
}

void UAbilityTask_WaitGameplayEffectApplied_Self::BroadcastDelegate(AActor* Avatar, FGameplayEffectSpecHandle SpecHandle, FActiveGameplayEffectHandle ActiveHandle)
{
	OnApplied.Broadcast(Avatar, SpecHandle, ActiveHandle);
}

void UAbilityTask_WaitGameplayEffectApplied_Self::RegisterDelegate()
{
	OnApplyGameplayEffectCallbackDelegateHandle = AbilitySystemComponent->OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UAbilityTask_WaitGameplayEffectApplied::OnApplyGameplayEffectCallback);
}

void UAbilityTask_WaitGameplayEffectApplied_Self::RemoveDelegate()
{
	AbilitySystemComponent->OnGameplayEffectAppliedDelegateToSelf.Remove(OnApplyGameplayEffectCallbackDelegateHandle);
}