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
	FGameplayAbilityTargetData_SingleTargetHit * TargetData = new FGameplayAbilityTargetData_SingleTargetHit(HitResult);

	// Give it a handle and return
	FGameplayAbilityTargetDataHandle	Handle;
	Handle.Data = TSharedPtr<FGameplayAbilityTargetData>(TargetData);

	return Handle;
}

FHitResult UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(FGameplayAbilityTargetDataHandle TargetData)
{
	FGameplayAbilityTargetData * Data = TargetData.Data.Get();
	if (Data)
	{
		const FHitResult * HitResultPtr = Data->GetHitResult();
		if (HitResultPtr)
		{
			return *HitResultPtr;
		}
	}

	return FHitResult();
}

// -------------------------------------------------------------------------------------

UAbilityTask_PlayMontageAndWait* UAbilitySystemBlueprintLibrary::CreatePlayMontageAndWaitProxy(class UObject* WorldContextObject, UAnimMontage *MontageToPlay)
{
	check(WorldContextObject);
	UGameplayAbility* Ability = CastChecked<UGameplayAbility>(WorldContextObject);
	if (Ability)
	{
		AActor * ActorOwner = Cast<AActor>(Ability->GetOuter());

		UAbilityTask_PlayMontageAndWait * MyObj = NULL;
		MyObj = NewObject<UAbilityTask_PlayMontageAndWait>();

		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(MyObj, &UAbilityTask_PlayMontageAndWait::OnMontageEnded);

		FGameplayAbilityActorInfo	ActorInfo;
		ActorInfo.InitFromActor(ActorOwner);

		ActorInfo.AnimInstance->Montage_Play(MontageToPlay, 1.f);
		ActorInfo.AnimInstance->Montage_SetEndDelegate(EndDelegate);

		return MyObj;
	}

	return NULL;	
}


UAbilityTask_WaitMovementModeChange* UAbilitySystemBlueprintLibrary::CreateWaitMovementModeChange(class UObject* WorldContextObject, EMovementMode NewMode)
{
	check(WorldContextObject);
	UGameplayAbility* Ability = CastChecked<UGameplayAbility>(WorldContextObject);
	if (Ability)
	{
		AActor * ActorOwner = Cast<AActor>(Ability->GetOuter());

		UAbilityTask_WaitMovementModeChange * MyObj = NULL;
		MyObj = NewObject<UAbilityTask_WaitMovementModeChange>();
		MyObj->RequiredMode = NewMode;

		FGameplayAbilityActorInfo	ActorInfo;
		ActorInfo.InitFromActor(ActorOwner);

		ACharacter * Character = Cast<ACharacter>(ActorInfo.Actor.Get());
		if (Character)
		{
			Character->MovementModeChangedDelegate.AddDynamic(MyObj, &UAbilityTask_WaitMovementModeChange::OnMovementModeChange);
		}
		return MyObj;
	}

	return NULL;
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