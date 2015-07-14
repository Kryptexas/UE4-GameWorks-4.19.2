// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "GameplayTagResponseTable.h"

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	UGameplayTagReponseTable
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------


UGameplayTagReponseTable::UGameplayTagReponseTable(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Query.OwningTagContainer = &QueryContainer;
}

void UGameplayTagReponseTable::RegisterResponseForEvents(UAbilitySystemComponent* ASC)
{
	if (RegisteredASCs.Contains(ASC))
	{
		return;
	}

	RegisteredASCs.Add(ASC, FGameplayTagResponseAppliedInfo() );

	for (int32 idx=0; idx < Entries.Num(); ++idx)
	{
		const FGameplayTagResponseTableEntry& Entry = Entries[idx];
		if (Entry.Positive.Tag.IsValid())
		{
			ASC->RegisterGameplayTagEvent( Entry.Positive.Tag, EGameplayTagEventType::AnyCountChange ).AddUObject(this, &UGameplayTagReponseTable::TagResponseEvent, ASC, idx);
		}
		if (Entry.Negative.Tag.IsValid())
		{
			ASC->RegisterGameplayTagEvent( Entry.Negative.Tag, EGameplayTagEventType::AnyCountChange ).AddUObject(this, &UGameplayTagReponseTable::TagResponseEvent, ASC, idx);
		}
	}
}

void UGameplayTagReponseTable::TagResponseEvent(const FGameplayTag Tag, int32 NewCount, UAbilitySystemComponent* ASC, int32 idx)
{
	if (!ensure(Entries.IsValidIndex(idx)))
	{
		return;
	}

	const FGameplayTagResponseTableEntry& Entry = Entries[idx];
	int32 TotalCount = 0;

	{
		QUICK_SCOPE_CYCLE_COUNTER(ABILITY_TRT_CALC_COUNT);

		TotalCount += ASC->GetAggregatedStackCount(MakeQuery(Entry.Positive.Tag));
		TotalCount -= ASC->GetAggregatedStackCount(MakeQuery(Entry.Negative.Tag));
	}

	FGameplayTagResponseAppliedInfo& Info = RegisteredASCs.FindChecked(ASC);

	if (TotalCount < 0)
	{
		Remove(ASC, Info.PositiveHandle);
		AddOrUpdate(ASC, Entry.Negative.ResponseGameplayEffect, TotalCount, Info.NegativeHandle);
	}
	else if (TotalCount > 0)
	{
		Remove(ASC, Info.NegativeHandle);
		AddOrUpdate(ASC, Entry.Positive.ResponseGameplayEffect, TotalCount, Info.PositiveHandle);
	}
	else if (TotalCount == 0)
	{
		Remove(ASC, Info.PositiveHandle);
		Remove(ASC, Info.NegativeHandle);
	}
}