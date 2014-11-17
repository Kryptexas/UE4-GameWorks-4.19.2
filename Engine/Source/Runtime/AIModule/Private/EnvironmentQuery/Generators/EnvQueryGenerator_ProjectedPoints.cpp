// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_ProjectedPoints.h"

UEnvQueryGenerator_ProjectedPoints::UEnvQueryGenerator_ProjectedPoints(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	ProjectionData.SetNavmeshOnly();
	ProjectionData.bCanProjectDown = true;
	ProjectionData.bCanDisableTrace = true;
	ProjectionData.ExtentX = 0.0f;
}

void UEnvQueryGenerator_ProjectedPoints::ProjectAndFilterNavPoints(TArray<FVector>& Points, const class ANavigationData* NavData)
{
	if (ProjectionData.TraceMode == EEnvQueryTrace::Navigation && NavData)
	{
		TSharedPtr<const FNavigationQueryFilter> NavigationFilter = UNavigationQueryFilter::GetQueryFilter(NavData, ProjectionData.NavigationFilter);	
		const ARecastNavMesh* NavMesh = Cast<const ARecastNavMesh>(NavData);

		const bool bProjectInRange = (ProjectionData.ProjectDown != ProjectionData.ProjectUp);
		if (bProjectInRange)
		{
			TArray<FNavLocation> TempPoints;

			if (NavMesh)
			{
				for (int32 PointIndex = Points.Num() -1; PointIndex >= 0; PointIndex--)
				{
#if WITH_RECAST
					const bool bProjected = ProjectNavPointInRange(Points[PointIndex], ProjectionData.ExtentX, ProjectionData.ProjectDown, ProjectionData.ProjectUp, NavMesh, NavigationFilter, TempPoints);
					if (!bProjected)
					{
						Points.RemoveAt(PointIndex);
					}
#endif
				}
			}
		}
		else
		{
			const FVector ProjectionExtent(ProjectionData.ExtentX, ProjectionData.ExtentX, ProjectionData.ProjectDown);

			for (int32 PointIndex = Points.Num() -1; PointIndex >= 0; PointIndex--)
			{
				const bool bProjected = ProjectNavPointSimple(Points[PointIndex], ProjectionExtent, NavData, NavigationFilter);
				if (!bProjected)
				{
					Points.RemoveAt(PointIndex);
				}
			}
		}
	}
}
