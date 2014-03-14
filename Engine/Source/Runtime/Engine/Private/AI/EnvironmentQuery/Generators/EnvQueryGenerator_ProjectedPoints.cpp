// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

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
				for (int32 i = Points.Num() -1; i >= 0; i--)
				{
#if WITH_RECAST
					const bool bProjected = ProjectNavPointInRange(Points[i], ProjectionData.ExtentX, ProjectionData.ProjectDown, ProjectionData.ProjectUp, NavMesh, NavigationFilter, TempPoints);
					if (!bProjected)
					{
						Points.RemoveAt(i);
					}
#endif
				}
			}
		}
		else
		{
			const FVector ProjectionExtent(ProjectionData.ExtentX, ProjectionData.ExtentX, ProjectionData.ProjectDown);

			for (int32 i = Points.Num() -1; i >= 0; i--)
			{
				const bool bProjected = ProjectNavPointSimple(Points[i], ProjectionExtent, NavData, NavigationFilter);
				if (!bProjected)
				{
					Points.RemoveAt(i);
				}
			}
		}
	}
}
