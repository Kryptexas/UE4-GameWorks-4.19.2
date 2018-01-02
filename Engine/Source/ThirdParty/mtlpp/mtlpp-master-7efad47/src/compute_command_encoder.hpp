/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_ComputeCommandEncoder.hpp"
#include "ns.hpp"
#include "command_encoder.hpp"
#include "texture.hpp"
#include "command_buffer.hpp"
#include "fence.hpp"
#include "heap.hpp"
#include "sampler.hpp"
#include "texture.hpp"
#include "buffer.hpp"

MTLPP_BEGIN

namespace mtlpp
{
	class ComputeCommandEncoder : public CommandEncoder<ns::Protocol<id<MTLComputeCommandEncoder>>::type>
    {
    public:
        ComputeCommandEncoder() { }
        ComputeCommandEncoder(ns::Protocol<id<MTLComputeCommandEncoder>>::type handle) : CommandEncoder<ns::Protocol<id<MTLComputeCommandEncoder>>::type>(handle) { }

        void SetComputePipelineState(const ComputePipelineState& state);
        void SetBytes(const void* data, NSUInteger length, NSUInteger index) MTLPP_AVAILABLE(10_11, 8_3);
        void SetBuffer(const Buffer& buffer, NSUInteger offset, NSUInteger index);
        void SetBufferOffset(NSUInteger offset, NSUInteger index) MTLPP_AVAILABLE(10_11, 8_3);
        void SetBuffers(const Buffer::Type* buffers, const uint64_t* offsets, const ns::Range& range);
        void SetTexture(const Texture& texture, NSUInteger index);
        void SetTextures(const Texture::Type* textures, const ns::Range& range);
        void SetSamplerState(const SamplerState& sampler, NSUInteger index);
        void SetSamplerStates(const SamplerState::Type* samplers, const ns::Range& range);
        void SetSamplerState(const SamplerState& sampler, float lodMinClamp, float lodMaxClamp, NSUInteger index);
        void SetSamplerStates(const SamplerState::Type* samplers, const float* lodMinClamps, const float* lodMaxClamps, const ns::Range& range);
        void SetThreadgroupMemory(NSUInteger length, NSUInteger index);
		void SetImageblock(NSUInteger width, NSUInteger height) MTLPP_AVAILABLE_IOS(11_0);
        void SetStageInRegion(const Region& region) MTLPP_AVAILABLE(10_12, 10_0);
        void DispatchThreadgroups(const Size& threadgroupsPerGrid, const Size& threadsPerThreadgroup);
        void DispatchThreadgroupsWithIndirectBuffer(const Buffer& indirectBuffer, NSUInteger indirectBufferOffset, const Size& threadsPerThreadgroup) MTLPP_AVAILABLE(10_11, 9_0);
		void DispatchThreads(const Size& threadsPerGrid, const Size& threadsPerThreadgroup) MTLPP_AVAILABLE(10_13, 11_0);
        void UpdateFence(const Fence& fence) MTLPP_AVAILABLE(10_13, 10_0);
        void WaitForFence(const Fence& fence) MTLPP_AVAILABLE(10_13, 10_0);
		void UseResource(const Resource& resource, ResourceUsage usage) MTLPP_AVAILABLE(10_13, 11_0);
		void UseResources(const Resource::Type* resource, NSUInteger count, ResourceUsage usage) MTLPP_AVAILABLE(10_13, 11_0);
		void UseHeap(const Heap& heap) MTLPP_AVAILABLE(10_13, 11_0);
		void UseHeaps(const Heap::Type* heap, NSUInteger count) MTLPP_AVAILABLE(10_13, 11_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);
}

MTLPP_END
