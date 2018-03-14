// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Copyright (C) Microsoft. All rights reserved.

/*=============================================================================
	GPUSkinCache.h: Performs skinning on a compute shader into a buffer to avoid vertex buffer skinning.
=============================================================================*/

// Requirements
// * Compute shader support (with Atomics)
// * Project settings needs to be enabled (r.SkinCache.CompileShaders)
// * feature need to be enabled (r.SkinCache.Mode)

// Features
// * Skeletal mesh, 4 / 8 weights per vertex, 16/32 index buffer
// * Supports Morph target animation (morph target blending is not done by this code)
// * Saves vertex shader computations when we render an object multiple times (EarlyZ, velocity, shadow, BasePass, CustomDepth, Shadow masking)
// * Fixes velocity rendering (needed for MotionBlur and TemporalAA) for WorldPosOffset animation and morph target animation
// * RecomputeTangents results in improved tangent space for WorldPosOffset animation and morph target animation
// * fixed amount of memory per Scene (r.SkinCache.SceneMemoryLimitInMB)
// * Velocity Rendering for MotionBlur and TemporalAA (test Velocity in BasePass)
// * r.SkinCache.Mode and r.SkinCache.RecomputeTangents can be toggled at runtime

// TODO:
// * Test: Tessellation
// * Quality/Optimization: increase TANGENT_RANGE for better quality or accumulate two components in one 32bit value
// * Bug: UpdateMorphVertexBuffer needs to handle SkinCacheObjects that have been rejected by the SkinCache (e.g. because it was running out of memory)
// * Refactor: Unify the 3 compute shaders to use the same C++ setup code for the variables
// * Optimization: Dispatch calls can be merged for better performance, stalls between Dispatch calls can be avoided (DX11 back door, DX12, console API)
// * Feature: Cloth is not supported yet (Morph targets is a similar code)
// * Feature: Support Static Meshes ?

#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "HAL/IConsoleManager.h"
#include "RHI.h"
#include "RenderResource.h"
#include "ShaderParameters.h"
#include "UniformBuffer.h"
#include "GPUSkinPublicDefs.h"


class FGPUSkinPassthroughVertexFactory;
class FGPUBaseSkinVertexFactory;
class FMorphVertexBuffer;
class FSkeletalMeshObjectGPUSkin;
class FSkeletalMeshVertexClothBuffer;
struct FClothSimulData;
struct FSkelMeshRenderSection;
struct FVertexBufferAndSRV;

// Can the skin cache be used (ie shaders added, etc)
extern ENGINE_API bool IsGPUSkinCacheAvailable();

// Is it actually enabled?
extern ENGINE_API int32 GEnableGPUSkinCache;

class FGPUSkinCacheEntry;

struct FClothSimulEntry
{
	FVector Position;
	FVector Normal;

	/**
	 * Serializer
	 *
	 * @param Ar - archive to serialize with
	 * @param V - vertex to serialize
	 * @return archive that was used
	 */
	friend FArchive& operator<<(FArchive& Ar, FClothSimulEntry& V)
	{
		Ar << V.Position
		   << V.Normal;
		return Ar;
	}
};

struct FGPUSkinBatchElementUserData
{
	FGPUSkinCacheEntry* Entry;
	int32 Section;
};

class FGPUSkinCache
{
public:
	struct FRWBufferTracker;

	enum ESkinCacheInitSettings
	{
		// max 256 bones as we use a byte to index
		MaxUniformBufferBones = 256,
		// Controls the output format on GpuSkinCacheComputeShader.usf
		RWTangentXOffsetInFloats = 0,	// Packed U8x4N
		RWTangentZOffsetInFloats = 1,	// Packed U8x4N

		// 3 ints for normal, 3 ints for tangent, 1 for orientation = 7, rounded up to 8 as it should result in faster math and caching
		IntermediateAccumBufferNumInts = 8,
	};
	ENGINE_API FGPUSkinCache(bool bInRequiresMemoryLimit);
	ENGINE_API ~FGPUSkinCache();

