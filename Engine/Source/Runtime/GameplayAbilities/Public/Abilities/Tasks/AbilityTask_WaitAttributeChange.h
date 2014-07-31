// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "AbilityTask.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AttributeSet.h"
#include "AbilityTask_WaitAttributeChange.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWaitAttributeChangeDelegate);

struct FGameplayEffectModCallbackData;

/**
 *	Waits for the actor to activate another ability
 */
UCLASS(MinimalAPI)
class UAbilityTask_WaitAttributeChange : public UAbilityTask
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintAssignable)
	FWaitAttributeChangeDelegate	OnChange;

	virtual void Activate() override;
		
	void OnAttributeChange(float NewValue, const FGameplayEffectModCallbackData*);

	/** Wait until an attribute changes. */
	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta = (HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_WaitAttributeChange* WaitForAttributeChange(UObject* WorldContextObject, FGameplayAttribute Attribute, FGameplayTag WithTag, FGameplayTag WithoutTag);

	FGameplayTag WithTag;
	FGameplayTag WithoutTag;
	FGameplayAttribute	Attribute;
};