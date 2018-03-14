// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ProxyLODMeshAttrTransfer.h"
#include "ProxyLODBarycentricUtilities.h" // for some of the barycentric stuff



template<int Size>
FColor AverageColor(const FColor(&Colors)[Size])
{
	FColor Result;
	// Accumulate with floats because FColor is only 8-bit
	float Tmp[4] = { 0,0,0,0 };
	for (int i = 0; i < Size; ++i)
	{
		Tmp[0] += Colors[i].R;
		Tmp[1] += Colors[i].G;
		Tmp[2] += Colors[i].B;
		Tmp[3] += Colors[i].A;
	}
	for (int i = 0; i < Size; ++i) Tmp[i] *= 1.f / float(Size);

	Result.R = Tmp[0];
	Result.G = Tmp[1];
	Result.B = Tmp[2];
	Result.A = Tmp[3];

	return Result;
}

template <int Size>
FVector AverageUnitVector(const FVector(&Vectors)[Size])
{
	FVector Result(0.f, 0.f, 0.f);

	for (int i = 0; i < Size; ++i)
	{
		Result += Vectors[i];
	}

	Result.Normalize();

	return Result;
}

template <int Size>
FVector2D AverageTexCoord(const FVector2D(&TexCoords)[Size])
{
	FVector2D Result(0.f, 0.f);

	for (int i = 0; i < Size; ++i)
	{
		Result += TexCoords[i];
	}

	Result *= 1.f / float(Size);

	return Result;
}

void ProxyLOD::TransferMeshAttributes(const FClosestPolyField& SrcPolyField, FRawMesh& InOutMesh)
{
	const int32 NumFaces = InOutMesh.WedgeIndices.Num() / 3;
	//const FRawMeshArrayAdapter& RawMeshArrayAdapter = SrcPolyField.MeshAdapter();
	ProxyLOD::Parallel_For(ProxyLOD::FIntRange(0, NumFaces),
		[&InOutMesh, &SrcPolyField](const ProxyLOD::FIntRange& Range)
	{
		auto& Pos = InOutMesh.VertexPositions;
		auto& WedgeToPos = InOutMesh.WedgeIndices;
		FClosestPolyField::FPolyConstAccessor ConstPolyAccessor = SrcPolyField.GetPolyConstAccessor();

		uint32  WIdxs[3];
		// loop over the faces
		for (int32 f = Range.begin(), F = Range.end(); f < F; ++f)
		{
			// get the three corners for this face
			WIdxs[0] = f * 3;
			WIdxs[1] = WIdxs[0] + 1;
			WIdxs[2] = WIdxs[0] + 2;

			int32 LastMaterialIndex = -1;
			for (int32 i = 0; i < 3; ++i)
			{
				const uint32 Idx = WIdxs[i];
				// world space location 
				FVector WSPos = Pos[WedgeToPos[Idx]];

				bool bFoundPoly;
				// The closest poly to this point
				FRawMeshArrayAdapter::FRawPoly RawPoly = ConstPolyAccessor.Get(WSPos, bFoundPoly);

				LastMaterialIndex = RawPoly.FaceMaterialIndex;

				// ----  Transfer the face - average values to each wedge --- //
				// NB: might replace with something more sophisticated later.

				// Compute the average color

				InOutMesh.WedgeColors[Idx] = AverageColor(RawPoly.WedgeColors);

				// The average Tangent Vectors
				InOutMesh.WedgeTangentX[Idx] = AverageUnitVector(RawPoly.WedgeTangentX);
				InOutMesh.WedgeTangentY[Idx] = AverageUnitVector(RawPoly.WedgeTangentY);
				InOutMesh.WedgeTangentZ[Idx] = AverageUnitVector(RawPoly.WedgeTangentZ);

				// Average Texture Coords
				InOutMesh.WedgeTexCoords[0][Idx] = AverageTexCoord(RawPoly.WedgeTexCoords[0]);
			}
			// Assign the material index that the last vertex of this face sees.
			InOutMesh.FaceMaterialIndices[f] = LastMaterialIndex;
		}
	}
	);
}


