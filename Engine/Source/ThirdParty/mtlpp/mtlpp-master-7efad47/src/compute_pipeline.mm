/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLComputePipeline.h>
#include "compute_pipeline.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    AutoReleasedComputePipelineReflection::AutoReleasedComputePipelineReflection() :
        ns::Object<MTLComputePipelineReflection*, true>([[MTLComputePipelineReflection alloc] init], false)
    {
    }
	
	ns::Array<Argument> AutoReleasedComputePipelineReflection::GetArguments() const
	{
		Validate();
		return [m_ptr arguments];
	}
	ComputePipelineReflection::ComputePipelineReflection() :
	ns::Object<MTLComputePipelineReflection*>([[MTLComputePipelineReflection alloc] init], false)
	{
	}
	
	ns::Array<Argument> ComputePipelineReflection::GetArguments() const
	{
		Validate();
		return [m_ptr arguments];
	}
	
	ComputePipelineReflection::ComputePipelineReflection(const AutoReleasedComputePipelineReflection& rhs)
	{
		operator=(rhs);
	}
#if MTLPP_CONFIG_RVALUE_REFERENCES
	ComputePipelineReflection::ComputePipelineReflection(const AutoReleasedComputePipelineReflection&& rhs)
	{
		operator=(rhs);
	}
#endif
	ComputePipelineReflection& ComputePipelineReflection::operator=(const AutoReleasedComputePipelineReflection& rhs)
	{
		if (m_ptr != rhs.m_ptr)
		{
			if(rhs.m_ptr)
			{
				rhs.m_table->Retain(rhs.m_ptr);
			}
			if(m_ptr)
			{
				m_table->Release(m_ptr);
			}
			m_ptr = rhs.m_ptr;
			m_table = rhs.m_table;
		}
		return *this;
	}
#if MTLPP_CONFIG_RVALUE_REFERENCES
	ComputePipelineReflection& ComputePipelineReflection::operator=(AutoReleasedComputePipelineReflection&& rhs)
	{
		if (m_ptr != rhs.m_ptr)
		{
			if(rhs.m_ptr)
			{
				rhs.m_table->Retain(rhs.m_ptr);
			}
			if(m_ptr)
			{
				m_table->Release(m_ptr);
			}
			m_ptr = rhs.m_ptr;
			m_table = rhs.m_table;
			rhs.m_ptr = nullptr;
			rhs.m_table = nullptr;
		}
		return *this;
	}
#endif
	

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
		return ns::Array<PipelineBufferDescriptor>((NSArray<MTLPipelineBufferDescriptor*>*)[(MTLComputePipelineDescriptor*)m_ptr buffers]);
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

    Device ComputePipelineState::GetDevice() const
    {
        Validate();
		return m_table->Device(m_ptr);
    }

    NSUInteger ComputePipelineState::GetMaxTotalThreadsPerThreadgroup() const
    {
        Validate();
		return m_table->maxTotalThreadsPerThreadgroup(m_ptr);
    }

    NSUInteger ComputePipelineState::GetThreadExecutionWidth() const
    {
        Validate();
		return m_table->threadExecutionWidth(m_ptr);
    }
	
	NSUInteger ComputePipelineState::GetStaticThreadgroupMemoryLength() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return m_table->staticThreadgroupMemoryLength(m_ptr);
#else
		return 0;
#endif
	}
	
	NSUInteger ComputePipelineState::GetImageblockMemoryLengthForDimensions(Size const& imageblockDimensions)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return m_table->imageblockMemoryLengthForDimensions(m_ptr, imageblockDimensions);
#else
		return 0;
#endif
	}
}

MTLPP_END
