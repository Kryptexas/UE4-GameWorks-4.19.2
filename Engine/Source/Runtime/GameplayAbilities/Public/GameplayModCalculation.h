// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayModCalculation.generated.h"

USTRUCT()
struct FGameplayModCalculationSpecHandle
{
	GENERATED_USTRUCT_BODY()

	FGameplayModCalculationSpecHandle()
		: Handle(INDEX_NONE)
	{

	}

	FGameplayModCalculationSpecHandle(int32 InHandle)
		: Handle(InHandle)
	{

	}

	bool IsValid() const
	{
		return Handle != INDEX_NONE;
	}

	static FGameplayModCalculationSpecHandle GenerateNewHandle();

	bool operator==(const FGameplayModCalculationSpecHandle& Other) const
	{
		return Handle == Other.Handle;
	}

	bool operator!=(const FGameplayModCalculationSpecHandle& Other) const
	{
		return Handle != Other.Handle;
	}

	friend uint32 GetTypeHash(const FGameplayModCalculationSpecHandle& InHandle)
	{
		return InHandle.Handle;
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("%d"), Handle);
	}

private:

	UPROPERTY()
	int32 Handle;
};

/**
 * UGameplayModCalculation
 *	@todo document
 */
UCLASS(BlueprintType)
class GAMEPLAYABILITIES_API UGameplayModCalculation : public UDataAsset
{

public:
	GENERATED_UCLASS_BODY()

	/** */
	UPROPERTY(EditDefaultsOnly, Category=Calculation)
	TArray<FGameplayAttribute> RelevantSourceAttributes;

	/** */
	UPROPERTY(EditDefaultsOnly, Category=Calculation)
	TArray<FGameplayAttribute> RelevantTargetAttributes;

	FGameplayModCalculationSpecHandle GetContextForSource(const FGameplayEffectSpec& Spec);
	FGameplayModCalculationSpecHandle GetContextForTarget(const FGameplayEffectSpec& Spec);
};