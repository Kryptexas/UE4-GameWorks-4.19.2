// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Copyright (C) Microsoft. All rights reserved.

/*=============================================================================
GPUSkinCache.cpp: Performs skinning on a compute shader into a buffer to avoid vertex buffer skinning.
=============================================================================*/

#include "GPUSkinCache.h"
#include "RawIndexBuffer.h"
#include "Shader.h"
#include "SceneUtils.h"
#include "GlobalShader.h"
#include "SkeletalRenderGPUSkin.h"
#include "ShaderParameterUtils.h"
#include "ClearQuad.h"
#include "Shader.h"

DEFINE_STAT(STAT_GPUSkinCache_TotalNumChunks);
DEFINE_STAT(STAT_GPUSkinCache_TotalNumVertices);
DEFINE_STAT(STAT_GPUSkinCache_TotalMemUsed);
DEFINE_STAT(STAT_GPUSkinCache_TangentsIntermediateMemUsed);
DEFINE_STAT(STAT_GPUSkinCache_NumTrianglesForRecomputeTangents);
DEFINE_STAT(STAT_GPUSkinCache_NumSectionsProcessed);
DEFINE_STAT(STAT_GPUSkinCache_NumSetVertexStreams);
DEFINE_STAT(STAT_GPUSkinCache_NumPreGDME);
DEFINE_LOG_CATEGORY_STATIC(LogSkinCache, Log, All);

static int32 GEnableGPUSkinCacheShaders = 0;
static FAutoConsoleVariableRef CVarEnableGPUSkinCacheShaders(
	TEXT("r.SkinCache.CompileShaders"),
	GEnableGPUSkinCacheShaders,
	TEXT("Whether or not to compile the GPU compute skinning cache shaders.\n")
	TEXT("This will compile the shaders for skinning on a compute job and not skin on the vertex shader.\n")
	TEXT("GPUSkinVertexFactory.usf needs to be touched to cause a recompile if this changes.\n")
	TEXT("0 is off(default), 1 is on"),
	ECVF_RenderThreadSafe | ECVF_ReadOnly
);

// 0/1
int32 GEnableGPUSkinCache = 1;
static TAutoConsoleVariable<int32> CVarEnableGPUSkinCache(
	TEXT("r.SkinCache.Mode"),
	1,
	TEXT("Whether or not to use the GPU compute skinning cache.\n")
	TEXT("This will perform skinning on a compute job and not skin on the vertex shader.\n")
	TEXT("Requires r.SkinCache.CompileShaders=1\n")
	TEXT(" 0: off\n")
	TEXT(" 1: on(default)\n")
	TEXT(" 2: only use skin cache for skinned meshes that ticked the Recompute Tangents checkbox (unavailable in shipping builds)"),
	ECVF_RenderThreadSafe
);

int32 GSkinCacheRecomputeTangents = 2;
TAutoConsoleVariable<int32> CVarGPUSkinCacheRecomputeTangents(
	TEXT("r.SkinCache.RecomputeTangents"),
	2,
	TEXT("This option enables recomputing the vertex tangents on the GPU.\n")
	TEXT("Can be changed at runtime, requires both r.SkinCache.CompileShaders=1 and r.SkinCache.Mode=1\n")
	TEXT(" 0: off\n")
	TEXT(" 1: on, forces all skinned object to Recompute Tangents\n")
	TEXT(" 2: on, only recompute tangents on skinned objects who ticked the Recompute Tangents checkbox(default)\n"),
	ECVF_RenderThreadSafe
);

static int32 GForceRecomputeTangents = 0;
FAutoConsoleVariableRef CVarGPUSkinCacheForceRecomputeTangents(
	TEXT("r.SkinCache.ForceRecomputeTangents"),
	GForceRecomputeTangents,
	TEXT("0: off (default)\n")
	TEXT("1: Forces enabling and using the skincache and forces all skinned object to Recompute Tangents\n"),
	ECVF_RenderThreadSafe | ECVF_ReadOnly
);

static int32 GNumTangentIntermediateBuffers = 1;
static TAutoConsoleVariable<float> CVarGPUSkinNumTangentIntermediateBuffers(
	TEXT("r.SkinCache.NumTangentIntermediateBuffers"),
	1,
	TEXT("How many intermediate buffers to use for intermediate results while\n")
	TEXT("doing Recompute Tangents; more may allow the GPU to overlap compute jobs."),
	ECVF_RenderThreadSafe
);

static TAutoConsoleVariable<float> CVarGPUSkinCacheDebug(
	TEXT("r.SkinCache.Debug"),
	1.0f,
	TEXT("A scaling constant passed to the SkinCache shader, useful for debugging"),
	ECVF_RenderThreadSafe
);

static float GSkinCacheSceneMemoryLimitInMB = 128.0f;
static TAutoConsoleVariable<float> CVarGPUSkinCacheSceneMemoryLimitInMB(
	TEXT("r.SkinCache.SceneMemoryLimitInMB"),
	128.0f,
	TEXT("Maximum memory allowed to be allocated per World/Scene in Megs"),
	ECVF_RenderThreadSafe
);

////temporary disable until resource lifetimes are safe for all cases
static int32 GAllowDupedVertsForRecomputeTangents = 0;
FAutoConsoleVariableRef CVarGPUSkinCacheAllowDupedVertesForRecomputeTangents(
	TEXT("r.SkinCache.AllowDupedVertsForRecomputeTangents"),
	GAllowDupedVertsForRecomputeTangents,
	TEXT("0: off (default)\n")
	TEXT("1: Forces that vertices at the same position will be treated differently and has the potential to cause seams when verts are split.\n"),
	ECVF_RenderThreadSafe
);

static int32 GGPUSkinCacheFlushCounter = 0;

bool IsGPUSkinCacheAvailable()
{
	return GEnableGPUSkinCacheShaders != 0 || GForceRecomputeTangents != 0;
}

static inline bool DoesPlatformSupportGPUSkinCache(EShaderPlatform Platform)
{
	return Platform == SP_PCD3D_SM5 || Platform == SP_METAL_SM5 || Platform == SP_METAL_SM5_NOTESS || Platform == SP_METAL_MRT_MAC || Platform == SP_METAL_MRT || Platform == SP_VULKAN_SM5 || Platform == SP_OPENGL_SM5;
}

// We don't have it always enabled as it's not clear if this has a performance cost
// Call on render thread only!
// Should only be called if SM5 (compute shaders, atomics) are supported.
ENGINE_API bool DoSkeletalMeshIndexBuffersNeedSRV()
{
	// currently only implemented and tested on Window SM5 (needs Compute, Atomics, SRV for index buffers, UAV for VertexBuffers)
	//#todo-gpuskin: Enable on PS4 when SRVs for IB exist
	return DoesPlatformSupportGPUSkinCache(GMaxRHIShaderPlatform) && IsGPUSkinCacheAvailable();
}

ENGINE_API bool DoRecomputeSkinTangentsOnGPU_RT()
{
	// currently only implemented and tested on Window SM5 (needs Compute, Atomics, SRV for index buffers, UAV for VertexBuffers)
	//#todo-gpuskin: Enable on PS4 when SRVs for IB exist
	return DoesPlatformSupportGPUSkinCache(GMaxRHIShaderPlatform) && GEnableGPUSkinCacheShaders != 0 && ((GEnableGPUSkinCache && GSkinCacheRecomputeTangents != 0) || GForceRecomputeTangents != 0);
}


class FGPUSkinCacheEntry
{
public:
	FGPUSkinCacheEntry(FGPUSkinCache* InSkinCache, FSkeletalMeshObjectGPUSkin* InGPUSkin, FGPUSkinCache::FRWBuffersAllocation* InPositionAllocation)
		: PositionAllocation(InPositionAllocation)
		, SkinCache(InSkinCache)
		, GPUSkin(InGPUSkin)
		, MorphBuffer(0)
		, LOD(InGPUSkin->GetLOD())
	{
		
		const TArray<FSkelMeshRenderSection>& Sections = InGPUSkin->GetRenderSections(LOD);
		DispatchData.AddDefaulted(Sections.Num());
		BatchElementsUserData.AddZeroed(Sections.Num());
		for (int32 Index = 0; Index < Sections.Num(); ++Index)
		{
			BatchElementsUserData[Index].Entry = this;
			BatchElementsUserData[Index].Section = Index;
		}

		FSkinWeightVertexBuffer* WeightBuffer = GPUSkin->GetSkinWeightVertexBuffer(LOD);
		InputWeightStride = WeightBuffer->GetStride();
		InputWeightStreamSRV = WeightBuffer->GetSRV();
	}

	~FGPUSkinCacheEntry()
	{
		check(!PositionAllocation);
	}

	struct FSectionDispatchData
	{
		FGPUSkinCache::FRWBufferTracker PositionTracker;

		FGPUBaseSkinVertexFactory* SourceVertexFactory;
		FGPUSkinPassthroughVertexFactory* TargetVertexFactory;

