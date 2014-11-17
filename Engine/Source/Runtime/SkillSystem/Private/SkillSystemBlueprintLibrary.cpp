// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"
#include "LatentActions.h"

USkillSystemBlueprintLibrary::USkillSystemBlueprintLibrary(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
}

void USkillSystemBlueprintLibrary::StaticFuncPlayMontageAndWait(AActor* WorldContextObject, UAnimMontage * Montage, FLatentActionInfo LatentInfo)
{
	check(WorldContextObject);

	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FPlayMontageAndWaitAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == NULL)
		{
			FGameplayAbilityActorInfo	ActorInfo;
			ActorInfo.InitFromActor(WorldContextObject);

			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FPlayMontageAndWaitAction(ActorInfo, Montage, LatentInfo));
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