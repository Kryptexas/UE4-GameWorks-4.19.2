// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ProxyLODMeshUtilities.h"

#include "CoreMinimal.h"

#include "ProxyLODkDOPInterface.h"
#include "ProxyLODMeshConvertUtils.h" // ComputeNormal

#include "Modules/ModuleManager.h"
#include "MeshUtilities.h" // IMeshUtilities

#include <DirectXMeshCode/DirectXMesh/DirectXMesh.h>

#include <vector>
#include <unordered_map>

#define LOCTEXT_NAMESPACE "ProxyLODMeshUtilities"

#ifndef  PROXYLOD_CLOCKWISE_TRIANGLES
	#define PROXYLOD_CLOCKWISE_TRIANGLES  1
#endif
// Compute a tangent space for a FRawMesh 

void ProxyLOD::ComputeTangentSpace( FRawMesh& RawMesh, const bool bRecomputeNormals)
{

	// Used to compute the Mikk-Tangent space
	IMeshUtilities& Utilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");

	const bool bRecomputeTangents = true;

	FMeshBuildSettings BuildSettings;
	BuildSettings.bUseMikkTSpace = true;
	BuildSettings.bRemoveDegenerates = true; // this is really being used to help identify duplicate 'wedges' 

	Utilities.RecomputeTangentsAndNormalsForRawMesh(bRecomputeTangents, bRecomputeNormals, BuildSettings, RawMesh);

}

// Calls into the direxXMesh library to compute the per-vertex normal, by default this will weight by area.
// Note this is different than computing on the raw mesh, which can result in per-index tangent space.

void ProxyLOD::ComputeVertexNormals( FVertexDataMesh& InOutMesh, const bool bAngleWeighted)
{
	// Note:
	// This code relies on the fact that a FVector can be cast as a XMFLOAT3, and a FVector2D can be cast as a XMFLOAT2

	// Data from the existing mesh

	const DirectX::XMFLOAT3* Pos = (DirectX::XMFLOAT3*) (InOutMesh.Points.GetData());
	const size_t NumVerts = InOutMesh.Points.Num();
	const size_t NumFaces = InOutMesh.Indices.Num() / 3;
	uint32* indices = InOutMesh.Indices.GetData();

	auto& NormalArray = InOutMesh.Normal;
	ResizeArray(NormalArray, NumVerts);

	DirectX::XMFLOAT3* Normals = (DirectX::XMFLOAT3*)NormalArray.GetData();

	// Default is weight by angle.
	DWORD NormalFlags = 0;
	if (bAngleWeighted)
	{
		NormalFlags |= DirectX::CNORM_FLAGS::CNORM_DEFAULT;
	}
	else
	{
		NormalFlags |= DirectX::CNORM_FLAGS::CNORM_WEIGHT_BY_AREA;
	}

#if (PROXYLOD_CLOCKWISE_TRIANGLES == 1)
	NormalFlags |= DirectX::CNORM_FLAGS::CNORM_WIND_CW;
#endif
	DirectX::ComputeNormals(indices, NumFaces, Pos, NumVerts, NormalFlags, Normals);

}


// Calls into the direxXMesh library to compute the per-vertex tangent and bitangent, optionally recomputes the normal.
// Note this is different than computing on the raw mesh, which can result in per-index tangent space.

void ProxyLOD::ComputeTangentSpace( FVertexDataMesh& InOutMesh, const bool bRecomputeNormals)
{

	// Note:
	// This code relies on the fact that a FVector can be cast as a XMFLOAT3, and a FVector2D can be cast as a XMFLOAT2

	// Data from the existing mesh

	const DirectX::XMFLOAT3* Pos = (DirectX::XMFLOAT3*) (InOutMesh.Points.GetData());
	const size_t NumVerts = InOutMesh.Points.Num();
	const size_t NumFaces = InOutMesh.Indices.Num() / 3;
	uint32* indices = InOutMesh.Indices.GetData();

	// Optional computation of the normal
	if (bRecomputeNormals)
	{
		auto& NormalArray = InOutMesh.Normal;
		ResizeArray(NormalArray, NumVerts);

		DirectX::XMFLOAT3* Normals = (DirectX::XMFLOAT3*)NormalArray.GetData();

		// Default is weight by angle.
		DWORD NormalFlags = 0;
		NormalFlags |= DirectX::CNORM_FLAGS::CNORM_DEFAULT;
#if (PROXYLOD_CLOCKWISE_TRIANGLES == 1)
		NormalFlags |= DirectX::CNORM_FLAGS::CNORM_WIND_CW;
#endif
		DirectX::ComputeNormals(indices, NumFaces, Pos, NumVerts, NormalFlags, Normals);
	}

	// Compute the tangent and bitangent

	const auto& NormalArray = InOutMesh.Normal;
	const DirectX::XMFLOAT3* Normals = (const DirectX::XMFLOAT3*)NormalArray.GetData();

	const auto& UVArray = InOutMesh.UVs;
	const DirectX::XMFLOAT2* TexCoords = (const DirectX::XMFLOAT2*)UVArray.GetData();


	auto& TangentArray = InOutMesh.Tangent;
	ResizeArray(TangentArray, NumVerts);

	auto& BiTangentArray = InOutMesh.BiTangent;
	ResizeArray(BiTangentArray, NumVerts);

	//DirectX::ComputeTangentFrame(indices, NumFaces, Pos, Normals, TexCoords, NumVerts, (DirectX::XMFLOAT3*)TangentArray.GetData(), (DirectX::XMFLOAT3*)BiTangentArray.GetData());

	// Compute the tangent and bitangent frame and record the handedness.
	DirectX::XMFLOAT4* TangentX = new DirectX::XMFLOAT4[NumVerts];
	DirectX::ComputeTangentFrame(indices, NumFaces, Pos, Normals, TexCoords, NumVerts, TangentX, (DirectX::XMFLOAT3*)BiTangentArray.GetData());

	auto& TangentHanded = InOutMesh.TangentHanded;
	ResizeArray(TangentHanded, NumVerts);

	for (int32 v = 0; v < NumVerts; ++v)
	{
		// the handedness result was stored in TangentX.w by ::ComputeTangentFrame

		TangentHanded[v] = (TangentX[v].w > 0) ? 1 : -1;

		TangentArray[v] = FVector(TangentX[v].x, TangentX[v].y, TangentX[v].z);
	}

	if (TangentX) delete[] TangentX;
}

