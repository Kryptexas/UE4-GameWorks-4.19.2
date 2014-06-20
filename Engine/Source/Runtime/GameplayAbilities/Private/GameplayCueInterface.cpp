// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "GameplayCueInterface.h"

UGameplayCueInterface::UGameplayCueInterface(const FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
}


void IGameplayCueInterface::DispatchBlueprintCustomHandler(AActor* Actor, UFunction* Func, EGameplayCueEvent::Type EventType, FGameplayCueParameters Parameters)
{
	GameplayCueInterface_eventBlueprintCustomHandler_Parms Parms;
	Parms.EventType = EventType;
	Parms.Parameters = Parameters;

	Actor->ProcessEvent(Func, &Parms);
}

void IGameplayCueInterface::HandleGameplayCues(AActor *Self, const FGameplayTagContainer& GameplayCueTags, EGameplayCueEvent::Type EventType, FGameplayCueParameters Parameters)
{
	for (auto TagIt = GameplayCueTags.CreateConstIterator(); TagIt; ++TagIt)
	{
		HandleGameplayCue(Self, *TagIt, EventType, Parameters);
	}
}

void IGameplayCueInterface::HandleGameplayCue(AActor *Self, FGameplayTag GameplayCueTag, EGameplayCueEvent::Type EventType, FGameplayCueParameters Parameters)
{
	// Look up a custom function for this gameplay tag. 
	// This is done in UAbilitySystemGlobals since we will eventually cache these functions off in a global map

	UFunction* Func = UAbilitySystemGlobals::Get().GetGameplayCueFunction(GameplayCueTag, Self->GetClass(), Parameters.MatchedTagName);
	if (Func)
	{
		IGameplayCueInterface::DispatchBlueprintCustomHandler(Self, Func, EventType, Parameters);
	}

	// Fall back to a possible generic handler if available
}