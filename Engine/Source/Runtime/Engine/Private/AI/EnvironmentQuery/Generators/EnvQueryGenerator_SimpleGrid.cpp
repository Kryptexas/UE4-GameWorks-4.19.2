// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UEnvQueryGenerator_SimpleGrid::UEnvQueryGenerator_SimpleGrid(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	GenerateDelegate.BindUObject(this, &UEnvQueryGenerator_SimpleGrid::GenerateItems);

	GenerateAround = UEnvQueryContext_Querier::StaticClass();
	ItemType = UEnvQueryItemType_Point::StaticClass();
	Radius.Value = 500.0f;
	Density.Value = 100.0f;
}

void UEnvQueryGenerator_SimpleGrid::GenerateItems(struct FEnvQueryInstance& QueryInstance)
{
	float RadiusValue = 0.0f;
	float DensityValue = 0.0f;

	if (!QueryInstance.GetParamValue(Radius, RadiusValue, TEXT("Radius")) ||
		!QueryInstance.GetParamValue(Density, DensityValue, TEXT("Density")) )
	{
		return;
	}

	const bool bProjectToNavigation = (ProjectionData.TraceMode == EEnvQueryTrace::Navigation);
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

#if WITH_RECAST
	const FVector ProjectExtent(ProjectionData.ExtentX, ProjectionData.ExtentX, 0);
	if (NavMesh)
	{
		NavMesh->BeginBatchQuery();
	}
#endif // WITH_RECAST

	TArray<FVector> GridPoints;
	GridPoints.Reserve(ItemCount * ItemCount * ContextLocations.Num());

	for (int32 ContextIndex = 0; ContextIndex < ContextLocations.Num(); ContextIndex++)
	{
		for (int32 IndexX = 0; IndexX <= ItemCount; ++IndexX)
		{
			for (int32 IndexY = 0; IndexY <= ItemCount; ++IndexY)
			{
				const FVector TestPoint = ContextLocations[ContextIndex] - FVector(DensityValue * (IndexX - ItemCountHalf), DensityValue * (IndexY - ItemCountHalf), 0);
				GridPoints.Add(TestPoint);
			}
		}
	}

#if WITH_RECAST
	if (NavMesh)
	{
		ProjectAndFilterNavPoints(GridPoints, NavMesh);
		NavMesh->FinishBatchQuery();
	}
#endif // WITH_RECAST

	QueryInstance.ReserveItemData(GridPoints.Num());
	for (int32 i = 0; i < GridPoints.Num(); i++)
	{
		QueryInstance.AddItemData<UEnvQueryItemType_Point>(GridPoints[i]);
	}
}

FString UEnvQueryGenerator_SimpleGrid::GetDescriptionTitle() const
{
	return FString::Printf(TEXT("%s: generate around %s"),
		*Super::GetDescriptionTitle(), *UEnvQueryTypes::DescribeContext(GenerateAround));
};

FString UEnvQueryGenerator_SimpleGrid::GetDescriptionDetails() const
{
	FString Desc = FString::Printf(TEXT("radius: %s, density: %s"),
		*UEnvQueryTypes::DescribeFloatParam(Radius),
		*UEnvQueryTypes::DescribeFloatParam(Density));

	FString ProjDesc = ProjectionData.ToString(FEnvTraceData::Brief);
	if (ProjDesc.Len() > 0)
	{
		Desc += TEXT(", ");
		Desc += ProjDesc;
	}

	return Desc;
}
