// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ProxyLODMeshConvertUtils.h" 
#include "ProxyLODMeshUtilities.h"

// Convert QuadMesh to Triangles by splitting
void ProxyLOD::MixedPolyMeshToRawMesh(const FMixedPolyMesh& SimpleMesh, FRawMesh& DstRawMesh)
{
	// Splitting a quad doesn't introduce any new verts.
	const uint32 DstNumVerts = SimpleMesh.Points.size();

	const uint32 NumQuads = SimpleMesh.Quads.size();

	// Each quad becomes 2 triangles.
	const uint32 DstNumTris = 2 * NumQuads + SimpleMesh.Triangles.size();

	// Each Triangle has 3 corners
	const uint32 DstNumIndexes = 3 * DstNumTris;
	// Copy the vertices over
	{
		// Allocate the space for the verts in the DstRawMesh

		ResizeArray(DstRawMesh.VertexPositions, DstNumVerts);

		ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumVerts),
			[&SimpleMesh, &DstRawMesh](const ProxyLOD::FUIntRange& Range)
		{
			// @todo DJH tbb::ProxyLOD::Parallel_For
			for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
			{
				const openvdb::Vec3s& Vertex = SimpleMesh.Points[i];
				DstRawMesh.VertexPositions[i] = FVector(Vertex[0], Vertex[1], Vertex[2]);
			}
		}
		);
	}

	// Connectivity: 
	{
		// convert each quad into two triangles.
		TArray<uint32>& WedgeIndices = DstRawMesh.WedgeIndices;
		ResizeArray(WedgeIndices, DstNumIndexes);

		ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, NumQuads),
			[&WedgeIndices, &SimpleMesh](const ProxyLOD::FUIntRange& Range)
		{
			for (uint32 q = Range.begin(), Q = Range.end(); q < Q; ++q)
			{
				const uint32 Offset = q * 6;
				const openvdb::Vec4I& Quad = SimpleMesh.Quads[q];
				// add as two triangles to raw mesh
#if (PROXYLOD_CLOCKWISE_TRIANGLES == 1)
				// first triangle
				WedgeIndices[Offset] = Quad[0];
				WedgeIndices[Offset + 1] = Quad[1];
				WedgeIndices[Offset + 2] = Quad[2];
				// second triangle
				WedgeIndices[Offset + 3] = Quad[2];
				WedgeIndices[Offset + 4] = Quad[3];
				WedgeIndices[Offset + 5] = Quad[0];
#else
				WedgeIndices[Offset] = Quad[0];
				WedgeIndices[Offset + 1] = Quad[3];
				WedgeIndices[Offset + 2] = Quad[2];
				// second triangle
				WedgeIndices[Offset + 3] = Quad[2];
				WedgeIndices[Offset + 4] = Quad[1];
				WedgeIndices[Offset + 5] = Quad[0];
#endif 
			}

		});

		// add the SimpleMesh triangles.
		ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, SimpleMesh.Triangles.size()),
			[&WedgeIndices, &SimpleMesh, NumQuads](const ProxyLOD::FUIntRange& Range)
		{
			for (uint32 t = Range.begin(), T = Range.end(); t < T; ++t)
			{
				const uint32 Offset = NumQuads * 6 + t * 3;
				const openvdb::Vec3I& Tri = SimpleMesh.Triangles[t];
				// add the triangle to the raw mesh
#if (PROXYLOD_CLOCKWISE_TRIANGLES == 1)
				WedgeIndices[Offset] = Tri[0];
				WedgeIndices[Offset + 1] = Tri[1];
				WedgeIndices[Offset + 2] = Tri[2];
#else
				WedgeIndices[Offset] = Tri[2];
				WedgeIndices[Offset + 1] = Tri[1];
				WedgeIndices[Offset + 2] = Tri[0];
#endif 
			}
		}
		);
	}


	ResizeArray(DstRawMesh.WedgeTangentX, DstNumIndexes);
	ResizeArray(DstRawMesh.WedgeTangentY, DstNumIndexes);
	ResizeArray(DstRawMesh.WedgeTangentZ, DstNumIndexes);


	// Generate Tangent-Space
	// djh: adding default values for now
	ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumIndexes), [&DstRawMesh](const ProxyLOD::FUIntRange& Range)
	{
		for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
		{
			DstRawMesh.WedgeTangentX[i] = FVector(1, 0, 0);
			DstRawMesh.WedgeTangentY[i] = FVector(0, 1, 0);
			// TangentZ is the normal - will be over-written later
			DstRawMesh.WedgeTangentZ[i] = FVector(0, 0, 1);
		}
	});



	DstRawMesh.FaceMaterialIndices.Empty(DstNumTris);
	DstRawMesh.FaceMaterialIndices.AddZeroed(DstNumTris);

	DstRawMesh.FaceSmoothingMasks.Empty(DstNumTris);
	DstRawMesh.FaceSmoothingMasks.AddZeroed(DstNumTris);

	DstRawMesh.WedgeColors.Empty();
	DstRawMesh.WedgeColors.AddZeroed(3 * DstNumTris);
	for (int32 TexCoordIndex = 0; TexCoordIndex < 8; ++TexCoordIndex)
	{
		DstRawMesh.WedgeTexCoords[TexCoordIndex].Empty();
	}
	// Need to have a a WedgeTextCoord channel
	auto& WedgeTexCoord = DstRawMesh.WedgeTexCoords[0];
	WedgeTexCoord.AddZeroed(DstNumIndexes);
}


