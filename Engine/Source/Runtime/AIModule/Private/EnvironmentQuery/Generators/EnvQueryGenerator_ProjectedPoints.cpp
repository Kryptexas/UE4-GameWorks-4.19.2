// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "AI/Navigation/RecastNavMesh.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_ProjectedPoints.h"

UEnvQueryGenerator_ProjectedPoints::UEnvQueryGenerator_ProjectedPoints(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ProjectionData.SetNavmeshOnly();
	ProjectionData.bCanProjectDown = true;
	ProjectionData.bCanDisableTrace = true;
	ProjectionData.ExtentX = 0.0f;

	ItemType = UEnvQueryItemType_Point::StaticClass();
}

void UEnvQueryGenerator_ProjectedPoints::ProjectAndFilterNavPoints(TArray<FNavLocation>& Points, FEnvQueryInstance& QueryInstance) const
{
	const bool bProjectToNavigation = (ProjectionData.TraceMode == EEnvQueryTrace::Navigation);
	ANavigationData* NavData = nullptr;
	if (bProjectToNavigation)
	{
#if WITH_RECAST
		NavData = (ANavigationData*)FEQSHelpers::FindNavMeshForQuery(QueryInstance);
#endif // WITH_RECAST
	}

	if (NavData)
	{
		TSharedPtr<const FNavigationQueryFilter> NavigationFilter = UNavigationQueryFilter::GetQueryFilter(NavData, ProjectionData.NavigationFilter);	
		TArray<FNavigationProjectionWork> Workload;
		Workload.Reserve(Points.Num());

		if (ProjectionData.ProjectDown == ProjectionData.ProjectUp)
		{
			for (const auto& Point : Points)
			{
				Workload.Add(FNavigationProjectionWork(Point.Location));
			}
		}
		else
		{
			const FVector VerticalOffset = FVector(0, 0, (ProjectionData.ProjectUp - ProjectionData.ProjectDown) / 2);
			for (const auto& Point : Points)
			{
				Workload.Add(FNavigationProjectionWork(Point.Location + VerticalOffset));
			}
		}

		const FVector ProjectionExtent(ProjectionData.ExtentX, ProjectionData.ExtentX, (ProjectionData.ProjectDown + ProjectionData.ProjectUp) / 2);
		NavData->BatchProjectPoints(Workload, ProjectionExtent, NavigationFilter);

		for (int32 Idx = Workload.Num() - 1; Idx >= 0; Idx--)
		{
			if (Workload[Idx].bResult)
			{
				Points[Idx] = Workload[Idx].OutLocation;
				Points[Idx].Location.Z += ProjectionData.PostProjectionVerticalOffset;
			}
			else
			{
				Points.RemoveAt(Idx);
			}
		}
	}
}

void UEnvQueryGenerator_ProjectedPoints::StoreNavPoints(const TArray<FNavLocation>& Points, FEnvQueryInstance& QueryInstance) const
{
	QueryInstance.ReserveItemData(QueryInstance.Items.Num() + Points.Num());
	for (int32 Idx = 0; Idx < Points.Num(); Idx++)
	{
		// store using default function to handle creating new data entry 
		QueryInstance.AddItemData<UEnvQueryItemType_Point>(Points[Idx].Location);
	}

	uint8* DataPtr = QueryInstance.RawData.GetData();
	for (int32 Idx = 0; Idx < Points.Num(); Idx++)
	{
		// overwrite with more detailed info
		UEnvQueryItemType_Point::SetNavValue(DataPtr + QueryInstance.Items[Idx].DataOffset, Points[Idx]);
	}

	FEnvQueryOptionInstance& OptionInstance = QueryInstance.Options[QueryInstance.OptionIndex];
	OptionInstance.bHasNavLocations = (ProjectionData.TraceMode == EEnvQueryTrace::Navigation);
}