// The vertex and edge adjacency information for a watertight triangle mesh.
class FAdjacencyData
{

public:
	typedef std::vector<uint32> FaceList;

	FAdjacencyData(const uint32* Indices, const uint32 NumIndices, const uint32 NumVerts) :
		NumEdgeAjacentFaces(NumIndices)
	{

		// The number of triangles
		const uint32 NumTris = NumIndices / 3;
		check(NumIndices % 3 == 0);


		//Allocate
		VertexAdjacentFaceArray = new FaceList[NumVerts];
		EdgeAdjacentFaceArray = new uint32[NumTris * 3];

		for (uint32 f = 0; f < NumTris; ++f)
		{
			// Register this face with the correct verts
			const uint32 TriOffset = 3 * f;
			const uint32 VertId[3] = { Indices[TriOffset + 0], Indices[TriOffset + 1], Indices[TriOffset + 2] };

			for (int v = 0; v < 3; ++v) {
				check(VertId[v] < NumVerts);
				VertexAdjacentFaceArray[VertId[v]].push_back(f);
			}
		}



		// Find the other face adjacent to this edge of CurFace.
		auto FindAdjacentFace = [this](const uint32 Vert0, const uint32 Vert1, const uint32 CurFace)->int32
		{
			const FaceList& V0Adjacent = this->VertexAdjacentFaceArray[Vert0];
			const FaceList& V1Adjacent = this->VertexAdjacentFaceArray[Vert1];


			int32 FoundFace = -1;
			for (std::vector<uint32>::const_iterator V0Iter = V0Adjacent.cbegin(); V0Iter != V0Adjacent.cend(); ++V0Iter) {
				const uint32 FaceId = *V0Iter;
				if (FaceId == CurFace) {
					continue;
				}
				else
				{
					auto result = std::find(std::begin(V1Adjacent), std::end(V1Adjacent), FaceId);
					if (result != std::end(V1Adjacent))
					{
						FoundFace = FaceId;
						break;
					}
				}
			}

			//checkSlow(FoundFace != -1)

			return FoundFace;
		};



		{
			int32 AdjacentFace;
			// Loop over faces
			for (uint32 f = 0; f < NumTris; ++f)
			{
				// Get the vertexIds for this face

				// The verts for this face
				const uint32 FaceOffset = 3 * f;
				const uint32 VertId[3] = { Indices[FaceOffset + 0], Indices[FaceOffset + 1], Indices[FaceOffset + 2] };

				// The first edge: Tri[0] -> Tri[1]
				const uint32 EdgeId0 = f * 3;
				AdjacentFace = FindAdjacentFace(VertId[0], VertId[1], f);
				EdgeAdjacentFaceArray[EdgeId0] = (uint32)AdjacentFace;


				// The second edge: VertId1 -> VertId2 
				const uint32 EdgeId1 = EdgeId0 + 1;
				AdjacentFace = FindAdjacentFace(VertId[1], VertId[2], f);
				EdgeAdjacentFaceArray[EdgeId1] = (uint32)AdjacentFace;


				// The third edge: VertId2 -> VertId0
				const uint32 EdgeId2 = EdgeId0 + 2;
				AdjacentFace = FindAdjacentFace(VertId[2], VertId[0], f);
				EdgeAdjacentFaceArray[EdgeId2] = (uint32)AdjacentFace;

			}

		}
	}

	// Clean up
	~FAdjacencyData()
	{
		if (EdgeAdjacentFaceArray)   delete[] EdgeAdjacentFaceArray;
		if (VertexAdjacentFaceArray) delete[] VertexAdjacentFaceArray;
	}


