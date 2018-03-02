// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "RenderResource.h"


class FMorphTargetVertexInfoBuffers : public FRenderResource
{
public:
	FMorphTargetVertexInfoBuffers() : NumTotalWorkItems(0)
	{
	}

	ENGINE_API virtual void InitRHI() override;
	ENGINE_API virtual void ReleaseRHI() override;

	uint32 GetNumWorkItems(uint32 index = UINT_MAX) const
	{
		check(index == UINT_MAX || index < (uint32)WorkItemsPerMorph.Num());
		return index != UINT_MAX ? WorkItemsPerMorph[index] : NumTotalWorkItems;
	}

	uint32 GetNumMorphs() const
	{
		return WorkItemsPerMorph.Num();
	}

	uint32 GetStartOffset(uint32 Index) const
	{
		check(Index < (uint32)StartOffsetPerMorph.Num());
		return StartOffsetPerMorph[Index];
	}

	const FVector4& GetMaximumMorphScale(uint32 Index) const
	{
		check(Index < (uint32)MaximumValuePerMorph.Num());
		return MaximumValuePerMorph[Index];
	}

	const FVector4& GetMinimumMorphScale(uint32 Index) const
	{
		check(Index < (uint32)MinimumValuePerMorph.Num());
		return MinimumValuePerMorph[Index];
	}

	uint32 GetNumPermutations() const
	{
		return PermuationStart.Num();
	}

	uint32 GetPermutationStartOffset(uint32 Index) const
	{
		check(Index < (uint32)PermuationStart.Num());
		return PermuationStart[Index];
	}

	uint32 GetPermutationSize(uint32 Index) const
	{
		check(Index < (uint32)PermuationSize.Num());
		return PermuationSize[Index];
	}

	void CalculateInverseAccumulatedWeights(const TArray<float>& MorphTargetWeights, TArray<float>& InverseAccumulatedWeights) const;

	FVertexBufferRHIRef VertexIndicesVB;
	FShaderResourceViewRHIRef VertexIndicesSRV;

	FVertexBufferRHIRef MorphDeltasVB;
	FShaderResourceViewRHIRef MorphDeltasSRV;

	FVertexBufferRHIRef MorphPermutationsVB;
	FShaderResourceViewRHIRef MorphPermutationsSRV;

	// Changes to this struct must be reflected in MorphTargets.usf
	struct FMorphDelta
	{
		FMorphDelta(FVector InPosDelta, FVector InTangentDelta)
		{
			PosDelta[0] = FFloat16(InPosDelta.X);
			PosDelta[1] = FFloat16(InPosDelta.Y);
			PosDelta[2] = FFloat16(InPosDelta.Z);

			TangentDelta[0] = FFloat16(InTangentDelta.X);
			TangentDelta[1] = FFloat16(InTangentDelta.Y);
			TangentDelta[2] = FFloat16(InTangentDelta.Z);
		}

		FFloat16 PosDelta[3];
		FFloat16 TangentDelta[3];
	};

	struct FPermuationNode
	{
		typedef uint32 LinkType;
		FPermuationNode(LinkType InDst, LinkType InOp0, LinkType InOp1) : Dst(InDst), Op0(InOp0), Op1(InOp1)
		{
		}
#if 1
		uint32 Dst : 32;
		uint32 Op0 : 32;
		uint32 Op1 : 32;
#else
		uint64 Dst : 21;
		uint64 Op0 : 21;
		uint64 Op1 : 21;
#endif
	};

	void Reset()
	{
		VertexIndices.Empty();
		MorphDeltas.Empty();
		MorphPermutations.Empty();
		MaximumValuePerMorph.Empty();
		MinimumValuePerMorph.Empty();
		StartOffsetPerMorph.Empty();
		WorkItemsPerMorph.Empty();
		PermuationStart.Empty();
		PermuationSize.Empty();
		AccumStrategyRules.Empty();
		AccumStrategyRules.Empty();
		TempStoreSize = 0;
		NumTotalWorkItems = 0;
	}

protected:

	// Transient data used while creating the vertex buffers, gets deleted as soon as VB gets initialized.
	TArray<uint32> VertexIndices;
	// Transient data used while creating the vertex buffers, gets deleted as soon as VB gets initialized.
	TArray<FMorphDelta> MorphDeltas;
	// Transient data used while creating the vertex buffers, gets deleted as soon as VB gets initialized.
	TArray<uint32> MorphPermutations;

	//x,y,y separate for position and shared w for tangent
	TArray<FVector4> MaximumValuePerMorph;
	TArray<FVector4> MinimumValuePerMorph;
	TArray<uint32> StartOffsetPerMorph;
	TArray<uint32> WorkItemsPerMorph;
	TArray<uint32> PermuationStart;
	TArray<uint32> PermuationSize;
	TArray<FPermuationNode> AccumStrategyRules;
	uint32 TempStoreSize;

	uint32 NumTotalWorkItems;

	friend class FSkeletalMeshLODRenderData;
};
