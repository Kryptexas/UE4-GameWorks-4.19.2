// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MetalRHIPrivate.h"
#if METAL_DEBUG_OPTIONS
#include "MetalDebugCommandEncoder.h"
#endif

enum EMetalPipelineHashBits
{
	NumBits_RenderTargetFormat = 5, //(x8=40),
	NumBits_DepthFormat = 3, //(x1=3),
	NumBits_StencilFormat = 3, //(x1=3),
	NumBits_SampleCount = 3, //(x1=3),

	NumBits_BlendState = 5, //(x8=40),
	NumBits_PrimitiveTopology = 2, //(x1=2)
	NumBits_IndexType = 2,
};

enum EMetalPipelineHashOffsets
{
	Offset_BlendState0 = 0,
	Offset_BlendState1 = Offset_BlendState0 + NumBits_BlendState,
	Offset_BlendState2 = Offset_BlendState1 + NumBits_BlendState,
	Offset_BlendState3 = Offset_BlendState2 + NumBits_BlendState,
	Offset_BlendState4 = Offset_BlendState3 + NumBits_BlendState,
	Offset_BlendState5 = Offset_BlendState4 + NumBits_BlendState,
	Offset_BlendState6 = Offset_BlendState5 + NumBits_BlendState,
	Offset_BlendState7 = Offset_BlendState6 + NumBits_BlendState,
	Offset_PrimitiveTopology = Offset_BlendState7 + NumBits_BlendState,
	Offset_IndexType = Offset_PrimitiveTopology + NumBits_PrimitiveTopology,
	Offset_RasterEnd = Offset_IndexType + NumBits_IndexType,

	Offset_RenderTargetFormat0 = 64,
	Offset_RenderTargetFormat1 = Offset_RenderTargetFormat0 + NumBits_RenderTargetFormat,
	Offset_RenderTargetFormat2 = Offset_RenderTargetFormat1 + NumBits_RenderTargetFormat,
	Offset_RenderTargetFormat3 = Offset_RenderTargetFormat2 + NumBits_RenderTargetFormat,
	Offset_RenderTargetFormat4 = Offset_RenderTargetFormat3 + NumBits_RenderTargetFormat,
	Offset_RenderTargetFormat5 = Offset_RenderTargetFormat4 + NumBits_RenderTargetFormat,
	Offset_RenderTargetFormat6 = Offset_RenderTargetFormat5 + NumBits_RenderTargetFormat,
	Offset_RenderTargetFormat7 = Offset_RenderTargetFormat6 + NumBits_RenderTargetFormat,
	Offset_DepthFormat = Offset_RenderTargetFormat7 + NumBits_RenderTargetFormat,
	Offset_StencilFormat = Offset_DepthFormat + NumBits_DepthFormat,
	Offset_SampleCount = Offset_StencilFormat + NumBits_StencilFormat,
	Offset_End = Offset_SampleCount + NumBits_SampleCount
};


@interface FMetalTessellationPipelineDesc : FApplePlatformObject
@property (nonatomic, retain) MTLVertexDescriptor* DomainVertexDescriptor;
@property (nonatomic) NSUInteger TessellationInputControlPointBufferIndex;
@property (nonatomic) NSUInteger TessellationOutputControlPointBufferIndex;
@property (nonatomic) NSUInteger TessellationPatchControlPointOutSize;
@property (nonatomic) NSUInteger TessellationPatchConstBufferIndex;
@property (nonatomic) NSUInteger TessellationInputPatchConstBufferIndex;
@property (nonatomic) NSUInteger TessellationPatchConstOutSize;
@property (nonatomic) NSUInteger TessellationTessFactorOutSize;
@property (nonatomic) NSUInteger TessellationFactorBufferIndex;
@property (nonatomic) NSUInteger TessellationPatchCountBufferIndex;
@property (nonatomic) NSUInteger TessellationControlPointIndexBufferIndex;
@property (nonatomic) NSUInteger TessellationIndexBufferIndex;
@property (nonatomic) NSUInteger DSNumUniformBuffers; // DEBUG ONLY
@end

