// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"
#include "SkillSystemBlueprintLibrary.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbility_Instanced.h"
#include "Abilities/Tasks/BlueprintPlayMontageAndWaitTaskProxy.h"
#include "Abilities/Tasks/BlueprintWaitMovementModeChangeTaskProxy.h"
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

UBlueprintPlayMontageAndWaitTaskProxy* USkillSystemBlueprintLibrary::CreatePlayMontageAndWaitProxy(class UObject* WorldContextObject, UAnimMontage *MontageToPlay)
{
	check(WorldContextObject);

	UGameplayAbility_Instanced * Ability = Cast<UGameplayAbility_Instanced>(WorldContextObject);
	if (Ability)
	{
		AActor * ActorOwner = Cast<AActor>(Ability->GetOuter());

		UBlueprintPlayMontageAndWaitTaskProxy * MyObj = NULL;
		MyObj = NewObject<UBlueprintPlayMontageAndWaitTaskProxy>();

		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(MyObj, &UBlueprintPlayMontageAndWaitTaskProxy::OnMontageEnded);

		FGameplayAbilityActorInfo	ActorInfo;
		ActorInfo.InitFromActor(ActorOwner);

		ActorInfo.AnimInstance->Montage_Play(MontageToPlay, 1.f);
		ActorInfo.AnimInstance->Montage_SetEndDelegate(EndDelegate);

		return MyObj;
	}

	return NULL;	
}


UBlueprintWaitMovementModeChangeTaskProxy* USkillSystemBlueprintLibrary::CreateWaitMovementModeChange(class UObject* WorldContextObject, EMovementMode NewMode)
{
	check(WorldContextObject);

	UGameplayAbility_Instanced * Ability = Cast<UGameplayAbility_Instanced>(WorldContextObject);
	if (Ability)
	{
		AActor * ActorOwner = Cast<AActor>(Ability->GetOuter());

		UBlueprintWaitMovementModeChangeTaskProxy * MyObj = NULL;
		MyObj = NewObject<UBlueprintWaitMovementModeChangeTaskProxy>();
		MyObj->RequiredMode = NewMode;

		FGameplayAbilityActorInfo	ActorInfo;
		ActorInfo.InitFromActor(ActorOwner);

		ACharacter * Character = Cast<ACharacter>(ActorInfo.Actor.Get());
		if (Character)
		{
			/*
				FScriptDelegate Delegate;
				Delegate.BindUFunction(this, "OnActorBump");
				MyPawn->OnActorHit.AddUnique(Delegate);
			*/

			//FMovementModeChangedSignature::FDelegate::CreateUObject(MyObj, &UBlueprintWaitMovementModeChangeTaskProxy::OnMovementModeChange);
			//MyDel.BindUObject( 
			//Character->MovementModeChangedDelegate.Add(MyObj, &UBlueprintWaitMovementModeChangeTaskProxy::OnMovementModeChange); // //FMovementModeChangedSignature::FDelegate::CreateUObject

			Character->MovementModeChangedDelegate.AddDynamic(MyObj, &UBlueprintWaitMovementModeChangeTaskProxy::OnMovementModeChange);

			//Character->MovementModeChangedDelegate.AddUObject(MyObj, &UBlueprintWaitMovementModeChangeTaskProxy::OnMovementModeChange);
		}

		return MyObj;
	}

	return NULL;
}