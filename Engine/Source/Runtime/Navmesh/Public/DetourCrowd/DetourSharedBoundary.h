// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DetourNavMeshQuery.h"

struct dtSharedBoundaryEdge
{
	float v0[3];
	float v1[3];
	dtPolyRef p0;
	dtPolyRef p1;
};

struct dtSharedBoundaryData
{
	float Center[3];
	float Radius;
	dtQueryFilter* Filter;
	int32 AccessTime;
	uint8 SingleAreaId;

	TArray<dtSharedBoundaryEdge> Edges;
	TArray<dtPolyRef> Polys;

	dtSharedBoundaryData() : Filter(nullptr) {}
};

class dtSharedBoundary
{
public:
	TArray<dtSharedBoundaryData> Data;
	dtQueryFilter SingleAreaFilter;
	float CurrentTime;
	float NextClearTime;

	void Initialize();
	void Tick(float DeltaTime);

	int32 FindData(float* Center, float Radius, dtQueryFilter* NavFilter) const;
	int32 FindData(float* Center, float Radius, uint8 SingleAreaId) const;

	int32 CacheData(float* Center, float Radius, dtPolyRef CenterPoly, dtNavMeshQuery* NavQuery, dtQueryFilter* NavFilter);
	int32 CacheData(float* Center, float Radius, dtPolyRef CenterPoly, dtNavMeshQuery* NavQuery, uint8 SingleAreaId);

	void FindEdges(dtSharedBoundaryData& Data, dtPolyRef CenterPoly, dtNavMeshQuery* NavQuery, dtQueryFilter* NavFilter);
};
