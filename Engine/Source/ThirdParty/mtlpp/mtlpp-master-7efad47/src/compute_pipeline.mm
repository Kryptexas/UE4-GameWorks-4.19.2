/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include "compute_pipeline.hpp"
#include <Metal/MTLComputePipeline.h>

namespace mtlpp
{
    ComputePipelineReflection::ComputePipelineReflection() :
        ns::Object<MTLComputePipelineReflection*>([[MTLComputePipelineReflection alloc] init], false)
    {
    }

    ComputePipelineDescriptor::ComputePipelineDescriptor() :
        ns::Object<MTLComputePipelineDescriptor*>([[MTLComputePipelineDescriptor alloc] init], false)
    {
    }

    ns::String ComputePipelineDescriptor::GetLabel() const
    {
        Validate();
        return [(MTLComputePipelineDescriptor*)m_ptr label];
    }

    Function ComputePipelineDescriptor::GetComputeFunction() const
    {
        Validate();
        return [(MTLComputePipelineDescriptor*)m_ptr computeFunction];
    }

    bool ComputePipelineDescriptor::GetThreadGroupSizeIsMultipleOfThreadExecutionWidth() const
    {
        Validate();
        return [(MTLComputePipelineDescriptor*)m_ptr threadGroupSizeIsMultipleOfThreadExecutionWidth];
    }

    StageInputOutputDescriptor ComputePipelineDescriptor::GetStageInputDescriptor() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return [(MTLComputePipelineDescriptor*)m_ptr stageInputDescriptor];
#else
        return nullptr;
#endif
    }
	
	ns::Array<PipelineBufferDescriptor> ComputePipelineDescriptor::GetBuffers() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return ns::Array<PipelineBufferDescriptor>((NSArray*)[(MTLComputePipelineDescriptor*)m_ptr buffers]);
#else
		return ns::Array<PipelineBufferDescriptor>();
#endif
	}

    void ComputePipelineDescriptor::SetLabel(const ns::String& label)
    {
        Validate();
        [(MTLComputePipelineDescriptor*)m_ptr setLabel:(NSString*)label.GetPtr()];
    }

    void ComputePipelineDescriptor::SetComputeFunction(const Function& function)
    {
        Validate();
        [(MTLComputePipelineDescriptor*)m_ptr setComputeFunction:(id<MTLFunction>)function.GetPtr()];
    }

    void ComputePipelineDescriptor::SetThreadGroupSizeIsMultipleOfThreadExecutionWidth(bool value)
    {
        Validate();
        [(MTLComputePipelineDescriptor*)m_ptr setThreadGroupSizeIsMultipleOfThreadExecutionWidth:value];
    }

    void ComputePipelineDescriptor::SetStageInputDescriptor(const StageInputOutputDescriptor& stageInputDescriptor) const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        [(MTLComputePipelineDescriptor*)m_ptr setStageInputDescriptor:(MTLStageInputOutputDescriptor*)stageInputDescriptor.GetPtr()];
#endif
    }
	
	void ComputePipelineDescriptor::SetBuffers(ns::Array<PipelineBufferDescriptor> const& array)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		ns::Array<PipelineBufferDescriptor>([(MTLComputePipelineDescriptor*)m_ptr setBuffers:(MTLPipelineBufferDescriptorArray *)array.GetPtr()]);
#endif
	}

    Device ComputePipelineState::GetDevice() const
    {
        Validate();
        return [(id<MTLComputePipelineState>)m_ptr device];
    }

    uint32_t ComputePipelineState::GetMaxTotalThreadsPerThreadgroup() const
    {
        Validate();
        return uint32_t([(id<MTLComputePipelineState>)m_ptr maxTotalThreadsPerThreadgroup]);
    }

    uint32_t ComputePipelineState::GetThreadExecutionWidth() const
    {
        Validate();
        return uint32_t([(id<MTLComputePipelineState>)m_ptr threadExecutionWidth]);
    }
	
	uint32_t ComputePipelineState::GetStaticThreadgroupMemoryLength() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return uint32_t([(id<MTLComputePipelineState>)m_ptr staticThreadgroupMemoryLength]);
#else
		return 0;
#endif
	}
	
	uint32_t ComputePipelineState::GetImageblockMemoryLengthForDimensions(Size const& imageblockDimensions)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return uint32_t([(id<MTLComputePipelineState>)m_ptr imageblockMemoryLengthForDimensions:MTLSizeMake(imageblockDimensions.Width, imageblockDimensions.Height, imageblockDimensions.Depth)]);
#else
		return 0;
#endif
	}
}
