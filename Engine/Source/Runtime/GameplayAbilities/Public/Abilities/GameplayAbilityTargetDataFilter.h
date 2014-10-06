// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayAbilityTargetDataFilter.generated.h"

USTRUCT(BlueprintType)
struct FGameplayTargetDataFilter
{
	GENERATED_USTRUCT_BODY()

	virtual ~FGameplayTargetDataFilter()
	{
	}

	virtual bool FilterPassesForActor(const AActor* ActorToBeFiltered) const
	{
		return true;
	}

	bool operator()(const TWeakObjectPtr<AActor> A) const
	{
		return FilterPassesForActor(A.Get());
	}

	bool operator()(const AActor* A) const
	{
		return FilterPassesForActor(A);
	}
};


USTRUCT(BlueprintType)
struct FGameplayTargetDataFilterHandle
{
	GENERATED_USTRUCT_BODY()

	TSharedPtr<FGameplayTargetDataFilter> Filter;
};
