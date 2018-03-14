// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "SkeletalMeshTools.h"
#include "Engine/SkeletalMesh.h"
#include "MeshBuild.h"
#include "MeshUtilities.h"
#include "RawIndexBuffer.h"
#include "Rendering/SkeletalMeshModel.h"

namespace SkeletalMeshTools
{
	bool AreSkelMeshVerticesEqual( const FSoftSkinBuildVertex& V1, const FSoftSkinBuildVertex& V2, const FOverlappingThresholds& OverlappingThresholds)
	{
		if(!PointsEqual(V1.Position, V2.Position, OverlappingThresholds))
		{
			return false;
		}

		for(int32 UVIdx = 0; UVIdx < MAX_TEXCOORDS; ++UVIdx)
		{
			if (!UVsEqual(V1.UVs[UVIdx], V2.UVs[UVIdx], OverlappingThresholds))
			{
				return false;
			}
		}

		if(!NormalsEqual(V1.TangentX, V2.TangentX, OverlappingThresholds))
		{
			return false;
		}

		if(!NormalsEqual(V1.TangentY, V2.TangentY, OverlappingThresholds))
		{
			return false;
		}

		if(!NormalsEqual(V1.TangentZ, V2.TangentZ, OverlappingThresholds))
		{
			return false;
		}

		bool	InfluencesMatch = 1;
		for(uint32 InfluenceIndex = 0; InfluenceIndex < MAX_TOTAL_INFLUENCES; InfluenceIndex++)
		{
			if(V1.InfluenceBones[InfluenceIndex] != V2.InfluenceBones[InfluenceIndex] ||
				V1.InfluenceWeights[InfluenceIndex] != V2.InfluenceWeights[InfluenceIndex])
			{
				InfluencesMatch = 0;
				break;
			}
		}

		if (V1.Color != V2.Color)
		{
			return false;
		}

		if(!InfluencesMatch)
		{
			return false;
		}

		return true;
	}

