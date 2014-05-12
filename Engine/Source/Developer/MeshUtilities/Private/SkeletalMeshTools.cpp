// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MeshUtilitiesPrivate.h"
#include "MeshUtilities.h"
#include "MeshBuild.h"
#include "SkeletalMeshTools.h"

namespace SkeletalMeshTools
{
	int32 AddSkinVertex(TArray<FSoftSkinBuildVertex>& Vertices,FSoftSkinBuildVertex& Vertex, bool bKeepOverlappingVertices )
	{
		if (!bKeepOverlappingVertices)
		{
			for(uint32 VertexIndex = 0;VertexIndex < (uint32)Vertices.Num();VertexIndex++)
			{
				FSoftSkinBuildVertex&	OtherVertex = Vertices[VertexIndex];

				if(!PointsEqual(OtherVertex.Position,Vertex.Position))
					continue;

				bool bUVsEqual = true;
				for( int32 UVIdx = 0; UVIdx < MAX_TEXCOORDS; ++UVIdx )
				{
					if(FMath::Abs(Vertex.UVs[UVIdx].X - OtherVertex.UVs[UVIdx].X) > (1.0f / 1024.0f))
					{
						bUVsEqual = false;
					};

					if(FMath::Abs(Vertex.UVs[UVIdx].Y - OtherVertex.UVs[UVIdx].Y) > (1.0f / 1024.0f))
					{
						bUVsEqual = false;
					}
				}

				if( !bUVsEqual )
					continue;

				if(!NormalsEqual( OtherVertex.TangentX, Vertex.TangentX))
					continue;

				if(!NormalsEqual(OtherVertex.TangentY, Vertex.TangentY))
					continue;

				if(!NormalsEqual(OtherVertex.TangentZ, Vertex.TangentZ))
					continue;

				bool	InfluencesMatch = 1;
				for(uint32 InfluenceIndex = 0;InfluenceIndex < MAX_TOTAL_INFLUENCES;InfluenceIndex++)
				{
					if( Vertex.InfluenceBones[InfluenceIndex] != OtherVertex.InfluenceBones[InfluenceIndex] || 
						Vertex.InfluenceWeights[InfluenceIndex] != OtherVertex.InfluenceWeights[InfluenceIndex])
					{
						InfluencesMatch = 0;
						break;
					}
				}
				if(!InfluencesMatch)
					continue;

				return VertexIndex;
			}
		}

		return Vertices.Add(Vertex);
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
	void CopySkinnedModelData(FSkinnedModelData& OutData, FStaticLODModel& Model)
	{
	#if WITH_EDITORONLY_DATA
		Model.GetVertices(OutData.Vertices);
		Model.MultiSizeIndexContainer.GetIndexBuffer(OutData.Indices);
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
		for (int32 ChunkIndex = 0; ChunkIndex < Model.Chunks.Num(); ++ChunkIndex)
		{
			TArray<FBoneIndexType>& DestBoneMap = *new(OutData.BoneMaps) TArray<FBoneIndexType>();
			DestBoneMap = Model.Chunks[ChunkIndex].BoneMap;
		}
		OutData.NumTexCoords = Model.NumTexCoords;
	#endif // #if WITH_EDITORONLY_DATA
	};

	void UnchunkSkeletalModel(TArray<FSkinnedMeshChunk*>& Chunks, TArray<int32>& PointToOriginalMap, const FSkinnedModelData& SrcModel)
	{
	#if WITH_EDITORONLY_DATA
		const TArray<FSoftSkinVertex>& SrcVertices = SrcModel.Vertices;
		const TArray<uint32>& SrcIndices = SrcModel.Indices;
		TArray<uint32> IndexMap;

		check(Chunks.Num() == 0);
		check(PointToOriginalMap.Num() == 0);

		IndexMap.Empty(SrcVertices.Num());
		IndexMap.AddUninitialized(SrcVertices.Num());
		for (int32 SectionIndex = 0; SectionIndex < SrcModel.Sections.Num(); ++SectionIndex)
		{
			const FSkelMeshSection& Section = SrcModel.Sections[SectionIndex];
			const TArray<FBoneIndexType>& BoneMap = SrcModel.BoneMaps[SectionIndex];
			FSkinnedMeshChunk* DestChunk = Chunks.Num() ? Chunks.Last() : NULL;

			if (DestChunk == NULL || DestChunk->MaterialIndex != Section.MaterialIndex)
			{
				DestChunk = new FSkinnedMeshChunk();
				Chunks.Add(DestChunk);
				DestChunk->MaterialIndex = Section.MaterialIndex;
				DestChunk->OriginalSectionIndex = SectionIndex;

				// When starting a new chunk reset the index map.
				FMemory::Memset(IndexMap.GetData(),0xff,IndexMap.Num()*IndexMap.GetTypeSize());
			}

			int32 NumIndicesThisSection = Section.NumTriangles * 3;
			for (uint32 SrcIndex = Section.BaseIndex; SrcIndex < Section.BaseIndex + NumIndicesThisSection; ++SrcIndex)
			{
				uint32 VertexIndex = SrcIndices[SrcIndex];
				uint32 DestVertexIndex = IndexMap[VertexIndex];
				if (DestVertexIndex == INDEX_NONE)
				{
					FSoftSkinBuildVertex NewVertex;
					const FSoftSkinVertex& SrcVertex = SrcVertices[VertexIndex];
					NewVertex.Position = SrcVertex.Position;
					NewVertex.TangentX = SrcVertex.TangentX;
					NewVertex.TangentY = SrcVertex.TangentY;
					NewVertex.TangentZ = SrcVertex.TangentZ;
					FMemory::Memcpy(NewVertex.UVs, SrcVertex.UVs, sizeof(FVector2D)*MAX_TEXCOORDS);
					NewVertex.Color = SrcVertex.Color;
					for (int32 i = 0; i < MAX_TOTAL_INFLUENCES; ++i)
					{
						uint8 BoneIndex = SrcVertex.InfluenceBones[i];
						check(BoneMap.IsValidIndex(BoneIndex));
						NewVertex.InfluenceBones[i] = BoneMap[BoneIndex];
						NewVertex.InfluenceWeights[i] = SrcVertex.InfluenceWeights[i];
					}
					NewVertex.PointWedgeIdx = SrcModel.RawPointIndices.Num() ? SrcModel.RawPointIndices[VertexIndex] : 0;
					int32 RawVertIndex = SrcModel.MeshToImportVertexMap.Num() ? SrcModel.MeshToImportVertexMap[VertexIndex] : INDEX_NONE;
					if ((int32)NewVertex.PointWedgeIdx >= PointToOriginalMap.Num())
					{
						PointToOriginalMap.AddZeroed(NewVertex.PointWedgeIdx + 1 - PointToOriginalMap.Num());
					}
					PointToOriginalMap[NewVertex.PointWedgeIdx] = RawVertIndex;				
					DestVertexIndex = AddSkinVertex(DestChunk->Vertices,NewVertex,/*bKeepOverlappingVertices=*/ false);
					IndexMap[VertexIndex] = DestVertexIndex;
				}
				DestChunk->Indices.Add(DestVertexIndex);
			}
		}
	#endif // #if WITH_EDITORONLY_DATA
	}

	
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
		FSkeletalMeshResource* ImportedResource = SkeletalMesh->GetImportedResource();
		if (ImportedResource->LODModels.Num() == 0)
			return;

		SkeletalMesh->CalculateInvRefMatrices();
		check( SkeletalMesh->RefSkeleton.GetNum() == SkeletalMesh->RefBasesInvMatrix.Num() );

		Infos.Empty();
		Infos.AddZeroed( SkeletalMesh->RefSkeleton.GetNum() );

		FStaticLODModel* LODModel = &ImportedResource->LODModels[0];
		for(int32 ChunkIndex = 0;ChunkIndex < LODModel->Chunks.Num();ChunkIndex++)
		{
			FSkelMeshChunk& Chunk = LODModel->Chunks[ChunkIndex];
			for(int32 i=0; i<Chunk.RigidVertices.Num(); i++)
			{
				FRigidSkinVertex* RigidVert = &Chunk.RigidVertices[i];
				int32 BoneIndex = Chunk.BoneMap[RigidVert->Bone];

				FVector LocalPos = SkeletalMesh->RefBasesInvMatrix[BoneIndex].TransformPosition(RigidVert->Position);
				Infos[BoneIndex].Positions.Add(LocalPos);

				FVector LocalNormal = SkeletalMesh->RefBasesInvMatrix[BoneIndex].TransformVector(RigidVert->TangentZ);
				Infos[BoneIndex].Normals.Add(LocalNormal);
			}

			for(int32 i=0; i<Chunk.SoftVertices.Num(); i++)
			{
				FSoftSkinVertex* SoftVert = &Chunk.SoftVertices[i];

				if(bOnlyDominant)
				{
					int32 BoneIndex = Chunk.BoneMap[GetDominantBoneIndex(SoftVert)];

					FVector LocalPos = SkeletalMesh->RefBasesInvMatrix[BoneIndex].TransformPosition(SoftVert->Position);
					Infos[BoneIndex].Positions.Add(LocalPos);

					FVector LocalNormal = SkeletalMesh->RefBasesInvMatrix[BoneIndex].TransformVector(SoftVert->TangentZ);
					Infos[BoneIndex].Normals.Add(LocalNormal);
				}
				else
				{
					for(int32 j=0; j<MAX_TOTAL_INFLUENCES; j++)
					{
						if(SoftVert->InfluenceWeights[j] > 0.01f)
						{
							int32 BoneIndex = Chunk.BoneMap[SoftVert->InfluenceBones[j]];

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

