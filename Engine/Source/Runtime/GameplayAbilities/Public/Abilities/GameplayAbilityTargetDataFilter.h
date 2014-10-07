// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayAbilityTargetDataFilter.generated.h"

UENUM(BlueprintType)
namespace ETargetDataFilterSelf
{
	enum Type
	{
		TDFS_Any 			UMETA(DisplayName = "Allow self or others"),
		TDFS_NoSelf 		UMETA(DisplayName = "Filter self out"),
		TDFS_NoOthers		UMETA(DisplayName = "Filter others out")
	};
}

USTRUCT(BlueprintType)
struct FGameplayTargetDataFilter
{
	GENERATED_USTRUCT_BODY()

	virtual ~FGameplayTargetDataFilter()
	{
	}

	virtual bool FilterPassesForActor(const AActor* ActorToBeFiltered) const
	{
		check(SourceAbility.IsValid());
		check(SourceAbility.Get()->GetOwningActorFromActorInfo());
		switch (SelfFilter.GetValue())
		{
		case ETargetDataFilterSelf::Type::TDFS_NoOthers:
			if (ActorToBeFiltered != SourceAbility.Get()->GetOwningActorFromActorInfo())
			{
				return false;
			}
			break;
		case ETargetDataFilterSelf::Type::TDFS_NoSelf:
			if (ActorToBeFiltered == SourceAbility.Get()->GetOwningActorFromActorInfo())
			{
				return false;
			}
			break;
		case ETargetDataFilterSelf::Type::TDFS_Any:
		default:
			break;
		}
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

	/** Filled out while running */
	TWeakObjectPtr<UGameplayAbility> SourceAbility;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Filter)
	TEnumAsByte<ETargetDataFilterSelf::Type> SelfFilter;
};


USTRUCT(BlueprintType)
struct FGameplayTargetDataFilterHandle
{
	GENERATED_USTRUCT_BODY()

	TSharedPtr<FGameplayTargetDataFilter> Filter;
};
