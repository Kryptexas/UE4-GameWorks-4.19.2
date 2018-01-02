// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ProxyLODMeshTypes.h"

// --- FRawMeshAdapter ----

FRawMeshAdapter::FRawMeshAdapter(const FRawMesh& InRawMesh, const openvdb::math::Transform& InTransform) :
	RawMesh(&InRawMesh), Transform(&InTransform)
{}

FRawMeshAdapter::FRawMeshAdapter(const FRawMeshAdapter& other)
	: RawMesh(other.RawMesh), Transform(other.Transform)
{}

size_t FRawMeshAdapter::polygonCount() const
{
	// RawMesh hold triangles
	return size_t(RawMesh->WedgeIndices.Num() / 3);
}

size_t FRawMeshAdapter::pointCount() const
{
	return size_t(RawMesh->VertexPositions.Num());
}

void FRawMeshAdapter::getIndexSpacePoint(size_t FaceNumber, size_t CornerNumber, openvdb::Vec3d& pos) const
{
	// Get the vertex position in local space.
	const int32 WedgeIndex = int32(FaceNumber * 3 + CornerNumber);

	// float3 position 
	FVector Position = RawMesh->GetWedgePosition(WedgeIndex);
	pos = Transform->worldToIndex(openvdb::Vec3d(Position.X, Position.Y, Position.Z));

};


// --- FRawMeshArrayAdapter ----

FRawMeshArrayAdapter::FRawMeshArrayAdapter(const TArray<FMeshMergeData>& InMergeDataArray)
{
	// Make a default transform.
	Transform = openvdb::math::Transform::createLinearTransform(1.);
	
	PointCount = 0;
	PolyCount = 0;

	PolyOffsetArray.push_back(PolyCount);
	for (int32 MeshIdx = 0, MeshCount = InMergeDataArray.Num(); MeshIdx < MeshCount; ++MeshIdx)
	{
		const FMeshMergeData* MergeData = &InMergeDataArray[MeshIdx];
		const FRawMesh* RawMesh = MergeData->RawMesh;
		PointCount += size_t(RawMesh->VertexPositions.Num());
		PolyCount += size_t(RawMesh->WedgeIndices.Num() / 3);

		PolyOffsetArray.push_back(PolyCount);
		RawMeshArray.push_back(RawMesh);

		MergeDataArray.push_back(MergeData);
	}

	// Compute the bbox
	ComputeAABB(this->BBox);
}

FRawMeshArrayAdapter::FRawMeshArrayAdapter(const TArray<FMeshMergeData>& InMergeDataArray, const openvdb::math::Transform::Ptr InTransform)
	:Transform(InTransform)
{
	PointCount = 0;
	PolyCount = 0;

	PolyOffsetArray.push_back(PolyCount);
	for (int32 MeshIdx = 0, MeshCount = InMergeDataArray.Num(); MeshIdx < MeshCount; ++MeshIdx)
	{
		const FMeshMergeData* MergeData = &InMergeDataArray[MeshIdx];
		const FRawMesh* RawMesh = MergeData->RawMesh;
		PointCount += size_t(RawMesh->VertexPositions.Num());
		PolyCount += size_t(RawMesh->WedgeIndices.Num() / 3);

		PolyOffsetArray.push_back(PolyCount);
		RawMeshArray.push_back(RawMesh);

		MergeDataArray.push_back(MergeData);
	}

	// Compute the bbox
	ComputeAABB(this->BBox);
}

FRawMeshArrayAdapter::FRawMeshArrayAdapter(const FRawMeshArrayAdapter& other)
	:Transform(other.Transform), PointCount(other.PointCount), PolyCount(other.PolyCount), BBox(other.BBox)
{
	RawMeshArray = other.RawMeshArray;
	PolyOffsetArray = other.PolyOffsetArray;
	MergeDataArray = other.MergeDataArray;
}

