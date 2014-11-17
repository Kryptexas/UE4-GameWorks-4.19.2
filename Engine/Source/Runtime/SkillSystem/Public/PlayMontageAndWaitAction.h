// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "LatentActions.h"

// FDelayAction
// A simple delay action; counts down and triggers it's output link when the time remaining falls to zero
class FPlayMontageAndWaitAction : public FPendingLatentAction
{
public:
	const FGameplayAbilityActorInfo ActorInfo;
	class UAnimMontage * MontageToPlay;

	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;

	FPlayMontageAndWaitAction(const FGameplayAbilityActorInfo InActorInfo, UAnimMontage * InMontageToPlay, const FLatentActionInfo& LatentInfo);

	virtual void UpdateOperation(FLatentResponse& Response);

#if WITH_EDITOR
	// Returns a human readable description of the latent operation's current state
	virtual FString GetDescription() const OVERRIDE
	{
		//return FString::Printf(*NSLOCTEXT("DelayAction", "DelayActionTime", "Delay (%.3f seconds left)").ToString(), TimeRemaining);
		return FString(TEXT("PlayMontageAndWait"));
	}
#endif
};


