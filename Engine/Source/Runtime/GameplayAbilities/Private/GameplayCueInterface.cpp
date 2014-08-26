// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemComponent.h"
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



void FActiveGameplayCue::PreReplicatedRemove(const struct FActiveGameplayCueContainer &InArray)
{
	InArray.Owner->InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::Removed);
}

void FActiveGameplayCue::PostReplicatedAdd(const struct FActiveGameplayCueContainer &InArray)
{
	InArray.Owner->InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::WhileActive);
}

void FActiveGameplayCueContainer::AddCue(const FGameplayTag& Tag)
{
	UWorld* World = Owner->GetWorld();

	FActiveGameplayCue	NewCue;
	NewCue.GameplayCueTag =Tag;
	MarkItemDirty(NewCue);

	GameplayCues.Add(NewCue);
}

void FActiveGameplayCueContainer::RemoveCue(const FGameplayTag& Tag)
{
	for (int32 idx=0; idx < GameplayCues.Num(); ++idx)
	{
		FActiveGameplayCue& Cue = GameplayCues[idx];

		if (Cue.GameplayCueTag == Tag)
		{
			GameplayCues.RemoveAt(idx);
			MarkArrayDirty();
			return;
		}
	}
}

bool FActiveGameplayCueContainer::HasMatchingGameplayTag(FGameplayTag TagToCheck) const
{
	for (const FActiveGameplayCue& Cue : GameplayCues)
	{
		/** We expect that if TagToCheck=(a.b) and we have a (a.b.c) tag, then we match  */
		if (Cue.GameplayCueTag.Matches(EGameplayTagMatchType::IncludeParentTags, TagToCheck, EGameplayTagMatchType::Explicit))
		{
			return true;
		}
	}

	return false;
}