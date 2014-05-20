// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "BlueprintGameplayTagLibrary.generated.h"

UCLASS(MinimalAPI)
class UBlueprintGameplayTagLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintPure, Category="GameplayTags")
	static bool DoGameplayTagsMatch(const struct FGameplayTag& TagOne, const struct FGameplayTag& TagTwo, TEnumAsByte<EGameplayTagMatchType::Type> TagOneMatchType, TEnumAsByte<EGameplayTagMatchType::Type> TagTwoMatchType);

	UFUNCTION(BlueprintPure, Category="GameplayTags")
	static int32 GetNumGameplayTagsInContainer(const struct FGameplayTagContainer& TagContainer);

};