	// Identify the adjacent triangles to a given edge.
	// FaceId = EdgeAdjacentFaceArray[EdgeId];
	// Where 
	//    EdgeId = TriId, TridId + 1, TriId + 2;

	uint32  NumEdgeAjacentFaces;
	uint32* EdgeAdjacentFaceArray;

	// Identify the adjacent triangles to a given vertex.
	// List of faces adjacent to each verted
	// ConcurrentFaceList& = VertexAdjacentFaceArray[VertexId] 

	FaceList*  VertexAdjacentFaceArray;

private:
	// don't copy
	FAdjacencyData(const FAdjacencyData& other) {};
};


// Testing function using for debugging.  Verifies that if A is adjacent to B, then B is adjacent to A.
// Returns the number of failure cases ( should be zero!)
int ProxyLOD::VerifyAdjacency(const uint32* EdgeAdjacentFaceArray, const uint32 NumEdgeAdjacentFaces)
{
	int FailureCount = 0;
	const uint32 NumFaces = NumEdgeAdjacentFaces / 3;
	check(NumEdgeAdjacentFaces % 3 == 0);

	// loop over faces.  Each three enteries hold the Ids of the adjacent faces.

	for (uint32 f = 0; f < NumFaces; ++f)
	{
		// offset to face 'f'
		const uint32 FaceIdx = f * 3;

		// The three faces adjacent to face 'f'
		const uint32 AdjFaces[3] = { EdgeAdjacentFaceArray[FaceIdx], EdgeAdjacentFaceArray[FaceIdx + 1], EdgeAdjacentFaceArray[FaceIdx + 2] };

		for (int i = 0; i < 3; ++i)
		{

			bool bValid = AdjFaces[i] < NumFaces;

			// offset to the 'i'-th face adjacent to face 'f' - call it fprime

			const uint32 AdjFaceIdx = 3 * AdjFaces[i];

			// faces adjacent to fprime.
			const uint32 AdjAdjFaces[3] = { EdgeAdjacentFaceArray[AdjFaceIdx], EdgeAdjacentFaceArray[AdjFaceIdx + 1], EdgeAdjacentFaceArray[AdjFaceIdx + 2] };

			// one of these should be 'f' itself.
			bValid = bValid && AdjAdjFaces[0] == f || AdjAdjFaces[1] == f || AdjAdjFaces[2] == f;

			if (!bValid)
			{
				FailureCount += 1;
			}
		}
	}

	return FailureCount;
}
// Face Averaged Normals. 
void ProxyLOD::ComputeFaceAveragedVertexNormals(FAOSMesh& InOutMesh)
{

	// Generate adjacency data
	FAdjacencyData  AdjacencyData(InOutMesh.Indexes, InOutMesh.GetNumIndexes(), InOutMesh.GetNumVertexes());

	// Test that if faceA is adjacent to faceB  then faceB is adjacent to faceA\
				// the desired result is zero
//int TempFailedAdjCount = VerifyAdjacency(TempAdjacencyData.EdgeAdjacentFaceArray, TempAdjacencyData.NumEdgeAjacentFaces);

	uint32 NumFaces = InOutMesh.GetNumIndexes() / 3;

	// Generate face normals.
	FVector* FaceNormals = new FVector[NumFaces];

	ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, NumFaces), [&InOutMesh, FaceNormals](const ProxyLOD::FUIntRange& Range)
	{
		FVector Pos[3];
		for (uint32 f = Range.begin(), F = Range.end(); f < F; ++f)
		{
			const openvdb::Vec3I Tri = InOutMesh.GetFace(f);
			Pos[0] = InOutMesh.Vertexes[Tri[0]].GetPos();
			Pos[1] = InOutMesh.Vertexes[Tri[1]].GetPos();
			Pos[2] = InOutMesh.Vertexes[Tri[2]].GetPos();

			FaceNormals[f] = ComputeNormal(Pos);

		}
	});


	ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, InOutMesh.GetNumVertexes()), [&AdjacencyData, &InOutMesh, FaceNormals](const ProxyLOD::FUIntRange&  Range)
	{
		// loop over vertexes in this range
		for (uint32 v = Range.begin(), V = Range.end(); v < V; ++v)
		{
			// This vertex
			auto& AOSVertex = InOutMesh.Vertexes[v];

			// zero the associated normal
			AOSVertex.Normal = FVector(0.f, 0.f, 0.f);

			// loop over all the faces that share this vertex, accumulating the normal
			const auto& AdjFaces = AdjacencyData.VertexAdjacentFaceArray[v];
			checkSlow(AdjFaces.size() != 0);

			if (AdjFaces.size() != 0)
			{
				FVector Pos[3];
				for (auto FaceCIter = AdjFaces.cbegin(); FaceCIter != AdjFaces.cend(); ++FaceCIter)
				{
					AOSVertex.Normal += FaceNormals[*FaceCIter];
				}
				AOSVertex.Normal.Normalize();
			}
		}
	});

	// clean up
	delete[] FaceNormals;
}


