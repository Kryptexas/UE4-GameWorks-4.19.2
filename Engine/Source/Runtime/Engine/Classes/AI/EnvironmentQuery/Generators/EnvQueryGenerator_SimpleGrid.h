// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvQueryGenerator_SimpleGrid.generated.h"

/**
 *  Simple grid, generates points in 2D square around context
 *  Z coord depends on OnNavmesh flag:
 *  - false: all points share the same Z coord, taken from context's location
 *  - true:  points are projected on navmesh, the ones outside are discarded
 */

UCLASS()
class UEnvQueryGenerator_SimpleGrid : public UEnvQueryGenerator
{
	GENERATED_UCLASS_BODY()

	/** square's extent */
	UPROPERTY(EditDefaultsOnly, Category=Generator)
	FEnvFloatParam Radius;

	/** generation density */
	UPROPERTY(EditDefaultsOnly, Category=Generator)
	FEnvFloatParam Density;

	/** if set, generated points will be projected to navmesh */
	UPROPERTY(EditDefaultsOnly, Category=Generator)
	FEnvBoolParam OnNavmesh;

	/** context */
	UPROPERTY(EditAnywhere, Category=Generator)
	TSubclassOf<class UEnvQueryContext> GenerateAround;
	
	UPROPERTY(EditAnywhere, Category=Generator, meta=(EditCondition="bProjectToNavmesh"))
	float NavigationProjectionHeight;

	UPROPERTY(EditAnywhere, Category=Generator)
	bool bProjectToNavigation;

	void GenerateItems(struct FEnvQueryInstance& QueryInstance); 

	virtual FString GetDescriptionTitle() const OVERRIDE;
	virtual FString GetDescriptionDetails() const OVERRIDE;
};
