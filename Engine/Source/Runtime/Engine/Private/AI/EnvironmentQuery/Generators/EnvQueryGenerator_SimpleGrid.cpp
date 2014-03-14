// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UEnvQueryGenerator_SimpleGrid::UEnvQueryGenerator_SimpleGrid(const class FPostConstructInitializeProperties& PCIP) 
	: Super(PCIP)
	, NavigationProjectionHeight(1024.f)
{
	GenerateDelegate.BindUObject(this, &UEnvQueryGenerator_SimpleGrid::GenerateItems);

	GenerateAround = UEnvQueryContext_Querier::StaticClass();
	ItemType = UEnvQueryItemType_Point::StaticClass();
	Radius.Value = 100.0f;
	Density.Value = 10.0f;
	OnNavmesh.Value = true;
}

void UEnvQueryGenerator_SimpleGrid::GenerateItems(struct FEnvQueryInstance& QueryInstance)
{
	float RadiusValue = 0.0f;
	float DensityValue = 0.0f;

	if (!QueryInstance.GetParamValue(Radius, RadiusValue, TEXT("Radius")) ||
		!QueryInstance.GetParamValue(Density, DensityValue, TEXT("Density")) ||
		!QueryInstance.GetParamValue(OnNavmesh, bProjectToNavigation, TEXT("OnNavmesh")))
	{
		return;
	}

#if WITH_RECAST
	const ARecastNavMesh* NavMesh = bProjectToNavigation ? FEQSHelpers::FindNavMeshForQuery(QueryInstance) : NULL;
	if (bProjectToNavigation && NavMesh == NULL) 
	{
		return;
	}
#endif // WITH_RECAST

	const int32 ItemCount = FPlatformMath::Trunc((RadiusValue * 2.0f / DensityValue) + 1);
	const int32 ItemCountHalf = ItemCount / 2;

	TArray<FVector> ContextLocations;
	QueryInstance.PrepareContext(GenerateAround, ContextLocations);
	QueryInstance.ReserveItemData(ItemCount * ItemCount * ContextLocations.Num());

#if WITH_RECAST
	const FVector NavMeshProjectExtent(0.0f, 0.0f, NavigationProjectionHeight);
	if (NavMesh)
	{
		NavMesh->BeginBatchQuery();
	}
#endif // WITH_RECAST

	// @todo these nested loops need to be made into a batch query to navmesh itself, best implemented on recast-level
	// maybe ARecastNavMesh::ProjectPointMulti will do the trick
	// remember to get rid of failed items.
	for (int32 ContextIndex = 0; ContextIndex < ContextLocations.Num(); ContextIndex++)
	{
		for (int32 IndexX = 0; IndexX <= ItemCount; ++IndexX)
		{
			for (int32 IndexY = 0; IndexY <= ItemCount; ++IndexY)
			{
				FVector TestPoint = ContextLocations[ContextIndex] - FVector(DensityValue * (IndexX - ItemCountHalf), DensityValue * (IndexY - ItemCountHalf), 0);
#if WITH_RECAST
				if (NavMesh)
				{
					FNavLocation ProjectedPoint;
					if (NavMesh->ProjectPoint(TestPoint, ProjectedPoint, NavMeshProjectExtent))
					{
						TestPoint.Z = ProjectedPoint.Location.Z;
					}
					else
					{
						// skip points not on navmesh
						continue;
					}
				}
#endif // WITH_RECAST

				QueryInstance.AddItemData<UEnvQueryItemType_Point>(TestPoint);
			}
		}
	}

#if WITH_RECAST
	if (NavMesh)
	{
		NavMesh->FinishBatchQuery();
	}
#endif // WITH_RECAST
}

FString UEnvQueryGenerator_SimpleGrid::GetDescriptionTitle() const
{
	return FString::Printf(TEXT("%s: generate around %s"),
		*Super::GetDescriptionTitle(), *UEnvQueryTypes::DescribeContext(GenerateAround));
};

FString UEnvQueryGenerator_SimpleGrid::GetDescriptionDetails() const
{
	return FString::Printf(TEXT("radius: %s, density: %s, on navmesh: %s"),
		*UEnvQueryTypes::DescribeFloatParam(Radius),
		*UEnvQueryTypes::DescribeFloatParam(Density),
		*UEnvQueryTypes::DescribeBoolParam(OnNavmesh));
}

