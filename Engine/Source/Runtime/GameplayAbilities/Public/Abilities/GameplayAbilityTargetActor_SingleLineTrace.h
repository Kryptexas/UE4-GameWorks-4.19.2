// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayAbilityTargetActor.h"
#include "GameplayAbilityTargetActor_SingleLineTrace.generated.h"

UCLASS(Blueprintable)
class GAMEPLAYABILITIES_API AGameplayAbilityTargetActor_SingleLineTrace : public AGameplayAbilityTargetActor
{
	GENERATED_UCLASS_BODY()

public:
	virtual FGameplayAbilityTargetDataHandle StaticGetTargetData(UWorld * World, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const override;

	virtual void StartTargeting(UGameplayAbility* Ability);
	
	virtual void ConfirmTargeting() override;	

	virtual void Tick(float DeltaSeconds) override;
	
	TWeakObjectPtr<UGameplayAbility> Ability;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Targeting)
	bool bDebug;

	//For remote clients, the actor who starts the trace (also gives us trace direction)
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Targeting)
	AActor* SourceActor;		//Maybe add a socket to this?

	UPROPERTY(BlueprintReadOnly, Category = Targeting)
	APlayerController* SourcePC;

	UPROPERTY(BlueprintReadOnly, meta=(ExposeOnSpawn=true), Category=Projectile)
	bool bBindToConfirmCancelInputs;

protected:
	virtual FHitResult PerformTrace(AActor *SourceActor) const;

	FGameplayAbilityTargetDataHandle MakeTargetData(FHitResult HitResult) const;
};