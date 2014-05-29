// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvironmentQuery/EnvQueryGenerator.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvQueryGenerator_ProjectedPoints.generated.h"

UCLASS(Abstract)
class UEnvQueryGenerator_ProjectedPoints : public UEnvQueryGenerator
{
	GENERATED_UCLASS_BODY()

	/** trace params */
	UPROPERTY(EditDefaultsOnly, Category=Generator)
	FEnvTraceData ProjectionData;

	struct FSortByHeight
	{
		float OriginalZ;

		FSortByHeight(const FVector& OriginalPt) : OriginalZ(OriginalPt.Z) {}

		FORCEINLINE bool operator()(const FNavLocation& A, const FNavLocation& B) const
		{
			return FMath::Abs(A.Location.Z - OriginalZ) < FMath::Abs(B.Location.Z - OriginalZ);
		}
	};

	FORCEINLINE_DEBUGGABLE bool ProjectNavPointSimple(FVector& Point, const FVector& Extent, const class ANavigationData* NavData, TSharedPtr<const FNavigationQueryFilter> NavFilter)
	{
		FNavLocation ProjectedPoint;
		if (NavData->ProjectPoint(Point, ProjectedPoint, Extent, NavFilter))
		{
			Point.Z = ProjectedPoint.Location.Z;
			return true;
		}

		return false;
	}

#if WITH_RECAST
	FORCEINLINE_DEBUGGABLE bool ProjectNavPointInRange(FVector& Point, const float Radius, const float MinZOffset, const float MaxZOffset, const class ARecastNavMesh* NavData, TSharedPtr<const FNavigationQueryFilter> NavFilter, TArray<FNavLocation>& TempPoints)
	{
		TempPoints.Reset();
		if (NavData->ProjectPointMulti(Point, TempPoints, FVector(Radius, Radius, 0), Point.Z - MinZOffset, Point.Z + MaxZOffset, NavFilter))
		{
			TempPoints.Sort(FSortByHeight(Point));
			Point.Z = TempPoints[0].Location.Z;
			return true;
		}

		return false;
	}
#endif

	/** project all points in array and remove those outside navmesh */
	void ProjectAndFilterNavPoints(TArray<FVector>& Points, const class ANavigationData* NavData);
};