	void BuildSkeletalMeshChunks( const TArray<FMeshFace>& Faces, const TArray<FSoftSkinBuildVertex>& RawVertices, TArray<FSkeletalMeshVertIndexAndZ>& RawVertIndexAndZ, const FOverlappingThresholds &OverlappingThresholds, TArray<FSkinnedMeshChunk*>& OutChunks, bool& bOutTooManyVerts )
	{
		TArray<int32> DupVerts;

		TMultiMap<int32, int32> RawVerts2Dupes;
		{

			// Sorting function for vertex Z/index pairs
			struct FCompareFSkeletalMeshVertIndexAndZ
			{
				FORCEINLINE bool operator()(const FSkeletalMeshVertIndexAndZ& A, const FSkeletalMeshVertIndexAndZ& B) const
				{
					return A.Z < B.Z;
				}
			};

			// Sort the vertices by z value
			RawVertIndexAndZ.Sort(FCompareFSkeletalMeshVertIndexAndZ());

			// Search for duplicates, quickly!
			for(int32 i = 0; i < RawVertIndexAndZ.Num(); i++)
			{
				// only need to search forward, since we add pairs both ways
				for(int32 j = i + 1; j < RawVertIndexAndZ.Num(); j++)
				{
					if(FMath::Abs(RawVertIndexAndZ[j].Z - RawVertIndexAndZ[i].Z) > OverlappingThresholds.ThresholdPosition)
					{
						// our list is sorted, so there can't be any more dupes
						break;
					}

					// check to see if the points are really overlapping
					if(PointsEqual(
						RawVertices[RawVertIndexAndZ[i].Index].Position,
						RawVertices[RawVertIndexAndZ[j].Index].Position, OverlappingThresholds))
					{
						RawVerts2Dupes.Add(RawVertIndexAndZ[i].Index, RawVertIndexAndZ[j].Index);
						RawVerts2Dupes.Add(RawVertIndexAndZ[j].Index, RawVertIndexAndZ[i].Index);
					}
				}
			}
		}

		TMap<FSkinnedMeshChunk* , TMap<int32, int32> > ChunkToFinalVerts;

	
		uint32 TriangleIndices[3];
		for(int32 FaceIndex = 0; FaceIndex < Faces.Num(); FaceIndex++)
		{
			const FMeshFace& Face = Faces[FaceIndex];

			// Find a chunk which matches this triangle.
			FSkinnedMeshChunk* Chunk = NULL;
			for(int32 i = 0; i < OutChunks.Num(); ++i)
			{
				if(OutChunks[i]->MaterialIndex == Face.MeshMaterialIndex)
				{
					Chunk = OutChunks[i];
					break;
				}
			}
			if(Chunk == NULL)
			{
				Chunk = new FSkinnedMeshChunk();
				Chunk->MaterialIndex = Face.MeshMaterialIndex;
				Chunk->OriginalSectionIndex = OutChunks.Num();
				OutChunks.Add(Chunk);
			}

			TMap<int32, int32>& FinalVerts = ChunkToFinalVerts.FindOrAdd( Chunk );

			for(int32 VertexIndex = 0; VertexIndex < 3; ++VertexIndex)
			{
				int32 WedgeIndex = FaceIndex * 3 + VertexIndex;
				const FSoftSkinBuildVertex& Vertex = RawVertices[WedgeIndex];

				int32 FinalVertIndex = INDEX_NONE;
				DupVerts.Reset();
				RawVerts2Dupes.MultiFind(WedgeIndex, DupVerts);
				DupVerts.Sort();


				for(int32 k = 0; k < DupVerts.Num(); k++)
				{
					if(DupVerts[k] >= WedgeIndex)
					{
						// the verts beyond me haven't been placed yet, so these duplicates are not relevant
						break;
					}

					int32 *Location = FinalVerts.Find(DupVerts[k]);
					if(Location != NULL)
					{
						if(SkeletalMeshTools::AreSkelMeshVerticesEqual(Vertex, Chunk->Vertices[*Location], OverlappingThresholds))
						{
							FinalVertIndex = *Location;
							break;
						}
					}
				}
				if(FinalVertIndex == INDEX_NONE)
				{
					FinalVertIndex = Chunk->Vertices.Add(Vertex);
					FinalVerts.Add(WedgeIndex, FinalVertIndex);
				}

				// set the index entry for the newly added vertex
				// TArray internally has int32 for capacity, so no need to test for uint32 as it's larger than int32
				TriangleIndices[VertexIndex] = (uint32)FinalVertIndex;
			}

			if(TriangleIndices[0] != TriangleIndices[1] && TriangleIndices[0] != TriangleIndices[2] && TriangleIndices[1] != TriangleIndices[2])
			{
				for(uint32 VertexIndex = 0; VertexIndex < 3; VertexIndex++)
				{
					Chunk->Indices.Add(TriangleIndices[VertexIndex]);
				}
			}
		}
	}