void ProxyLOD::AOSMeshToRawMesh(const FAOSMesh& AOSMesh, FRawMesh& OutRawMesh)
{


	const uint32 DstNumPositions = AOSMesh.GetNumVertexes();
	const uint32 DstNumIndexes = AOSMesh.GetNumIndexes();

	checkSlow(DstNumIndexes % 3 == 0);
	// Copy the vertices over
	{
		// Allocate the space for the verts in the DstRawMesh
		auto& VertexPositions = OutRawMesh.VertexPositions;
		ResizeArray(VertexPositions, DstNumPositions);

		ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumPositions),
			[&AOSMesh, &VertexPositions](const ProxyLOD::FUIntRange& Range)
		{
			const auto& AOSVertexes = AOSMesh.Vertexes;
			for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
			{
				const FVector& Position = AOSVertexes[i].GetPos();
				VertexPositions[i] = Position;
			}
		});

		checkSlow(OutRawMesh.VertexPositions.Num() == DstNumPositions);
	}

	// Connectivity: 
	{
		// convert each quad into two triangles.
		TArray<uint32>& WedgeIndices = OutRawMesh.WedgeIndices;

		ResizeArray(WedgeIndices, DstNumIndexes);

		ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumIndexes),
			[&AOSMesh, &WedgeIndices](const ProxyLOD::FUIntRange& Range)
		{
			for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
			{
				WedgeIndices[i] = AOSMesh.Indexes[i];
			}
		});

		checkSlow(WedgeIndices.Num() == DstNumIndexes);
	}

	// Add default Tangents
	{

		ResizeArray(OutRawMesh.WedgeTangentX, DstNumIndexes);
		ResizeArray(OutRawMesh.WedgeTangentY, DstNumIndexes);

		// Generate Tangent-Space
		// djh: adding default values for now
		ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumIndexes), [&OutRawMesh](const ProxyLOD::FUIntRange& Range)
		{
			for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
			{
				OutRawMesh.WedgeTangentX[i] = FVector(1, 0, 0);
				OutRawMesh.WedgeTangentY[i] = FVector(0, 1, 0);
			}
		});
	}

	// Transfer the normal
	{
		TArray<FVector>& NormalArray = OutRawMesh.WedgeTangentZ;
		ResizeArray(NormalArray, DstNumIndexes);

		ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumIndexes),
			[&AOSMesh, &NormalArray](const ProxyLOD::FUIntRange& Range)
		{
			const uint32* Indexes = AOSMesh.Indexes;

			for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
			{
				const auto& AOSVertex = AOSMesh.Vertexes[Indexes[i]];
				NormalArray[i] = AOSVertex.Normal;
			}
		});
	}

	// Makeup other data.
	// DJH - Note I should be able to 'transfer' the FaceMatieralIdx, but this will require generating
	// Auxiliary data because I only have this data on verts right now.
	{
		const uint32 DstNumTris = DstNumIndexes / 3;

		OutRawMesh.FaceMaterialIndices.Empty(DstNumTris);
		OutRawMesh.FaceMaterialIndices.AddZeroed(DstNumTris);

		OutRawMesh.FaceSmoothingMasks.Empty(DstNumTris);
		OutRawMesh.FaceSmoothingMasks.AddZeroed(DstNumTris);

		OutRawMesh.WedgeColors.Empty();
		OutRawMesh.WedgeColors.AddZeroed(3 * DstNumTris);
		// what about red?
		for (uint32 i = 0; i < 3 * DstNumTris; ++i)
		{
			OutRawMesh.WedgeColors[i] = FColor(255, 0, 0, 0);
		}

		for (int32 TexCoordIndex = 0; TexCoordIndex < 8; ++TexCoordIndex)
		{
			OutRawMesh.WedgeTexCoords[TexCoordIndex].Empty();
		}
		// Need to have a a WedgeTextCoord channel
		auto& WedgeTexCoord = OutRawMesh.WedgeTexCoords[0];
		WedgeTexCoord.AddZeroed(DstNumIndexes);
	}
}


