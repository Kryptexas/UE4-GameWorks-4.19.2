// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnvQueryGenerator_Composite.generated.h"

/**
 * Composite generator allows using multiple generators in single query option
 * Currently it's available only from code
 */

UCLASS(MinimalAPI)
class UEnvQueryGenerator_Composite : public UEnvQueryGenerator
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TArray<class UEnvQueryGenerator*> Generators;

	void GenerateItems(struct FEnvQueryInstance& QueryInstance);
	virtual FText GetDescriptionTitle() const OVERRIDE;
};
