// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayAbilityTargetActor.h"
#include "GameplayAbilityWorldReticle.h"
#include "GameplayAbilityTargetActor_SingleLineTrace.generated.h"

UCLASS(Blueprintable)
class GAMEPLAYABILITIES_API AGameplayAbilityTargetActor_SingleLineTrace : public AGameplayAbilityTargetActor
{
	GENERATED_UCLASS_BODY()

public:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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

	UPROPERTY(BlueprintReadOnly, meta=(ExposeOnSpawn=true), Category=Projectile)
	bool bBindToConfirmCancelInputs;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Projectile)
	float MaxRange;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Projectile)
	bool bAutoFire;				//Automatically confirm after the timer runs out. Should also have support for an auto-cancel feature.

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Projectile)
	float TimeUntilAutoFire;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Projectile)
	TSubclassOf<AGameplayAbilityWorldReticle> ReticleClass;		//Using a special class for replication purposes

protected:
	virtual FHitResult PerformTrace(AActor *SourceActor) const;

	FGameplayAbilityTargetDataHandle MakeTargetData(FHitResult HitResult) const;

	TWeakObjectPtr<AGameplayAbilityWorldReticle> ReticleActor;
};