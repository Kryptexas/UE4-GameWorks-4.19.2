// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "Abilities/GameplayAbility_Instanced.h"

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	UGameplayAbility_Instanced
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------


UGameplayAbility_Instanced::UGameplayAbility_Instanced(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	InstancedPerExecution = true;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateOwner;
}

bool UGameplayAbility_Instanced::CanActivateAbility(const FGameplayAbilityActorInfo* ActorInfo) const
{
	CurrentActorInfo = ActorInfo;

	if (!Super::CanActivateAbility(ActorInfo))
	{
		return false;
	}

	// Call into blueprint (fixme: cache this)
	static FName FuncName = FName(TEXT("K2_CanActivateAbility"));
	UFunction* CanActivateFunction = GetClass()->FindFunctionByName(FuncName);

	if (CanActivateFunction
		&& CanActivateFunction->GetOuter()->IsA(UBlueprintGeneratedClass::StaticClass())
		&& K2_CanActivateAbility() == false)
	{
		ABILITY_LOG(Log, TEXT("CanActivateAbility %s failed, blueprint refused"), *GetName());
		return false;
	}

	return true;
}

void UGameplayAbility_Instanced::CommitExecute(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	CurrentActorInfo = ActorInfo;
	CurrentActivationInfo = ActivationInfo;

	Super::CommitExecute(ActorInfo, ActivationInfo);

	ensure(!HasAnyFlags(RF_ClassDefaultObject));

	// Call into blueprint (fixme: cache this)
	static FName FuncName = FName(TEXT("K2_CommitAbility"));
	UFunction* CanActivateFunction = GetClass()->FindFunctionByName(FuncName);

	if (CanActivateFunction	&& CanActivateFunction->GetOuter()->IsA(UBlueprintGeneratedClass::StaticClass()))
	{
		K2_CommitExecute();
	}
}

void UGameplayAbility_Instanced::ActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	CurrentActorInfo = ActorInfo;
	CurrentActivationInfo = ActivationInfo;

	// Call into blueprint (fixme: cache this)
	static FName FuncName = FName(TEXT("K2_ActivateAbility"));
	UFunction* CanActivateFunction = GetClass()->FindFunctionByName(FuncName);

	if (CanActivateFunction	&& CanActivateFunction->GetOuter()->IsA(UBlueprintGeneratedClass::StaticClass()))
	{
		K2_ActivateAbility();
	}
}

void UGameplayAbility_Instanced::PredictiveActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	check(ActivationInfo.ActivationMode == EGameplayAbilityActivationMode::Predicting);

	CurrentActorInfo = ActorInfo;
	CurrentActivationInfo = ActivationInfo;

	Super::PredictiveActivateAbility(ActorInfo, ActivationInfo);

	// FIXME
	if(!HasAnyFlags(RF_ClassDefaultObject))
	{
		// Call into blueprint (fixme: cache this)
		static FName FuncName = FName(TEXT("K2_PredictiveActivateAbility"));
		UFunction* PredictiveActivateFunction = GetClass()->FindFunctionByName(FuncName);

		if (PredictiveActivateFunction&& PredictiveActivateFunction->GetOuter()->IsA(UBlueprintGeneratedClass::StaticClass()))
		{
			K2_PredictiveActivateAbility();
		}
	}
}

void UGameplayAbility_Instanced::EndAbility(const FGameplayAbilityActorInfo* ActorInfo)
{
	CurrentActorInfo = ActorInfo;

	Super::EndAbility(ActorInfo);

	if (InstancedPerExecution)
	{
		MarkPendingKill();
	}

	// Remove from owning AbilitySystemComponent?
	// generic way of releasing all callbacks!
}

FGameplayAbilityActorInfo UGameplayAbility_Instanced::GetActorInfo()
{
	check(CurrentActorInfo);
	return *CurrentActorInfo;
}

/**
* Attempts to commit the ability (spend resources, etc). This our last chance to fail.
*	-Child classes that override ActivateAbility must call this themselves!
*/


bool UGameplayAbility_Instanced::K2_CommitAbility()
{
	check(CurrentActorInfo);
	return CommitAbility(CurrentActorInfo, CurrentActivationInfo);
}

void UGameplayAbility_Instanced::K2_EndAbility()
{
	check(CurrentActorInfo);
	EndAbility(CurrentActorInfo);
}

void UGameplayAbility_Instanced::ClientActivateAbilitySucceed_Internal(int32 PredictionKey)
{
	check(CurrentActorInfo);
	CurrentActivationInfo.SetActivationConfirmed();

	CallActivateAbility(CurrentActorInfo, CurrentActivationInfo);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	Replication boilerplate
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------


int32 UGameplayAbility_Instanced::GetFunctionCallspace(UFunction* Function, void* Parameters, FFrame* Stack)
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return FunctionCallspace::Local;
	}
	check(GetOuter() != NULL);
	return GetOuter()->GetFunctionCallspace(Function, Parameters, Stack);
}

bool UGameplayAbility_Instanced::CallRemoteFunction(UFunction* Function, void* Parameters, FOutParmRec* OutParms, FFrame* Stack)
{
	check(!HasAnyFlags(RF_ClassDefaultObject));
	check(GetOuter() != NULL);

	AActor* Owner = CastChecked<AActor>(GetOuter());
	UNetDriver* NetDriver = Owner->GetNetDriver();
	if (NetDriver)
	{
		NetDriver->ProcessRemoteFunction(Owner, Function, Parameters, OutParms, Stack, this);
		return true;
	}

	return false;
}




