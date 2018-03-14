/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_ComputePipeline.hpp"
#include "device.hpp"
#include "argument.hpp"
#include "pipeline.hpp"
#include "stage_input_output_descriptor.hpp"

MTLPP_BEGIN

namespace mtlpp
{
	class PipelineBufferDescriptor;
	
    class AutoReleasedComputePipelineReflection : public ns::Object<MTLComputePipelineReflection*, true>
    {
		AutoReleasedComputePipelineReflection(const AutoReleasedComputePipelineReflection& rhs) = delete;
#if MTLPP_CONFIG_RVALUE_REFERENCES
		AutoReleasedComputePipelineReflection(AutoReleasedComputePipelineReflection&& rhs) = delete;
#endif
		AutoReleasedComputePipelineReflection& operator=(const AutoReleasedComputePipelineReflection& rhs) = delete;
#if MTLPP_CONFIG_RVALUE_REFERENCES
		AutoReleasedComputePipelineReflection& operator=(AutoReleasedComputePipelineReflection&& rhs) = delete;
#endif
		
		friend class ComputePipelineReflection;
    public:
        AutoReleasedComputePipelineReflection();
        AutoReleasedComputePipelineReflection(MTLComputePipelineReflection* handle) : ns::Object<MTLComputePipelineReflection*, true>(handle) { }

        ns::Array<Argument> GetArguments() const;
    }
    MTLPP_AVAILABLE(10_11, 9_0);
	
	class ComputePipelineReflection : public ns::Object<MTLComputePipelineReflection*>
	{
	public:
		ComputePipelineReflection();
		ComputePipelineReflection(MTLComputePipelineReflection* handle) : ns::Object<MTLComputePipelineReflection*>(handle) { }
		ComputePipelineReflection(const AutoReleasedComputePipelineReflection& rhs);
#if MTLPP_CONFIG_RVALUE_REFERENCES
		ComputePipelineReflection(const AutoReleasedComputePipelineReflection&& rhs);
#endif
		ComputePipelineReflection& operator=(const AutoReleasedComputePipelineReflection& rhs);
#if MTLPP_CONFIG_RVALUE_REFERENCES
		ComputePipelineReflection& operator=(AutoReleasedComputePipelineReflection&& rhs);
#endif
		
		ns::Array<Argument> GetArguments() const;
	}
	MTLPP_AVAILABLE(10_11, 9_0);

    class ComputePipelineDescriptor : public ns::Object<MTLComputePipelineDescriptor*>
    {
    public:
        ComputePipelineDescriptor();
        ComputePipelineDescriptor(MTLComputePipelineDescriptor* handle) : ns::Object<MTLComputePipelineDescriptor*>(handle) { }

        ns::String                 GetLabel() const;
        Function                   GetComputeFunction() const;
        bool                       GetThreadGroupSizeIsMultipleOfThreadExecutionWidth() const;
        StageInputOutputDescriptor GetStageInputDescriptor() const MTLPP_AVAILABLE(10_12, 10_0);
		ns::Array<PipelineBufferDescriptor> GetBuffers() const MTLPP_AVAILABLE(10_13, 11_0);

        void SetLabel(const ns::String& label);
        void SetComputeFunction(const Function& function);
        void SetThreadGroupSizeIsMultipleOfThreadExecutionWidth(bool value);
        void SetStageInputDescriptor(const StageInputOutputDescriptor& stageInputDescriptor) const MTLPP_AVAILABLE(10_12, 10_0);

        void Reset();
    }
    MTLPP_AVAILABLE(10_11, 9_0);

    class ComputePipelineState : public ns::Object<ns::Protocol<id<MTLComputePipelineState>>::type>
    {
    public:
        ComputePipelineState() { }
        ComputePipelineState(ns::Protocol<id<MTLComputePipelineState>>::type handle) : ns::Object<ns::Protocol<id<MTLComputePipelineState>>::type>(handle) { }

        Device   GetDevice() const;
        NSUInteger GetMaxTotalThreadsPerThreadgroup() const;
        NSUInteger GetThreadExecutionWidth() const;
		NSUInteger GetStaticThreadgroupMemoryLength() const MTLPP_AVAILABLE(10_13, 11_0);
		
		NSUInteger GetImageblockMemoryLengthForDimensions(Size const& imageblockDimensions) MTLPP_AVAILABLE_IOS(11_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);
}

MTLPP_END