void ProxyLOD::VertexDataMeshToRawMesh(const FVertexDataMesh& SrcVertexDataMesh, FRawMesh& DstRawMesh)
{
	

	const uint32 DstNumPositions = SrcVertexDataMesh.Points.Num();
	const uint32 DstNumIndexes = SrcVertexDataMesh.Indices.Num();

	checkSlow(DstNumIndexes % 3 == 0);
	// Copy the vertices over
	{
		// Allocate the space for the verts in the DstRawMesh

		const TArray<FVector>& SrcPositions = SrcVertexDataMesh.Points;
		TArray<FVector>& DstPositions = DstRawMesh.VertexPositions;

		ResizeArray(DstPositions, DstNumPositions);


		ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumPositions),
			[&SrcPositions, &DstPositions](const ProxyLOD::FUIntRange& Range)
		{
			for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
			{
				DstPositions[i] = SrcPositions[i];
			}
		});

		checkSlow(DstPositions.Num() == DstNumPositions);
	}

	// Connectivity: 
	{
		const TArray<uint32>& SrcIndices = SrcVertexDataMesh.Indices;
		TArray<uint32>& DstIndices = DstRawMesh.WedgeIndices;

		ResizeArray(DstIndices, DstNumIndexes);

		ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumIndexes),
			[&SrcIndices, &DstIndices](const ProxyLOD::FUIntRange& Range)
		{
			for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
			{
				DstIndices[i] = SrcIndices[i];
			}
		});

		checkSlow(DstIndices.Num() == DstNumIndexes);
	}

	// Copy the tangent space
	{
		const TArray<FVector>& SrcTangentArray = SrcVertexDataMesh.Tangent;
		const TArray<FVector>& SrcBiTangentArray = SrcVertexDataMesh.BiTangent;
		const TArray<FVector>& SrcNormalArray = SrcVertexDataMesh.Normal;

		TArray<FVector>& DstTangentArray = DstRawMesh.WedgeTangentX;
		TArray<FVector>& DstBiTangentArray = DstRawMesh.WedgeTangentY;
		TArray<FVector>& DstNormalArray = DstRawMesh.WedgeTangentZ;

		ResizeArray(DstTangentArray, DstNumIndexes);
		ResizeArray(DstBiTangentArray, DstNumIndexes);
		ResizeArray(DstNormalArray, DstNumIndexes);

		const bool bSrcHasTangentSpace = SrcVertexDataMesh.Tangent.Num() != 0 && SrcVertexDataMesh.BiTangent.Num() != 0 && SrcVertexDataMesh.Normal.Num() != 0;
		if (bSrcHasTangentSpace)
		{

			const auto& SrcIndexes = SrcVertexDataMesh.Indices;

			// Copy tangent space.  
			// NB: The raw mesh potentially stores a different tangent space
			// for each index.  For the vertex data mesh, the tangent space is per-vertex.
			ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumIndexes),
				[&SrcTangentArray, &SrcIndexes, &DstTangentArray](const ProxyLOD::FUIntRange& Range)
			{
				for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
				{
					DstTangentArray[i] = SrcTangentArray[SrcIndexes[i]];
				}
			});

			ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumIndexes),
				[&SrcBiTangentArray, &SrcIndexes, &DstBiTangentArray](const ProxyLOD::FUIntRange& Range)
			{
				for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
				{
					DstBiTangentArray[i] = SrcBiTangentArray[SrcIndexes[i]];
				}
			});

			ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumIndexes),
				[&SrcNormalArray, &SrcIndexes, &DstNormalArray](const ProxyLOD::FUIntRange& Range)
			{
				for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
				{
					DstNormalArray[i] = SrcNormalArray[SrcIndexes[i]];
				}
			});
		}
		else // generate a default tangent space
		{
			for (uint32 i = 0; i < DstNumIndexes; ++i)
			{
				DstTangentArray[i] = FVector(1, 0, 0);
				DstBiTangentArray[i] = FVector(0, 1, 0);
				DstNormalArray[i] = FVector(0, 0, 1);
			}
		}
	}

	// DJH Testing.
	// Is the handedness the same for all verts on a given face?
	{
		const uint32 DstNumTris = DstNumIndexes / 3;


		ResizeArray(DstRawMesh.FaceMaterialIndices, DstNumTris);
		ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumTris), [&DstRawMesh](const ProxyLOD::FUIntRange& Range)
		{
			auto& FaceMaterialIndices = DstRawMesh.FaceMaterialIndices;
			for (uint32 f = Range.begin(), F = Range.end(); f < F; ++f)
			{
				FaceMaterialIndices[f] = 0;
			}
		});

		ResizeArray(DstRawMesh.FaceSmoothingMasks, DstNumTris);
		if (SrcVertexDataMesh.FacePartition.Num() != 0)
		{

			ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumTris), [&DstRawMesh, &SrcVertexDataMesh](const ProxyLOD::FUIntRange& Range)
			{
				const auto& FacePartition = SrcVertexDataMesh.FacePartition;
				auto& FaceSmoothingMask = DstRawMesh.FaceSmoothingMasks;
				for (uint32 f = Range.begin(), F = Range.end(); f < F; ++f)
				{
					FaceSmoothingMask[f] = (1 << (FacePartition[f] % 32));
				}
			});
		}
		else // default 
		{
			for (uint32 i = 0; i < DstNumTris; ++i)
			{
				DstRawMesh.FaceSmoothingMasks[i] = 1;
			}
		}

		ResizeArray(DstRawMesh.WedgeColors, 3 * DstNumTris);
		if (SrcVertexDataMesh.FaceColors.Num() == 0)
		{

			// what about red?
			for (uint32 i = 0; i < 3 * DstNumTris; ++i)
			{
				DstRawMesh.WedgeColors[i] = FColor(255, 0, 0, 0);
			}
		}
		else
		{
			// Transfer color to the verts.
			for (uint32 i = 0; i < DstNumTris; ++i)
			{
				const uint32 WedgeId = i * 3;
#if 1
				const FColor& Color = SrcVertexDataMesh.FaceColors[i];

				DstRawMesh.WedgeColors[WedgeId + 0] = Color;
				DstRawMesh.WedgeColors[WedgeId + 1] = Color;
				DstRawMesh.WedgeColors[WedgeId + 2] = Color;
#else

				uint32 Idx[3];
				for (uint32 i = 0; i < 3; ++i) Idx[i] = WedgeId + i;

				for (uint32 i = 0; i < 3; ++i) Idx[i] = SrcVertexDataMesh.Indices[Idx[i]];

				for (uint32 i = 0; i < 3; ++i) DstRawMesh.WedgeColors[WedgeId + i] = (SrcVertexDataMesh.TangentHanded[Idx[i]] > 0) ? FColor(255, 0, 0) : FColor(0, 0, 255);
#endif
			}
		}
