// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "Abilities/Tasks/AbilityTask_StartAbilityState.h"
#include "AbilitySystemComponent.h"

UAbilityTask_StartAbilityState::UAbilityTask_StartAbilityState(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bEndCurrentState = true;
}

UAbilityTask_StartAbilityState* UAbilityTask_StartAbilityState::StartAbilityState(UObject* WorldContextObject, FName StateName, bool bEndCurrentState)
{
	auto Task = NewTask<UAbilityTask_StartAbilityState>(WorldContextObject, StateName);
	Task->bEndCurrentState = bEndCurrentState;
	return Task;
}

void UAbilityTask_StartAbilityState::Activate()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();

	if (ASC)
	{
		if (bEndCurrentState && ASC->OnAbilityStateEnded.IsBound())
		{
			ASC->OnAbilityStateEnded.Broadcast(NAME_None);
		}

		EndStateHandle = ASC->OnAbilityStateEnded.AddUObject(this, &UAbilityTask_StartAbilityState::OnEndState);
		InterruptStateHandle = ASC->OnAbilityStateInterrupted.AddUObject(this, &UAbilityTask_StartAbilityState::OnInterruptState);
	}
}

void UAbilityTask_StartAbilityState::OnDestroy(bool AbilityEnded)
{	
	// If the ability was ended, make sure to first notify the BP via 'OnStateEnded'
	if (AbilityEnded && OnStateEnded.IsBound())
	{
		OnStateEnded.Broadcast();
	}

	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();

	if (ASC)
	{
		ASC->OnAbilityStateInterrupted.Remove(InterruptStateHandle);
		ASC->OnAbilityStateEnded.Remove(EndStateHandle);
	}

	Super::OnDestroy(AbilityEnded);
}

void UAbilityTask_StartAbilityState::OnEndState(FName StateNameToEnd)
{
	// All states end if 'Name_NONE' is passed to this delegate
	if (StateNameToEnd.IsNone() || StateNameToEnd == InstanceName)
	{
		EndTask();

		if (OnStateEnded.IsBound())
		{
			OnStateEnded.Broadcast();
		}
	}
}

void UAbilityTask_StartAbilityState::OnInterruptState()
{
	EndTask();

	if (OnStateInterrupted.IsBound())
	{
		OnStateInterrupted.Broadcast();
	}
}

FString UAbilityTask_StartAbilityState::GetDebugString() const
{
	return FString::Printf(TEXT("%s (AbilityState)"), *InstanceName.ToString());
}
