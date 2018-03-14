// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ProxyLODkDOPInterface.h"

#include "ProxyLODMeshTypes.h"
#include "ProxyLODThreadedWrappers.h"

// Utils for building a kdop tree from different mesh types.

void ProxyLOD::BuildkDOPTree(const FRawMeshArrayAdapter& SrcGeometry, ProxyLOD::FkDOPTree& kDOPTree)
{

	const auto NumSrcPoly = SrcGeometry.polygonCount();

	TArray<FkDOPBuildTriangle> BuildTriangleArray;

	// pre-allocated
	ResizeArray(BuildTriangleArray, NumSrcPoly);

	ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, NumSrcPoly),
		[&BuildTriangleArray, &SrcGeometry](const ProxyLOD::FUIntRange& Range)
	{
		FkDOPBuildTriangle* BuildTriangles = BuildTriangleArray.GetData();
		for (uint32 r = Range.begin(), R = Range.end(); r < R; ++r)
		{
			const auto& Poly = SrcGeometry.GetRawPoly(r);
			BuildTriangles[r] = FkDOPBuildTriangle(r, Poly.VertexPositions[0], Poly.VertexPositions[1], Poly.VertexPositions[2]);
		}

	});

	// Add everything to the tree.
	kDOPTree.Build(BuildTriangleArray);

}

void ProxyLOD::BuildkDOPTree(const FRawMesh& SrcRawMesh, ProxyLOD::FkDOPTree& kDOPTree)
{

	const auto NumSrcPoly = SrcRawMesh.WedgeIndices.Num() / 3;

	TArray<FkDOPBuildTriangle> BuildTriangleArray;

	// pre-allocated
	ResizeArray(BuildTriangleArray, NumSrcPoly);

	ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, NumSrcPoly),
		[&BuildTriangleArray, &SrcRawMesh](const ProxyLOD::FUIntRange& Range)
	{
		FkDOPBuildTriangle* BuildTriangles = BuildTriangleArray.GetData();
		const uint32* Idxs = SrcRawMesh.WedgeIndices.GetData();
		const FVector* Positions = SrcRawMesh.VertexPositions.GetData();

		for (uint32 r = Range.begin(), R = Range.end(); r < R; ++r)
		{
			FVector Pos[3] = { Positions[Idxs[3 * r]],  Positions[Idxs[3 * r + 1]],  Positions[Idxs[3 * r + 2]] };
			BuildTriangles[r] = FkDOPBuildTriangle(r, Pos[0], Pos[1], Pos[2]);
		}

	});

	// Add everything to the tree.
	kDOPTree.Build(BuildTriangleArray);

}

void ProxyLOD::BuildkDOPTree(const FVertexDataMesh& SrcVertexDataMesh, ProxyLOD::FkDOPTree& kDOPTree)
{

	const auto NumSrcPoly = SrcVertexDataMesh.Indices.Num() / 3;

	TArray<FkDOPBuildTriangle> BuildTriangleArray;

	// pre-allocated
	ResizeArray(BuildTriangleArray, NumSrcPoly);

	ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, NumSrcPoly),
		[&BuildTriangleArray, &SrcVertexDataMesh](const ProxyLOD::FUIntRange& Range)
	{
		FkDOPBuildTriangle* BuildTriangles = BuildTriangleArray.GetData();
		const uint32* Idxs = SrcVertexDataMesh.Indices.GetData();
		const FVector* Positions = SrcVertexDataMesh.Points.GetData();

		for (uint32 r = Range.begin(), R = Range.end(); r < R; ++r)
		{
			FVector Pos[3] = { Positions[Idxs[3 * r]],  Positions[Idxs[3 * r + 1]],  Positions[Idxs[3 * r + 2]] };
			BuildTriangles[r] = FkDOPBuildTriangle(r, Pos[0], Pos[1], Pos[2]);
		}

	});

	// Add everything to the tree.
	kDOPTree.Build(BuildTriangleArray);

}