#if 0
		// DJH
		// Testing with the handedness

		for (uint32 i = 0; i < DstNumIndexes; ++i)
		{
			uint32 v = DstRawMesh.WedgeIndices[i];
			DstRawMesh.WedgeColors[i] = (SrcVertexDataMesh.TangentHanded[v] > 0) ? FColor(255, 0, 0) : FColor(0, 255, 0);
		}
#endif


		{
			// Empty all the texture coords
			for (int32 TexCoordIndex = 0; TexCoordIndex < 8; ++TexCoordIndex)
			{
				DstRawMesh.WedgeTexCoords[TexCoordIndex].Empty();
			}

			// Copy the UVs over to the first and second texture coord channel.  The second one is used by lightmass, 
			// if we don't provide it, unreal will make it from the first for us
			for (int32 channel = 0; channel < 2; ++channel)
			{
				// Need to have a a WedgeTextCoord channel
				auto& WedgeTexCoord = DstRawMesh.WedgeTexCoords[channel];

				if (SrcVertexDataMesh.UVs.Num() == 0)
				{
					WedgeTexCoord.AddZeroed(DstNumIndexes);
				}
				else
				{
					WedgeTexCoord.AddUninitialized(DstNumIndexes);
					const uint32* Indices = SrcVertexDataMesh.Indices.GetData();
					const auto& SrcUVs = SrcVertexDataMesh.UVs;
					ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumIndexes), [&Indices, &SrcUVs, &WedgeTexCoord](const ProxyLOD::FUIntRange& Range)
					{
						for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
						{
							const uint32 VertId = Indices[i];
							const FVector2D& UV = SrcUVs[VertId];

							WedgeTexCoord[i] = UV;
						}
					});
				}
			}
		}

	}
}


