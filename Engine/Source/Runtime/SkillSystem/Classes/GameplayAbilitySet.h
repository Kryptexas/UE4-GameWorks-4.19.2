// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayAbilitySet.generated.h"

USTRUCT()
struct FGameplayAbilityBindInfoCommandIDPair
{
	GENERATED_USTRUCT_BODY()

	// We want this to be a drop down of valid lists but needs to be defined per game
	UPROPERTY(EditAnywhere, Category = BindInfo)
	FString		CommandString;

	UPROPERTY(EditAnywhere, Category = BindInfo)
	int32		InputID;
};

USTRUCT()
struct FGameplayAbilityBindInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=BindInfo)
	TArray<FGameplayAbilityBindInfoCommandIDPair>	CommandBindings;

	UPROPERTY(EditAnywhere, Category=BindInfo)
	TSubclassOf<UGameplayAbility>	GameplayAbilityClass;
};

/**
* UGameplayAbilitySet
*	This contains a list of abilities along with key bindings. THis will be very game specific, so it is expected for games to override this clas.
*/
UCLASS(Blueprintable)
class SKILLSYSTEM_API UGameplayAbilitySet : public UDataAsset
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(EditAnywhere, Category=AbilitySet)
	TArray<FGameplayAbilityBindInfo>	Abilities;
};