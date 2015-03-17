// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvQueryOption.generated.h"

class UEnvQueryGenerator;
class UEnvQueryTest;

UCLASS()
class AIMODULE_API UEnvQueryOption : public UObject
{
	GENERATED_BODY()
public:
	UEnvQueryOption(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY()
	UEnvQueryGenerator* Generator;

	UPROPERTY()
	TArray<UEnvQueryTest*> Tests;

	FText GetDescriptionTitle() const;
	FText GetDescriptionDetails() const;
};