		// triangle index buffer (input for the RecomputeSkinTangents, might need special index buffer unique to position and normal, not considering UV/vertex color)
		FShaderResourceViewRHIParamRef IndexBuffer;

		const FSkelMeshRenderSection* Section;

		// for debugging / draw events, -1 if not set
		uint32 SectionIndex;

		// 0:normal, 1:with morph target, 2:with APEX cloth (not yet implemented)
		uint32 SkinType;
		//
		bool bExtraBoneInfluences;

		// in floats (4 bytes)
		uint32 OutputStreamStart;
		uint32 NumVertices;

		// in vertices
		uint32 InputStreamStart;

		FShaderResourceViewRHIRef TangentBufferSRV;
		FShaderResourceViewRHIRef UVsBufferSRV;
		FShaderResourceViewRHIRef PositionBufferSRV;

		// skin weight input
		uint32 InputWeightStart;

		// morph input
		uint32 MorphBufferOffset;

        // cloth input
        float ClothBlendWeight;

        FMatrix ClothLocalToWorld;
        FMatrix ClothWorldToLocal;

		// triangle index buffer (input for the RecomputeSkinTangents, might need special index buffer unique to position and normal, not considering UV/vertex color)
		uint32 IndexBufferOffsetValue;
		uint32 NumTriangles;

		FRWBuffer* TangentBuffer;
		FRWBuffer* PositionBuffer;
		FRWBuffer* PreviousPositionBuffer;

        // Handle duplicates
        FShaderResourceViewRHIRef DuplicatedIndicesIndices;
        FShaderResourceViewRHIRef DuplicatedIndices;

		FSectionDispatchData()
			: SourceVertexFactory(nullptr)
			, TargetVertexFactory(nullptr)
			, IndexBuffer(nullptr)
			, Section(nullptr)
			, SectionIndex(-1)
			, SkinType(0)
			, bExtraBoneInfluences(false)
			, OutputStreamStart(0)
			, NumVertices(0)
			, InputStreamStart(0)
			, MorphBufferOffset(0)
			, IndexBufferOffsetValue(0)
			, NumTriangles(0)
			, TangentBuffer(nullptr)
			, PositionBuffer(nullptr)
			, PreviousPositionBuffer(nullptr)
		{
		}

		inline FRWBuffer* GetPreviousPositionRWBuffer()
		{
			check(PreviousPositionBuffer);
			return PreviousPositionBuffer;
		}

		inline FRWBuffer* GetPositionRWBuffer()
		{
			check(PositionBuffer);
			return PositionBuffer;
		}

		inline FRWBuffer* GetTangentRWBuffer()
		{
			return TangentBuffer;
		}

		void UpdateVertexFactoryDeclaration()
		{
			TargetVertexFactory->UpdateVertexDeclaration(SourceVertexFactory, GetPositionRWBuffer(), GetTangentRWBuffer());
		}
	};

	void UpdateVertexFactoryDeclaration(int32 Section)
	{
		DispatchData[Section].UpdateVertexFactoryDeclaration();
	}

	bool IsSectionValid(int32 Section) const
	{
		const FSectionDispatchData& SectionData = DispatchData[Section];
		return SectionData.SectionIndex == Section;
	}

	bool IsSourceFactoryValid(int32 Section, FGPUBaseSkinVertexFactory* SourceVertexFactory) const
	{
		const FSectionDispatchData& SectionData = DispatchData[Section];
		return SectionData.SourceVertexFactory == SourceVertexFactory;
	}

	bool IsValid(FSkeletalMeshObjectGPUSkin* InSkin) const
	{
		return GPUSkin == InSkin && GPUSkin->GetLOD() == LOD;
	}

	void SetupSection(int32 SectionIndex, FGPUSkinCache::FRWBuffersAllocation* InPositionAllocation, FSkelMeshRenderSection* Section, const FMorphVertexBuffer* MorphVertexBuffer, const FSkeletalMeshVertexClothBuffer* ClothVertexBuffer,
		uint32 NumVertices, uint32 InputStreamStart, FGPUBaseSkinVertexFactory* InSourceVertexFactory, FGPUSkinPassthroughVertexFactory* InTargetVertexFactory)
	{
		//UE_LOG(LogSkinCache, Warning, TEXT("*** SetupSection E %p Alloc %p Sec %d(%p) LOD %d"), this, InAllocation, SectionIndex, Section, LOD);
		FSectionDispatchData& Data = DispatchData[SectionIndex];
		check(!Data.PositionTracker.Allocation || Data.PositionTracker.Allocation == InPositionAllocation);

		Data.PositionTracker.Allocation = InPositionAllocation;

		Data.SectionIndex = SectionIndex;
		Data.Section = Section;

		check(GPUSkin->GetLOD() == LOD);
		FSkeletalMeshRenderData& SkelMeshRenderData = GPUSkin->GetSkeletalMeshRenderData();
		FSkeletalMeshLODRenderData& LodData = SkelMeshRenderData.LODRenderData[LOD];
		check(Data.SectionIndex == LodData.FindSectionIndex(*Section));

		Data.NumVertices = NumVertices;
		const bool bMorph = MorphVertexBuffer && MorphVertexBuffer->SectionIds.Contains(SectionIndex);
		if (bMorph)
		{
			// in bytes
			const uint32 MorphStride = sizeof(FMorphGPUSkinVertex);

			// see GPU code "check(MorphStride == sizeof(float) * 6);"
			check(MorphStride == sizeof(float) * 6);

			Data.MorphBufferOffset = Section->BaseVertexIndex;
		}

		//INC_DWORD_STAT(STAT_GPUSkinCache_TotalNumChunks);

		// SkinType 0:normal, 1:with morph target, 2:with cloth
		Data.SkinType = ClothVertexBuffer ? 2 : (bMorph ? 1 : 0);
		Data.InputStreamStart = InputStreamStart;
		Data.OutputStreamStart = Section->BaseVertexIndex;

		Data.TangentBufferSRV = InSourceVertexFactory->GetTangentsSRV();
		Data.UVsBufferSRV = InSourceVertexFactory->GetTextureCoordinatesSRV();
		Data.PositionBufferSRV = InSourceVertexFactory->GetPositionsSRV();

		Data.bExtraBoneInfluences = InSourceVertexFactory->UsesExtraBoneInfluences();
		check(Data.TangentBufferSRV && Data.PositionBufferSRV);

		// weight buffer
		Data.InputWeightStart = (InputWeightStride * Section->BaseVertexIndex) / sizeof(float);
		Data.SourceVertexFactory = InSourceVertexFactory;
		Data.TargetVertexFactory = InTargetVertexFactory;

		int32 RecomputeTangentsMode = GForceRecomputeTangents > 0 ? 1 : GSkinCacheRecomputeTangents;
		if (RecomputeTangentsMode > 0)
		{
			if (Section->bRecomputeTangent || RecomputeTangentsMode == 1)
			{
				FRawStaticIndexBuffer16or32Interface* IndexBuffer = LodData.MultiSizeIndexContainer.GetIndexBuffer();
				Data.IndexBuffer = IndexBuffer->GetSRV();
				if (Data.IndexBuffer)
				{
					Data.NumTriangles = Section->NumTriangles;
					Data.IndexBufferOffsetValue = Section->BaseIndex;
				}
			}
		}
	}

protected:
	FGPUSkinCache::FRWBuffersAllocation* PositionAllocation;
	FGPUSkinCache* SkinCache;
	TArray<FGPUSkinBatchElementUserData> BatchElementsUserData;
	TArray<FSectionDispatchData> DispatchData;
	FSkeletalMeshObjectGPUSkin* GPUSkin;
	uint32 InputWeightStride;
	FShaderResourceViewRHIRef InputWeightStreamSRV;
	FShaderResourceViewRHIParamRef MorphBuffer;
	FShaderResourceViewRHIRef ClothBuffer;
	FShaderResourceViewRHIRef ClothPositionsAndNormalsBuffer;
	int32 LOD;

	friend class FGPUSkinCache;
	friend class FBaseGPUSkinCacheCS;
	friend class FBaseRecomputeTangents;
};

class FBaseGPUSkinCacheCS : public FGlobalShader
{
public:
	FBaseGPUSkinCacheCS() {}

	FBaseGPUSkinCacheCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		//DebugParameter.Bind(Initializer.ParameterMap, TEXT("DebugParameter"));

		NumVertices.Bind(Initializer.ParameterMap, TEXT("NumVertices"));
		SkinCacheStart.Bind(Initializer.ParameterMap, TEXT("SkinCacheStart"));
		BoneMatrices.Bind(Initializer.ParameterMap, TEXT("BoneMatrices"));
		TangentInputBuffer.Bind(Initializer.ParameterMap, TEXT("TangentInputBuffer"));
		PositionInputBuffer.Bind(Initializer.ParameterMap, TEXT("PositionInputBuffer"));

		InputStreamStart.Bind(Initializer.ParameterMap, TEXT("InputStreamStart"));

