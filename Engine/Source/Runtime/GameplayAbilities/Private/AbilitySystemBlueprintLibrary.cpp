// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbility_Instanced.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitMovementModeChange.h"
#include "Abilities/Tasks/AbilityTask_WaitOverlap.h"
#include "LatentActions.h"

UAbilitySystemBlueprintLibrary::UAbilitySystemBlueprintLibrary(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
}

UAbilityTask_PlayMontageAndWait* UAbilitySystemBlueprintLibrary::CreatePlayMontageAndWaitProxy(class UObject* WorldContextObject, UAnimMontage *MontageToPlay)
{
	check(WorldContextObject);
	UGameplayAbility_Instanced * Ability = CastChecked<UGameplayAbility_Instanced>(WorldContextObject);
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
	UGameplayAbility_Instanced * Ability = CastChecked<UGameplayAbility_Instanced>(WorldContextObject);
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

/**
 *	Need:
 *	-Easy way to specify which primitive components should be used for this overlap test
 *	-Easy way to specify which types of actors/collision overlaps that we care about/want to block on
 */

UAbilityTask_WaitOverlap* UAbilitySystemBlueprintLibrary::CreateWaitOverlap(class UObject* WorldContextObject, EMovementMode NewMode)
{
	check(WorldContextObject);
	UGameplayAbility_Instanced * Ability = CastChecked<UGameplayAbility_Instanced>(WorldContextObject);
	if (Ability)
	{
		AActor * ActorOwner = Cast<AActor>(Ability->GetOuter());

		UAbilityTask_WaitOverlap * MyObj = NULL;
		MyObj = NewObject<UAbilityTask_WaitOverlap>();


		// TEMP
		UPrimitiveComponent * PrimComponent = Cast<UPrimitiveComponent>(ActorOwner->GetRootComponent());
		if (!PrimComponent)
		{			
			PrimComponent = ActorOwner->FindComponentByClass<UPrimitiveComponent>();
		}
		check(PrimComponent);

		PrimComponent->OnComponentBeginOverlap.AddDynamic(MyObj, &UAbilityTask_WaitOverlap::OnOverlapCallback);
		PrimComponent->OnComponentHit.AddDynamic(MyObj, &UAbilityTask_WaitOverlap::OnHitCallback);

		return MyObj;
	}

	return NULL;
}

/**
 *	Fixme: This is supposed to be a small optimization but here we are going through module code, then a global uobject, then finally a virtual function - yuck!
 *		-think of a better way to have a global, overridable per project function. Thats it!
 */

UAbilitySystemComponent* UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AActor *Actor)
{
	return UAbilitySystemGlobals::Get().GetAbilitySystemComponentFromActor(Actor);
}

void UAbilitySystemBlueprintLibrary::ApplyGameplayEffectToTargetData(FGameplayAbilityTargetDataHandle Target, UGameplayEffect *GameplayEffect, const FGameplayAbilityActorInfo InstigatorInfo)
{
	if (Target.Data.IsValid())
	{
		Target.Data->ApplyGameplayEffect(GameplayEffect, InstigatorInfo);
	}
}