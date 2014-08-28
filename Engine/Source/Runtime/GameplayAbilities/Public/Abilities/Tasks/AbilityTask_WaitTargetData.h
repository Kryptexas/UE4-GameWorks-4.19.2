// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "AbilityTask.h"
#include "GameplayAbilityTargetTypes.h"
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

	/** Spawns Targeting actor and waits for it to return valid data or to be cancelled. */
	UFUNCTION(BlueprintCallable, meta=(HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject", BlueprintInternalUseOnly = "true", HideSpawnParms="Instigator"), Category="Ability|Tasks")
	static UAbilityTask_WaitTargetData* WaitTargetData(UObject* WorldContextObject, TEnumAsByte<EGameplayTargetingConfirmation::Type> ConfirmationType, TSubclassOf<AGameplayAbilityTargetActor> Class);

	UFUNCTION(BlueprintCallable, meta = (HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject", BlueprintInternalUseOnly = "true"), Category = "Abilities")
	bool BeginSpawningActor(UObject* WorldContextObject, TSubclassOf<AGameplayAbilityTargetActor> Class, AGameplayAbilityTargetActor*& SpawnedActor);

	UFUNCTION(BlueprintCallable, meta = (HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject", BlueprintInternalUseOnly = "true"), Category = "Abilities")
	void FinishSpawningActor(UObject* WorldContextObject, AGameplayAbilityTargetActor* SpawnedActor);

protected:

	virtual void OnDestroy(bool AbilityEnded) override;

	bool ShouldReplicateDataToServer() const;

	TSubclassOf<AGameplayAbilityTargetActor> TargetClass;

	/** The TargetActor that we spawned, or the class CDO if this is a static targeting task */
	TWeakObjectPtr<AGameplayAbilityTargetActor>	MyTargetActor;

	TEnumAsByte<EGameplayTargetingConfirmation::Type> ConfirmationType;
};


/**
*	Requirements for using Begin/Finish SpawningActor functionality:
*		-Have a parameters named 'Class' in your Proxy factor function (E.g., WaitTargetdata)
*		-Have a function named BeginSpawningActor w/ the same Class parameter
*			-This function should spawn the actor with SpawnActorDeferred and return true/false if it spawned something.
*		-Have a function named FinishSpawningActor w/ an AActor* of the class you spawned
*			-This function *must* call ExecuteConstruction + PostActorConstruction
*/