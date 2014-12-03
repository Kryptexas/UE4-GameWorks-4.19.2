// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "GameplayCueNotify_Blueprint.h"

UGameplayCueNotify_Blueprint::UGameplayCueNotify_Blueprint(const class FObjectInitializer& PCIP)
: Super(PCIP)
{
	
}

void UGameplayCueNotify_Blueprint::HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, FGameplayCueParameters Parameters)
{
	Super::HandleGameplayCue(MyTarget, EventType, Parameters);

	if (MyTarget && !MyTarget->IsPendingKill())
	{
		K2_HandleGameplayCue(MyTarget, EventType, Parameters);

		switch(EventType)
		{
		case EGameplayCueEvent::OnActive:
			OnActive(MyTarget, Parameters);
			break;

		case EGameplayCueEvent::Executed:
			OnExecute(MyTarget, Parameters);
			break;

		case EGameplayCueEvent::Removed:
			OnRemove(MyTarget, Parameters);
			break;
		};
	}
	else
	{
		ABILITY_LOG(Warning, TEXT("Null Target"));
	}
}

bool UGameplayCueNotify_Blueprint::HandlesEvent(EGameplayCueEvent::Type EventType) const
{
	return true;
}

/** Does this GameplayCueNotify need a per source actor instance for this event? */
bool UGameplayCueNotify_Blueprint::NeedsInstanceForEvent(EGameplayCueEvent::Type EventType) const
{
	return (EventType != EGameplayCueEvent::Executed);
}