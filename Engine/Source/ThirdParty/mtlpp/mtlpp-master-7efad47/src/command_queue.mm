/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLCommandQueue.h>
#include "command_queue.hpp"
#include "command_buffer.hpp"
#include "device.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    ns::String CommandQueue::GetLabel() const
    {
        Validate();
		return m_table->Label(m_ptr);
    }

    Device CommandQueue::GetDevice() const
    {
        Validate();
		return m_table->Device(m_ptr);
    }

    void CommandQueue::SetLabel(const ns::String& label)
    {
        Validate();
		m_table->SetLabel(m_ptr, label.GetPtr());
    }

    CommandBuffer CommandQueue::CommandBufferWithUnretainedReferences()
    {
        Validate();
		return m_table->CommandBufferWithUnretainedReferences(m_ptr);
    }

    CommandBuffer CommandQueue::CommandBuffer()
    {
        Validate();
		return m_table->CommandBuffer(m_ptr);
    }

    void CommandQueue::InsertDebugCaptureBoundary()
    {
        Validate();
		m_table->InsertDebugCaptureBoundary(m_ptr);
    }
}

MTLPP_END
