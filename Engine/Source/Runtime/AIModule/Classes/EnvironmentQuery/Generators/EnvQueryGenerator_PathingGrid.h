// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvironmentQuery/Generators/EnvQueryGenerator_SimpleGrid.h"
#include "EnvQueryGenerator_PathingGrid.generated.h"

class UNavigationQueryFilter;

/**
 *  Navigation grid, generates points on navmesh
 *  with paths to/from context no further than given limit
 */

UCLASS(meta = (DeprecatedNode, DeprecationMessage = "This class is now deprecated, please use SimpleGrid generator with Pathfinding Batch test"))
class UEnvQueryGenerator_PathingGrid : public UEnvQueryGenerator_SimpleGrid
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	FAIDataProviderFloatValue MaxDistance;

	/** DEPRECATED, please use pathfinding (batch) test instead! */
	UPROPERTY(VisibleDefaultsOnly, Category=Generator)
	FAIDataProviderBoolValue PathToItem;

	/** DEPRECATED, please use pathfinding (batch) test instead! */
	UPROPERTY(VisibleDefaultsOnly, Category = Generator)
	TSubclassOf<UNavigationQueryFilter> NavigationFilter;

	virtual void PostLoad() override;
};