		InputWeightStart.Bind(Initializer.ParameterMap, TEXT("InputWeightStart"));
		InputWeightStride.Bind(Initializer.ParameterMap, TEXT("InputWeightStride"));
		InputWeightStream.Bind(Initializer.ParameterMap, TEXT("InputWeightStream"));

		PositionBufferUAV.Bind(Initializer.ParameterMap, TEXT("PositionBufferUAV"));
		TangentBufferUAV.Bind(Initializer.ParameterMap, TEXT("TangentBufferUAV"));

		MorphBuffer.Bind(Initializer.ParameterMap, TEXT("MorphBuffer"));
		MorphBufferOffset.Bind(Initializer.ParameterMap, TEXT("MorphBufferOffset"));
		SkinCacheDebug.Bind(Initializer.ParameterMap, TEXT("SkinCacheDebug"));

		ClothBuffer.Bind(Initializer.ParameterMap, TEXT("ClothBuffer"));
		ClothPositionsAndNormalsBuffer.Bind(Initializer.ParameterMap, TEXT("ClothPositionsAndNormalsBuffer"));
		ClothBlendWeight.Bind(Initializer.ParameterMap, TEXT("ClothBlendWeight"));
		ClothLocalToWorld.Bind(Initializer.ParameterMap, TEXT("ClothLocalToWorld"));
		ClothWorldToLocal.Bind(Initializer.ParameterMap, TEXT("ClothWorldToLocal"));
	}

	void SetParameters(FRHICommandListImmediate& RHICmdList, const FVertexBufferAndSRV& BoneBuffer,
		FGPUSkinCacheEntry* Entry,
		FGPUSkinCacheEntry::FSectionDispatchData& DispatchData,
		FUnorderedAccessViewRHIParamRef PositionUAV, FUnorderedAccessViewRHIParamRef TangentUAV)
	{
		FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();

		SetShaderValue(RHICmdList, ShaderRHI, NumVertices, DispatchData.NumVertices);
		SetShaderValue(RHICmdList, ShaderRHI, InputStreamStart, DispatchData.InputStreamStart);

		check(BoneBuffer.VertexBufferSRV);
		SetSRVParameter(RHICmdList, ShaderRHI, BoneMatrices, BoneBuffer.VertexBufferSRV);

		SetSRVParameter(RHICmdList, ShaderRHI, TangentInputBuffer, DispatchData.TangentBufferSRV);
		SetSRVParameter(RHICmdList, ShaderRHI, PositionInputBuffer, DispatchData.PositionBufferSRV);

		SetShaderValue(RHICmdList, ShaderRHI, InputWeightStart, DispatchData.InputWeightStart);
		SetShaderValue(RHICmdList, ShaderRHI, InputWeightStride, Entry->InputWeightStride);
		SetSRVParameter(RHICmdList, ShaderRHI, InputWeightStream, Entry->InputWeightStreamSRV);

		// output UAV
		SetUAVParameter(RHICmdList, ShaderRHI, PositionBufferUAV, PositionUAV);
		SetUAVParameter(RHICmdList, ShaderRHI, TangentBufferUAV, TangentUAV);
		SetShaderValue(RHICmdList, ShaderRHI, SkinCacheStart, DispatchData.OutputStreamStart);

		const bool bMorph = DispatchData.SkinType == 1;
		if (bMorph)
		{
			SetSRVParameter(RHICmdList, ShaderRHI, MorphBuffer, Entry->MorphBuffer);
			SetShaderValue(RHICmdList, ShaderRHI, MorphBufferOffset, DispatchData.MorphBufferOffset);
		}

		const bool bCloth = DispatchData.SkinType == 2;
		if (bCloth)
		{
			SetSRVParameter(RHICmdList, ShaderRHI, ClothBuffer, Entry->ClothBuffer);
			SetSRVParameter(RHICmdList, ShaderRHI, ClothPositionsAndNormalsBuffer, Entry->ClothPositionsAndNormalsBuffer);
			SetShaderValue(RHICmdList, ShaderRHI, ClothBlendWeight, DispatchData.ClothBlendWeight);
			SetShaderValue(RHICmdList, ShaderRHI, ClothLocalToWorld, DispatchData.ClothLocalToWorld);
			SetShaderValue(RHICmdList, ShaderRHI, ClothWorldToLocal, DispatchData.ClothWorldToLocal);
		}

		SetShaderValue(RHICmdList, ShaderRHI, SkinCacheDebug, CVarGPUSkinCacheDebug.GetValueOnRenderThread());
	}


	void UnsetParameters(FRHICommandList& RHICmdList)
	{
		FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();

		SetUAVParameter(RHICmdList, ShaderRHI, PositionBufferUAV, 0);
		SetUAVParameter(RHICmdList, ShaderRHI, TangentBufferUAV, 0);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar  << NumVertices << InputStreamStart << SkinCacheStart
			<< TangentInputBuffer << PositionInputBuffer << PositionBufferUAV << TangentBufferUAV << BoneMatrices << MorphBuffer << MorphBufferOffset
			<< SkinCacheDebug;

		Ar << InputWeightStart << InputWeightStride << InputWeightStream;

        Ar << ClothBuffer << ClothPositionsAndNormalsBuffer << ClothBlendWeight << ClothLocalToWorld << ClothWorldToLocal;

		//Ar << DebugParameter;
		return bShaderHasOutdatedParameters;
	}

private:

	FShaderParameter NumVertices;
	FShaderParameter SkinCacheDebug;
	FShaderParameter InputStreamStart;
	FShaderParameter SkinCacheStart;

	//FShaderParameter DebugParameter;

	FShaderUniformBufferParameter SkinUniformBuffer;

	FShaderResourceParameter BoneMatrices;
	FShaderResourceParameter TangentInputBuffer;
	FShaderResourceParameter PositionInputBuffer;
	FShaderResourceParameter PositionBufferUAV;
	FShaderResourceParameter TangentBufferUAV;

	FShaderParameter InputWeightStart;
	FShaderParameter InputWeightStride;
	FShaderResourceParameter InputWeightStream;

	FShaderResourceParameter MorphBuffer;
	FShaderParameter MorphBufferOffset;

	FShaderResourceParameter ClothBuffer;
	FShaderResourceParameter ClothPositionsAndNormalsBuffer;
	FShaderParameter ClothBlendWeight;
	FShaderParameter ClothLocalToWorld;
	FShaderParameter ClothWorldToLocal;
};

/** Compute shader that skins a batch of vertices. */
// @param SkinType 0:normal, 1:with morph targets calculated outside the cache, 2: with cloth, 3:with morph target calculated insde the cache (not yet implemented)
template <int Permutation>
class TGPUSkinCacheCS : public FBaseGPUSkinCacheCS
{
    constexpr static bool bUseExtraBoneInfluencesT = (4 == (Permutation & 4));
    constexpr static bool bMorphBlend = (1 == (Permutation & 3));
	constexpr static bool bApexCloth = (2 == (Permutation & 3));

	DECLARE_SHADER_TYPE(TGPUSkinCacheCS, Global)
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsGPUSkinCacheAvailable() && IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		const uint32 UseExtraBoneInfluences = bUseExtraBoneInfluencesT;
		const uint32 MorphBlend = bMorphBlend;
		const uint32 ApexCloth = bApexCloth;
		OutEnvironment.SetDefine(TEXT("GPUSKIN_USE_EXTRA_INFLUENCES"), UseExtraBoneInfluences);
		OutEnvironment.SetDefine(TEXT("GPUSKIN_MORPH_BLEND"), MorphBlend);
		OutEnvironment.SetDefine(TEXT("GPUSKIN_APEX_CLOTH"), ApexCloth);
		OutEnvironment.SetDefine(TEXT("GPUSKIN_RWBUFFER_OFFSET_TANGENT_X"), FGPUSkinCache::RWTangentXOffsetInFloats);
		OutEnvironment.SetDefine(TEXT("GPUSKIN_RWBUFFER_OFFSET_TANGENT_Z"), FGPUSkinCache::RWTangentZOffsetInFloats);
	}

	TGPUSkinCacheCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FBaseGPUSkinCacheCS(Initializer)
	{
	}

	TGPUSkinCacheCS()
	{
	}
};