void FRawMeshArrayAdapter::getWorldSpacePoint(size_t FaceNumber, size_t CornerNumber, openvdb::Vec3d& pos) const
{
	int32 MeshIdx, LocalFaceNumber;

	const FRawMesh& RawMesh = GetRawMesh(FaceNumber, MeshIdx, LocalFaceNumber);


	// Get the vertex position in local space.
	const int32 WedgeIndex = 3 * LocalFaceNumber + int32(CornerNumber);

	// float3 position 
	FVector Position = RawMesh.GetWedgePosition(WedgeIndex);
	pos = openvdb::Vec3d(Position.X, Position.Y, Position.Z);

};

void FRawMeshArrayAdapter::getIndexSpacePoint(size_t FaceNumber, size_t CornerNumber, openvdb::Vec3d& pos) const
{
	openvdb::Vec3d Position;
	getWorldSpacePoint(FaceNumber, CornerNumber, Position);
	pos = Transform->worldToIndex(Position);

};

const FMeshMergeData& FRawMeshArrayAdapter::GetMeshMergeData(uint32 Idx) const
{
	checkSlow(Idx < MergeDataArray.size());
	return *MergeDataArray[Idx];
}

FRawMeshArrayAdapter::FRawPoly FRawMeshArrayAdapter::GetRawPoly(const size_t FaceNumber, int32& OutMeshIdx, int32& OutLocalFaceNumber) const
{
	checkSlow(FaceNumber < PolyCount);

	int32 MeshIdx, LocalFaceNumber;

	const FRawMesh& RawMesh = GetRawMesh(FaceNumber, MeshIdx, LocalFaceNumber);
	OutMeshIdx = MeshIdx;
	OutLocalFaceNumber = LocalFaceNumber;
	checkSlow(LocalFaceNumber < RawMesh.WedgeIndices.Num() / 3)

		int32 WedgeIdx[3];
	WedgeIdx[0] = 3 * LocalFaceNumber;
	WedgeIdx[1] = WedgeIdx[0] + 1;
	WedgeIdx[2] = WedgeIdx[0] + 2;


	FRawPoly RawPoly;
	RawPoly.MeshIdx = MeshIdx;
	RawPoly.FaceMaterialIndex = RawMesh.FaceMaterialIndices[LocalFaceNumber];
	RawPoly.FaceSmoothingMask = RawMesh.FaceSmoothingMasks[LocalFaceNumber];

	for (int32 i = 0; i < 3; ++i)  RawPoly.VertexPositions[i] = RawMesh.GetWedgePosition(WedgeIdx[i]);


	if (RawMesh.WedgeTangentX.Num() > 0)
		for (int32 i = 0; i < 3; ++i)
			RawPoly.WedgeTangentX[i] = RawMesh.WedgeTangentX[WedgeIdx[i]];

	if (RawMesh.WedgeTangentY.Num() > 0)
		for (int32 i = 0; i < 3; ++i)
			RawPoly.WedgeTangentY[i] = RawMesh.WedgeTangentY[WedgeIdx[i]];

	if (RawMesh.WedgeTangentZ.Num() > 0)
		for (int32 i = 0; i < 3; ++i)
			RawPoly.WedgeTangentZ[i] = RawMesh.WedgeTangentZ[WedgeIdx[i]];

	if (RawMesh.WedgeColors.Num() > 0)
		for (int32 i = 0; i < 3; ++i)
			RawPoly.WedgeColors[i] = RawMesh.WedgeColors[WedgeIdx[i]];
	else
		for (int32 i = 0; i < 3; ++i)
			RawPoly.WedgeColors[i] = FColor::White;

	// Copy Texture coords
	for (int Idx = 0; Idx < MAX_MESH_TEXTURE_COORDS; ++Idx)
	{

		const TArray<FVector2D>& WedgeTexCoords = RawMesh.WedgeTexCoords[Idx];

		if (WedgeTexCoords.Num() > 0)
			for (int32 i = 0; i < 3; ++i)
			{
				RawPoly.WedgeTexCoords[Idx][i] = WedgeTexCoords[WedgeIdx[i]];
			}
		else
			for (int32 i = 0; i < 3; ++i)
				RawPoly.WedgeTexCoords[Idx][i] = FVector2D(0.f, 0.f);
	}
	return RawPoly;
}