// Transfers the normals from the source geometry to the ArrayOfStructs mesh.
void ProxyLOD::TransferSrcNormals(const FClosestPolyField& SrcPolyField, FAOSMesh& InOutMesh)
{

	const uint32 NumVertexes = InOutMesh.GetNumVertexes();
	FPositionNormalVertex* Vertexes = InOutMesh.Vertexes;

	ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, NumVertexes),
		[&Vertexes, &SrcPolyField](const ProxyLOD::FUIntRange& Range)
	{
		const auto PolyAccessor = SrcPolyField.GetPolyConstAccessor();
		for (uint32 n = Range.begin(), N = Range.end(); n < N; ++n)
		{
			// Get the closest poly to this vertex.
			FVector& Normal = Vertexes[n].Normal;
			const FVector& Pos = Vertexes[n].Position;

			bool bSuccess = false;
			const FRawMeshArrayAdapter::FRawPoly  RawPoly = PolyAccessor.Get(openvdb::Vec3d(Pos.X, Pos.Y, Pos.Z), bSuccess);
			if (bSuccess)
			{
				bool bValidSizedPoly = true;
#if 0
				// compute 4 * area * area  of raw poly
				const FVector AB = RawPoly.VertexPositions[1] - RawPoly.VertexPositions[0];
				const FVector AC = RawPoly.VertexPositions[2] - RawPoly.VertexPositions[0];
				const float FourAreaSqr = FVector::CrossProduct(AB, AC).SizeSquared();

				bValidSizedPoly = FourAreaSqr > 0.001;
#endif
				// Compute the barycentric weights of the vertex projected onto the nearest face.


				const auto Weights = ProxyLOD::ComputeBarycentricWeights(RawPoly.VertexPositions, Pos);

				bool MissedPoly = Weights[0] > 1 || Weights[0] < 0 || Weights[1] > 1 || Weights[1] < 0 || Weights[2] > 1 || Weights[2] < 0;

				if (!MissedPoly && bValidSizedPoly)
				{
					FVector TransferedNormal = ProxyLOD::InterpolateVertexData(Weights, RawPoly.WedgeTangentZ);
					//bool bNormal = !TransferedNormal.ContainsNaN();
					bool bNormal = TransferedNormal.Normalize(0.1f);
					// assume that the transfered normal is more accurate if it is somewhat aligned with the local geometric normal
					if (bNormal && FVector::DotProduct(TransferedNormal, Normal) > 0.2f)
					{
						Normal += 3.f * TransferedNormal;
						Normal.Normalize();
					}

				}
			}

		}
	});

};