void ProxyLOD::AddDefaultTangentSpace(FVertexDataMesh& VertexDataMesh)
{

	// Copy the tangent space attributes.

	const uint32 DstNumPositions = VertexDataMesh.Points.Num();

	TArray<FVector>& NormalArray = VertexDataMesh.Normal;
	TArray<FVector>& TangetArray = VertexDataMesh.Tangent;
	TArray<FVector>& BiTangetArray = VertexDataMesh.BiTangent;

	// Allocate space
	ProxyLOD::FTaskGroup TaskGroup;


	TaskGroup.Run([&]
	{
		ResizeArray(NormalArray, DstNumPositions);
	});

	TaskGroup.Run([&]
	{
		ResizeArray(TangetArray, DstNumPositions);
	});

	TaskGroup.RunAndWait([&]
	{
		ResizeArray(BiTangetArray, DstNumPositions);
	});



	// Transfer the normal
	{
		ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumPositions),
			[&NormalArray](const ProxyLOD::FUIntRange& Range)
		{
			for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
			{
				NormalArray[i] = FVector(0, 0, 1);
			}
		}
		);
	}

	// Add default values for tangents and BiTangent for testing
	{
		ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumPositions),
			[&TangetArray](const ProxyLOD::FUIntRange& Range)
		{
			for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
			{
				TangetArray[i] = FVector(1, 0, 0);
			}
		}
		);
	}
	{
		ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumPositions),
			[&BiTangetArray](const ProxyLOD::FUIntRange& Range)
		{
			for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
			{
				BiTangetArray[i] = FVector(0, 1, 0);
			}
		}
		);
	}
}

void ProxyLOD::ComputeBogusTangentAndBiTangent(FVertexDataMesh& VertexDataMesh)
{
	const int32 NumVerts = VertexDataMesh.Points.Num();
	// Lets go ahead and add a BS tangent space.
	ProxyLOD::Parallel_For(ProxyLOD::FIntRange(0, NumVerts),
		[&VertexDataMesh](const ProxyLOD::FIntRange&  Range)
	{
		auto& TangentArray = VertexDataMesh.Tangent;
		auto& BiTangentArray = VertexDataMesh.BiTangent;
		auto& NormalArray = VertexDataMesh.Normal;

		for (int32 i = Range.begin(), I = Range.end(); i < I; ++i)
		{

			TangentArray[i] = FVector(1, 0, 0);
			BiTangentArray[i] = FVector::CrossProduct(NormalArray[i], TangentArray[i]);
			TangentArray[i] = FVector::CrossProduct(BiTangentArray[i], NormalArray[i]);

		}

	});

}

void ProxyLOD::ComputeBogusNormalTangentAndBiTangent(FVertexDataMesh& VertexDataMesh)
{
	const int32 NumVerts = VertexDataMesh.Points.Num();
	auto& TangentArray = VertexDataMesh.Tangent;
	auto& BiTangentArray = VertexDataMesh.BiTangent;
	auto& NormalArray = VertexDataMesh.Normal;
	auto& H = VertexDataMesh.TangentHanded;

	ResizeArray(TangentArray, NumVerts);
	ResizeArray(BiTangentArray, NumVerts);
	ResizeArray(NormalArray, NumVerts);
	ResizeArray(H, NumVerts);


	// Lets go ahead and add a BS tangent space.
	ProxyLOD::Parallel_For(ProxyLOD::FIntRange(0, NumVerts),
		[&](const ProxyLOD::FIntRange&  Range)
	{
		for (int32 i = Range.begin(), I = Range.end(); i < I; ++i)
		{

			TangentArray[i] = FVector(1, 0, 0);
			BiTangentArray[i] = FVector(0, 1, 0);
			NormalArray[i] = FVector(0, 0, 1);
			H[i] = 1;
		}

	});

}


template <typename T>
static void TAddNormals(TAOSMesh<T>& Mesh)
{
	ProxyLOD::ComputeFaceAveragedVertexNormals(Mesh);
}

template<>
void TAddNormals<FPositionOnlyVertex>(TAOSMesh<FPositionOnlyVertex>& Mesh)
{
	// Doesn't support normals.
}