FRawMeshArrayAdapter::FRawPoly FRawMeshArrayAdapter::GetRawPoly(const size_t FaceNumber) const
{
	int32 IgnoreMeshId, IgnoreLocalFaceNumber;
	return GetRawPoly(FaceNumber, IgnoreMeshId, IgnoreLocalFaceNumber);
}

// protected functions

const FRawMesh& FRawMeshArrayAdapter::GetRawMesh(const size_t FaceNumber, int32& MeshIdx, int32& LocalFaceNumber) const
{
	// Find the correct raw mesh
	MeshIdx = 0;
	while (FaceNumber >= PolyOffsetArray[MeshIdx + 1])
	{
		MeshIdx++;
	}

	// Offset the face number to get the correct index into this mesh.
	LocalFaceNumber = int32(FaceNumber) - PolyOffsetArray[MeshIdx];

	const FRawMesh* RawMesh = RawMeshArray[MeshIdx];

	return *RawMesh;
}

void FRawMeshArrayAdapter::ComputeAABB(ProxyLOD::FBBox& InOutBBox)
{
	uint32 NumTris = this->polygonCount();
	InOutBBox = ProxyLOD::Parallel_Reduce(ProxyLOD::FIntRange(0, NumTris), ProxyLOD::FBBox(),
		[this](const ProxyLOD::FIntRange& Range, ProxyLOD::FBBox BBox)->ProxyLOD::FBBox
	{
		// loop over faces
		for (int32 f = Range.begin(), F = Range.end(); f < F; ++f)
		{
			openvdb::Vec3d Pos;
			// loop over verts
			for (int32 v = 0; v < 3; ++v)
			{
				this->getWorldSpacePoint(f, v, Pos);

				BBox.expand(Pos);
			}

		}

		return BBox;

	}, [](const ProxyLOD::FBBox& BBoxA, const ProxyLOD::FBBox& BBoxB)->ProxyLOD::FBBox
	{
		ProxyLOD::FBBox Result(BBoxA);
		Result.expand(BBoxB);

		return Result;
	}

	);
}

// --- FClosestPolyField ----
FClosestPolyField::FClosestPolyField(const FRawMeshArrayAdapter& MeshArray, const openvdb::Int32Grid::Ptr& SrcPolyIndexGrid) :
	RawMeshArrayAdapter(&MeshArray),
	ClosestPolyGrid(SrcPolyIndexGrid) 
{}

FClosestPolyField::FClosestPolyField(const FClosestPolyField& other) :
	RawMeshArrayAdapter(other.RawMeshArrayAdapter),
	ClosestPolyGrid(other.ClosestPolyGrid) 
{}

FClosestPolyField::FPolyConstAccessor::FPolyConstAccessor(const openvdb::Int32Grid* PolyIndexGrid, const FRawMeshArrayAdapter* MeshArrayAdapter) :
	MeshArray(MeshArrayAdapter),
	CAccessor(PolyIndexGrid->getConstAccessor()),
	XForm(&(PolyIndexGrid->transform()))
{
}

FRawMeshArrayAdapter::FRawPoly FClosestPolyField::FPolyConstAccessor::Get(const openvdb::Vec3d& WorldPos, bool& bSuccess) const
{
	checkSlow(MeshArray != NULL);
	const openvdb::Coord ijk = XForm->worldToIndexCellCentered(WorldPos);
	openvdb::Int32 SrcPolyId;
	bSuccess = CAccessor.probeValue(ijk, SrcPolyId);
	// return the first poly if this failed..
	SrcPolyId = (bSuccess) ? SrcPolyId : 0;
	return MeshArray->GetRawPoly(SrcPolyId);
}


FClosestPolyField::FPolyConstAccessor FClosestPolyField::GetPolyConstAccessor() const
{
	checkSlow(RawMeshArrayAdapter != NULL);
	checkSlow(ClosestPolyGrid != NULL);

	return FPolyConstAccessor(ClosestPolyGrid.get(), RawMeshArrayAdapter);
}


