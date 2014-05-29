// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"
#include "SkillSystemBlueprintLibrary.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbility_Instanced.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitMovementModeChange.h"
#include "LatentActions.h"

USkillSystemBlueprintLibrary::USkillSystemBlueprintLibrary(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
}

// Fixme: remove this function!!!
void USkillSystemBlueprintLibrary::StaticFuncPlayMontageAndWait(AActor* WorldContextObject, UAnimMontage * Montage, FLatentActionInfo LatentInfo)
{
	check(WorldContextObject);

	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FPlayMontageAndWaitAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == NULL)
		{
			FGameplayAbilityActorInfo	ActorInfo;

			AActor * ActorOwner = NULL;
			UObject * TestObj = WorldContextObject;
			while(ActorOwner == NULL && TestObj != NULL)
			{
				ActorOwner = Cast<AActor>(TestObj);
				TestObj = TestObj->GetOuter();
			}

			if (ActorOwner)
			{
				ActorInfo.InitFromActor(ActorOwner);  
				LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FPlayMontageAndWaitAction(ActorInfo, Montage, LatentInfo));
			}
		}
	}
}

UAbilityTask_PlayMontageAndWait* USkillSystemBlueprintLibrary::CreatePlayMontageAndWaitProxy(class UObject* WorldContextObject, UAnimMontage *MontageToPlay)
{
	check(WorldContextObject);

	UGameplayAbility_Instanced * Ability = Cast<UGameplayAbility_Instanced>(WorldContextObject);
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


UAbilityTask_WaitMovementModeChange* USkillSystemBlueprintLibrary::CreateWaitMovementModeChange(class UObject* WorldContextObject, EMovementMode NewMode)
{
	check(WorldContextObject);

	UGameplayAbility_Instanced * Ability = Cast<UGameplayAbility_Instanced>(WorldContextObject);
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