// Converts a raw mesh to a vertex data mesh.  This is potentially has some loss since the 
// raw mesh is nominally a per-index data structure and the vertex data mesh is a per-vertex structure.
// In addition, this only transfers the first texture coordinate and ignores material ids and vertex colors.

void ProxyLOD::RawMeshToVertexDataMesh(const FRawMesh& SrcRawMesh, FVertexDataMesh& DstVertexDataMesh)
{


	const uint32 DstNumPositions = SrcRawMesh.VertexPositions.Num();
	const uint32 DstNumIndexes = SrcRawMesh.WedgeIndices.Num();

	checkSlow(DstNumIndexes % 3 == 0);
	// Copy the vertices over
	{
		// Allocate the space for the verts in the VertexDataMesh

		ResizeArray(DstVertexDataMesh.Points, DstNumPositions);

		ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumPositions),
			[&DstVertexDataMesh, &SrcRawMesh](const ProxyLOD::FUIntRange& Range)
		{
			const FVector* SrcPos = SrcRawMesh.VertexPositions.GetData();
			FVector* DstPos = DstVertexDataMesh.Points.GetData();
			for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
			{
				DstPos[i] = SrcPos[i];

			}
		});

		checkSlow(DstVertexDataMesh.Points.Num() == DstNumPositions);
	}

	// Connectivity: 
	{
		// convert each quad into two triangles.
		const TArray<uint32>& SrcIndices = SrcRawMesh.WedgeIndices;
		TArray<uint32>& DstIndices = DstVertexDataMesh.Indices;
		ResizeArray(DstIndices, DstNumIndexes);

		ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumIndexes),
			[&DstIndices, &SrcIndices](const ProxyLOD::FUIntRange& Range)
		{;
		for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
		{
			DstIndices[i] = SrcIndices[i];
		}
		});

		checkSlow(DstIndices.Num() == DstNumIndexes);
	}

	// Copy the tangent space:
	// NB: The tangent space is stored per-index in the raw mesh, but only per-vertex in the vertex data mesh.
	// We assume that the raw mesh per-index data is really duplicated per-vertex data!

	{
		// Note, these aren't threaded to avoid possible data-races.
		const bool bThreaded = false;


		const TArray<FVector>& SrcTangentArray = SrcRawMesh.WedgeTangentX;
		const TArray<FVector>& SrcBiTangentArray = SrcRawMesh.WedgeTangentY;
		const TArray<FVector>& SrcNormalArray = SrcRawMesh.WedgeTangentZ;

		TArray<FVector>& DstTangentArray = DstVertexDataMesh.Tangent;
		TArray<FVector>& DstBiTangentArray = DstVertexDataMesh.BiTangent;
		TArray<FVector>& DstNormalArray = DstVertexDataMesh.Normal;

		ResizeArray(DstTangentArray, DstNumPositions);
		ResizeArray(DstBiTangentArray, DstNumPositions);
		ResizeArray(DstNormalArray, DstNumPositions);

		const auto& DstIndexes = DstVertexDataMesh.Indices;
		// Copy tangent space
		ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumIndexes),
			[&DstIndexes, &SrcTangentArray, &DstTangentArray](const ProxyLOD::FUIntRange& Range)
		{
			for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
			{
				DstTangentArray[DstIndexes[i]] = SrcTangentArray[i];
			}
		}, bThreaded);

		ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumIndexes),
			[&DstIndexes, &DstBiTangentArray, &SrcBiTangentArray](const ProxyLOD::FUIntRange& Range)
		{
			for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
			{
				DstBiTangentArray[DstIndexes[i]] = SrcBiTangentArray[i];
			}
		}, bThreaded);

		ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumIndexes),
			[&DstIndexes, &DstNormalArray, &SrcNormalArray](const ProxyLOD::FUIntRange& Range)
		{
			for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
			{
				DstNormalArray[DstIndexes[i]] = SrcNormalArray[i];
			}
		}, bThreaded);
	}


	// Copy UVs
	{
		// Note, these aren't threaded to avoid possible data-races.
		const bool bThreaded = false;

		// Need to have a a WedgeTextCoord channel
		auto& SrcUVs = SrcRawMesh.WedgeTexCoords[0];
		TArray<FVector2D>& DstUVs = DstVertexDataMesh.UVs;

		ResizeArray(DstUVs, DstNumPositions);

		const auto& DstIndexes = DstVertexDataMesh.Indices;
		ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumIndexes),
			[&DstIndexes, &DstUVs, &SrcUVs](const ProxyLOD::FUIntRange& Range)
		{
			for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
			{
				DstUVs[DstIndexes[i]] = SrcUVs[i];
			}
		}, bThreaded);
	}
}