void ProxyLOD::AddNormals(TAOSMesh<FPositionOnlyVertex>& InOutMesh)
{
	TAddNormals(InOutMesh);
}



	/**
	* Attempt to correct the collapsed walls.  
	* NB: The kDOP tree is already built, using the same mesh.
	*
	* @param Indices  - mesh conectivity
	* @param Positions - vertex locations: maybe changed by this function.
	* @param VoxelSize - length scale used in heuristic that determins how far to move vertices.
	*/
	int32 CorrectCollapsedWalls( const ProxyLOD::FkDOPTree& kDOPTree, 
		                         const TArray<uint32>& IndexArray, 
		                         TArray<FVector>& PositionArray,
		                         const float VoxelSize)
	{
		typedef uint32 EdgeIdType;
		typedef uint32 FaceIdType;
		

		ProxyLOD::FUnitTransformDataProvider kDOPDataProvider(kDOPTree);

		// Number of edges in our mesh

		int32 NumEdges = IndexArray.Num();

		// This will hold the intersecting faces for each edge.

		std::unordered_map<FaceIdType, std::vector<FaceIdType>> IntersectionListMap;

		const auto* Indices = IndexArray.GetData();
		auto* Pos           = PositionArray.GetData();

		// loop over the polys and collect the names of the faces that intersect.
		auto GetFace = [Indices, Pos](int32 FaceIdx, FVector(&Verts)[3])
		{
			const uint32 Idx[3] = { Indices[3 * FaceIdx], Indices[3 * FaceIdx + 1], Indices[3 * FaceIdx + 2] };

			Verts[0] = Pos[Idx[0]];
			Verts[1] = Pos[Idx[1]];
			Verts[2] = Pos[Idx[2]];
		};

		auto GetFaceNormal = [&GetFace](int32 FaceIdx)->FVector
		{
			FVector Verts[3];
			GetFace(FaceIdx, Verts);

			return ComputeNormal(Verts);
		};

		int32 TestCount = 0;
		for (int32 FaceIdx = 0; FaceIdx < NumEdges / 3; ++FaceIdx)
		{

			FVector Verts[3];
			GetFace(FaceIdx, Verts);

			const FVector FaceNormal = ComputeNormal(Verts);

			// loop over these three edges.
			for (int32 j = 0; j < 3; ++j)
			{
				int32 sV = j;
				int32 eV = (j + 1) % 3;

				FkHitResult kDOPResult;

				TkDOPLineCollisionCheck<const ProxyLOD::FUnitTransformDataProvider, uint32>  EdgeRay(Verts[sV], Verts[eV], true, kDOPDataProvider, &kDOPResult);

				bool bHit = kDOPTree.LineCheck(EdgeRay);

				if (bHit)
				{
					// Triangle we hit
					int32 HitTriId = kDOPResult.Item;

					// Don't count a hit against myself
					if (HitTriId == FaceIdx)
					{
						continue;
					}

					// Make sure the hit wasn't just one of the verts.
					if (kDOPResult.Time > 0.999 || kDOPResult.Time < 0.001)
					{
						continue;
					}

					// We only care about faces pointing in opposing directions
					const FVector HitFaceNormal = GetFaceNormal(HitTriId);
					if (FVector::DotProduct(FaceNormal, HitFaceNormal) > -0.94f) // not in 160 to 200 degrees
					{
						continue;
					}

					TestCount++;

					auto Search = IntersectionListMap.find(FaceIdx);
					if (Search != IntersectionListMap.end())
					{
						auto& FaceList = Search->second;
						if (std::find(FaceList.begin(), FaceList.end(), HitTriId) == FaceList.end())
						{
							FaceList.push_back(HitTriId);
						}
					}
					else
					{
						std::vector<FaceIdType> FaceList;
						FaceList.push_back(HitTriId);
						IntersectionListMap[FaceIdx] = FaceList;
					}
				}
			}
		}

		//For each triangle that collides, push it a small fixed distance in the normal direction.
		{
			for (auto ListMapIter = IntersectionListMap.begin(); ListMapIter != IntersectionListMap.end(); ++ListMapIter)
			{
				int32 FaceIdx = ListMapIter->first;
				const FVector TriNormal = GetFaceNormal(FaceIdx);

				// Scale by a small amount

				const FVector NormDisplacement = TriNormal * (VoxelSize / 7.f);
				const uint32 Idx[3] = { Indices[3 * FaceIdx], Indices[3 * FaceIdx + 1], Indices[3 * FaceIdx + 2] };

				Pos[Idx[0]] += NormDisplacement;
				Pos[Idx[1]] += NormDisplacement;
				Pos[Idx[2]] += NormDisplacement;

			}

		}
		return TestCount;
	}


int32 ProxyLOD::CorrectCollapsedWalls( FRawMesh& InOutMesh, 
	                                   const float VoxelSize )
{
	// Build an acceleration structure

	FkDOPTree kDOPTree;
	BuildkDOPTree(InOutMesh, kDOPTree);

	return CorrectCollapsedWalls(kDOPTree, InOutMesh.WedgeIndices, InOutMesh.VertexPositions, VoxelSize);
	
}


int32 ProxyLOD::CorrectCollapsedWalls(FVertexDataMesh& InOutMesh,
	const float VoxelSize)
{

	// Build an acceleration structure

	FkDOPTree kDOPTree;
	BuildkDOPTree(InOutMesh, kDOPTree);

	return CorrectCollapsedWalls(kDOPTree, InOutMesh.Indices, InOutMesh.Points, VoxelSize);

}

// Test to insure that no two vertexes share the same position.
void ProxyLOD::TestUniqueVertexes(const FMixedPolyMesh& InMesh)
{
	const uint32 NumVertexes = InMesh.Points.size();
	const openvdb::Vec3s* Vertexes = &InMesh.Points[0];

	ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, NumVertexes),
		[&NumVertexes, &Vertexes](const ProxyLOD::FUIntRange& Range)
	{
		for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
		{
			const auto& VertexI = Vertexes[i];
			for (uint32 j = i + 1; j < NumVertexes; ++j)
			{
				const auto& VertexJ = Vertexes[j];
				checkSlow(VertexI != VertexJ);
			}
		}
	});

}


