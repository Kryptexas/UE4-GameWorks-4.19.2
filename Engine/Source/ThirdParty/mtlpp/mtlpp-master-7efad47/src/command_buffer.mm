/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLCommandBuffer.h>
#include "command_buffer.hpp"
#include "command_queue.hpp"
#include "drawable.hpp"
#include "blit_command_encoder.hpp"
#include "render_command_encoder.hpp"
#include "compute_command_encoder.hpp"
#include "parallel_render_command_encoder.hpp"
#include "render_pass.hpp"

MTLPP_BEGIN


namespace mtlpp
{
    Device CommandBuffer::GetDevice() const
    {
        Validate();
		return m_table->Device(m_ptr);
    }

    CommandQueue CommandBuffer::GetCommandQueue() const
    {
        Validate();
		return m_table->CommandQueue(m_ptr);
    }

    bool CommandBuffer::GetRetainedReferences() const
    {
        Validate();
		return m_table->RetainedReferences(m_ptr);
    }

    ns::String CommandBuffer::GetLabel() const
    {
        Validate();
		return m_table->Label(m_ptr);
    }

    CommandBufferStatus CommandBuffer::GetStatus() const
    {
        Validate();
		return (CommandBufferStatus)m_table->Status(m_ptr);
    }

    ns::Error CommandBuffer::GetError() const
    {
        Validate();
		return m_table->Error(m_ptr);
    }

    double CommandBuffer::GetKernelStartTime() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_3)
		return m_table->KernelStartTime(m_ptr);
#else
		return 0.0;
#endif
	}

    double CommandBuffer::GetKernelEndTime() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_3)
		return m_table->KernelEndTime(m_ptr);
#else
        return 0.0;
#endif
    }

    double CommandBuffer::GetGpuStartTime() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_3)
		return m_table->GPUStartTime(m_ptr);
#else
        return 0.0;
#endif
    }

    double CommandBuffer::GetGpuEndTime() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_3)
		return m_table->GPUEndTime(m_ptr);
#else
        return 0.0;
#endif
    }

    void CommandBuffer::SetLabel(const ns::String& label)
    {
        Validate();
		m_table->SetLabel(m_ptr, (NSString*)label.GetPtr());
    }

    void CommandBuffer::Enqueue()
    {
        Validate();
		m_table->Enqueue(m_ptr);
    }

    void CommandBuffer::Commit()
    {
        Validate();
		m_table->Commit(m_ptr);
    }

    void CommandBuffer::AddScheduledHandler(CommandBufferHandler handler)
    {
        Validate();
		ITable* table = m_table;
		m_table->AddScheduledHandler(m_ptr, ^(id <MTLCommandBuffer> mtlCommandBuffer){
            CommandBuffer commandBuffer(mtlCommandBuffer, table);
            handler(commandBuffer);
        });
    }

    void CommandBuffer::AddCompletedHandler(CommandBufferHandler handler)
    {
        Validate();
		ITable* table = m_table;
		m_table->AddCompletedHandler(m_ptr, ^(id <MTLCommandBuffer> mtlCommandBuffer){
            CommandBuffer commandBuffer(mtlCommandBuffer, table);
            handler(commandBuffer);
        });
    }

    void CommandBuffer::Present(const Drawable& drawable)
    {
        Validate();
		m_table->PresentDrawable(m_ptr, (id<MTLDrawable>)drawable.GetPtr());
    }

    void CommandBuffer::PresentAtTime(const Drawable& drawable, double presentationTime)
    {
        Validate();
		m_table->PresentDrawableAtTime(m_ptr, (id<MTLDrawable>)drawable.GetPtr(), presentationTime);
    }

    void CommandBuffer::PresentAfterMinimumDuration(const Drawable& drawable, double duration)
    {
        Validate();
#if MTLPP_PLATFORM_IOS
        [(id<MTLCommandBuffer>)m_ptr presentDrawable:(id<MTLDrawable>)drawable.GetPtr() afterMinimumDuration:duration];
#endif
    }

    void CommandBuffer::WaitUntilScheduled()
    {
        Validate();
		m_table->WaitUntilScheduled(m_ptr);
    }

    void CommandBuffer::WaitUntilCompleted()
    {
        Validate();
		m_table->WaitUntilCompleted(m_ptr);
    }

    BlitCommandEncoder CommandBuffer::BlitCommandEncoder()
    {
        Validate();
		return m_table->BlitCommandEncoder(m_ptr);
    }

    RenderCommandEncoder CommandBuffer::RenderCommandEncoder(const RenderPassDescriptor& renderPassDescriptor)
    {
        Validate();
        MTLRenderPassDescriptor* mtlRenderPassDescriptor = (MTLRenderPassDescriptor*)renderPassDescriptor.GetPtr();
		return m_table->RenderCommandEncoderWithDescriptor(m_ptr, mtlRenderPassDescriptor);
    }

    ComputeCommandEncoder CommandBuffer::ComputeCommandEncoder()
    {
        Validate();
		return m_table->ComputeCommandEncoder(m_ptr);
    }

    ParallelRenderCommandEncoder CommandBuffer::ParallelRenderCommandEncoder(const RenderPassDescriptor& renderPassDescriptor)
    {
        Validate();
        MTLRenderPassDescriptor* mtlRenderPassDescriptor = (MTLRenderPassDescriptor*)renderPassDescriptor.GetPtr();
		return m_table->ParallelRenderCommandEncoderWithDescriptor(m_ptr, mtlRenderPassDescriptor);
    }
	
	void CommandBuffer::PushDebugGroup(const ns::String& string)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->PushDebugGroup(m_ptr, string.GetPtr());
#endif
	}
	
	void CommandBuffer::PopDebugGroup()
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->PopDebugGroup(m_ptr);
#endif
	}
}

MTLPP_END
