// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "AbilityTask.h"
#include "AbilityTask_WaitTargetData.generated.h"

class AGameplayAbilityTargetActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWaitTargetDataDelegate, FGameplayAbilityTargetDataHandle, Data);

UCLASS(MinimalAPI)
class UAbilityTask_WaitTargetData: public UAbilityTask
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintAssignable)
	FWaitTargetDataDelegate	ValidData;

	UPROPERTY(BlueprintAssignable)
	FWaitTargetDataDelegate	Cancelled;

	UFUNCTION()
	void OnTargetDataReplicatedCallback(FGameplayAbilityTargetDataHandle Data);

	UFUNCTION()
	void OnTargetDataReplicatedCancelledCallback();

	UFUNCTION()
	void OnTargetDataReadyCallback(FGameplayAbilityTargetDataHandle Data);

	UFUNCTION()
	void OnTargetDataCancelledCallback(FGameplayAbilityTargetDataHandle Data);
	
	
	UFUNCTION(BlueprintCallable, meta=(HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject", BlueprintInternalUseOnly = "true"), Category = "Abilities")
	static UAbilityTask_WaitTargetData* WaitTargetData(UObject* WorldContextObject, TSubclassOf<AGameplayAbilityTargetActor> TargetClass);

	virtual void Activate() override;

	TSubclassOf<AGameplayAbilityTargetActor> TargetClass;

	void Cleanup();

	TWeakObjectPtr<AActor>	MySpawnedTargetActor;
};