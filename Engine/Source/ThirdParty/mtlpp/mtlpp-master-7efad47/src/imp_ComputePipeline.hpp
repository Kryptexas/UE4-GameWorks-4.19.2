// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#ifndef imp_ComputePipeline_hpp
#define imp_ComputePipeline_hpp

#include "imp_State.hpp"

MTLPP_BEGIN

template<>
struct IMPTable<id<MTLComputePipelineState>, void> : public IMPTableState<id<MTLComputePipelineState>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableState<id<MTLComputePipelineState>>(C)
	, INTERPOSE_CONSTRUCTOR(maxTotalThreadsPerThreadgroup, C)
	, INTERPOSE_CONSTRUCTOR(threadExecutionWidth, C)
	, INTERPOSE_CONSTRUCTOR(staticThreadgroupMemoryLength, C)
	, INTERPOSE_CONSTRUCTOR(imageblockMemoryLengthForDimensions, C)
	{
	}
	
	INTERPOSE_SELECTOR(id<MTLComputePipelineState>, maxTotalThreadsPerThreadgroup, maxTotalThreadsPerThreadgroup, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLComputePipelineState>, threadExecutionWidth, threadExecutionWidth, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLComputePipelineState>, staticThreadgroupMemoryLength, staticThreadgroupMemoryLength, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLComputePipelineState>, imageblockMemoryLengthForDimensions:, imageblockMemoryLengthForDimensions,NSUInteger,MTLPPSize);
};

template<typename InterposeClass>
struct IMPTable<id<MTLComputePipelineState>, InterposeClass> : public IMPTable<id<MTLComputePipelineState>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLComputePipelineState>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableState<id<MTLComputePipelineState>>::RegisterInterpose<InterposeClass>(C);
	}
};

MTLPP_END

#endif /* imp_ComputePipeline_hpp */
