/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include "command_queue.hpp"
#include "command_buffer.hpp"
#include "device.hpp"
#include <Metal/MTLCommandQueue.h>

namespace mtlpp
{
    ns::String CommandQueue::GetLabel() const
    {
        Validate();
        return [(id<MTLCommandQueue>)m_ptr label];
    }

    Device CommandQueue::GetDevice() const
    {
        Validate();
        return [(id<MTLCommandQueue>)m_ptr device];
    }

    void CommandQueue::SetLabel(const ns::String& label)
    {
        Validate();
        [(id<MTLCommandQueue>)m_ptr setLabel:(NSString*)label.GetPtr()];
    }

    CommandBuffer CommandQueue::CommandBufferWithUnretainedReferences()
    {
        Validate();
        return [(id<MTLCommandQueue>)m_ptr commandBufferWithUnretainedReferences];
    }

    CommandBuffer CommandQueue::CommandBuffer()
    {
        Validate();
        return [(id<MTLCommandQueue>)m_ptr commandBuffer];
    }

    void CommandQueue::InsertDebugCaptureBoundary()
    {
        Validate();
        [(id<MTLCommandQueue>)m_ptr insertDebugCaptureBoundary];
    }
}
