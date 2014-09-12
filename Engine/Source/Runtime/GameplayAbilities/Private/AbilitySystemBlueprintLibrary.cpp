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

TArray<FActiveGameplayEffectHandle> UAbilitySystemBlueprintLibrary::ApplyGameplayEffectToTargetData(FGameplayAbilityTargetDataHandle Target, UGameplayEffect* GameplayEffect, const FGameplayAbilityActorInfo InstigatorInfo, int32 Level)
{
	TArray<FActiveGameplayEffectHandle> Handles;

	if (GameplayEffect == nullptr)
	{
		return Handles;
	}

	for (auto Data : Target.Data)
	{
		if (Data.IsValid())
		{
			Handles.Append(Data->ApplyGameplayEffect(GameplayEffect, InstigatorInfo, (float)Level));
		}
	}

	return Handles;
}

FActiveGameplayEffectHandle UAbilitySystemBlueprintLibrary::ApplyGameplayEffectToTargetData_Single(FGameplayAbilityTargetDataHandle Target, UGameplayEffect* GameplayEffect, const FGameplayAbilityActorInfo InstigatorInfo, int32 Level)
{
	FActiveGameplayEffectHandle Handle;

	if (GameplayEffect == nullptr)
	{
		return Handle;
	}

	if (Target.Data.Num() != 1)
	{
		ABILITY_LOG(Warning, TEXT("ApplyGameplayEffectToTargetData_Single called on invalid TargetDataHandle."));
		return Handle;
	}

	
	if (Target.Data[0].IsValid())
	{
		TArray<FActiveGameplayEffectHandle> Handles = Target.Data[0]->ApplyGameplayEffect(GameplayEffect, InstigatorInfo, (float)Level);
		int32 Num = Handles.Num();
		if (Num > 0)
		{
			Handle = Handles[0];
			if (Num > 1)
			{
				ABILITY_LOG(Warning, TEXT("ApplyGameplayEffectToTargetData_Single called on TargetData with multiple actor targets"));
			}
		}
	}

	return Handle;
}

FGameplayAbilityTargetDataHandle UAbilitySystemBlueprintLibrary::AppendTargetDataHandle(FGameplayAbilityTargetDataHandle TargetHandle, FGameplayAbilityTargetDataHandle HandleToAdd)
{
	TargetHandle.Append(&HandleToAdd);
	return TargetHandle;
}

FGameplayAbilityTargetDataHandle UAbilitySystemBlueprintLibrary::AbilityTargetDataFromLocations(const FGameplayAbilityTargetingLocationInfo& SourceLocation, const FGameplayAbilityTargetingLocationInfo& TargetLocation)
{
	// Construct TargetData
	FGameplayAbilityTargetData_LocationInfo*	NewData = new FGameplayAbilityTargetData_LocationInfo();
	NewData->SourceLocation = SourceLocation;
	NewData->TargetLocation = TargetLocation;

	// Give it a handle and return
	FGameplayAbilityTargetDataHandle	Handle;
	Handle.Data.Add(TSharedPtr<FGameplayAbilityTargetData_LocationInfo>(NewData));
	return Handle;
}

FGameplayAbilityTargetDataHandle UAbilitySystemBlueprintLibrary::AbilityTargetDataHandleFromAbilityTargetDataMesh(FGameplayAbilityTargetData_Mesh Data)
{
	// Construct TargetData
	FGameplayAbilityTargetData_Mesh*	NewData = new FGameplayAbilityTargetData_Mesh(Data);
	FGameplayAbilityTargetDataHandle	Handle;

	// Give it a handle and return
	Handle.Data.Add(TSharedPtr<FGameplayAbilityTargetData_Mesh>(NewData));
	return Handle;
}


FGameplayAbilityTargetDataHandle UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(AActor* Actor)
{
	// Construct TargetData
	FGameplayAbilityTargetData_ActorArray*	NewData = new FGameplayAbilityTargetData_ActorArray();
	NewData->TargetActorArray.Add(Actor);

	FGameplayAbilityTargetDataHandle		Handle(NewData);
	return Handle;
}

FGameplayAbilityTargetDataHandle UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(FHitResult HitResult)
{
	// Construct TargetData
	FGameplayAbilityTargetData_SingleTargetHit* TargetData = new FGameplayAbilityTargetData_SingleTargetHit(HitResult);

	// Give it a handle and return
	FGameplayAbilityTargetDataHandle	Handle;
	Handle.Data.Add(TSharedPtr<FGameplayAbilityTargetData>(TargetData));

	return Handle;
}

int32 UAbilitySystemBlueprintLibrary::GetDataCountFromTargetData(FGameplayAbilityTargetDataHandle TargetData)
{
	return TargetData.Data.Num();
}

TArray<AActor*> UAbilitySystemBlueprintLibrary::GetActorsFromTargetData(FGameplayAbilityTargetDataHandle TargetData, int32 Index)
{
	FGameplayAbilityTargetData* Data = TargetData.Data[Index].Get();
	TArray<AActor*>	ResolvedArray;
	if (Data)
	{
		TArray<TWeakObjectPtr<AActor> > WeakArray = Data->GetActors();
		for (TWeakObjectPtr<AActor> WeakPtr : WeakArray)
		{
			ResolvedArray.Add(WeakPtr.Get());
		}
	}
	return ResolvedArray;
}

bool UAbilitySystemBlueprintLibrary::TargetDataHasHitResult(FGameplayAbilityTargetDataHandle TargetData, int32 Index)
{
	FGameplayAbilityTargetData* Data = TargetData.Data[Index].Get();
	if (Data)
	{
		return Data->HasHitResult();
	}
	return false;
}

FHitResult UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(FGameplayAbilityTargetDataHandle TargetData, int32 Index)
{
	FGameplayAbilityTargetData* Data = TargetData.Data[Index].Get();
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

bool UAbilitySystemBlueprintLibrary::TargetDataHasOrigin(FGameplayAbilityTargetDataHandle TargetData, int32 Index)
{
	FGameplayAbilityTargetData* Data = TargetData.Data[Index].Get();
	if (Data)
	{
		return (Data->HasHitResult() || Data->HasOrigin());
	}
	return false;
}

FTransform UAbilitySystemBlueprintLibrary::GetTargetDataOrigin(FGameplayAbilityTargetDataHandle TargetData, int32 Index)
{
	FGameplayAbilityTargetData* Data = TargetData.Data[Index].Get();
	if (Data)
	{
		if (Data->HasOrigin())
		{
			return Data->GetOrigin();
		}
		if (Data->HasHitResult())
		{
			const FHitResult* HitResultPtr = Data->GetHitResult();
			FTransform ReturnTransform;
			ReturnTransform.SetLocation(HitResultPtr->TraceStart);
			ReturnTransform.SetRotation((HitResultPtr->Location - HitResultPtr->TraceStart).SafeNormal().Rotation().Quaternion());
			return ReturnTransform;
		}
	}

	return FTransform::Identity;
}

bool UAbilitySystemBlueprintLibrary::TargetDataHasEndPoint(FGameplayAbilityTargetDataHandle TargetData, int32 Index)
{
	FGameplayAbilityTargetData* Data = TargetData.Data[Index].Get();
	if (Data)
	{
		return (Data->HasHitResult() || Data->HasEndPoint());
	}
	return false;
}

FVector UAbilitySystemBlueprintLibrary::GetTargetDataEndPoint(FGameplayAbilityTargetDataHandle TargetData, int32 Index)
{
	FGameplayAbilityTargetData* Data = TargetData.Data[Index].Get();
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

	return FVector::ZeroVector;
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
