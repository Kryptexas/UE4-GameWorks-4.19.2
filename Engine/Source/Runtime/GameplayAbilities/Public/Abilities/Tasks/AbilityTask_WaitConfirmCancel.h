// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "AbilityTask.h"
#include "GameplayAbility_Instanced.h"
#include "AbilityTask_WaitConfirmCancel.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWaitConfirmCancelDelegate, bool, Test);

UCLASS(MinimalAPI)
class UAbilityTask_WaitConfirmCancel : public UAbilityTask
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintAssignable)
	FWaitConfirmCancelDelegate	OnConfirm;

	UPROPERTY(BlueprintAssignable)
	FWaitConfirmCancelDelegate	OnCancel;
	
	UFUNCTION()
	void OnConfirmCallback();

	UFUNCTION()
	void OnCancelCallback();

	UFUNCTION(BlueprintCallable, meta=(HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject", BlueprintInternalUseOnly = "true", FriendlyName="Wait for Confirm Input"), Category="Abilities")
	static UAbilityTask_WaitConfirmCancel* WaitConfirmCancel(UObject* WorldContextObject);	

	virtual void Activate() OVERRIDE;

	UPROPERTY()
	UAbilitySystemComponent* ASC;

public:
	
};