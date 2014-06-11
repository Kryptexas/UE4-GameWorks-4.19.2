// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayAbilityTargetActor.h"
#include "GameplayAbilityTargetActor_SingleLineTrace.generated.h"

UCLASS(Blueprintable)
class GAMEPLAYABILITIES_API AGameplayAbilityTargetActor_SingleLineTrace : public AGameplayAbilityTargetActor
{
	GENERATED_UCLASS_BODY()

public:
	
	virtual FGameplayAbilityTargetDataHandle StaticGetTargetData(UWorld * World, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void StartTargeting(UGameplayAbility* Ability);

	//temp

	void DoIt(TWeakObjectPtr<UGameplayAbility> Ability);

	UFUNCTION()
	void Confirm();

	UFUNCTION()
	void Cancel();

	virtual void Tick(float DeltaSeconds) override;
	
	TWeakObjectPtr<UGameplayAbility> Ability;

	bool bDebug;
};