	void ChunkSkinnedVertices(TArray<FSkinnedMeshChunk*>& Chunks,int32 MaxBonesPerChunk)
	{
#if WITH_EDITORONLY_DATA
		// Copy over the old chunks (this is just copying pointers).
		TArray<FSkinnedMeshChunk*> SrcChunks;
		Exchange(Chunks,SrcChunks);

		// Sort the chunks by material index.
		struct FCompareSkinnedMeshChunk
		{
			FORCEINLINE bool operator()(const FSkinnedMeshChunk& A,const FSkinnedMeshChunk& B) const
			{
				return A.MaterialIndex < B.MaterialIndex;
			}
		};
		SrcChunks.Sort(FCompareSkinnedMeshChunk());

		// Now split chunks to respect the desired bone limit.
		TIndirectArray<TArray<int32> > IndexMaps;
		TArray<FBoneIndexType, TInlineAllocator<MAX_TOTAL_INFLUENCES*3> > UniqueBones;
		for (int32 SrcChunkIndex = 0; SrcChunkIndex < SrcChunks.Num(); ++SrcChunkIndex)
		{
			FSkinnedMeshChunk* SrcChunk = SrcChunks[SrcChunkIndex];
			int32 FirstChunkIndex = Chunks.Num();

			for (int32 i = 0; i < SrcChunk->Indices.Num(); i += 3)
			{
				// Find all bones needed by this triangle.
				UniqueBones.Reset();
				for (int32 Corner = 0; Corner < 3; Corner++)
				{
					int32 VertexIndex = SrcChunk->Indices[i + Corner];
					FSoftSkinBuildVertex& V = SrcChunk->Vertices[VertexIndex];
					for (int32 InfluenceIndex = 0; InfluenceIndex < MAX_TOTAL_INFLUENCES; InfluenceIndex++)
					{
						if (V.InfluenceWeights[InfluenceIndex] > 0)
						{
							UniqueBones.AddUnique(V.InfluenceBones[InfluenceIndex]);
						}
					}
				}

				// Now find a chunk for them.
				FSkinnedMeshChunk* DestChunk = NULL;
				int32 DestChunkIndex = FirstChunkIndex;
				for (; DestChunkIndex < Chunks.Num(); ++DestChunkIndex)
				{
					TArray<FBoneIndexType>& BoneMap = Chunks[DestChunkIndex]->BoneMap;
					int32 NumUniqueBones = 0;
					for (int32 j = 0; j < UniqueBones.Num(); ++j)
					{
						NumUniqueBones += (BoneMap.Contains(UniqueBones[j]) ? 0 : 1);
					}
					if (NumUniqueBones + BoneMap.Num() <= MaxBonesPerChunk)
					{
						DestChunk = Chunks[DestChunkIndex];
						break;
					}
				}

				// If no chunk was found, create one!
				if (DestChunk == NULL)
				{
					DestChunk = new FSkinnedMeshChunk();
					Chunks.Add(DestChunk);
					DestChunk->MaterialIndex = SrcChunk->MaterialIndex;
					DestChunk->OriginalSectionIndex = SrcChunk->OriginalSectionIndex;
					TArray<int32>& IndexMap = *new(IndexMaps) TArray<int32>();
					IndexMap.AddUninitialized(SrcChunk->Vertices.Num());
					FMemory::Memset(IndexMap.GetData(),0xff,IndexMap.GetTypeSize()*IndexMap.Num());
				}
				TArray<int32>& IndexMap = IndexMaps[DestChunkIndex];

				// Add the unique bones to this chunk's bone map.
				for (int32 j = 0; j < UniqueBones.Num(); ++j)
				{
					DestChunk->BoneMap.AddUnique(UniqueBones[j]);
				}

				// For each vertex, add it to the chunk's arrays of vertices and indices.
				for (int32 Corner = 0; Corner < 3; Corner++)
				{
					int32 VertexIndex = SrcChunk->Indices[i + Corner];
					int32 DestIndex = IndexMap[VertexIndex];
					if (DestIndex == INDEX_NONE)
					{
						DestIndex = DestChunk->Vertices.Add(SrcChunk->Vertices[VertexIndex]);
						FSoftSkinBuildVertex& V = DestChunk->Vertices[DestIndex];
						for (int32 InfluenceIndex = 0; InfluenceIndex < MAX_TOTAL_INFLUENCES; InfluenceIndex++)
						{
							if (V.InfluenceWeights[InfluenceIndex] > 0)
							{
								int32 MappedIndex = DestChunk->BoneMap.Find(V.InfluenceBones[InfluenceIndex]);
								check(DestChunk->BoneMap.IsValidIndex(MappedIndex));
								V.InfluenceBones[InfluenceIndex] = MappedIndex;
							}
						}
						IndexMap[VertexIndex] = DestIndex;
					}
					DestChunk->Indices.Add(DestIndex);
				}
			}

			// Source chunks are no longer needed.
			delete SrcChunks[SrcChunkIndex];
			SrcChunks[SrcChunkIndex] = NULL;
		}
#endif // #if WITH_EDITORONLY_DATA
	}