// Test to insure that no two vertexes share the same position.
void ProxyLOD::TestUniqueVertexes(const FAOSMesh& InMesh)
{
	const uint32 NumVertexes = InMesh.GetNumVertexes();
	const FPositionNormalVertex* Vertexes = InMesh.Vertexes;

	ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, NumVertexes),
		[&NumVertexes, &Vertexes](const ProxyLOD::FUIntRange& Range)
	{
		for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
		{
			const FPositionNormalVertex& VertexI = Vertexes[i];
			for (uint32 j = i + 1; j < NumVertexes; ++j)
			{
				const FPositionNormalVertex& VertexJ = Vertexes[j];
				checkSlow(VertexI.GetPos() != VertexJ.GetPos());
				checkSlow(false == VertexI.GetPos().Equals(VertexJ.GetPos(), 1.e-6));
			}
		}
	});

}


void ProxyLOD::ColorPartitions(FRawMesh& InOutRawMesh, const std::vector<uint32>& partitionResults)
{

	// testing - coloring the simplified mesh by the partitions generated by uvatlas
	FColor Range[13] = { FColor(255, 0, 0), FColor(0, 255, 0), FColor(0, 0, 255), FColor(255, 255, 0), FColor(0, 255, 255),
		FColor(153, 102, 0), FColor(249, 129, 162), FColor(29, 143, 177), FColor(118, 42, 145),
		FColor(255, 121, 75), FColor(102, 204, 51), FColor(153, 153, 255), FColor(255, 255, 255) };

	for (int i = 0; i < partitionResults.size(); ++i)
	{
		uint32 PId = partitionResults[i];
		uint32 WedgeId = i * 3;
		InOutRawMesh.WedgeColors[WedgeId + 0] = Range[PId % 13];
		InOutRawMesh.WedgeColors[WedgeId + 1] = Range[PId % 13];
		InOutRawMesh.WedgeColors[WedgeId + 2] = Range[PId % 13];
	}
}


// Add Face colors to the a mesh according the the partitionResults array.

void ProxyLOD::ColorPartitions(FVertexDataMesh& InOutMesh, const std::vector<uint32>& PartitionResults)
{

	// testing - coloring the simplified mesh by the partitions generated by uvatlas
	FColor Range[13] = { FColor(255, 0, 0), FColor(0, 255, 0), FColor(0, 0, 255), FColor(255, 255, 0), FColor(0, 255, 255),
		FColor(153, 102, 0), FColor(249, 129, 162), FColor(29, 143, 177), FColor(118, 42, 145),
		FColor(255, 121, 75), FColor(102, 204, 51), FColor(153, 153, 255), FColor(255, 255, 255) };

	const uint32 NumFaces = InOutMesh.Indices.Num() / 3;
	ResizeArray(InOutMesh.FaceColors, NumFaces);

	for (int i = 0; i < PartitionResults.size(); ++i)
	{
		uint32 PId = PartitionResults[i];
		InOutMesh.FaceColors[i] = Range[PId % 13];
	}
}


void ProxyLOD::AddWedgeColors(FRawMesh& RawMesh)
{

	FColor ColorRange[13] = { FColor(255, 0, 0), FColor(0, 255, 0), FColor(0, 0, 255), FColor(255, 255, 0), FColor(0, 255, 255),
		FColor(153, 102, 0), FColor(249, 129, 162), FColor(29, 143, 177), FColor(118, 42, 145),
		FColor(255, 121, 75), FColor(102, 204, 51), FColor(153, 153, 255), FColor(255, 255, 255) };

	int32 NumIndices = RawMesh.WedgeIndices.Num();
	auto& WedgeColors = RawMesh.WedgeColors;

	ResizeArray(WedgeColors, NumIndices);

	ProxyLOD::Parallel_For(ProxyLOD::FIntRange(0, NumIndices),
		[&ColorRange, &WedgeColors](const ProxyLOD::FIntRange& Range)
	{
		for (int32 i = Range.begin(), I = Range.end(); i < I; ++i)
		{
			WedgeColors[i] = ColorRange[i % 13];
		}
	});

}




