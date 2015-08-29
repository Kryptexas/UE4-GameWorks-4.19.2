// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagResponseTable.generated.h"


class UGameplayEffect;

USTRUCT()
struct FGameplayTagReponsePair
{
	GENERATED_USTRUCT_BODY()

	/** Tag that triggers this response */
	UPROPERTY(EditAnywhere, Category="Response")
	FGameplayTag	Tag;

	/** The GameplayEffect to apply in reponse to the tag */
	UPROPERTY(EditAnywhere, Category="Response")
	TSubclassOf<UGameplayEffect> ResponseGameplayEffect;
};

USTRUCT()
struct FGameplayTagResponseTableEntry
{
	GENERATED_USTRUCT_BODY()

	/** Tags that count as "positive" toward to final response count. If the overall count is positive, this ResponseGameplayEffect is applied. */
	UPROPERTY(EditAnywhere, Category="Response")
	FGameplayTagReponsePair		Positive;

	/** Tags that count as "negative" toward to final response count. If the overall count is negative, this ResponseGameplayEffect is applied. */
	UPROPERTY(EditAnywhere, Category="Response")
	FGameplayTagReponsePair		Negative;
};

/**
 *	A data driven table for applying gameplay effects based on tag count. This allows designers to map a 
 *	"tag count" -> "response Gameplay Effect" relationship.
 *	
 *	For example, "for every stack of "Status.Haste" I get 1 level of GE_Response_Haste. This class facilitates
 *	building this table and automatically registering and responding to tag events on the ability system component.
 */
UCLASS()
class GAMEPLAYABILITIES_API UGameplayTagReponseTable : public UDataAsset
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category="Response")
	TArray<FGameplayTagResponseTableEntry>	Entries;

	/** Registers for tag events for the given ability system component. Note this will happen to every spawned ASC, we may want to allow games
	 *	to limit what classe this is called on, or potentially build into the table class restrictions for each response entry.
	 */
	void RegisterResponseForEvents(UAbilitySystemComponent* ASC);

protected:

	UFUNCTION()
	void TagResponseEvent(const FGameplayTag Tag, int32 NewCount, UAbilitySystemComponent* ASC, int32 idx);

	FGameplayEffectQuery MakeQuery(const FGameplayTag& Tag)
	{
		return FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Tag));
	}


	// ----------------------------------------------------

	struct FGameplayTagResponseAppliedInfo
	{
		FActiveGameplayEffectHandle PositiveHandle;
		FActiveGameplayEffectHandle NegativeHandle;
	};

	TMap< TWeakObjectPtr<UAbilitySystemComponent>, FGameplayTagResponseAppliedInfo > RegisteredASCs;

	void Remove(UAbilitySystemComponent* ASC, FActiveGameplayEffectHandle& Handle)
	{
		if (Handle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Handle);
			Handle.Invalidate();
		}
	}

	void AddOrUpdate(UAbilitySystemComponent* ASC, const TSubclassOf<UGameplayEffect>& ResponseGameplayEffect, int32 TotalCount, FActiveGameplayEffectHandle& Handle)
	{
		if (ResponseGameplayEffect)
		{
			if (Handle.IsValid())
			{
				ASC->SetActiveGameplayEffectLevel(Handle, TotalCount);
			}
			else
			{
				Handle = ASC->ApplyGameplayEffectToSelf(Cast<UGameplayEffect>(ResponseGameplayEffect->ClassDefaultObject), TotalCount, ASC->GetEffectContext());
			}
		}
	}
};