// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"
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

bool UGameplayAbility_Instanced::CanActivateAbility(const FGameplayAbilityActorInfo ActorInfo) const
{
	if (!Super::CanActivateAbility(ActorInfo))
	{
		return false;
	}

	// Call into blueprint (fixme: cache this)
	static FName FuncName = FName(TEXT("K2_CanActivateAbility"));
	UFunction* CanActivateFunction = GetClass()->FindFunctionByName(FuncName);

	if (CanActivateFunction
		&& CanActivateFunction->GetOuter()->IsA(UBlueprintGeneratedClass::StaticClass())
		&& K2_CanActivateAbility(ActorInfo) == false)
	{
		SKILL_LOG(Log, TEXT("CanActivateAbility %s failed, blueprint refused"), *GetName());
		return false;
	}

	return true;
}

void UGameplayAbility_Instanced::CommitExecute(const FGameplayAbilityActorInfo ActorInfo)
{
	Super::CommitExecute(ActorInfo);

	check(!HasAnyFlags(RF_ClassDefaultObject));

	// Call into blueprint (fixme: cache this)
	static FName FuncName = FName(TEXT("K2_CommitAbility"));
	UFunction* CanActivateFunction = GetClass()->FindFunctionByName(FuncName);

	if (CanActivateFunction	&& CanActivateFunction->GetOuter()->IsA(UBlueprintGeneratedClass::StaticClass()))
	{
		K2_CommitExecute(ActorInfo);
	}
}

void UGameplayAbility_Instanced::ActivateAbility(FGameplayAbilityActorInfo ActorInfo)
{
	// Call into blueprint (fixme: cache this)
	static FName FuncName = FName(TEXT("K2_ActivateAbility"));
	UFunction* CanActivateFunction = GetClass()->FindFunctionByName(FuncName);

	if (CanActivateFunction	&& CanActivateFunction->GetOuter()->IsA(UBlueprintGeneratedClass::StaticClass()))
	{
		K2_ActivateAbility(ActorInfo);
	}
}

void UGameplayAbility_Instanced::PredictiveActivateAbility(const FGameplayAbilityActorInfo ActorInfo)
{
	Super::PredictiveActivateAbility(ActorInfo);

	// FIXME
	if(!HasAnyFlags(RF_ClassDefaultObject))
	{
		// Call into blueprint (fixme: cache this)
		static FName FuncName = FName(TEXT("K2_PredictiveActivateAbility"));
		UFunction* PredictiveActivateFunction = GetClass()->FindFunctionByName(FuncName);

		if (PredictiveActivateFunction&& PredictiveActivateFunction->GetOuter()->IsA(UBlueprintGeneratedClass::StaticClass()))
		{
			K2_PredictiveActivateAbility(ActorInfo);
		}
	}
}

int32 UGameplayAbility_Instanced::GetFunctionCallspace(UFunction* Function, void* Parameters, FFrame* Stack)
{
	if(HasAnyFlags(RF_ClassDefaultObject))
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

void UGameplayAbility_Instanced::EndAbility(const FGameplayAbilityActorInfo ActorInfo)
{
	Super::EndAbility(ActorInfo);

	if (InstancedPerExecution)
	{
		MarkPendingKill();
	}

	// Remove from owning attributecomponent?
	// generic way of releasing all callbacks!
}