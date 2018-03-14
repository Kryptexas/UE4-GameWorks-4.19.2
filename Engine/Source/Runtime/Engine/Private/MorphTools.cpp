// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MorphTools.cpp: Morph target creation helper classes.
=============================================================================*/ 

#include "CoreMinimal.h"
#include "RawIndexBuffer.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/MorphTarget.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"

/** compare based on base mesh source vertex indices */
struct FCompareMorphTargetDeltas
{
	FORCEINLINE bool operator()( const FMorphTargetDelta& A, const FMorphTargetDelta& B ) const
	{
		return ((int32)A.SourceIdx - (int32)B.SourceIdx) < 0 ? true : false;
	}
};

FMorphTargetDelta* UMorphTarget::GetMorphTargetDelta(int32 LODIndex, int32& OutNumDeltas)
{
	if(LODIndex < MorphLODModels.Num())
	{
		FMorphTargetLODModel& MorphModel = MorphLODModels[LODIndex];
		OutNumDeltas = MorphModel.Vertices.Num();
		return MorphModel.Vertices.GetData();
	}

	return NULL;
}

bool UMorphTarget::HasDataForLOD(int32 LODIndex) 
{
	// If we have an entry for this LOD, and it has verts
	return (MorphLODModels.IsValidIndex(LODIndex) && MorphLODModels[LODIndex].Vertices.Num() > 0);
}

bool UMorphTarget::HasValidData() const
{
	for (const FMorphTargetLODModel& Model : MorphLODModels)
	{
		if (Model.Vertices.Num() > 0)
		{
			return true;
		}
	}

	return false;
}

#if WITH_EDITOR

void UMorphTarget::PopulateDeltas(const TArray<FMorphTargetDelta>& Deltas, const int32 LODIndex, const TArray<FSkelMeshSection>& Sections, const bool bCompareNormal)
{
	// create the LOD entry if it doesn't already exist
	if (LODIndex >= MorphLODModels.Num())
	{
		MorphLODModels.AddDefaulted(LODIndex - MorphLODModels.Num() + 1);
	}

	// morph mesh data to modify
	FMorphTargetLODModel& MorphModel = MorphLODModels[LODIndex];
	// copy the wedge point indices
	// for now just keep every thing 

	// set the original number of vertices
	MorphModel.NumBaseMeshVerts = Deltas.Num();

	// empty morph mesh vertices first
	MorphModel.Vertices.Empty(Deltas.Num());

	// Still keep this (could remove in long term due to incoming data)
	for (const FMorphTargetDelta& Delta : Deltas)
	{
		if (Delta.PositionDelta.SizeSquared() > FMath::Square(THRESH_POINTS_ARE_NEAR) || 
			( bCompareNormal && Delta.TangentZDelta.SizeSquared() > 0.01f))
		{
			MorphModel.Vertices.Add(Delta);
			for (int32 SectionIdx = 0; SectionIdx < Sections.Num(); ++SectionIdx)
			{
				if (MorphModel.SectionIndices.Contains(SectionIdx))
				{
					continue;
				}
				const uint32 BaseVertexBufferIndex = (uint32)(Sections[SectionIdx].GetVertexBufferIndex());
				const uint32 LastVertexBufferIndex = (uint32)(BaseVertexBufferIndex + Sections[SectionIdx].GetNumVertices());
				if (BaseVertexBufferIndex <= Delta.SourceIdx && Delta.SourceIdx < LastVertexBufferIndex)
				{
					MorphModel.SectionIndices.AddUnique(SectionIdx);
					break;
				}
			}
		}
	}

	// sort the array of vertices for this morph target based on the base mesh indices
	// that each vertex is associated with. This allows us to sequentially traverse the list
	// when applying the morph blends to each vertex.
	MorphModel.Vertices.Sort(FCompareMorphTargetDeltas());

	// remove array slack
	MorphModel.Vertices.Shrink();
}

#endif // WITH_EDITOR


