// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "NavmeshModulePrivatePCH.h"
#include "DetourSharedBoundary.h"

void dtSharedBoundary::Initialize()
{
	CurrentTime = 0.0f;
	NextClearTime = 0.0f;

	for (int32 Idx = 0; Idx < DT_MAX_AREAS; Idx++)
	{
		SingleAreaFilter.setAreaCost(Idx, DT_UNWALKABLE_POLY_COST);
	}
}

void dtSharedBoundary::Tick(float DeltaTime)
{
	CurrentTime += DeltaTime;
	
	// clear unused entries
	if (CurrentTime > NextClearTime)
	{
		const float MaxLifeTime = 3.0f;
		NextClearTime = CurrentTime + MaxLifeTime;

		for (int32 Idx = Data.Num() - 1; Idx >= 0; Idx--)
		{
			const float LastAccess = CurrentTime - Data[Idx].AccessTime;
			if (LastAccess >= MaxLifeTime)
			{
				Data.RemoveAt(Idx);
			}
		}
	}
}

int32 dtSharedBoundary::CacheData(float* Center, float Radius, dtPolyRef CenterPoly, dtNavMeshQuery* NavQuery, dtQueryFilter* NavFilter)
{
	Radius *= 1.5f;

	const int32 ExistingIdx = FindData(Center, Radius, NavFilter);
	if (ExistingIdx >= 0)
	{
		Data[ExistingIdx].AccessTime = CurrentTime;
		return ExistingIdx;
	}

	dtSharedBoundaryData NewData;
	dtVcopy(NewData.Center, Center);
	NewData.Radius = Radius;
	NewData.Filter = NavFilter;
	NewData.SingleAreaId = 0;
	NewData.AccessTime = CurrentTime;

	FindEdges(NewData, CenterPoly, NavQuery, NavFilter);
	const int32 NewIdx = Data.Add(NewData);
	return NewIdx;
}

int32 dtSharedBoundary::CacheData(float* Center, float Radius, dtPolyRef CenterPoly, dtNavMeshQuery* NavQuery, uint8 SingleAreaId)
{
	Radius *= 1.5f;

	const int32 ExistingIdx = FindData(Center, Radius, SingleAreaId);
	if (ExistingIdx >= 0)
	{
		Data[ExistingIdx].AccessTime = CurrentTime;
		return ExistingIdx;
	}

	dtSharedBoundaryData NewData;
	dtVcopy(NewData.Center, Center);
	NewData.Radius = Radius;
	NewData.SingleAreaId = SingleAreaId;
	NewData.AccessTime = CurrentTime;

	SingleAreaFilter.setAreaCost(SingleAreaId, 1.0f);

	FindEdges(NewData, CenterPoly, NavQuery, &SingleAreaFilter);
	const int32 NewIdx = Data.Add(NewData);

	SingleAreaFilter.setAreaCost(SingleAreaId, DT_UNWALKABLE_POLY_COST);
	return NewIdx;
}

void dtSharedBoundary::FindEdges(dtSharedBoundaryData& Data, dtPolyRef CenterPoly, dtNavMeshQuery* NavQuery, dtQueryFilter* NavFilter)
{
	const int32 MaxWalls = 64;
	int32 NumWalls = 0;
	float WallSegments[MaxWalls * 3 * 2] = { 0 };
	dtPolyRef WallPolys[MaxWalls * 2] = { 0 };

	const int32 MaxNeis = 64;
	int32 NumNeis = 0;
	dtPolyRef NeiPolys[MaxNeis] = { 0 };

	NavQuery->findWallsInNeighbourhood(CenterPoly, Data.Center, Data.Radius, NavFilter,
		NeiPolys, &NumNeis, MaxNeis, WallSegments, WallPolys, &NumWalls, MaxWalls);
	
	dtSharedBoundaryEdge NewEdge;
	for (int32 Idx = 0; Idx < NumWalls; Idx++)
	{
		dtVcopy(NewEdge.v0, &WallSegments[Idx * 6]);
		dtVcopy(NewEdge.v1, &WallSegments[Idx * 6 + 3]);
		NewEdge.p0 = WallPolys[Idx * 2];
		NewEdge.p1 = WallPolys[Idx * 2 + 1];

		Data.Edges.Add(NewEdge);
	}

	Data.Polys.Reserve(NumNeis);
	for (int32 Idx = 0; Idx < NumNeis; Idx++)
	{
		Data.Polys.Add(NeiPolys[Idx]);
	}
}

int32 dtSharedBoundary::FindData(float* Center, float Radius, dtQueryFilter* NavFilter) const
{
	const float RadiusThr = 50.0f;
	const float DistThrSq = FMath::Square(Radius * 0.4f);

	for (int32 Idx = 0; Idx < Data.Num(); Idx++)
	{
		if (Data[Idx].Filter == NavFilter)
		{
			const float DistSq = dtVdistSqr(Center, Data[Idx].Center);
			if (DistSq <= DistThrSq && dtAbs(Data[Idx].Radius - Radius) < RadiusThr)
			{
				return Idx;
			}
		}
	}

	return -1;
}

int32 dtSharedBoundary::FindData(float* Center, float Radius, uint8 SingleAreaId) const
{
	const float DistThrSq = FMath::Square(Radius * 0.4f);
	const float RadiusThr = 50.0f;

	for (int32 Idx = 0; Idx < Data.Num(); Idx++)
	{
		if (Data[Idx].SingleAreaId == SingleAreaId)
		{
			const float DistSq = dtVdistSqr(Center, Data[Idx].Center);
			if (DistSq <= DistThrSq && dtAbs(Data[Idx].Radius - Radius) < RadiusThr)
			{
				return Idx;
			}
		}
	}

	return -1;
}
