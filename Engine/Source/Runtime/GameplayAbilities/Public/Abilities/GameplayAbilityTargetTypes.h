// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayAbilityTargetTypes.generated.h"

class UGameplayEffect;
class UAnimInstance;
class UAbilitySystemComponent;
class UGameplayAbility;
class AGameplayAbilityTargetActor;
class UAbilityTask;
class UAttributeSet;

UENUM(BlueprintType)
namespace EGameplayTargetingConfirmation
{
	enum Type
	{
		Instant,			// The targeting happens instantly without special logic or user input deciding when to 'fire'.
		UserConfirmed,		// The targeting happens when the user confirms the targeting.
		Custom,				// The GameplayTargeting Ability is responsible for deciding when the targeting data is ready. Not supported by all TargetingActors.
	};
}


USTRUCT(BlueprintType)
struct GAMEPLAYABILITIES_API FGameplayAbilityTargetData_Radius : public FGameplayAbilityTargetData
{
	GENERATED_USTRUCT_BODY()

	FGameplayAbilityTargetData_Radius()
	: Origin(0.f) { }

	FGameplayAbilityTargetData_Radius(const TArray<AActor*>& InActors, const FVector& InOrigin)
	: Actors(InActors), Origin(InOrigin) { }

	virtual TArray<AActor*>	GetActors() const { return Actors; }

	virtual bool HasOrigin() const { return true; }

	virtual FTransform GetOrigin() const { return FTransform(Origin); }

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	virtual UScriptStruct* GetScriptStruct()
	{
		return FGameplayAbilityTargetData_Radius::StaticStruct();
	}

private:

	TArray<AActor*> Actors;
	FVector Origin;
};

template<>
struct TStructOpsTypeTraits<FGameplayAbilityTargetData_Radius> : public TStructOpsTypeTraitsBase
{
	enum
	{
		WithNetSerializer = true	// For now this is REQUIRED for FGameplayAbilityTargetDataHandle net serialization to work
	};
};