void ProxyLOD::TransferVertexColors(const FClosestPolyField& SrcPolyField, FRawMesh& InOutMesh)
{
	const uint32 NumVertexes = InOutMesh.VertexPositions.Num();
	FVector* Vertexes = InOutMesh.VertexPositions.GetData();

	const uint32 NumWedges = InOutMesh.WedgeIndices.Num();
	uint32* Indices = InOutMesh.WedgeIndices.GetData();

	FColor* Colors = InOutMesh.WedgeColors.GetData();

	// Loop over the polys in the result mesh.

	ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, NumWedges),
		[Vertexes, Indices, Colors, &SrcPolyField](const ProxyLOD::FUIntRange& Range)
	{
		const auto PolyAccessor = SrcPolyField.GetPolyConstAccessor();

		// loop over wedges.
		for (uint32 w = Range.begin(), W = Range.end(); w < W; ++w)
		{
			// Get the closest poly to this vertex.
			
			uint32 Idx = Indices[w];
			const FVector& Pos = Vertexes[Idx];

			// Find the closest poly to this wedge.  
			// NB: all wedges that share a vert location will end up with the same color this way

			bool bSuccess = false;
			const FRawMeshArrayAdapter::FRawPoly  RawPoly = PolyAccessor.Get(openvdb::Vec3d(Pos.X, Pos.Y, Pos.Z), bSuccess);
			
			// default to white
			Colors[w] = FColor::White;
			if (bSuccess)
			{

				// Compute the barycentric weights of the vertex projected onto the nearest face.
				// We use these to determine the closest corner of the poly

				ProxyLOD::DArray3d Weights = ProxyLOD::ComputeBarycentricWeights(RawPoly.VertexPositions, Pos);

				bool MissedPoly = Weights[0] > 1 || Weights[0] < 0 || Weights[1] > 1 || Weights[1] < 0 || Weights[2] > 1 || Weights[2] < 0;

				if (!MissedPoly)
				{
					FLinearColor WedgeColors[3] = { FLinearColor(RawPoly.WedgeColors[0]), 
						                            FLinearColor(RawPoly.WedgeColors[1]), 
						                            FLinearColor(RawPoly.WedgeColors[2]) };

					FLinearColor InterpolatedColor = InterpolateVertexData(Weights, WedgeColors);

					float AveLum = WedgeColors[0].ComputeLuminance() + WedgeColors[1].ComputeLuminance() + WedgeColors[2].ComputeLuminance();
					AveLum /= 3.f;

					float LumTmp = InterpolatedColor.ComputeLuminance();
					if (LumTmp > 1.e-5 && AveLum > 1.e-5)
					{
						InterpolatedColor *= AveLum / LumTmp;
					}

					// fix up the intensity.

					Colors[w] = InterpolatedColor.ToFColor(true);
				}
			}
			
		}
	});
}


template <typename ProjectionOperatorType>
void ProjectVerticiesOntoSrc(const ProjectionOperatorType& ProjectionOperator, const FClosestPolyField& SrcPolyField, FRawMesh& InOutMesh)
{
	const uint32 NumVertexes = InOutMesh.VertexPositions.Num();
	FVector* Vertexes = InOutMesh.VertexPositions.GetData();

	ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, NumVertexes),
		[Vertexes, &SrcPolyField, ProjectionOperator](const ProxyLOD::FUIntRange& Range)
	{
		const auto PolyAccessor = SrcPolyField.GetPolyConstAccessor();

		for (uint32 n = Range.begin(), N = Range.end(); n < N; ++n)
		{
			// Get the closest poly to this vertex.
			FVector& Pos = Vertexes[n];

			bool bSuccess = false;
			const FRawMeshArrayAdapter::FRawPoly  RawPoly = PolyAccessor.Get(openvdb::Vec3d(Pos.X, Pos.Y, Pos.Z), bSuccess);
			if (bSuccess)
			{

				// Compute the barycentric weights of the vertex projected onto the nearest face.
				// We use these to determine the closest corner of the poly

				ProxyLOD::DArray3d Weights = ProxyLOD::ComputeBarycentricWeights(RawPoly.VertexPositions, Pos);

				bool MissedPoly = Weights[0] > 1 || Weights[0] < 0 || Weights[1] > 1 || Weights[1] < 0 || Weights[2] > 1 || Weights[2] < 0;

				if (!MissedPoly)

				{
					Pos = ProjectionOperator(Weights, RawPoly.VertexPositions, Pos);
				}
			}
		}
	});
}

template <typename ProjectionOperatorType>
void ProjectVerticiesOntoSrc(const ProjectionOperatorType& ProjectionOperator, const FClosestPolyField& SrcPolyField, FVertexDataMesh& InOutMesh)
{
	const uint32 NumVertexes = InOutMesh.Points.Num();
	FVector* Vertexes = InOutMesh.Points.GetData();

	ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, NumVertexes),
		[Vertexes, &SrcPolyField, ProjectionOperator](const ProxyLOD::FUIntRange& Range)
	{
		const auto PolyAccessor = SrcPolyField.GetPolyConstAccessor();

		for (uint32 n = Range.begin(), N = Range.end(); n < N; ++n)
		{
			// Get the closest poly to this vertex.
			FVector& Pos = Vertexes[n];

			bool bSuccess = false;
			const FRawMeshArrayAdapter::FRawPoly  RawPoly = PolyAccessor.Get(openvdb::Vec3d(Pos.X, Pos.Y, Pos.Z), bSuccess);
			if (bSuccess)
			{

				// Compute the barycentric weights of the vertex projected onto the nearest face.
				// We use these to determine the closest corner of the poly

				ProxyLOD::DArray3d Weights = ProxyLOD::ComputeBarycentricWeights(RawPoly.VertexPositions, Pos);

				bool MissedPoly = Weights[0] > 1 || Weights[0] < 0 || Weights[1] > 1 || Weights[1] < 0 || Weights[2] > 1 || Weights[2] < 0;

				if (!MissedPoly)

				{
					Pos = ProjectionOperator(Weights, RawPoly.VertexPositions, Pos);
				}
			}
		}
	});
}