template <typename  AOSVertexType>
static void CopyIndexAndPos(const TAOSMesh<AOSVertexType>& AOSMesh, FVertexDataMesh& VertexDataMesh)
{

	const uint32 DstNumPositions = AOSMesh.GetNumVertexes();
	const uint32 DstNumIndexes = AOSMesh.GetNumIndexes();

	checkSlow(DstNumIndexes % 3 == 0);

	TArray<uint32>& WedgeIndices = VertexDataMesh.Indices;
	ResizeArray(VertexDataMesh.Points, DstNumPositions);
	ResizeArray(WedgeIndices, DstNumIndexes);


	// Copy the vertices over
	{
		ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumPositions),
			[&VertexDataMesh, &AOSMesh](const ProxyLOD::FUIntRange& Range)
		{
			FVector* Points = VertexDataMesh.Points.GetData();
			for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
			{
				const FVector& Position = AOSMesh.Vertexes[i].GetPos();
				Points[i] = Position;
			}
		}
		);

	}


	//  Connectivity
	{
		ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumIndexes),
			[&AOSMesh, &WedgeIndices](const ProxyLOD::FUIntRange& Range)
		{
			for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
			{
				WedgeIndices[i] = AOSMesh.Indexes[i];
			}
		}
		);

		checkSlow(WedgeIndices.Num() == DstNumIndexes);
	}

}

