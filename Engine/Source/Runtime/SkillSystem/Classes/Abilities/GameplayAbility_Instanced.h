// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "GameplayAbility.h"
#include "GameplayAbility_Instanced.generated.h"

class UAnimInstance;
class UAttributeComponent;

/**
* UGameplayEffect
*	The GameplayEffect definition. This is the data asset defined in the editor that drives everything.
*/
UCLASS(Blueprintable)
class SKILLSYSTEM_API UGameplayAbility_Instanced : public UGameplayAbility
{
	GENERATED_UCLASS_BODY()

public:
	
	virtual void EndAbility(const FGameplayAbilityActorInfo ActorInfo) OVERRIDE;
	
	// -------------------------------------------------

	UFUNCTION(BlueprintImplementableEvent, Category = Ability)
	virtual bool K2_CanActivateAbility(const FGameplayAbilityActorInfo ActorInfo) const;

	virtual bool CanActivateAbility(const FGameplayAbilityActorInfo ActorInfo) const OVERRIDE;

	// -------------------------------------------------

	UFUNCTION(BlueprintImplementableEvent, Category = Ability)
	virtual void K2_CommitExecute(const FGameplayAbilityActorInfo ActorInfo);

	virtual void CommitExecute(const FGameplayAbilityActorInfo ActorInfo) OVERRIDE;

	// -------------------------------------------------

	UFUNCTION(BlueprintImplementableEvent, Category = Ability)
	virtual void K2_ActivateAbility(const FGameplayAbilityActorInfo ActorInfo);

	virtual void ActivateAbility(const FGameplayAbilityActorInfo ActorInfo) OVERRIDE;

	// -------------------------------------------------

	UFUNCTION(BlueprintImplementableEvent, Category = Ability)
	virtual void K2_PredictiveActivateAbility(const FGameplayAbilityActorInfo ActorInfo);

	virtual void PredictiveActivateAbility(const FGameplayAbilityActorInfo ActorInfo) OVERRIDE;

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

private:
	
};