// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "AbilityTask.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilityTask_WaitAbilityActivate.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWaitAbilityActivateDelegate, UGameplayAbility*, ActivatedAbility);

class AActor;
class UPrimitiveComponent;

/**
 *	Waits for the actor to activate another ability
 */
UCLASS(MinimalAPI)
class UAbilityTask_WaitAbilityActivate : public UAbilityTask
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintAssignable)
	FWaitAbilityActivateDelegate	OnActivate;

	virtual void Activate() override;

	UFUNCTION()
	void OnAbilityActivate(UGameplayAbility *ActivatedAbility);

	/** Wait until an overlap occurs. This will need to be better fleshed out so we can specify game specific collision requirements */
	UFUNCTION(BlueprintCallable, meta = (HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_WaitAbilityActivate* WaitForAbilityActivate(UObject* WorldContextObject, FGameplayTag WithTag, FGameplayTag WithoutTag);

	FGameplayTag WithTag;
	FGameplayTag WithoutTag;
};