	void ProcessEntry(FRHICommandListImmediate& RHICmdList, FGPUBaseSkinVertexFactory* VertexFactory,
		FGPUSkinPassthroughVertexFactory* TargetVertexFactory, const FSkelMeshRenderSection& BatchElement, FSkeletalMeshObjectGPUSkin* Skin,
		const FMorphVertexBuffer* MorphVertexBuffer, const FSkeletalMeshVertexClothBuffer* ClothVertexBuffer, const FClothSimulData* SimData,
		const FMatrix& ClothLocalToWorld, float ClothBlendWeight, uint32 RevisionNumber, int32 Section, FGPUSkinCacheEntry*& InOutEntry);

	static void SetVertexStreams(FGPUSkinCacheEntry* Entry, int32 Section, FRHICommandList& RHICmdList,
		class FShader* Shader, const FGPUSkinPassthroughVertexFactory* VertexFactory,
		uint32 BaseVertexIndex, FShaderResourceParameter PreviousStreamBuffer);

	static void Release(FGPUSkinCacheEntry*& SkinCacheEntry);

	static inline FGPUSkinBatchElementUserData* GetFactoryUserData(FGPUSkinCacheEntry* Entry, int32 Section)
	{
		if (Entry)
		{
			return InternalGetFactoryUserData(Entry, Section);
		}
		return nullptr;
	}

	static bool IsEntryValid(FGPUSkinCacheEntry* SkinCacheEntry, int32 Section);

	inline uint64 GetExtraRequiredMemoryAndReset()
	{
		uint64 OriginalValue = ExtraRequiredMemory;
		ExtraRequiredMemory = 0;
		return OriginalValue;
	}

	enum
	{
		NUM_BUFFERS = 2,
	};

	struct FRWBuffersAllocation
	{
		friend struct FRWBufferTracker;

		FRWBuffersAllocation(uint32 InNumVertices, bool InWithTangents)
			: NumVertices(InNumVertices), WithTangents(InWithTangents)
		{
			for (int32 Index = 0; Index < NUM_BUFFERS; ++Index)
			{
				RWBuffers[Index].Initialize(4, NumVertices * 3, PF_R32_FLOAT, BUF_Static);
			}
			if (WithTangents)
			{
				Tangents.Initialize(4, NumVertices * 2, PF_R8G8B8A8, BUF_Static);
			}
		}

		~FRWBuffersAllocation()
		{
			for (int32 Index = 0; Index < NUM_BUFFERS; ++Index)
			{
				RWBuffers[Index].Release();
			}
			if (WithTangents)
			{
				Tangents.Release();
			}
		}

		static uint64 CalculateRequiredMemory(uint32 NumVertices, bool WithTangents)
		{
			uint64 PositionBufferSize = 4 * 3 * NumVertices * NUM_BUFFERS;
			uint64 TangentBufferSize = WithTangents ? 2 * 4 * NumVertices : 0;
			return TangentBufferSize + PositionBufferSize;
		}

		uint64 GetNumBytes() const
		{
			return CalculateRequiredMemory(NumVertices, WithTangents);
		}

		FRWBuffer* GetTangentBuffer()
		{
			return WithTangents ? &Tangents : nullptr;
		}

		void RemoveAllFromTransitionArray(TArray<FUnorderedAccessViewRHIParamRef>& BuffersToTransition);

	private:
		// Output of the GPU skinning (ie Pos, Normals)
		FRWBuffer RWBuffers[NUM_BUFFERS];

		FRWBuffer Tangents;
		const uint32 NumVertices;
		const bool WithTangents;
	};

	struct FRWBufferTracker
	{
		FRWBuffersAllocation* Allocation;

		FRWBufferTracker()
			: Allocation(nullptr)
		{
			Reset();
		}

		void Reset()
		{
			for (int32 Index = 0; Index < NUM_BUFFERS; ++Index)
			{
				Revisions[Index] = 0;
				BoneBuffers[Index] = nullptr;
			}
		}

		inline uint32 GetNumBytes() const
		{
			return Allocation->GetNumBytes();
		}

		FRWBuffer* Find(const FVertexBufferAndSRV& BoneBuffer, uint32 Revision)
		{
			for (int32 Index = 0; Index < NUM_BUFFERS; ++Index)
			{
				if (Revisions[Index] == Revision && BoneBuffers[Index] == &BoneBuffer)
				{
					return &Allocation->RWBuffers[Index];
				}
			}

			return nullptr;
		}

