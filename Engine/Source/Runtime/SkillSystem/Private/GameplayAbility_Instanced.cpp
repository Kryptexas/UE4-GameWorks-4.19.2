// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	UGameplayAbility_Instanced
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------


UGameplayAbility_Instanced::UGameplayAbility_Instanced(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
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
