// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SkillSystemModulePrivatePCH.h"

void FPlayMontageAndWaitAction::UpdateOperation(FLatentResponse& Response)
{
	//TimeRemaining -= Response.ElapsedTime();
	//Response.FinishAndTriggerIf(TimeRemaining <= 0.0f, ExecutionFunction, OutputLink, CallbackTarget);
}


FPlayMontageAndWaitAction::FPlayMontageAndWaitAction(const FGameplayAbilityActorInfo InActorInfo, UAnimMontage * InMontageToPlay, const FLatentActionInfo& LatentInfo)
	: ActorInfo(InActorInfo)
	, MontageToPlay(InMontageToPlay)
	, ExecutionFunction(LatentInfo.ExecutionFunction)
	, OutputLink(LatentInfo.Linkage)
	, CallbackTarget(LatentInfo.CallbackTarget)
{
	

	ActorInfo.AnimInstance->Montage_Play(MontageToPlay, 1.f);

	/*
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UGameplayAbility_Montage::OnMontageEnded, OwnerInfo, AppliedEffects);
	OwnerInfo.AnimInstance->Montage_SetEndDelegate(EndDelegate);
	*/
}