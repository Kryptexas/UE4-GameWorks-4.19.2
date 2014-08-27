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
	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
}

void UAbilitySystemBlueprintLibrary::ApplyGameplayEffectToTargetData(FGameplayAbilityTargetDataHandle Target, UGameplayEffect *GameplayEffect, const FGameplayAbilityActorInfo InstigatorInfo)
{
	if (Target.Data.IsValid())
	{
		Target.Data->ApplyGameplayEffect(GameplayEffect, InstigatorInfo);
	}
}

FGameplayAbilityTargetDataHandle UAbilitySystemBlueprintLibrary::AbilityTargetDataFromLocations(FGameplayAbilityTargetingLocationInfo SourceLocation, FGameplayAbilityTargetingLocationInfo TargetLocation)
{
	// Construct TargetData
	FGameplayAbilityTargetData_LocationInfo*	NewData = new FGameplayAbilityTargetData_LocationInfo();
	NewData->SourceLocation = SourceLocation;
	NewData->TargetLocation = TargetLocation;

	// Give it a handle and return
	FGameplayAbilityTargetDataHandle	Handle;
	Handle.Data = TSharedPtr<FGameplayAbilityTargetData_LocationInfo>(NewData);
	return Handle;
}

FGameplayAbilityTargetDataHandle UAbilitySystemBlueprintLibrary::AbilityTargetDataHandleFromAbilityTargetDataMesh(FGameplayAbilityTargetData_Mesh Data)
{
	// Construct TargetData
	FGameplayAbilityTargetData_Mesh*	NewData = new FGameplayAbilityTargetData_Mesh(Data);
	FGameplayAbilityTargetDataHandle	Handle;

	// Give it a handle and return
	Handle.Data = TSharedPtr<FGameplayAbilityTargetData_Mesh>(NewData);
	return Handle;
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

TArray<AActor*> UAbilitySystemBlueprintLibrary::GetActorsFromTargetData(FGameplayAbilityTargetDataHandle TargetData)
{
	FGameplayAbilityTargetData* Data = TargetData.Data.Get();
	if (Data)
	{
		return Data->GetActors();
	}
	return TArray<AActor*>();
}

bool UAbilitySystemBlueprintLibrary::TargetDataHasHitResult(FGameplayAbilityTargetDataHandle TargetData)
{
	FGameplayAbilityTargetData* Data = TargetData.Data.Get();
	if (Data)
	{
		return Data->HasHitResult();
	}
	return false;
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

bool UAbilitySystemBlueprintLibrary::TargetDataHasEndPoint(FGameplayAbilityTargetDataHandle TargetData)
{
	FGameplayAbilityTargetData* Data = TargetData.Data.Get();
	if (Data)
	{
		const FHitResult* HitResultPtr = Data->GetHitResult();
		return (Data->HasHitResult() || Data->HasEndPoint());
	}
	return false;
}

FVector UAbilitySystemBlueprintLibrary::GetTargetDataEndPoint(FGameplayAbilityTargetDataHandle TargetData)
{
	FGameplayAbilityTargetData* Data = TargetData.Data.Get();
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
		else if (Data->HasEndPoint())
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