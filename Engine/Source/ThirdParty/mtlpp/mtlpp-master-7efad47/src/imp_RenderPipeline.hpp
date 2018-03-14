// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#ifndef imp_RenderPipeline_hpp
#define imp_RenderPipeline_hpp

#include "imp_State.hpp"

MTLPP_BEGIN

template<>
struct IMPTable<id<MTLRenderPipelineState>, void> : public IMPTableState<id<MTLRenderPipelineState>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableState<id<MTLRenderPipelineState>>(C)
	, INTERPOSE_CONSTRUCTOR(maxTotalThreadsPerThreadgroup, C)
	, INTERPOSE_CONSTRUCTOR(threadgroupSizeMatchesTileSize, C)
	, INTERPOSE_CONSTRUCTOR(imageblockSampleLength, C)
	, INTERPOSE_CONSTRUCTOR(imageblockMemoryLengthForDimensions, C)
	{
	}
	
	INTERPOSE_SELECTOR(id<MTLRenderPipelineState>, maxTotalThreadsPerThreadgroup, maxTotalThreadsPerThreadgroup, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderPipelineState>, threadgroupSizeMatchesTileSize, threadgroupSizeMatchesTileSize, BOOL);
	INTERPOSE_SELECTOR(id<MTLRenderPipelineState>, imageblockSampleLength, imageblockSampleLength, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLRenderPipelineState>, imageblockMemoryLengthForDimensions:, imageblockMemoryLengthForDimensions, NSUInteger, MTLPPSize);
};

template<typename InterposeClass>
struct IMPTable<id<MTLRenderPipelineState>, InterposeClass> : public IMPTable<id<MTLRenderPipelineState>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLRenderPipelineState>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableState<id<MTLRenderPipelineState>>::RegisterInterpose<InterposeClass>(C);
	}
};

MTLPP_END

#endif /* imp_RenderPipeline_hpp */
