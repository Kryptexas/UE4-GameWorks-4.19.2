// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "GameplayCueView.h"
#include "AbilitySystemBlueprintLibrary.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitMovementModeChange;
class UAbilityTask_WaitOverlap;
class UAbilityTask_WaitConfirmCancel;

// meta =(RestrictedToClasses="GameplayAbility")
UCLASS(MinimalAPI)
class UAbilitySystemBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintPure, Category = Ability)
	static UAbilitySystemComponent* GetAbilitySystemComponent(AActor *Actor);

	// -------------------------------------------------------------------------------
	//		TargetData
	// -------------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Ability|TargetData")
	static TArray<FActiveGameplayEffectHandle> ApplyGameplayEffectToTargetData(FGameplayAbilityTargetDataHandle Target, UGameplayEffect *GameplayEffect, const FGameplayAbilityActorInfo InstigatorInfo, int32 Level=1);

	/** Applies GameplayEffect to TargetData and returns the handle of the  */
	UFUNCTION(BlueprintCallable, Category = "Ability|TargetData")
	static FActiveGameplayEffectHandle ApplyGameplayEffectToTargetData_Single(FGameplayAbilityTargetDataHandle Target, UGameplayEffect *GameplayEffect, const FGameplayAbilityActorInfo InstigatorInfo, int32 Level=1);

	UFUNCTION(BlueprintCallable, Category = "Ability|TargetData")
	static FGameplayAbilityTargetDataHandle AppendTargetDataHandle(FGameplayAbilityTargetDataHandle TargetHandle, FGameplayAbilityTargetDataHandle HandleToAdd);

	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static FGameplayAbilityTargetDataHandle	AbilityTargetDataFromLocations(const FGameplayAbilityTargetingLocationInfo& SourceLocation, const FGameplayAbilityTargetingLocationInfo& TargetLocation);

	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static FGameplayAbilityTargetDataHandle	AbilityTargetDataHandleFromAbilityTargetDataMesh(FGameplayAbilityTargetData_Mesh Data);

	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static FGameplayAbilityTargetDataHandle	AbilityTargetDataFromHitResult(FHitResult HitResult);

	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static int32 GetDataCountFromTargetData(FGameplayAbilityTargetDataHandle TargetData);

	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static FGameplayAbilityTargetDataHandle	AbilityTargetDataFromActor(AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static TArray<AActor*> GetActorsFromTargetData(FGameplayAbilityTargetDataHandle TargetData, int32 Index);

	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static bool TargetDataHasHitResult(FGameplayAbilityTargetDataHandle HitResult, int32 Index);

	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static FHitResult GetHitResultFromTargetData(FGameplayAbilityTargetDataHandle HitResult, int32 Index);

	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static bool TargetDataHasEndPoint(FGameplayAbilityTargetDataHandle TargetData, int32 Index);

	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static FVector GetTargetDataEndPoint(FGameplayAbilityTargetDataHandle TargetData, int32 Index);

	// -------------------------------------------------------------------------------
	//		GameplayCue
	// -------------------------------------------------------------------------------
	
	UFUNCTION(BlueprintPure, Category="Ability|GameplayCue")
	static bool IsInstigatorLocallyControlled(FGameplayCueParameters Parameters);

	UFUNCTION(BlueprintPure, Category = "Ability|GameplayCue")
	static FHitResult GetHitResult(FGameplayCueParameters Parameters);

	UFUNCTION(BlueprintPure, Category = "Ability|GameplayCue")
	static bool HasHitResult(FGameplayCueParameters Parameters);


	// -------------------------------------------------------------------------------

};