	/**
	 * Copies data out of Model so that the data can be processed in the background.
	 */
	void CopySkinnedModelData(FSkinnedModelData& OutData, FSkeletalMeshLODModel& Model)
	{
	#if WITH_EDITORONLY_DATA
		Model.GetVertices(OutData.Vertices);
		OutData.Indices = Model.IndexBuffer;
		if (Model.RawPointIndices.GetElementCount() == OutData.Vertices.Num())
		{
			OutData.RawPointIndices.Empty(Model.RawPointIndices.GetElementCount());
			OutData.RawPointIndices.AddUninitialized(Model.RawPointIndices.GetElementCount());
			void* DestPtr = OutData.RawPointIndices.GetData();
			Model.RawPointIndices.GetCopy(&DestPtr, /*bDiscardInternalCopy=*/false);
			check(DestPtr == OutData.RawPointIndices.GetData());
		}
		OutData.MeshToImportVertexMap = Model.MeshToImportVertexMap;
		OutData.Sections = Model.Sections;
		for (int32 SectionIndex = 0; SectionIndex < Model.Sections.Num(); ++SectionIndex)
		{
			TArray<FBoneIndexType>& DestBoneMap = *new(OutData.BoneMaps) TArray<FBoneIndexType>();
			DestBoneMap = Model.Sections[SectionIndex].BoneMap;
		}
		OutData.NumTexCoords = Model.NumTexCoords;
	#endif // #if WITH_EDITORONLY_DATA
	};
	
	// Find the most dominant bone for each vertex
	int32 GetDominantBoneIndex(FSoftSkinVertex* SoftVert)
	{
		uint8 MaxWeightBone = 0;
		uint8 MaxWeightWeight = 0;

		for(int32 i=0; i<MAX_TOTAL_INFLUENCES; i++)
		{
			if(SoftVert->InfluenceWeights[i] > MaxWeightWeight)
			{
				MaxWeightWeight = SoftVert->InfluenceWeights[i];
				MaxWeightBone = SoftVert->InfluenceBones[i];
			}
		}

		return MaxWeightBone;
	}

	
	void CalcBoneVertInfos(USkeletalMesh* SkeletalMesh, TArray<FBoneVertInfo>& Infos, bool bOnlyDominant)
	{
		FSkeletalMeshModel* ImportedResource = SkeletalMesh->GetImportedModel();
		if (ImportedResource->LODModels.Num() == 0)
			return;

		SkeletalMesh->CalculateInvRefMatrices();
		check( SkeletalMesh->RefSkeleton.GetRawBoneNum() == SkeletalMesh->RefBasesInvMatrix.Num() );

		Infos.Empty();
		Infos.AddZeroed( SkeletalMesh->RefSkeleton.GetRawBoneNum() );

		FSkeletalMeshLODModel* LODModel = &ImportedResource->LODModels[0];
		for(int32 SectionIndex = 0; SectionIndex < LODModel->Sections.Num(); SectionIndex++)
		{
			FSkelMeshSection& Section = LODModel->Sections[SectionIndex];
			for(int32 i=0; i<Section.SoftVertices.Num(); i++)
			{
				FSoftSkinVertex* SoftVert = &Section.SoftVertices[i];

				if(bOnlyDominant)
				{
					int32 BoneIndex = Section.BoneMap[GetDominantBoneIndex(SoftVert)];

					FVector LocalPos = SkeletalMesh->RefBasesInvMatrix[BoneIndex].TransformPosition(SoftVert->Position);
					Infos[BoneIndex].Positions.Add(LocalPos);

					FVector LocalNormal = SkeletalMesh->RefBasesInvMatrix[BoneIndex].TransformVector(SoftVert->TangentZ);
					Infos[BoneIndex].Normals.Add(LocalNormal);
				}
				else
				{
					for(int32 j=0; j<MAX_TOTAL_INFLUENCES; j++)
					{
						if(SoftVert->InfluenceWeights[j] > 0)
						{
							int32 BoneIndex = Section.BoneMap[SoftVert->InfluenceBones[j]];

							FVector LocalPos = SkeletalMesh->RefBasesInvMatrix[BoneIndex].TransformPosition(SoftVert->Position);
							Infos[BoneIndex].Positions.Add(LocalPos);

							FVector LocalNormal = SkeletalMesh->RefBasesInvMatrix[BoneIndex].TransformVector(SoftVert->TangentZ);
							Infos[BoneIndex].Normals.Add(LocalNormal);
						}
					}
				}
			}
		}
	}
}