template <typename  AOSVertexType>
static void CopyNormals(const TAOSMesh<AOSVertexType>& AOSMesh, FVertexDataMesh& VertexDataMesh)
{

	// Copy the tangent space attributes.

	const uint32 DstNumPositions = AOSMesh.GetNumVertexes();

	checkSlow(AOSMesh.GetNumIndexes() % 3 == 0);

	TArray<FVector>& NormalArray = VertexDataMesh.Normal;
	ResizeArray(NormalArray, DstNumPositions);


	// Transfer the normal

	{
		ProxyLOD::Parallel_For(ProxyLOD::FUIntRange(0, DstNumPositions),
			[&AOSMesh, &NormalArray](const ProxyLOD::FUIntRange& Range)
		{
			for (uint32 i = Range.begin(), I = Range.end(); i < I; ++i)
			{
				const auto& AOSVertex = AOSMesh.Vertexes[i];
				NormalArray[i] = AOSVertex.Normal;
			}
		}
		);
	}
}


// Populate a VertexDataMesh with the information in the Array of Structs mesh.
template <typename  AOSVertexType>
static void AOSMeshToVertexDataMesh(const TAOSMesh<AOSVertexType>& AOSMesh, FVertexDataMesh& VertexDataMesh)
{

	// Copy the topology and geometry of the mesh

	CopyIndexAndPos(AOSMesh, VertexDataMesh);

	// adds t = (1,0,0)  bt = (0, 1, 0)  n = (0, 0, 1)
	ProxyLOD::AddDefaultTangentSpace(VertexDataMesh);

	// Copy the tangent space attributes.

	CopyNormals(AOSMesh, VertexDataMesh);
}


// The posistion only specialization only adds a default tangent space
template <>
void AOSMeshToVertexDataMesh<FPositionOnlyVertex>(const TAOSMesh<FPositionOnlyVertex>& AOSMesh, FVertexDataMesh& VertexDataMesh)
{

	// Copy the topology and geometry of the mesh

	CopyIndexAndPos(AOSMesh, VertexDataMesh);


	// adds t = (1,0,0)  bt = (0, 1, 0)  n = (0, 0, 1)
	ProxyLOD::AddDefaultTangentSpace(VertexDataMesh);
}


void ProxyLOD::ConvertMesh(const FAOSMesh& InMesh, FVertexDataMesh& OutMesh)
{
	AOSMeshToVertexDataMesh(InMesh, OutMesh);
}


void ProxyLOD::ConvertMesh(const FAOSMesh& InMesh, FRawMesh& OutMesh)
{
	AOSMeshToRawMesh(InMesh, OutMesh);
}

void ProxyLOD::ConvertMesh(const FVertexDataMesh& InMesh, FRawMesh& OutMesh)
{
	VertexDataMeshToRawMesh(InMesh, OutMesh);
}

void ProxyLOD::ConvertMesh(const FRawMesh& InMesh, FVertexDataMesh& OutMesh)
{
	RawMeshToVertexDataMesh(InMesh, OutMesh);
}