template <typename T>
static void TMakeCube(TAOSMesh<T>& Mesh, float Length)
{


	// The 8 corners of unit cube

	FVector Pos[8];

	Pos[0] = FVector(0, 0, 1);
	Pos[1] = FVector(1, 0, 1);
	Pos[2] = FVector(1, 0, 0);
	Pos[3] = FVector(0, 0, 0);

	Pos[4] = FVector(0, 1, 1);
	Pos[5] = FVector(1, 1, 1);
	Pos[6] = FVector(1, 1, 0);
	Pos[7] = FVector(0, 1, 0);


	const uint32 IndexList[36] = {
		/* front */
		0, 1, 2,
		2, 3, 0,
		/* right */
		2, 1, 5,
		5, 6, 2,
		/* back */
		5, 4, 7,
		7, 6, 5,
		/*left*/
		7, 4, 0,
		0, 3, 7,
		/*top*/
		0, 4, 5,
		5, 1, 0,
		/*bottom*/
		7, 3, 2,
		2, 6, 7
	};

	// Scale to Length size cube

	for (int32 i = 0; i < 8; ++i) Pos[i] *= Length;


	// Create the mesh

	const uint32 NumVerts = 8;
	const uint32 NumTris = 12; // two per cube face

	Mesh.Resize(NumVerts, NumTris);

	// copy the indices into the mesh

	for (int32 i = 0; i < NumTris * 3; ++i) Mesh.Indexes[i] = IndexList[i];

	// copy the locations into the mesh

	for (int32 i = 0; i < NumVerts; ++i) Mesh.Vertexes[i].GetPos() = Pos[i];

	TAddNormals(Mesh);

}

void ProxyLOD::MakeCube(FAOSMesh& InOutMesh, float Length)
{
	TMakeCube(InOutMesh, Length);
}

void ProxyLOD::MakeCube(TAOSMesh<FPositionOnlyVertex>& InOutMesh, float Length)
{
	TMakeCube(InOutMesh, Length);
}

void ProxyLOD::AddNormals(FAOSMesh& InOutMesh)
{
	TAddNormals(InOutMesh);
}

// Unused
#if 0
// Computes the face normals and assigns them to the wedges TangentZ
static void ComputeRawMeshNormals(FRawMesh& InOutMesh)
{
	const int32 NumFaces = InOutMesh.WedgeIndices.Num() / 3;

	ProxyLOD::Parallel_For(ProxyLOD::FIntRange(0, NumFaces),
		[&InOutMesh](const ProxyLOD::FIntRange& Range)
	{
		auto& Normal = InOutMesh.WedgeTangentZ;
		auto& Pos = InOutMesh.VertexPositions;
		auto& WedgeToPos = InOutMesh.WedgeIndices;
		// loop over these faces
		uint32  WIdxs[3];
		FVector Verts[3];
		for (uint32 f = Range.begin(), F = Range.end(); f < F; ++f)
		{
			// get the three corners for this face
			WIdxs[0] = f * 3;
			WIdxs[1] = WIdxs[0] + 1;
			WIdxs[2] = WIdxs[0] + 2;

			Verts[0] = Pos[WedgeToPos[WIdxs[0]]];
			Verts[1] = Pos[WedgeToPos[WIdxs[1]]];
			Verts[2] = Pos[WedgeToPos[WIdxs[2]]];

			// Compute the face normal
			// NB: this assumes a counter clockwise orientation.
			FVector FaceNormal = ComputeNormal(Verts);

			// Assign this to all the corners (wedges)
			Normal[WIdxs[0]] = FaceNormal;
			Normal[WIdxs[1]] = FaceNormal;
			Normal[WIdxs[2]] = FaceNormal;

		}
	}
	);
}