template <typename ProjectionOperatorType, typename T>
void ProjectVerticiesOntoSrc(const ProjectionOperatorType& ProjectionOperator, const FClosestPolyField& SrcPolyField, TAOSMesh<T>& InOutMesh)
{
	uint32 NumVertexes = InOutMesh.GetNumVertexes();
	T* Vertexes = InOutMesh.Vertexes;

	ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, NumVertexes),
		[Vertexes, &SrcPolyField, ProjectionOperator](const ProxyLOD::FUIntRange& Range)
	{
		const auto PolyAccessor = SrcPolyField.GetPolyConstAccessor();

		for (uint32 n = Range.begin(), N = Range.end(); n < N; ++n)
		{
			// Get the closest poly to this vertex.
			FVector& Pos = Vertexes[n].Position;

			bool bSuccess = false;
			const FRawMeshArrayAdapter::FRawPoly  RawPoly = PolyAccessor.Get(openvdb::Vec3d(Pos.X, Pos.Y, Pos.Z), bSuccess);
			if (bSuccess)
			{

				// Compute the barycentric weights of the vertex projected onto the nearest face.
				// We use these to determine the closest corner of the poly

				ProxyLOD::DArray3d Weights = ProxyLOD::ComputeBarycentricWeights(RawPoly.VertexPositions, Pos);

				bool MissedPoly = Weights[0] > 1 || Weights[0] < 0 || Weights[1] > 1 || Weights[1] < 0 || Weights[2] > 1 || Weights[2] < 0;

				if (!MissedPoly)

				{
					Pos = ProjectionOperator(Weights, RawPoly.VertexPositions, Pos);
				}
			}
		}
	});
}

class FSnapProjectionOperator
{
public:
	FSnapProjectionOperator(float MaxDistSqr) 
		: MaxCloseDistSqr(MaxDistSqr) 
	{}

	FVector operator()(const ProxyLOD::DArray3d& Weights, const FVector(&VertexPos)[3], const FVector& CurrentPos) const 
	{
		// Identify the closest vertex.

		int MinIdx = 0;
		if (Weights[1] > Weights[MinIdx]) MinIdx = 1;
		if (Weights[2] > Weights[MinIdx]) MinIdx = 2;

		// Form a vector to the closest vertex.

		FVector ToClosestVertex = VertexPos[MinIdx] - CurrentPos;

		// Test distance to the closest vertex, if we are further than the cutoff, we
		// just project the vertex onto the surface.
		FVector ResultPos = 0.1 * CurrentPos;
		if (ToClosestVertex.SizeSquared() < MaxCloseDistSqr)
		{
			ResultPos += 0.9f * VertexPos[MinIdx];
		}
		else // just project
		{
			ResultPos += 0.9f * ProxyLOD::InterpolateVertexData(Weights, VertexPos);
		}

		return ResultPos;
	}
private:
	float MaxCloseDistSqr;

};

void ProxyLOD::ProjectVertexWithSnapToNearest(const FClosestPolyField& SrcPolyField, FRawMesh& InOutMesh)
{

	const double VoxelSize = SrcPolyField.GetVoxelSize();

	// 3 voxel distance.  When projecting to the nearest vert, use this as a distance cut off.
	float MaxCloseDistSqr = 9.f * VoxelSize * VoxelSize;

	// Do the work

	ProjectVerticiesOntoSrc(FSnapProjectionOperator(MaxCloseDistSqr), SrcPolyField, InOutMesh);

}


