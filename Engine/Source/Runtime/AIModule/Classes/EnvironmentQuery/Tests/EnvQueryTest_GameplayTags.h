// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "GameplayTagContainer.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvQueryTest_GameplayTags.generated.h"

class IGameplayTagAssetInterface;

UCLASS(MinimalAPI)
class UEnvQueryTest_GameplayTags : public UEnvQueryTest
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;

	virtual FText GetDescriptionDetails() const override;

	bool SatisfiesTest(IGameplayTagAssetInterface* ItemGameplayTagAssetInterface) const;

	UPROPERTY(EditAnywhere, Category=GameplayTagCheck)
	EGameplayContainerMatchType TagsToMatch;

	UPROPERTY(EditAnywhere, Category=GameplayTagCheck)
	FGameplayTagContainer GameplayTags;
};