static void ComputeAngleAveragedNormal(FVertexDataMesh& VertexDataMesh)
{

	//djh testing
	const int32 NormalType = 1;
	if (NormalType == 0) return;

	// Generate adjacency data
	FAdjacencyData  AdjacencyData(VertexDataMesh.Indices.GetData(), VertexDataMesh.Indices.Num(), VertexDataMesh.Points.Num());


	const int32 NumFaces = VertexDataMesh.Indices.Num() / 3;

	// - Compute array of face normals.

	// FaceNormals 
	TArray<FVector> FaceNormalArray;
	FaceNormalArray.Empty(NumFaces);
	FaceNormalArray.AddUninitialized(NumFaces);

	ProxyLOD::Parallel_For(ProxyLOD::FIntRange(0, NumFaces),
		[&FaceNormalArray, &VertexDataMesh](const ProxyLOD::FIntRange& Range)
	{
		const auto& Indices = VertexDataMesh.Indices;
		const auto& Pos = VertexDataMesh.Points;

		FVector Verts[3];
		for (int32 f = Range.begin(), F = Range.end(); f < F; ++f)
		{
			int32 Idx0 = f * 3;
			Verts[0] = Pos[Indices[Idx0 + 0]];
			Verts[1] = Pos[Indices[Idx0 + 1]];
			Verts[2] = Pos[Indices[Idx0 + 2]];

			FaceNormalArray[f] = ComputeNormal(Verts);
		}

	});

	const int32 NumVerts = VertexDataMesh.Points.Num();
	auto& VertexNormalArray = VertexDataMesh.Normal;

	ResizeArray(VertexNormalArray, NumVerts);

	// for each vertex, get the associated faces.
	ProxyLOD::Parallel_For(ProxyLOD::FIntRange(0, NumVerts),
		[NormalType, &AdjacencyData, &VertexDataMesh, &FaceNormalArray, &VertexNormalArray](const ProxyLOD::FIntRange&  Range)
	{
		const auto& PositionArray = VertexDataMesh.Points;
		const auto& Indices = VertexDataMesh.Indices;
		// loop over vertexes in this range
		for (int32 v = Range.begin(), V = Range.end(); v < V; ++v)
		{
			// zero the associated normal
			VertexNormalArray[v] = FVector(0.f, 0.f, 0.f);

			// loop over all the faces that share this vertex, accumulating the normal
			const auto& AdjFaces = AdjacencyData.VertexAdjacentFaceArray[v];
			checkSlow(AdjFaces.size() != 0);

			if (AdjFaces.size() != 0)
			{
				for (auto FaceCIter = AdjFaces.cbegin(); FaceCIter != AdjFaces.cend(); ++FaceCIter)
				{
					int32 FaceIdx = *FaceCIter;

					if (NormalType != 1)
					{
						int32 Idx = FaceIdx * 3;

						FVector NextMinusCurrent;
						FVector PrevMinusCurrent;

						if (v == Indices[Idx + 0])
						{
							NextMinusCurrent = PositionArray[Indices[Idx + 1]] - PositionArray[v];
							PrevMinusCurrent = PositionArray[Indices[Idx + 2]] - PositionArray[v];

						}
						else if (v == Indices[Idx + 1])
						{
							NextMinusCurrent = PositionArray[Indices[Idx + 2]] - PositionArray[v];
							PrevMinusCurrent = PositionArray[Indices[Idx + 0]] - PositionArray[v];
						}
						else if (v == Indices[Idx + 2])
						{
							NextMinusCurrent = PositionArray[Indices[Idx + 0]] - PositionArray[v];
							PrevMinusCurrent = PositionArray[Indices[Idx + 1]] - PositionArray[v];
						}
						else
						{
							// Should never happen
							check(0);
						}

						NextMinusCurrent.Normalize();
						PrevMinusCurrent.Normalize();

						// compute the angle
						float CosAngle = FVector::DotProduct(NextMinusCurrent, PrevMinusCurrent);
						CosAngle = FMath::Clamp(CosAngle, -1.f, 1.f);
						const float Angle = FMath::Acos(CosAngle);

						VertexNormalArray[v] += FaceNormalArray[FaceIdx] * Angle;
					}
					else
					{
						VertexNormalArray[v] += FaceNormalArray[FaceIdx];
					}
				}

				VertexNormalArray[v].Normalize();
			}
		}
	});

	TArray<FVector> TestVertexNormalArray;
	ResizeArray(TestVertexNormalArray, NumVerts);

	for (int32 v = 0; v < NumVerts; ++v)  TestVertexNormalArray[v] = FVector(0, 0, 0);
	// Testing. Loop over all the verts and make sure they are mostly in the same direction as the face normals.
	for (int32 face = 0; face < NumFaces; ++face)
	{
		int32 offset = face * 3;

		const auto& faceNormal = FaceNormalArray[face];
		for (int32 i = offset; i < offset + 3; ++i)
		{
			uint32 v = VertexDataMesh.Indices[i];
			TestVertexNormalArray[v] += faceNormal;
		}
	}

	for (int32 v = 0; v < NumVerts; ++v)  TestVertexNormalArray[v].Normalize();

	for (int32 v = 0; v < NumVerts; ++v)
	{

		const auto& vertexNormal = VertexNormalArray[v];
		const auto& testVertexNormal = TestVertexNormalArray[v];

		checkSlow(FVector::DotProduct(vertexNormal, testVertexNormal) > 0.99);

	}

	// Testing. Loop over all the verts and make sure they are mostly in the same direction as the face normals.
	for (int32 face = 0; face < NumFaces; ++face)
	{
		int32 offset = face * 3;

		const auto& faceNormal = FaceNormalArray[face];
		for (int32 i = offset; i < offset + 3; ++i)
		{
			uint32 v = VertexDataMesh.Indices[i];
			const auto& vertexNormal = VertexNormalArray[v];

			//	checkSlow(FVector::DotProduct(vertexNormal, faceNormal) > 0.3);
		}
	}



	// Lets go ahead and add a BS tangent space.
	ProxyLOD::Parallel_For(ProxyLOD::FIntRange(0, NumVerts),
		[&VertexDataMesh](const ProxyLOD::FIntRange&  Range)
	{
		auto& TangentArray = VertexDataMesh.Tangent;
		auto& BiTangentArray = VertexDataMesh.BiTangent;
		const auto& NormalArray = VertexDataMesh.Normal;

		for (int32 i = Range.begin(), I = Range.end(); i < I; ++i)
		{
			const FVector& Normal = NormalArray[i];
			FVector Tangent(1, 0, 0);
			Tangent = Tangent - Normal * FVector::DotProduct(Normal, Tangent);
			Tangent.Normalize();

			FVector BiTangent(0, 1, 0);
			BiTangent = BiTangent - Normal * FVector::DotProduct(Normal, BiTangent);
			BiTangent = BiTangent - Tangent * FVector::DotProduct(Tangent, BiTangent);
			BiTangent.Normalize();

			TangentArray[i] = Tangent;
			BiTangentArray[i] = BiTangent;

		}

	});
}
#endif 


#undef LOCTEXT_NAMESPACE