IMPLEMENT_SHADER_TYPE(template<>, TGPUSkinCacheCS<0>, TEXT("/Engine/Private/GpuSkinCacheComputeShader.usf"), TEXT("SkinCacheUpdateBatchCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(template<>, TGPUSkinCacheCS<1>, TEXT("/Engine/Private/GpuSkinCacheComputeShader.usf"), TEXT("SkinCacheUpdateBatchCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(template<>, TGPUSkinCacheCS<2>, TEXT("/Engine/Private/GpuSkinCacheComputeShader.usf"), TEXT("SkinCacheUpdateBatchCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(template<>, TGPUSkinCacheCS<4>, TEXT("/Engine/Private/GpuSkinCacheComputeShader.usf"), TEXT("SkinCacheUpdateBatchCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(template<>, TGPUSkinCacheCS<5>, TEXT("/Engine/Private/GpuSkinCacheComputeShader.usf"), TEXT("SkinCacheUpdateBatchCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(template<>, TGPUSkinCacheCS<6>, TEXT("/Engine/Private/GpuSkinCacheComputeShader.usf"), TEXT("SkinCacheUpdateBatchCS"), SF_Compute);

FGPUSkinCache::FGPUSkinCache(bool bInRequiresMemoryLimit)
	: UsedMemoryInBytes(0)
	, ExtraRequiredMemory(0)
	, FlushCounter(0)
	, bRequiresMemoryLimit(bInRequiresMemoryLimit)
	, CurrentStagingBufferIndex(0)
{
}

FGPUSkinCache::~FGPUSkinCache()
{
	Cleanup();
}

void FGPUSkinCache::Cleanup()
{
	for (int32 Index = 0; Index < StagingBuffers.Num(); ++Index)
	{
		StagingBuffers[Index].Release();
	}

	while (Entries.Num() > 0)
	{
		ReleaseSkinCacheEntry(Entries.Last());
	}
	ensure(Allocations.Num() == 0);
}

void FGPUSkinCache::TransitionAllToReadable(FRHICommandList& RHICmdList)
{
	if (BuffersToTransition.Num() > 0)
	{
		RHICmdList.TransitionResources(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToGfx, BuffersToTransition.GetData(), BuffersToTransition.Num());
		BuffersToTransition.SetNum(0, false);
	}
}

#if 0
void FGPUSkinCache::TransitionToWriteable(FRHICommandList& RHICmdList)
{
	int32 BufferIndex = InternalUpdateCount % GPUSKINCACHE_FRAMES;
	FUnorderedAccessViewRHIParamRef OutUAVs[] = { SkinCacheBuffer[BufferIndex].UAV };
	RHICmdList.TransitionResources(EResourceTransitionAccess::ERWNoBarrier, EResourceTransitionPipeline::EGfxToCompute, OutUAVs, ARRAY_COUNT(OutUAVs));
}

void FGPUSkinCache::TransitionAllToWriteable(FRHICommandList& RHICmdList)
{
	if (bInitialized)
	{
		FUnorderedAccessViewRHIParamRef OutUAVs[GPUSKINCACHE_FRAMES];

		for (int32 Index = 0; Index < GPUSKINCACHE_FRAMES; ++Index)
		{
			OutUAVs[Index] = SkinCacheBuffer[Index].UAV;
		}

		RHICmdList.TransitionResources(EResourceTransitionAccess::ERWNoBarrier, EResourceTransitionPipeline::EGfxToCompute, OutUAVs, ARRAY_COUNT(OutUAVs));
	}
}
#endif

/** base of the FRecomputeTangentsPerTrianglePassCS class */
class FBaseRecomputeTangents : public FGlobalShader
{
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		// currently only implemented and tested on Window SM5 (needs Compute, Atomics, SRV for index buffers, UAV for VertexBuffers)
		return DoesPlatformSupportGPUSkinCache(Parameters.Platform) && IsGPUSkinCacheAvailable();
	}

	static const uint32 ThreadGroupSizeX = 64;

	FShaderResourceParameter IntermediateAccumBufferUAV;
	FShaderParameter NumTriangles;
	FShaderResourceParameter GPUPositionCacheBuffer;
	FShaderResourceParameter GPUTangentCacheBuffer;
	FShaderParameter SkinCacheStart;
	FShaderResourceParameter IndexBuffer;
	FShaderParameter IndexBufferOffset;
	FShaderParameter InputStreamStart;
	FShaderResourceParameter TangentInputBuffer;
	FShaderResourceParameter UVsInputBuffer;
	FShaderResourceParameter DuplicatedIndices;
	FShaderResourceParameter DuplicatedIndicesIndices;

	FBaseRecomputeTangents()
	{}

	FBaseRecomputeTangents(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		IntermediateAccumBufferUAV.Bind(Initializer.ParameterMap, TEXT("IntermediateAccumBufferUAV"));
		NumTriangles.Bind(Initializer.ParameterMap, TEXT("NumTriangles"));
		GPUPositionCacheBuffer.Bind(Initializer.ParameterMap, TEXT("GPUPositionCacheBuffer"));
		GPUTangentCacheBuffer.Bind(Initializer.ParameterMap, TEXT("GPUTangentCacheBuffer"));
		SkinCacheStart.Bind(Initializer.ParameterMap, TEXT("SkinCacheStart"));
		IndexBuffer.Bind(Initializer.ParameterMap, TEXT("IndexBuffer"));
		IndexBufferOffset.Bind(Initializer.ParameterMap, TEXT("IndexBufferOffset"));

		InputStreamStart.Bind(Initializer.ParameterMap, TEXT("InputStreamStart"));
		TangentInputBuffer.Bind(Initializer.ParameterMap, TEXT("TangentInputBuffer"));
		UVsInputBuffer.Bind(Initializer.ParameterMap, TEXT("UVsInputBuffer"));

        DuplicatedIndices.Bind(Initializer.ParameterMap, TEXT("DuplicatedIndices"));
        DuplicatedIndicesIndices.Bind(Initializer.ParameterMap, TEXT("DuplicatedIndicesIndices"));
	}

	void SetParameters(FRHICommandListImmediate& RHICmdList, FGPUSkinCacheEntry* Entry, FGPUSkinCacheEntry::FSectionDispatchData& DispatchData, FRWBuffer& StagingBuffer)
	{
		const FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();

//later		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, ShaderRHI, View);

		SetShaderValue(RHICmdList, ShaderRHI, NumTriangles, DispatchData.NumTriangles);

		SetSRVParameter(RHICmdList, ShaderRHI, GPUPositionCacheBuffer, DispatchData.GetPositionRWBuffer()->SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, GPUTangentCacheBuffer, DispatchData.GetTangentRWBuffer()->SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, UVsInputBuffer, DispatchData.UVsBufferSRV);

		SetShaderValue(RHICmdList, ShaderRHI, SkinCacheStart, DispatchData.OutputStreamStart);

		SetSRVParameter(RHICmdList, ShaderRHI, IndexBuffer, DispatchData.IndexBuffer);
		SetShaderValue(RHICmdList, ShaderRHI, IndexBufferOffset, DispatchData.IndexBufferOffsetValue);
		
		SetShaderValue(RHICmdList, ShaderRHI, InputStreamStart, DispatchData.InputStreamStart);
		SetSRVParameter(RHICmdList, ShaderRHI, TangentInputBuffer, DispatchData.TangentBufferSRV);
		SetSRVParameter(RHICmdList, ShaderRHI, TangentInputBuffer, DispatchData.UVsBufferSRV);

		// UAV
		SetUAVParameter(RHICmdList, ShaderRHI, IntermediateAccumBufferUAV, StagingBuffer.UAV);

        if (!GAllowDupedVertsForRecomputeTangents)
        {
		    SetSRVParameter(RHICmdList, ShaderRHI, DuplicatedIndices, DispatchData.DuplicatedIndices);
            SetSRVParameter(RHICmdList, ShaderRHI, DuplicatedIndicesIndices, DispatchData.DuplicatedIndicesIndices);
        }
	}

	void UnsetParameters(FRHICommandList& RHICmdList)
	{
		FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();

		SetUAVParameter(RHICmdList, ShaderRHI, IntermediateAccumBufferUAV, 0);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << IntermediateAccumBufferUAV << NumTriangles << GPUPositionCacheBuffer << GPUTangentCacheBuffer << SkinCacheStart << IndexBuffer << IndexBufferOffset
			<< InputStreamStart << TangentInputBuffer << UVsInputBuffer;
        Ar << DuplicatedIndices << DuplicatedIndicesIndices;
		return bShaderHasOutdatedParameters;
	}
};

/** Encapsulates the RecomputeSkinTangents compute shader. */
template <int Permutation>
class FRecomputeTangentsPerTrianglePassCS : public FBaseRecomputeTangents
{
    constexpr static bool bMergeDuplicatedVerts = (4 == (Permutation & 4));
	constexpr static bool bUseExtraBoneInfluencesT = (2 == (Permutation & 2));
	constexpr static bool bFullPrecisionUV = (1 == (Permutation & 1));

	DECLARE_SHADER_TYPE(FRecomputeTangentsPerTrianglePassCS, Global);

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		const uint32 UseExtraBoneInfluences = bUseExtraBoneInfluencesT;
		OutEnvironment.SetDefine(TEXT("MERGE_DUPLICATED_VERTICES"), bMergeDuplicatedVerts);
		OutEnvironment.SetDefine(TEXT("GPUSKIN_USE_EXTRA_INFLUENCES"), UseExtraBoneInfluences);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), ThreadGroupSizeX);
		OutEnvironment.SetDefine(TEXT("INTERMEDIATE_ACCUM_BUFFER_NUM_INTS"), FGPUSkinCache::IntermediateAccumBufferNumInts);
		OutEnvironment.SetDefine(TEXT("FULL_PRECISION_UV"), bFullPrecisionUV);
	}

	FRecomputeTangentsPerTrianglePassCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FBaseRecomputeTangents(Initializer)
	{
	}

	FRecomputeTangentsPerTrianglePassCS()
	{}
};

IMPLEMENT_SHADER_TYPE(template<>, FRecomputeTangentsPerTrianglePassCS<0>, TEXT("/Engine/Private/RecomputeTangentsPerTrianglePass.usf"), TEXT("MainCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(template<>, FRecomputeTangentsPerTrianglePassCS<1>, TEXT("/Engine/Private/RecomputeTangentsPerTrianglePass.usf"), TEXT("MainCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(template<>, FRecomputeTangentsPerTrianglePassCS<2>, TEXT("/Engine/Private/RecomputeTangentsPerTrianglePass.usf"), TEXT("MainCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(template<>, FRecomputeTangentsPerTrianglePassCS<3>, TEXT("/Engine/Private/RecomputeTangentsPerTrianglePass.usf"), TEXT("MainCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(template<>, FRecomputeTangentsPerTrianglePassCS<4>, TEXT("/Engine/Private/RecomputeTangentsPerTrianglePass.usf"), TEXT("MainCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(template<>, FRecomputeTangentsPerTrianglePassCS<5>, TEXT("/Engine/Private/RecomputeTangentsPerTrianglePass.usf"), TEXT("MainCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(template<>, FRecomputeTangentsPerTrianglePassCS<6>, TEXT("/Engine/Private/RecomputeTangentsPerTrianglePass.usf"), TEXT("MainCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(template<>, FRecomputeTangentsPerTrianglePassCS<7>, TEXT("/Engine/Private/RecomputeTangentsPerTrianglePass.usf"), TEXT("MainCS"), SF_Compute);

/** Encapsulates the RecomputeSkinTangentsResolve compute shader. */
class FRecomputeTangentsPerVertexPassCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FRecomputeTangentsPerVertexPassCS, Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		// currently only implemented and tested on Window SM5 (needs Compute, Atomics, SRV for index buffers, UAV for VertexBuffers)
		return DoesPlatformSupportGPUSkinCache(Parameters.Platform) && IsGPUSkinCacheAvailable();
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		// this pass cannot read the input as it doesn't have the permutation
		OutEnvironment.SetDefine(TEXT("GPUSKIN_USE_EXTRA_INFLUENCES"), (uint32)0);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), ThreadGroupSizeX);
		OutEnvironment.SetDefine(TEXT("GPUSKIN_RWBUFFER_OFFSET_TANGENT_X"), FGPUSkinCache::RWTangentXOffsetInFloats);
		OutEnvironment.SetDefine(TEXT("GPUSKIN_RWBUFFER_OFFSET_TANGENT_Z"), FGPUSkinCache::RWTangentZOffsetInFloats);
		OutEnvironment.SetDefine(TEXT("INTERMEDIATE_ACCUM_BUFFER_NUM_INTS"), FGPUSkinCache::IntermediateAccumBufferNumInts);
	}

	static const uint32 ThreadGroupSizeX = 64;

	FRecomputeTangentsPerVertexPassCS() {}

public:
	FShaderResourceParameter IntermediateAccumBufferUAV;
	FShaderResourceParameter TangentBufferUAV;
	FShaderParameter SkinCacheStart;
	FShaderParameter NumVertices;
	FShaderParameter InputStreamStart;

	FRecomputeTangentsPerVertexPassCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		IntermediateAccumBufferUAV.Bind(Initializer.ParameterMap, TEXT("IntermediateAccumBufferUAV"));
		TangentBufferUAV.Bind(Initializer.ParameterMap, TEXT("TangentBufferUAV"));
		SkinCacheStart.Bind(Initializer.ParameterMap, TEXT("SkinCacheStart"));
		NumVertices.Bind(Initializer.ParameterMap, TEXT("NumVertices"));
		InputStreamStart.Bind(Initializer.ParameterMap, TEXT("InputStreamStart"));
	}

	void SetParameters(FRHICommandListImmediate& RHICmdList, FGPUSkinCacheEntry* Entry, FGPUSkinCacheEntry::FSectionDispatchData& DispatchData, FRWBuffer& StagingBuffer)
	{
		const FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();

		check(StagingBuffer.UAV);

		//later		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, ShaderRHI, View);

		SetShaderValue(RHICmdList, ShaderRHI, SkinCacheStart, DispatchData.OutputStreamStart);
		SetShaderValue(RHICmdList, ShaderRHI, NumVertices, DispatchData.NumVertices);
		SetShaderValue(RHICmdList, ShaderRHI, InputStreamStart, DispatchData.InputStreamStart);

		// UAVs
		SetUAVParameter(RHICmdList, ShaderRHI, IntermediateAccumBufferUAV, StagingBuffer.UAV);
		SetUAVParameter(RHICmdList, ShaderRHI, TangentBufferUAV, DispatchData.GetTangentRWBuffer()->UAV);
	}

	void UnsetParameters(FRHICommandList& RHICmdList)
	{
		FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();

		SetUAVParameter(RHICmdList, ShaderRHI, TangentBufferUAV, 0);
		SetUAVParameter(RHICmdList, ShaderRHI, IntermediateAccumBufferUAV, 0);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << IntermediateAccumBufferUAV << TangentBufferUAV << SkinCacheStart << NumVertices << InputStreamStart;
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_SHADER_TYPE(, FRecomputeTangentsPerVertexPassCS, TEXT("/Engine/Private/RecomputeTangentsPerVertexPass.usf"), TEXT("MainCS"), SF_Compute);

void FGPUSkinCache::DispatchUpdateSkinTangents(FRHICommandListImmediate& RHICmdList, FGPUSkinCacheEntry* Entry, int32 SectionIndex)
{
	FGPUSkinCacheEntry::FSectionDispatchData& DispatchData = Entry->DispatchData[SectionIndex];

	{
		// no need to clear the intermediate buffer because we create it cleared and clear it after each usage in the per vertex pass

		FSkeletalMeshRenderData& SkelMeshRenderData = Entry->GPUSkin->GetSkeletalMeshRenderData();
		int32 LODIndex = Entry->LOD;
		FSkeletalMeshLODRenderData& LodData = SkelMeshRenderData.LODRenderData[Entry->LOD];

		//SetRenderTarget(RHICmdList, FTextureRHIRef(), FTextureRHIRef());

		FRawStaticIndexBuffer16or32Interface* IndexBuffer = LodData.MultiSizeIndexContainer.GetIndexBuffer();
		FIndexBufferRHIParamRef IndexBufferRHI = IndexBuffer->IndexBufferRHI;

		const uint32 RequiredVertexCount = LodData.GetNumVertices();

		uint32 MaxVertexCount = RequiredVertexCount;

		if (StagingBuffers.Num() != GNumTangentIntermediateBuffers)
		{
			// Release extra buffers if shrinking
			for (int32 Index = GNumTangentIntermediateBuffers; Index < StagingBuffers.Num(); ++Index)
			{
				StagingBuffers[Index].Release();
			}
			StagingBuffers.SetNum(GNumTangentIntermediateBuffers, false);
		}

		uint32 NumIntsPerBuffer = DispatchData.NumTriangles * 3 * FGPUSkinCache::IntermediateAccumBufferNumInts;
		CurrentStagingBufferIndex = (CurrentStagingBufferIndex + 1) % StagingBuffers.Num();
		FRWBuffer& StagingBuffer = StagingBuffers[CurrentStagingBufferIndex];
		if (StagingBuffers[CurrentStagingBufferIndex].NumBytes < NumIntsPerBuffer * sizeof(uint32))
		{
			StagingBuffer.Release();
			StagingBuffer.Initialize(sizeof(int32), NumIntsPerBuffer, PF_R32_SINT, BUF_UnorderedAccess);
			RHICmdList.BindDebugLabelName(StagingBuffer.UAV, TEXT("SkinTangentIntermediate"));
			SET_MEMORY_STAT(STAT_GPUSkinCache_TangentsIntermediateMemUsed, (NumIntsPerBuffer * sizeof(uint32)));
		}

		// This code can be optimized by batched up and doing it with less Dispatch calls (costs more memory)
		{
			auto* GlobalShaderMap = GetGlobalShaderMap(ERHIFeatureLevel::SM5);
			TShaderMapRef<FRecomputeTangentsPerTrianglePassCS<0>> ComputeShader000(GlobalShaderMap);
			TShaderMapRef<FRecomputeTangentsPerTrianglePassCS<1>> ComputeShader010(GlobalShaderMap);
			TShaderMapRef<FRecomputeTangentsPerTrianglePassCS<2>> ComputeShader100(GlobalShaderMap);
			TShaderMapRef<FRecomputeTangentsPerTrianglePassCS<3>> ComputeShader110(GlobalShaderMap);
			TShaderMapRef<FRecomputeTangentsPerTrianglePassCS<4>> ComputeShader001(GlobalShaderMap);
			TShaderMapRef<FRecomputeTangentsPerTrianglePassCS<5>> ComputeShader011(GlobalShaderMap);
			TShaderMapRef<FRecomputeTangentsPerTrianglePassCS<6>> ComputeShader101(GlobalShaderMap);
			TShaderMapRef<FRecomputeTangentsPerTrianglePassCS<7>> ComputeShader111(GlobalShaderMap);

			bool bFullPrecisionUV = LodData.StaticVertexBuffers.StaticMeshVertexBuffer.GetUseFullPrecisionUVs();

			FBaseRecomputeTangents* Shader = 0;

            if (bFullPrecisionUV)
            {
                if (DispatchData.bExtraBoneInfluences)
                {
                    Shader = GAllowDupedVertsForRecomputeTangents ? (FBaseRecomputeTangents*)*ComputeShader100 : (FBaseRecomputeTangents*)*ComputeShader101;
                }
                else
                {
                    Shader = GAllowDupedVertsForRecomputeTangents ? (FBaseRecomputeTangents*)*ComputeShader000 : (FBaseRecomputeTangents*)*ComputeShader001;
                }
            }
            else
			{
                if (DispatchData.bExtraBoneInfluences)
                {
                    Shader = GAllowDupedVertsForRecomputeTangents ? (FBaseRecomputeTangents*)*ComputeShader110 : (FBaseRecomputeTangents*)*ComputeShader111;
                }
                else
                {
                    Shader = GAllowDupedVertsForRecomputeTangents ? (FBaseRecomputeTangents*)*ComputeShader010 : (FBaseRecomputeTangents*)*ComputeShader011;
                }
			}

			check(Shader);

			uint32 NumTriangles = DispatchData.NumTriangles;
			uint32 ThreadGroupCountValue = FMath::DivideAndRoundUp(NumTriangles, FBaseRecomputeTangents::ThreadGroupSizeX);

			SCOPED_DRAW_EVENTF(RHICmdList, SkinTangents_PerTrianglePass, TEXT("TangentsTri IndexStart=%d Tri=%d ExtraBoneInfluences=%d UVPrecision=%d"),
				DispatchData.IndexBufferOffsetValue, DispatchData.NumTriangles, DispatchData.bExtraBoneInfluences, bFullPrecisionUV);

			const FComputeShaderRHIParamRef ShaderRHI = Shader->GetComputeShader();
			RHICmdList.SetComputeShader(ShaderRHI);

			RHICmdList.TransitionResource(EResourceTransitionAccess::ERWNoBarrier, EResourceTransitionPipeline::EComputeToCompute, StagingBuffer.UAV.GetReference());

            if (!GAllowDupedVertsForRecomputeTangents)
            {
                check(LodData.RenderSections[SectionIndex].DuplicatedVerticesBuffer.DupVertData.Num() && LodData.RenderSections[SectionIndex].DuplicatedVerticesBuffer.DupVertIndexData.Num());
                DispatchData.DuplicatedIndices = LodData.RenderSections[SectionIndex].DuplicatedVerticesBuffer.DuplicatedVerticesIndexBuffer.VertexBufferSRV;
                DispatchData.DuplicatedIndicesIndices = LodData.RenderSections[SectionIndex].DuplicatedVerticesBuffer.LengthAndIndexDuplicatedVerticesIndexBuffer.VertexBufferSRV;
            }

			INC_DWORD_STAT_BY(STAT_GPUSkinCache_NumTrianglesForRecomputeTangents, NumTriangles);
			Shader->SetParameters(RHICmdList, Entry, DispatchData, StagingBuffer);
			DispatchComputeShader(RHICmdList, Shader, ThreadGroupCountValue, 1, 1);
			Shader->UnsetParameters(RHICmdList);
		}

		{
			SCOPED_DRAW_EVENTF(RHICmdList, SkinTangents_PerVertexPass, TEXT("TangentsVertex InputStreamStart=%d, OutputStreamStart=%d, Vert=%d"),
				DispatchData.InputStreamStart, DispatchData.OutputStreamStart, DispatchData.NumVertices);
			//#todo-gpuskin Feature level?
			TShaderMapRef<FRecomputeTangentsPerVertexPassCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			const FComputeShaderRHIParamRef ShaderRHI = ComputeShader->GetComputeShader();
			RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());

			uint32 VertexCount = DispatchData.NumVertices;
			uint32 ThreadGroupCountValue = FMath::DivideAndRoundUp(VertexCount, FRecomputeTangentsPerVertexPassCS::ThreadGroupSizeX);

			RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EComputeToCompute, StagingBuffer.UAV.GetReference());

			ComputeShader->SetParameters(RHICmdList, Entry, DispatchData, StagingBuffer);
			DispatchComputeShader(RHICmdList, *ComputeShader, ThreadGroupCountValue, 1, 1);
			ComputeShader->UnsetParameters(RHICmdList);
		}
		// todo				RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToGfx, TangentsBlendBuffer.VertexBufferSRV);
		//			ensureMsgf(DestRenderTarget.TargetableTexture == DestRenderTarget.ShaderResourceTexture, TEXT("%s should be resolved to a separate SRV"), *DestRenderTarget.TargetableTexture->GetName().ToString());	
	}
}

FGPUSkinCache::FRWBuffersAllocation* FGPUSkinCache::TryAllocBuffer(uint32 NumVertices, bool WithTangnents)
{
	uint64 MaxSizeInBytes = (uint64)(GSkinCacheSceneMemoryLimitInMB * 1024.0f * 1024.0f);
	uint64 RequiredMemInBytes = FRWBuffersAllocation::CalculateRequiredMemory(NumVertices, WithTangnents);
	if (bRequiresMemoryLimit && UsedMemoryInBytes + RequiredMemInBytes >= MaxSizeInBytes)
	{
		ExtraRequiredMemory += RequiredMemInBytes;

		// Can't fit
		return nullptr;
	}

	FRWBuffersAllocation* NewAllocation = new FRWBuffersAllocation(NumVertices, WithTangnents);
	Allocations.Add(NewAllocation);

	UsedMemoryInBytes += RequiredMemInBytes;
	INC_MEMORY_STAT_BY(STAT_GPUSkinCache_TotalMemUsed, RequiredMemInBytes);

	return NewAllocation;
}


void FGPUSkinCache::DoDispatch(FRHICommandListImmediate& RHICmdList, FGPUSkinCacheEntry* SkinCacheEntry, int32 Section, int32 RevisionNumber)
{
	INC_DWORD_STAT(STAT_GPUSkinCache_TotalNumChunks);
	FGPUSkinCacheEntry::FSectionDispatchData& DispatchData = SkinCacheEntry->DispatchData[Section];
	DispatchUpdateSkinning(RHICmdList, SkinCacheEntry, Section, RevisionNumber);
	//RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EGfxToCompute, DispatchData.GetRWBuffer());
	SkinCacheEntry->UpdateVertexFactoryDeclaration(Section);

	if (SkinCacheEntry->DispatchData[Section].IndexBuffer)
	{
		DispatchUpdateSkinTangents(RHICmdList, SkinCacheEntry, Section);
	}
}

void FGPUSkinCache::ProcessEntry(FRHICommandListImmediate& RHICmdList, FGPUBaseSkinVertexFactory* VertexFactory,
	FGPUSkinPassthroughVertexFactory* TargetVertexFactory, const FSkelMeshRenderSection& BatchElement, FSkeletalMeshObjectGPUSkin* Skin,
	const FMorphVertexBuffer* MorphVertexBuffer, const FSkeletalMeshVertexClothBuffer* ClothVertexBuffer, const FClothSimulData* SimData,
	const FMatrix& ClothLocalToWorld, float ClothBlendWeight, uint32 RevisionNumber, int32 Section, FGPUSkinCacheEntry*& InOutEntry)
{
	INC_DWORD_STAT(STAT_GPUSkinCache_NumSectionsProcessed);

	const uint32 NumVertices = BatchElement.GetNumVertices();
	uint32 StreamStrides[MaxVertexElementCount];
	//#todo-gpuskin Check that stream 0 is the position stream
	const uint32 StreamStrideCount = VertexFactory->GetStreamStrides(StreamStrides);
	const uint32 InputStreamStart = BatchElement.BaseVertexIndex;

	FSkeletalMeshRenderData& SkelMeshRenderData = Skin->GetSkeletalMeshRenderData();
	int32 LODIndex = Skin->GetLOD();
	FSkeletalMeshLODRenderData& LodData = SkelMeshRenderData.LODRenderData[LODIndex];

	if (FlushCounter < GGPUSkinCacheFlushCounter)
	{
		FlushCounter = GGPUSkinCacheFlushCounter;
		InvalidateAllEntries();
	}

	if (InOutEntry)
	{
		// If the LOD changed, the entry has to be invalidated
		if (!InOutEntry->IsValid(Skin))
		{
			Release(InOutEntry);
			InOutEntry = nullptr;
		}
		else
		{
			if (!InOutEntry->IsSectionValid(Section) || !InOutEntry->IsSourceFactoryValid(Section, VertexFactory))
			{
				// This section might not be valid yet, so set it up
				InOutEntry->SetupSection(Section, InOutEntry->PositionAllocation, &LodData.RenderSections[Section], MorphVertexBuffer, ClothVertexBuffer, NumVertices, InputStreamStart, VertexFactory, TargetVertexFactory);
			}
		}
	}

	int32 RecomputeTangentsMode = GForceRecomputeTangents > 0 ? 1 : GSkinCacheRecomputeTangents;
	// Try to allocate a new entry
	if (!InOutEntry)
	{
		bool WithTangents = RecomputeTangentsMode > 0;
		int32 TotalNumVertices = VertexFactory->GetNumVertices();
		FRWBuffersAllocation* NewPositionAllocation = TryAllocBuffer(TotalNumVertices, WithTangents);
		if (!NewPositionAllocation)
		{
			// Couldn't fit; caller will notify OOM
			return;
		}

		InOutEntry = new FGPUSkinCacheEntry(this, Skin, NewPositionAllocation);
		InOutEntry->GPUSkin = Skin;

		InOutEntry->SetupSection(Section, NewPositionAllocation, &LodData.RenderSections[Section], MorphVertexBuffer, ClothVertexBuffer, NumVertices, InputStreamStart, VertexFactory, TargetVertexFactory);
		Entries.Add(InOutEntry);
	}

	const bool bMorph = MorphVertexBuffer && MorphVertexBuffer->SectionIds.Contains(Section);
	if (bMorph)
	{
		InOutEntry->MorphBuffer = MorphVertexBuffer->GetSRV();
		check(InOutEntry->MorphBuffer);

		const uint32 MorphStride = sizeof(FMorphGPUSkinVertex);

		// see GPU code "check(MorphStride == sizeof(float) * 6);"
		check(MorphStride == sizeof(float) * 6);

		InOutEntry->DispatchData[Section].MorphBufferOffset = BatchElement.BaseVertexIndex;

		// weight buffer
		FSkinWeightVertexBuffer* WeightBuffer = Skin->GetSkinWeightVertexBuffer(LODIndex);
		uint32 WeightStride = WeightBuffer->GetStride();
		InOutEntry->DispatchData[Section].InputWeightStart = (WeightStride * BatchElement.BaseVertexIndex) / sizeof(float);
		InOutEntry->InputWeightStride = WeightStride;
		InOutEntry->InputWeightStreamSRV = WeightBuffer->GetSRV();
	}

    FVertexBufferAndSRV ClothPositionAndNormalsBuffer;
    TSkeletalMeshVertexData<FClothSimulEntry> VertexAndNormalData(true);
    if (ClothVertexBuffer)
    {
        InOutEntry->ClothBuffer = ClothVertexBuffer->GetSRV();
        check(InOutEntry->ClothBuffer);

        check(SimData->Positions.Num() == SimData->Normals.Num());
        VertexAndNormalData.ResizeBuffer( SimData->Positions.Num() );

        uint8* Data = VertexAndNormalData.GetDataPointer();
        uint32 Stride = VertexAndNormalData.GetStride();

        // Copy the vertices into the buffer.
        checkSlow(Stride*VertexAndNormalData.GetNumVertices() == sizeof(FClothSimulEntry) * SimData->Positions.Num());
        check(sizeof(FClothSimulEntry) == 6 * sizeof(float));

        for (int32 Index = 0;Index < SimData->Positions.Num();Index++)
        {
            FClothSimulEntry NewEntry;
            NewEntry.Position = SimData->Positions[Index];
            NewEntry.Normal = SimData->Normals[Index];
            *((FClothSimulEntry*)(Data + Index * Stride)) = NewEntry;
        }

        FResourceArrayInterface* ResourceArray = VertexAndNormalData.GetResourceArray();
        check(ResourceArray->GetResourceDataSize() > 0);

        FRHIResourceCreateInfo CreateInfo(ResourceArray);
        ClothPositionAndNormalsBuffer.VertexBufferRHI = RHICreateVertexBuffer( ResourceArray->GetResourceDataSize(), BUF_Static | BUF_ShaderResource, CreateInfo);
        ClothPositionAndNormalsBuffer.VertexBufferSRV = RHICreateShaderResourceView(ClothPositionAndNormalsBuffer.VertexBufferRHI, sizeof(FVector2D), PF_G32R32F);
        InOutEntry->ClothPositionsAndNormalsBuffer = ClothPositionAndNormalsBuffer.VertexBufferSRV;

        InOutEntry->DispatchData[Section].ClothBlendWeight = ClothBlendWeight;
        InOutEntry->DispatchData[Section].ClothLocalToWorld = ClothLocalToWorld;
        InOutEntry->DispatchData[Section].ClothWorldToLocal = ClothLocalToWorld.Inverse();
    }
    InOutEntry->DispatchData[Section].SkinType = ClothVertexBuffer ? 2 : (bMorph ? 1 : 0);


	DoDispatch(RHICmdList, InOutEntry, Section, RevisionNumber);

	InOutEntry->UpdateVertexFactoryDeclaration(Section);
}

void FGPUSkinCache::Release(FGPUSkinCacheEntry*& SkinCacheEntry)
{
	if (SkinCacheEntry)
	{
		ReleaseSkinCacheEntry(SkinCacheEntry);
		SkinCacheEntry = nullptr;
	}
}

void FGPUSkinCache::SetVertexStreams(FGPUSkinCacheEntry* Entry, int32 Section, FRHICommandList& RHICmdList,
	FShader* Shader, const FGPUSkinPassthroughVertexFactory* VertexFactory,
	uint32 BaseVertexIndex, FShaderResourceParameter GPUSkinCachePreviousPositionBuffer)
{
	INC_DWORD_STAT(STAT_GPUSkinCache_NumSetVertexStreams);
	check(Entry);
	check(Entry->IsSectionValid(Section));

	FGPUSkinCacheEntry::FSectionDispatchData& DispatchData = Entry->DispatchData[Section];

	//UE_LOG(LogSkinCache, Warning, TEXT("*** SetVertexStreams E %p All %p Sec %d(%p) LOD %d"), Entry, Entry->DispatchData[Section].Allocation, Section, Entry->DispatchData[Section].Section, Entry->LOD);
	RHICmdList.SetStreamSource(VertexFactory->GetPositionStreamIndex(), DispatchData.GetPositionRWBuffer()->Buffer, 0);
	if (VertexFactory->GetTangentStreamIndex() > -1 && DispatchData.GetTangentRWBuffer())
	{
		RHICmdList.SetStreamSource(VertexFactory->GetTangentStreamIndex(), DispatchData.GetTangentRWBuffer()->Buffer, 0);
	}

	FVertexShaderRHIParamRef ShaderRHI = Shader->GetVertexShader();
	if (ShaderRHI && GPUSkinCachePreviousPositionBuffer.IsBound())
	{
		RHICmdList.SetShaderResourceViewParameter(ShaderRHI, GPUSkinCachePreviousPositionBuffer.GetBaseIndex(), DispatchData.GetPreviousPositionRWBuffer()->SRV);
	}
}

void FGPUSkinCache::DispatchUpdateSkinning(FRHICommandListImmediate& RHICmdList, FGPUSkinCacheEntry* Entry, int32 Section, uint32 RevisionNumber)
{
	FGPUSkinCacheEntry::FSectionDispatchData& DispatchData = Entry->DispatchData[Section];
	FGPUBaseSkinVertexFactory::FShaderDataType& ShaderData = DispatchData.SourceVertexFactory->GetShaderData();

	SCOPED_DRAW_EVENTF(RHICmdList, SkinCacheDispatch,
		TEXT("Skinning%d%d Chunk=%d InStreamStart=%d OutStart=%d Vert=%d Morph=%d/%d"),
		(int32)DispatchData.bExtraBoneInfluences, DispatchData.SkinType,
		DispatchData.SectionIndex, DispatchData.InputStreamStart, DispatchData.OutputStreamStart, DispatchData.NumVertices, Entry->MorphBuffer != 0, DispatchData.MorphBufferOffset);
	auto* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	TShaderMapRef<TGPUSkinCacheCS<0>> SkinCacheCS00(GlobalShaderMap);
	TShaderMapRef<TGPUSkinCacheCS<1>> SkinCacheCS01(GlobalShaderMap);
    TShaderMapRef<TGPUSkinCacheCS<2>> SkinCacheCS02(GlobalShaderMap);
    TShaderMapRef<TGPUSkinCacheCS<4>> SkinCacheCS10(GlobalShaderMap);
    TShaderMapRef<TGPUSkinCacheCS<5>> SkinCacheCS11(GlobalShaderMap);
    TShaderMapRef<TGPUSkinCacheCS<6>> SkinCacheCS12(GlobalShaderMap);

	FBaseGPUSkinCacheCS* Shader = 0;

	switch (DispatchData.SkinType)
	{
	case 0: Shader = DispatchData.bExtraBoneInfluences ? (FBaseGPUSkinCacheCS*)*SkinCacheCS10 : (FBaseGPUSkinCacheCS*)*SkinCacheCS00;
		break;
	case 1: Shader = DispatchData.bExtraBoneInfluences ? (FBaseGPUSkinCacheCS*)*SkinCacheCS11 : (FBaseGPUSkinCacheCS*)*SkinCacheCS01;
		break;
	case 2: Shader = DispatchData.bExtraBoneInfluences ? (FBaseGPUSkinCacheCS*)*SkinCacheCS12 : (FBaseGPUSkinCacheCS*)*SkinCacheCS02;
		break;
	default:
		check(0);
	}
	check(Shader);

	const FVertexBufferAndSRV& BoneBuffer = ShaderData.GetBoneBufferForReading(false);
	const FVertexBufferAndSRV& PrevBoneBuffer = ShaderData.GetBoneBufferForReading(true);

	uint32 CurrentRevision = ShaderData.GetRevisionNumber(false);
	uint32 PreviousRevision = ShaderData.GetRevisionNumber(true);

	DispatchData.TangentBuffer = DispatchData.PositionTracker.GetTangentBuffer();
	if (DispatchData.TangentBuffer)
	{
		BuffersToTransition.Add(DispatchData.TangentBuffer->UAV);
	}

	DispatchData.PreviousPositionBuffer = DispatchData.PositionTracker.Find(PrevBoneBuffer, PreviousRevision);
	if (!DispatchData.PreviousPositionBuffer)
	{
		DispatchData.PositionTracker.Advance(PrevBoneBuffer, PreviousRevision, BoneBuffer, CurrentRevision);
		DispatchData.PreviousPositionBuffer = DispatchData.PositionTracker.Find(PrevBoneBuffer, PreviousRevision);
		check(DispatchData.PreviousPositionBuffer)

		RHICmdList.SetComputeShader(Shader->GetComputeShader());
		Shader->SetParameters(RHICmdList, PrevBoneBuffer, Entry, DispatchData, DispatchData.GetPreviousPositionRWBuffer()->UAV, DispatchData.GetTangentRWBuffer() ? DispatchData.GetTangentRWBuffer()->UAV : nullptr);

		RHICmdList.TransitionResource(EResourceTransitionAccess::EWritable, EResourceTransitionPipeline::EGfxToCompute, DispatchData.GetPreviousPositionRWBuffer()->UAV.GetReference());

		uint32 VertexCountAlign64 = FMath::DivideAndRoundUp(DispatchData.NumVertices, (uint32)64);
		INC_DWORD_STAT_BY(STAT_GPUSkinCache_TotalNumVertices, VertexCountAlign64 * 64);
		RHICmdList.DispatchComputeShader(VertexCountAlign64, 1, 1);
		Shader->UnsetParameters(RHICmdList);

		BuffersToTransition.Add(DispatchData.GetPreviousPositionRWBuffer()->UAV);
	}

	DispatchData.PositionBuffer = DispatchData.PositionTracker.Find(BoneBuffer, CurrentRevision);
	if (!DispatchData.PositionBuffer)
	{
		DispatchData.PositionTracker.Advance(BoneBuffer, CurrentRevision, PrevBoneBuffer, PreviousRevision);
		DispatchData.PositionBuffer = DispatchData.PositionTracker.Find(BoneBuffer, CurrentRevision);
		check(DispatchData.PositionBuffer);

		RHICmdList.SetComputeShader(Shader->GetComputeShader());
		Shader->SetParameters(RHICmdList, BoneBuffer, Entry, DispatchData, DispatchData.GetPositionRWBuffer()->UAV, DispatchData.GetTangentRWBuffer() ? DispatchData.GetTangentRWBuffer()->UAV : nullptr);

		RHICmdList.TransitionResource(EResourceTransitionAccess::EWritable, EResourceTransitionPipeline::EGfxToCompute, DispatchData.GetPositionRWBuffer()->UAV.GetReference());

		uint32 VertexCountAlign64 = FMath::DivideAndRoundUp(DispatchData.NumVertices, (uint32)64);
		INC_DWORD_STAT_BY(STAT_GPUSkinCache_TotalNumVertices, VertexCountAlign64 * 64);
		RHICmdList.DispatchComputeShader(VertexCountAlign64, 1, 1);
		Shader->UnsetParameters(RHICmdList);

		BuffersToTransition.Add(DispatchData.GetPositionRWBuffer()->UAV);
	}

	check(DispatchData.PreviousPositionBuffer != DispatchData.PositionBuffer);
}

void FGPUSkinCache::FRWBuffersAllocation::RemoveAllFromTransitionArray(TArray<FUnorderedAccessViewRHIParamRef>& InBuffersToTransition)
{
	for (uint32 i = 0; i < NUM_BUFFERS; i++)
	{
		FRWBuffer& RWBuffer = RWBuffers[i];
		if (RWBuffer.UAV.IsValid())
		{
			InBuffersToTransition.Remove(RWBuffer.UAV);
		}
		if (auto TangentBuffer = GetTangentBuffer())
		{
			if (TangentBuffer->UAV.IsValid())
			{
				InBuffersToTransition.Remove(TangentBuffer->UAV);
			}
		}
	}
}

void FGPUSkinCache::ReleaseSkinCacheEntry(FGPUSkinCacheEntry* SkinCacheEntry)
{
	FGPUSkinCache* SkinCache = SkinCacheEntry->SkinCache;
	FRWBuffersAllocation* PositionAllocation = SkinCacheEntry->PositionAllocation;
	if (PositionAllocation)
	{
		uint64 RequiredMemInBytes = PositionAllocation->GetNumBytes();
		SkinCache->UsedMemoryInBytes -= RequiredMemInBytes;
		DEC_MEMORY_STAT_BY(STAT_GPUSkinCache_TotalMemUsed, RequiredMemInBytes);

		SkinCache->Allocations.Remove(PositionAllocation);
		PositionAllocation->RemoveAllFromTransitionArray(SkinCache->BuffersToTransition);

		delete PositionAllocation;

		SkinCacheEntry->PositionAllocation = nullptr;
	}

	SkinCache->Entries.RemoveSingleSwap(SkinCacheEntry, false);
	delete SkinCacheEntry;
}

bool FGPUSkinCache::IsEntryValid(FGPUSkinCacheEntry* SkinCacheEntry, int32 Section)
{
	return SkinCacheEntry->IsSectionValid(Section);
}

FGPUSkinBatchElementUserData* FGPUSkinCache::InternalGetFactoryUserData(FGPUSkinCacheEntry* Entry, int32 Section)
{
	return &Entry->BatchElementsUserData[Section];
}

void FGPUSkinCache::InvalidateAllEntries()
{
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		Entries[Index]->LOD = -1;
	}

	for (int32 Index = 0; Index < StagingBuffers.Num(); ++Index)
	{
		StagingBuffers[Index].Release();
	}
	StagingBuffers.SetNum(0, false);
	SET_MEMORY_STAT(STAT_GPUSkinCache_TangentsIntermediateMemUsed, 0);
}

void FGPUSkinCache::CVarSinkFunction()
{
	int32 NewGPUSkinCacheValue = CVarEnableGPUSkinCache.GetValueOnAnyThread() != 0;
	int32 NewRecomputeTangentsValue = CVarGPUSkinCacheRecomputeTangents.GetValueOnAnyThread();
	float NewSceneMaxSizeInMb = CVarGPUSkinCacheSceneMemoryLimitInMB.GetValueOnAnyThread();
	int32 NewNumTangentIntermediateBuffers = CVarGPUSkinNumTangentIntermediateBuffers.GetValueOnAnyThread();

	if (!GEnableGPUSkinCacheShaders)
	{
		NewGPUSkinCacheValue = 0;
		NewRecomputeTangentsValue = 0;
	}

	if (NewGPUSkinCacheValue != GEnableGPUSkinCache || NewRecomputeTangentsValue != GSkinCacheRecomputeTangents
		|| NewSceneMaxSizeInMb != GSkinCacheSceneMemoryLimitInMB || NewNumTangentIntermediateBuffers != GNumTangentIntermediateBuffers)
	{
		ENQUEUE_RENDER_COMMAND(DoEnableSkinCaching)(
			[NewRecomputeTangentsValue, NewGPUSkinCacheValue, NewSceneMaxSizeInMb, NewNumTangentIntermediateBuffers](FRHICommandList& RHICmdList)
		{
			GNumTangentIntermediateBuffers = FMath::Max(NewNumTangentIntermediateBuffers, 1);
			GEnableGPUSkinCache = NewGPUSkinCacheValue;
			GSkinCacheRecomputeTangents = NewRecomputeTangentsValue;
			GSkinCacheSceneMemoryLimitInMB = NewSceneMaxSizeInMb;
			++GGPUSkinCacheFlushCounter;
		}
		);
	}
}

FAutoConsoleVariableSink FGPUSkinCache::CVarSink(FConsoleCommandDelegate::CreateStatic(&CVarSinkFunction));
