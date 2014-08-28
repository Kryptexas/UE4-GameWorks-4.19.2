// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayAbilityTargetActor.h"
#include "GameplayAbilityWorldReticle.h"
#include "GameplayAbilityTargetActor_Radius.generated.h"

UCLASS(Blueprintable)
class GAMEPLAYABILITIES_API AGameplayAbilityTargetActor_Radius : public AGameplayAbilityTargetActor
{
	GENERATED_UCLASS_BODY()

public:

	virtual void StartTargeting(UGameplayAbility* Ability);
	
	virtual void ConfirmTargeting() override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Radius)
	float Radius;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = Targeting)
	AActor* SourceActor;

protected:

	TArray<AActor*>	PerformOverlap(const FVector& Origin);

	FGameplayAbilityTargetDataHandle MakeTargetData(const TArray<AActor*> Actors, const FVector& Origin) const;

};