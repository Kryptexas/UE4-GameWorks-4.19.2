// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "RawMesh.h"
#include "PhysicsEngine/AggregateGeom.h"
#include "Engine/StaticMesh.h"

/** Structure holding intermediate mesh merging data that is used throughout the mesh merging and proxy creation processes */
struct FMeshMergeData
{
	FMeshMergeData() {}
	/** Raw mesh data from the source static mesh */
	FRawMesh RawMesh;	
	/** Contains the original texture bounds, if the material requires baking down per-vertex data */
	TArray<FBox2D> TexCoordBounds;
	/** Will contain non-overlapping texture coordinates, if the material requires baking down per-vertex data */
	TArray<FVector2D> NewUVs;	
};

/** Structure for encapsulating per LOD mesh merging data */
struct FRawMeshExt
{
	FRawMeshExt()
		: SourceStaticMesh(nullptr)
	{}

	FMeshMergeData		MeshLODData[MAX_STATIC_MESH_LODS];
	FKAggregateGeom		AggGeom;

	/** Pointer to the source static mesh instance */
	class UStaticMesh* SourceStaticMesh;

	/** Specific LOD index that is being exported */
	int32 ExportLODIndex;
};