@interface FMetalShaderPipeline : FApplePlatformObject
{
#if METAL_DEBUG_OPTIONS
@public
	FMetalDebugShaderResourceMask ResourceMask[EMetalShaderStagesNum];
#endif
}
@property (nonatomic, retain) id<MTLRenderPipelineState> RenderPipelineState;
@property (nonatomic, retain) id<MTLComputePipelineState> ComputePipelineState;
@property (nonatomic, retain) FMetalTessellationPipelineDesc* TessellationPipelineDesc;
#if METAL_DEBUG_OPTIONS
@property (nonatomic, retain) MTLRenderPipelineReflection* RenderPipelineReflection;
@property (nonatomic, retain) MTLComputePipelineReflection* ComputePipelineReflection;
@property (nonatomic, retain) NSString* VertexSource;
@property (nonatomic, retain) NSString* FragmentSource;
@property (nonatomic, retain) NSString* ComputeSource;
- (void)initResourceMask;
- (void)initResourceMask:(EMetalShaderFrequency)Frequency;
#endif
@end

struct FMetalRenderPipelineDesc
{
	FMetalRenderPipelineDesc();
	~FMetalRenderPipelineDesc();

	template<typename Type>
	inline void SetHashValue(uint32 Offset, uint32 NumBits, Type Value)
	{
		if (Offset < Offset_RasterEnd)
		{
			uint64 BitMask = ((((uint64)1ULL) << NumBits) - 1) << Offset;
			Hash.RasterBits = (Hash.RasterBits & ~BitMask) | (((uint64)Value << Offset) & BitMask);
		}
		else
		{
			Offset -= Offset_RenderTargetFormat0;
			uint64 BitMask = ((((uint64)1ULL) << NumBits) - 1) << Offset;
			Hash.TargetBits = (Hash.TargetBits & ~BitMask) | (((uint64)Value << Offset) & BitMask);
		}
	}

	/**
	 * @return the hash of the current pipeline state
	 */
	inline FMetalRenderPipelineHash GetHash() const { return Hash; }
	
	/**
	 * @return an pipeline state object that matches the current state and the BSS
	 */
	FMetalShaderPipeline* CreatePipelineStateForBoundShaderState(FMetalBoundShaderState* BSS, FMetalHashedVertexDescriptor const& VertexDesc) const;

	MTLRenderPipelineDescriptor* PipelineDescriptor;
	uint32 SampleCount;
	EMetalIndexType IndexType;

	// running hash of the pipeline state
	FMetalRenderPipelineHash Hash;

	struct FMetalRenderPipelineKey
	{
		FMetalRenderPipelineHash RenderPipelineHash;
		FMetalHashedVertexDescriptor VertexDescriptorHash;
		TPair<id<MTLFunction>, id<MTLLibrary>> VertexFunction;
		TPair<id<MTLFunction>, id<MTLLibrary>> PixelFunction;
		TPair<id<MTLFunction>, id<MTLLibrary>> DomainFunction;
		TArray<uint32> FunctionConstants;
		
		bool operator==(FMetalRenderPipelineKey const& Other) const;
		
		friend uint32 GetTypeHash(FMetalRenderPipelineKey const& Key)
		{
			uintptr_t Data[6];
			Data[0] = (uintptr_t)(Key.VertexFunction.Key);
			Data[1] = (uintptr_t)(Key.VertexFunction.Value);
			Data[2] = (uintptr_t)(Key.PixelFunction.Key);
			Data[3] = (uintptr_t)(Key.PixelFunction.Value);
			Data[4] = (uintptr_t)(Key.DomainFunction.Key);
			Data[5] = (uintptr_t)(Key.DomainFunction.Value);
			
			uint32 H = FCrc::MemCrc32(&Key.RenderPipelineHash, sizeof(Key.RenderPipelineHash), GetTypeHash(Key.VertexDescriptorHash));
			H = FCrc::MemCrc32(Data, sizeof(Data), H);
			H = Key.FunctionConstants.Num() ? FCrc::MemCrc32(Key.FunctionConstants.GetData(), Key.FunctionConstants.Num() * sizeof(uint32), H) : H;
			
			return H;
		}
	};
	
	struct FMetalRWMutex
	{
		FMetalRWMutex()
		{
			int Err = pthread_rwlock_init(&Mutex, nullptr);
			checkf(Err == 0, TEXT("pthread_rwlock_init failed with error: %d"), errno);
		}
		
		~FMetalRWMutex()
		{
			int Err = pthread_rwlock_destroy(&Mutex);
			checkf(Err == 0, TEXT("pthread_rwlock_destroy failed with error: %d"), errno);
		}
		
		pthread_rwlock_t Mutex;
	};
	
	static FMetalRWMutex MetalPipelineMutex;
	static TMap<FMetalRenderPipelineKey, FMetalShaderPipeline*> MetalPipelineCache;
	static uint32 BlendBitOffsets[8];
	static uint32 RTBitOffsets[8];
};
