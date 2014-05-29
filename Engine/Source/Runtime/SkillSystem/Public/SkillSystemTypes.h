// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SkillSystemTypes.generated.h"

class UAttributeComponent;

UENUM(BlueprintType)
namespace EGameplayAbilityActivationMode
{
	enum Type
	{
		Authority,			// We are the authority activating this ability
		NonAuthority,		// We are not the authority but aren't predicting yet. This is a mostly invalid state to be in.

		Predicting,			// We are predicting the activation of this ability
		Confirmed,			// We are not the authority, but the authority has confirmed this activation
	};
}

USTRUCT(BlueprintType)
struct SKILLSYSTEM_API FGameplayAbilityActorInfo
{
	GENERATED_USTRUCT_BODY()

	FGameplayAbilityActorInfo()
		: ActivationMode(EGameplayAbilityActivationMode::Authority)
		, PredictionKey(0)
	{

	}

	FGameplayAbilityActorInfo(EGameplayAbilityActivationMode::Type InType)
		: ActivationMode(InType)
		, PredictionKey(0)
	{
	}


	UPROPERTY(BlueprintReadOnly, Category = "ActorInfo")
	TWeakObjectPtr<AActor>	Actor;

	UPROPERTY(BlueprintReadOnly, Category = "ActorInfo")
	TWeakObjectPtr<UAnimInstance>	AnimInstance;

	UPROPERTY(BlueprintReadOnly, Category = "ActorInfo")
	TWeakObjectPtr<UAttributeComponent>	AttributeComponent;

	UPROPERTY(BlueprintReadOnly, Category = "ActorInfo")
	TWeakObjectPtr<UMovementComponent>	MovementComponent;

	UPROPERTY(BlueprintReadOnly, Category = "ActorInfo")
	mutable TEnumAsByte<EGameplayAbilityActivationMode::Type>	ActivationMode;

	UPROPERTY()
	mutable int32 PredictionKey;

	void InitFromActor(AActor *Actor);

	void GeneratePredictionKey() const;

	void SetPredictionKey(int32 InPredictionKey);

	void SetActivationConfirmed();
};