		FRWBuffer* GetTangentBuffer()
		{
			return Allocation->GetTangentBuffer();
		}

		void Advance(const FVertexBufferAndSRV& BoneBuffer1, uint32 Revision1, const FVertexBufferAndSRV& BoneBuffer2, uint32 Revision2)
		{
			const FVertexBufferAndSRV* InBoneBuffers[2] = { &BoneBuffer1 , &BoneBuffer2 };
			uint32 InRevisions[2] = { Revision1 , Revision2 };

			for (int32 Index = 0; Index < NUM_BUFFERS; ++Index)
			{
				bool Needed = false;
				for (int32 i = 0; i < 2; ++i)
				{
					if (Revisions[Index] == InRevisions[i] && BoneBuffers[Index] == InBoneBuffers[i])
					{
						Needed = true;
					}
				}

				if (!Needed)
				{
					Revisions[Index] = Revision1;
					BoneBuffers[Index] = &BoneBuffer1;
					break;
				}
			}
		}

	private:
		uint32 Revisions[NUM_BUFFERS];
		const FVertexBufferAndSRV* BoneBuffers[NUM_BUFFERS];
	};

	ENGINE_API void TransitionAllToReadable(FRHICommandList& RHICmdList);

protected:
	TArray<FUnorderedAccessViewRHIParamRef> BuffersToTransition;

	TArray<FRWBuffersAllocation*> Allocations;
	TArray<FGPUSkinCacheEntry*> Entries;
	FRWBuffersAllocation* TryAllocBuffer(uint32 NumVertices, bool WithTangnents);
	void DoDispatch(FRHICommandListImmediate& RHICmdList, FGPUSkinCacheEntry* SkinCacheEntry, int32 Section, int32 RevisionNumber);
	void DispatchUpdateSkinTangents(FRHICommandListImmediate& RHICmdList, FGPUSkinCacheEntry* Entry, int32 SectionIndex);
	void DispatchUpdateSkinning(FRHICommandListImmediate& RHICmdList, FGPUSkinCacheEntry* Entry, int32 Section, uint32 RevisionNumber);

	void Cleanup();

	static void ReleaseSkinCacheEntry(FGPUSkinCacheEntry* SkinCacheEntry);
	static FGPUSkinBatchElementUserData* InternalGetFactoryUserData(FGPUSkinCacheEntry* Entry, int32 Section);
	void InvalidateAllEntries();
	uint64 UsedMemoryInBytes;
	uint64 ExtraRequiredMemory;
	int32 FlushCounter;
	bool bRequiresMemoryLimit;

	// For recompute tangents, holds the data required between compute shaders
	TArray<FRWBuffer> StagingBuffers;
	int32 CurrentStagingBufferIndex;

	static void CVarSinkFunction();
	static FAutoConsoleVariableSink CVarSink;
};

DECLARE_STATS_GROUP(TEXT("GPU Skin Cache"), STATGROUP_GPUSkinCache, STATCAT_Advanced);
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Num Sections Skinned"), STAT_GPUSkinCache_TotalNumChunks, STATGROUP_GPUSkinCache,);
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Num Vertices Skinned"), STAT_GPUSkinCache_TotalNumVertices, STATGROUP_GPUSkinCache,);
DECLARE_MEMORY_STAT_EXTERN(TEXT("Total Memory Bytes Used"), STAT_GPUSkinCache_TotalMemUsed, STATGROUP_GPUSkinCache, );
DECLARE_MEMORY_STAT_EXTERN(TEXT("Intermediate buffer for Recompute Tangents"), STAT_GPUSkinCache_TangentsIntermediateMemUsed, STATGROUP_GPUSkinCache, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Num Triangles for Recompute Tangents"), STAT_GPUSkinCache_NumTrianglesForRecomputeTangents, STATGROUP_GPUSkinCache, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Num Sections Processed"), STAT_GPUSkinCache_NumSectionsProcessed, STATGROUP_GPUSkinCache, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Num SetVertexStreams"), STAT_GPUSkinCache_NumSetVertexStreams, STATGROUP_GPUSkinCache, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Num PreGDME"), STAT_GPUSkinCache_NumPreGDME, STATGROUP_GPUSkinCache, );
