// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include <Metal/MTLCaptureScope.h>
#include <Metal/MTLDevice.h>
#include <Metal/MTLCommandQueue.h>
#include <Foundation/NSString.h>
#include "capture_scope.hpp"
#include "device.hpp"
#include "command_queue.hpp"

MTLPP_BEGIN

namespace mtlpp
{
	void CaptureScope::BeginScope()
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->beginScope(m_ptr);
#endif
	}
	
	void CaptureScope::EndScope()
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->endScope(m_ptr);
#endif
	}
	
	ns::String   CaptureScope::GetLabel() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return m_table->label(m_ptr);
#else
		return ns::String();
#endif
	}
	
	void CaptureScope::SetLabel(const ns::String& label)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->Setlabel(m_ptr, label.GetPtr());
#endif
	}
	
	Device CaptureScope::GetDevice() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return m_table->device(m_ptr);
#else
		return Device();
#endif
	}
	
	CommandQueue CaptureScope::GetCommandQueue() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return m_table->commandQueue(m_ptr);
#else
		return CommandQueue();
#endif
	}
}

MTLPP_END
