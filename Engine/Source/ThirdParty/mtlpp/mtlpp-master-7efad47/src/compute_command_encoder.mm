/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLComputeCommandEncoder.h>
#include <Metal/MTLHeap.h>
#include <Metal/MTLResource.h>
#include "compute_command_encoder.hpp"
#include "buffer.hpp"
#include "compute_pipeline.hpp"
#include "sampler.hpp"
#include "heap.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    void ComputeCommandEncoder::SetComputePipelineState(const ComputePipelineState& state)
    {
        Validate();
		m_table->Setcomputepipelinestate(m_ptr, state.GetPtr());
    }

    void ComputeCommandEncoder::SetBytes(const void* data, NSUInteger length, NSUInteger index)
    {
        Validate();
		m_table->Setbyteslengthatindex(m_ptr, data, length, index);
    }

    void ComputeCommandEncoder::SetBuffer(const Buffer& buffer, NSUInteger offset, NSUInteger index)
    {
        Validate();
		m_table->Setbufferoffsetatindex(m_ptr, (id<MTLBuffer>)buffer.GetPtr(), offset, index);
    }

    void ComputeCommandEncoder::SetBufferOffset(NSUInteger offset, NSUInteger index)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 8_3)
		m_table->SetBufferOffsetatindex(m_ptr, offset, index);
#endif
    }

    void ComputeCommandEncoder::SetBuffers(const Buffer::Type* buffers, const uint64_t* offsets, const ns::Range& range)
    {
        Validate();
		m_table->Setbuffersoffsetswithrange(m_ptr, (id<MTLBuffer>*)buffers, (NSUInteger const*)offsets, NSMakeRange(range.Location, range.Length));
    }

    void ComputeCommandEncoder::SetTexture(const Texture& texture, NSUInteger index)
    {
        Validate();
		
		m_table->Settextureatindex(m_ptr, (id<MTLTexture>)texture.GetPtr(), index);
    }

    void ComputeCommandEncoder::SetTextures(const Texture::Type* textures, const ns::Range& range)
    {
        Validate();
		m_table->Settextureswithrange(m_ptr, (id<MTLTexture>*)textures, NSMakeRange(range.Location, range.Length));
    }

    void ComputeCommandEncoder::SetSamplerState(const SamplerState& sampler, NSUInteger index)
    {
        Validate();
		m_table->Setsamplerstateatindex(m_ptr, sampler.GetPtr(), index);
    }

    void ComputeCommandEncoder::SetSamplerStates(const SamplerState::Type* samplers, const ns::Range& range)
    {
        Validate();
		m_table->Setsamplerstateswithrange(m_ptr, samplers, NSMakeRange(range.Location, range.Length));
    }

    void ComputeCommandEncoder::SetSamplerState(const SamplerState& sampler, float lodMinClamp, float lodMaxClamp, NSUInteger index)
    {
        Validate();
		
		m_table->Setsamplerstatelodminclamplodmaxclampatindex(m_ptr, (id<MTLSamplerState>)sampler.GetPtr(), lodMinClamp, lodMaxClamp, index);
    }

    void ComputeCommandEncoder::SetSamplerStates(const SamplerState::Type* samplers, const float* lodMinClamps, const float* lodMaxClamps, const ns::Range& range)
    {
        Validate();
		m_table->Setsamplerstateslodminclampslodmaxclampswithrange(m_ptr, samplers, lodMinClamps, lodMaxClamps, NSMakeRange(range.Location, range.Length));
    }

    void ComputeCommandEncoder::SetThreadgroupMemory(NSUInteger length, NSUInteger index)
    {
        Validate();
		m_table->Setthreadgroupmemorylengthatindex(m_ptr, length, index);
    }
	
	void ComputeCommandEncoder::SetImageblock(NSUInteger width, NSUInteger height)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		m_table->SetImageblockWidthHeight(m_ptr, width, height);
#endif
	}

    void ComputeCommandEncoder::SetStageInRegion(const Region& region)
    {
		Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
		m_table->Setstageinregion(m_ptr, region);
#endif
    }

    void ComputeCommandEncoder::DispatchThreadgroups(const Size& threadgroupsPerGrid, const Size& threadsPerThreadgroup)
    {
        Validate();
		m_table->Dispatchthreadgroupsthreadsperthreadgroup(m_ptr, threadgroupsPerGrid, threadsPerThreadgroup);
    }

    void ComputeCommandEncoder::DispatchThreadgroupsWithIndirectBuffer(const Buffer& indirectBuffer, NSUInteger indirectBufferOffset, const Size& threadsPerThreadgroup)
    {
        Validate();
		m_table->Dispatchthreadgroupswithindirectbufferindirectbufferoffsetthreadsperthreadgroup(m_ptr, (id<MTLBuffer>)indirectBuffer.GetPtr(), indirectBufferOffset, threadsPerThreadgroup);
    }
	
	void ComputeCommandEncoder::DispatchThreads(const Size& threadsPerGrid, const Size& threadsPerThreadgroup)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->Dispatchthreadsthreadsperthreadgroup(m_ptr, threadsPerGrid, threadsPerThreadgroup);
#endif
	}

    void ComputeCommandEncoder::UpdateFence(const Fence& fence)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
			m_table->Updatefence(m_ptr, fence.GetPtr());
#endif
    }

    void ComputeCommandEncoder::WaitForFence(const Fence& fence)
    {
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
		m_table->Waitforfence(m_ptr, fence.GetPtr());
#endif
    }
	
	void ComputeCommandEncoder::UseResource(const Resource& resource, ResourceUsage usage)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->Useresourceusage(m_ptr, resource.GetPtr(), MTLResourceUsage(usage));
#endif
	}

	void ComputeCommandEncoder::UseResources(const Resource::Type* resource, NSUInteger count, ResourceUsage usage)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->Useresourcescountusage(m_ptr, resource, count, MTLResourceUsage(usage));
#endif
	}
	
	void ComputeCommandEncoder::UseHeap(const Heap& heap)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->Useheap(m_ptr, heap.GetPtr());
#endif
	}
	
	void ComputeCommandEncoder::UseHeaps(const Heap::Type* heap, NSUInteger count)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->Useheapscount(m_ptr, heap, count);
#endif
	}
}

MTLPP_END
