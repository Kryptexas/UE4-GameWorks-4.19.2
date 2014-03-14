// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvQueryOption.generated.h"

UCLASS()
class ENGINE_API UEnvQueryOption : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	class UEnvQueryGenerator* Generator;

	UPROPERTY()
	TArray<class UEnvQueryTest*> Tests;

	FString GetDescriptionTitle() const;
	FString GetDescriptionDetails() const;
};