void ProxyLOD::ProjectVertexWithSnapToNearest(const FClosestPolyField& SrcPolyField, FAOSMesh& InOutMesh)
{

	const double VoxelSize = SrcPolyField.GetVoxelSize();

	// 3 voxel distance.  When projecting to the nearest vert, use this as a distance cut off.
	float MaxCloseDistSqr = 9.f * VoxelSize * VoxelSize;

	// Do the work

	ProjectVerticiesOntoSrc(FSnapProjectionOperator(MaxCloseDistSqr), SrcPolyField, InOutMesh);

}


void ProxyLOD::ProjectVertexWithSnapToNearest(const FClosestPolyField& SrcPolyField, FVertexDataMesh& InOutMesh)
{

	const double VoxelSize = SrcPolyField.GetVoxelSize();

	// 3 voxel distance.  When projecting to the nearest vert, use this as a distance cut off.
	float MaxCloseDistSqr = 9.f * VoxelSize * VoxelSize;

	// Do the work

	ProjectVerticiesOntoSrc(FSnapProjectionOperator(MaxCloseDistSqr), SrcPolyField, InOutMesh);

}

class FProjectionOperator
{
public:
	FProjectionOperator(float MaxDistSqr)
		: MaxCloseDistSqr(MaxDistSqr)
	{}

	FVector operator()(const ProxyLOD::DArray3d& Weights, const FVector(&VertexPos)[3], const FVector& CurrentPos) const
	{

		// Closest location on the surface

		const FVector ProjectedLocation = ProxyLOD::InterpolateVertexData(Weights, VertexPos);

		// Form a vector to the closest vertex.

		FVector ToClosestVertex = ProjectedLocation - CurrentPos;

		// Test distance to the closest vertex, if we are further than the cutoff, we
		// just project the vertex onto the surface.
		FVector ResultPos = CurrentPos;
		if (ToClosestVertex.SizeSquared() < MaxCloseDistSqr)
		{
			ResultPos = 0.25f * CurrentPos +  0.75f * ProjectedLocation;
		}

		return ResultPos;
	}
private:
	float MaxCloseDistSqr;

};

void ProxyLOD::ProjectVertexOntoSrcSurface(const FClosestPolyField& SrcPolyField, FRawMesh& InOutMesh)
{

	const double VoxelSize = SrcPolyField.GetVoxelSize();

	// 3 voxel distance.  When projecting to the nearest vert, use this as a distance cut off.
	float MaxCloseDistSqr = 9.f * VoxelSize * VoxelSize;

	// Do the work

	ProjectVerticiesOntoSrc(FProjectionOperator(MaxCloseDistSqr), SrcPolyField, InOutMesh);

}

void ProxyLOD::ProjectVertexOntoSrcSurface(const FClosestPolyField& SrcPolyField, FAOSMesh& InOutMesh)
{

	const double VoxelSize = SrcPolyField.GetVoxelSize();

	// 3 voxel distance.  When projecting to the nearest vert, use this as a distance cut off.
	float MaxCloseDistSqr = 9.f * VoxelSize * VoxelSize;

	// Do the work

	ProjectVerticiesOntoSrc(FProjectionOperator(MaxCloseDistSqr), SrcPolyField, InOutMesh);

}

void ProxyLOD::ProjectVertexOntoSrcSurface(const FClosestPolyField& SrcPolyField, FVertexDataMesh& InOutMesh)
{

	const double VoxelSize = SrcPolyField.GetVoxelSize();

	// 3 voxel distance.  When projecting to the nearest vert, use this as a distance cut off.
	float MaxCloseDistSqr = 9.f * VoxelSize * VoxelSize;

	// Do the work

	ProjectVerticiesOntoSrc(FProjectionOperator(MaxCloseDistSqr), SrcPolyField, InOutMesh);

}

