// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Rendering/MorphTargetVertexInfoBuffers.h"


void FMorphTargetVertexInfoBuffers::InitRHI()
{
	check(NumTotalWorkItems > 0);

	{
		FRHIResourceCreateInfo CreateInfo;
		void* VertexIndicesVBData = nullptr;
		VertexIndicesVB = RHICreateAndLockVertexBuffer(VertexIndices.GetAllocatedSize(), BUF_Static | BUF_ShaderResource, CreateInfo, VertexIndicesVBData);
		FMemory::Memcpy(VertexIndicesVBData, VertexIndices.GetData(), VertexIndices.GetAllocatedSize());
		RHIUnlockVertexBuffer(VertexIndicesVB);
		VertexIndicesSRV = RHICreateShaderResourceView(VertexIndicesVB, 4, PF_R32_UINT);
	}
	{
		FRHIResourceCreateInfo CreateInfo;
		void* MorphDeltasVBData = nullptr;
		MorphDeltasVB = RHICreateAndLockVertexBuffer(MorphDeltas.GetAllocatedSize(), BUF_Static | BUF_ShaderResource, CreateInfo, MorphDeltasVBData);
		FMemory::Memcpy(MorphDeltasVBData, MorphDeltas.GetData(), MorphDeltas.GetAllocatedSize());
		RHIUnlockVertexBuffer(MorphDeltasVB);
		MorphDeltasSRV = RHICreateShaderResourceView(MorphDeltasVB, 2, PF_R16F);
	}
	{
		FRHIResourceCreateInfo CreateInfo;
		void* MorphPermutationsVBData = nullptr;
		MorphPermutationsVB = RHICreateAndLockVertexBuffer(MorphPermutations.GetAllocatedSize(), BUF_Static | BUF_ShaderResource, CreateInfo, MorphPermutationsVBData);
		FMemory::Memcpy(MorphPermutationsVBData, MorphPermutations.GetData(), MorphPermutations.GetAllocatedSize());
		RHIUnlockVertexBuffer(MorphPermutationsVB);
		MorphPermutationsSRV = RHICreateShaderResourceView(MorphPermutationsVB, 4, PF_R32_UINT);
	}
	MorphPermutations.Empty();
	VertexIndices.Empty();
	MorphDeltas.Empty();
}

void FMorphTargetVertexInfoBuffers::ReleaseRHI()
{
	VertexIndicesVB.SafeRelease();
	VertexIndicesSRV.SafeRelease();
	MorphDeltasVB.SafeRelease();
	MorphDeltasSRV.SafeRelease();
}