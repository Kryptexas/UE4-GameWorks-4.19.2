// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvironmentQuery/EnvQueryGenerator.h"
#include "EnvQueryGenerator_PathingGrid.generated.h"

/**
 *  Navigation grid, generates points on navmesh
 *  with paths to/from context no further than given limit
 */

UCLASS()
class UEnvQueryGenerator_PathingGrid : public UEnvQueryGenerator
{
	GENERATED_UCLASS_BODY()

	/** max distance of path between point and context */
	UPROPERTY(EditDefaultsOnly, Category=Generator)
	FEnvFloatParam MaxPathDistance;

	/** generation density */
	UPROPERTY(EditDefaultsOnly, Category=Generator)
	FEnvFloatParam Density;

	/** path direction switch: set = path from context to points, clear = path from points to context */
	UPROPERTY(EditDefaultsOnly, Category=Generator)
	FEnvBoolParam PathFromContext;

	/** context */
	UPROPERTY(EditAnywhere, Category=Generator)
	TSubclassOf<class UEnvQueryContext> GenerateAround;

	/** context */
	UPROPERTY(EditAnywhere, Category=Generator)
	TEnumAsByte<EEnvQueryTrace::Type> TraceType;

	/** navigation filter for tracing */
	UPROPERTY(EditDefaultsOnly, Category=Generator)
	TSubclassOf<class UNavigationQueryFilter> NavigationFilter;

	void GenerateItems(struct FEnvQueryInstance& QueryInstance); 

	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;

protected:
#if WITH_RECAST

	/** find all navmesh node refs in pathing distance */
	void FindNodeRefsInPathDistance(const class ARecastNavMesh* NavMesh, const FVector& ContextLocation, float MaxPathDistance, bool bPathFromContext, TArray<NavNodeRef>& NodeRefs, FBox& NodeRefsBounds) const;

	/** check if nav location is in allowed set */
	bool IsNavLocationInPathDistance(const class ARecastNavMesh* NavMesh, const struct FNavLocation& NavLocation, const TArray<NavNodeRef>& NodeRefs) const;
#endif
};
