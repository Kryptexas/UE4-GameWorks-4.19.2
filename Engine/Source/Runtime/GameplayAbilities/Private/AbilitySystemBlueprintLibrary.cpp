// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitMovementModeChange.h"
#include "Abilities/Tasks/AbilityTask_WaitOverlap.h"
#include "Abilities/Tasks/AbilityTask_WaitConfirmCancel.h"
#include "LatentActions.h"

UAbilitySystemBlueprintLibrary::UAbilitySystemBlueprintLibrary(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
}

UAbilitySystemComponent* UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AActor *Actor)
{
	/**
	 *	Fixme: This is supposed to be a small optimization but here we are going through module code, then a global uobject, then finally a virtual function - yuck!
	 *		-think of a better way to have a global, overridable per project function. Thats it!
	 */
	return UAbilitySystemGlobals::Get().GetAbilitySystemComponentFromActor(Actor);
}

void UAbilitySystemBlueprintLibrary::ApplyGameplayEffectToTargetData(FGameplayAbilityTargetDataHandle Target, UGameplayEffect *GameplayEffect, const FGameplayAbilityActorInfo InstigatorInfo)
{
	if (Target.Data.IsValid())
	{
		Target.Data->ApplyGameplayEffect(GameplayEffect, InstigatorInfo);
	}
}

FGameplayAbilityTargetDataHandle UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(FHitResult HitResult)
{
	// Construct TargetData
	FGameplayAbilityTargetData_SingleTargetHit* TargetData = new FGameplayAbilityTargetData_SingleTargetHit(HitResult);

	// Give it a handle and return
	FGameplayAbilityTargetDataHandle	Handle;
	Handle.Data = TSharedPtr<FGameplayAbilityTargetData>(TargetData);

	return Handle;
}

FHitResult UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(FGameplayAbilityTargetDataHandle TargetData)
{
	FGameplayAbilityTargetData* Data = TargetData.Data.Get();
	if (Data)
	{
		const FHitResult* HitResultPtr = Data->GetHitResult();
		if (HitResultPtr)
		{
			return *HitResultPtr;
		}
	}

	return FHitResult();
}

FVector UAbilitySystemBlueprintLibrary::GetTargetDataEndPoint(FGameplayAbilityTargetDataHandle TargetData)
{
	FGameplayAbilityTargetData * Data = TargetData.Data.Get();
	if (Data)
	{
		const FHitResult* HitResultPtr = Data->GetHitResult();
		if (HitResultPtr)
		{
			if (HitResultPtr->Component.IsValid())
			{
				return HitResultPtr->ImpactPoint;
			}
			else
			{
				return HitResultPtr->TraceEnd;
			}
		}
		else if(Data->HasEndPoint())
		{
			return Data->GetEndPoint();
		}
	}

	return FVector();
}

// -------------------------------------------------------------------------------------


bool UAbilitySystemBlueprintLibrary::IsInstigatorLocallyControlled(FGameplayCueParameters Parameters)
{
	return Parameters.InstigatorContext.IsLocallyControlled();
}


FHitResult UAbilitySystemBlueprintLibrary::GetHitResult(FGameplayCueParameters Parameters)
{
	if (Parameters.InstigatorContext.HitResult.IsValid())
	{
		return *Parameters.InstigatorContext.HitResult.Get();
	}
	
	return FHitResult();
}

bool UAbilitySystemBlueprintLibrary::HasHitResult(FGameplayCueParameters Parameters)
{
	return Parameters.InstigatorContext.HitResult.IsValid();
}