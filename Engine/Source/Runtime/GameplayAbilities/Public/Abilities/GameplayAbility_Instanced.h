// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "GameplayAbility.h"
#include "GameplayAbility_Instanced.generated.h"

class UAnimInstance;
class UAbilitySystemComponent;

/**
* UGameplayEffect
*	The GameplayEffect definition. This is the data asset defined in the editor that drives everything.
*/
UCLASS(Blueprintable)
class GAMEPLAYABILITIES_API UGameplayAbility_Instanced : public UGameplayAbility
{
	GENERATED_UCLASS_BODY()

public:

	virtual void ClientActivateAbilitySucceed_Internal(int32 PredictionKey) OVERRIDE;
	
	// -------------------------------------------------

	UFUNCTION(BlueprintImplementableEvent, Category = Ability)
	virtual bool K2_CanActivateAbility() const;

	virtual bool CanActivateAbility(const FGameplayAbilityActorInfo* ActorInfo) const OVERRIDE;

	// -------------------------------------------------

	UFUNCTION(BlueprintImplementableEvent, Category = Ability)
	virtual void K2_CommitExecute();

	virtual void CommitExecute(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) OVERRIDE;

	// -------------------------------------------------

	UFUNCTION(BlueprintImplementableEvent, Category = Ability)
	virtual void K2_ActivateAbility();

	virtual void ActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) OVERRIDE;

	// -------------------------------------------------

	UFUNCTION(BlueprintImplementableEvent, Category = Ability)
	virtual void K2_PredictiveActivateAbility();

	virtual void PredictiveActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) OVERRIDE;

	// -------------------------------------------------

	UFUNCTION(BlueprintPure, Category = Ability)
	FGameplayAbilityActorInfo GetActorInfo();

	// -------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = Ability)
	virtual bool K2_CommitAbility();

	UFUNCTION(BlueprintCallable, Category = Ability)
	virtual void K2_EndAbility();
	

	// -------------------------------------------------


	virtual EGameplayAbilityInstancingPolicy::Type GetInstancingPolicy() const OVERRIDE
	{
		return InstancedPerExecution ? EGameplayAbilityInstancingPolicy::InstancedPerExecution : EGameplayAbilityInstancingPolicy::InstancedPerActor;
	}

	virtual EGameplayAbilityReplicationPolicy::Type GetReplicationPolicy() const OVERRIDE
	{
		return ReplicationPolicy;
	}

	UPROPERTY(EditDefaultsOnly, Category=Advanced)
	TEnumAsByte<EGameplayAbilityReplicationPolicy::Type> ReplicationPolicy;

	UPROPERTY(EditDefaultsOnly, Category=Advanced)
	bool	InstancedPerExecution;

	int32 GetFunctionCallspace(UFunction* Function, void* Parameters, FFrame* Stack);

	bool CallRemoteFunction(UFunction* Function, void* Parameters, FOutParmRec* OutParms, FFrame* Stack);


	// -------------------------------------------------


	/** This is shared, cached information about the thing using us
	 *		E.g, Actor*, MovementComponent*, AnimInstance, etc.
	 *		This is hopefully allocated once per actor and shared by many abilities.
	 *		The actual struct may be overridden per game to include game specific data.
	 *		(E.g, child classes may want to cast to FMyGameAbilityActorInfo)
	 */
	mutable const FGameplayAbilityActorInfo * CurrentActorInfo;

	/** This is information specific to this instance of the ability. E.g, whether it is predicting, authorting, confirmed, etc. */
	UPROPERTY(BlueprintReadOnly, Category=Ability)
	FGameplayAbilityActivationInfo	CurrentActivationInfo;

protected:

	virtual void EndAbility(const FGameplayAbilityActorInfo* ActorInfo) OVERRIDE;
};