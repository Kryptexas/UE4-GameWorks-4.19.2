// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include <Metal/MTLCaptureManager.h>
#include <Metal/MTLCaptureScope.h>
#include <Metal/MTLDevice.h>
#include <Metal/MTLCommandQueue.h>
#include "capture_manager.hpp"
#include "capture_scope.hpp"
#include "device.hpp"
#include "command_queue.hpp"

MTLPP_BEGIN

namespace mtlpp
{
	CaptureManager& CaptureManager::SharedCaptureManager()
	{
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		static CaptureManager CaptureMan([MTLCaptureManager sharedCaptureManager]);
#else
		static CaptureManager;
#endif
		return CaptureMan;
	}
	
	CaptureScope CaptureManager::NewCaptureScopeWithDevice(Device Device)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return m_table->newCaptureScopeWithDevice(m_ptr, Device.GetPtr());
#else
		return CaptureScope();
#endif
	}
	
	CaptureScope CaptureManager::NewCaptureScopeWithCommandQueue(CommandQueue Queue)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return m_table->newCaptureScopeWithCommandQueue(m_ptr, Queue.GetPtr());
#else
		return CaptureScope();
#endif
	}
	
	void CaptureManager::StartCaptureWithDevice(Device device)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->startCaptureWithDevice(m_ptr, device.GetPtr());
#endif
	}
	
	void CaptureManager::StartCaptureWithCommandQueue(CommandQueue queue)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->startCaptureWithCommandQueue(m_ptr, queue.GetPtr());
#endif
	}
	
	void CaptureManager::StartCaptureWithScope(CaptureScope scope)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->startCaptureWithScope(m_ptr, scope.GetPtr());
#endif
	}
	
	void CaptureManager::StopCapture()
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->stopCapture(m_ptr);
#endif
	}
	
	CaptureScope CaptureManager::GetDefaultCaptureScope() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return m_table->defaultCaptureScope(m_ptr);
#else
		return CaptureScope();
#endif
	}
	
	void CaptureManager::SetDefaultCaptureScope(CaptureScope scope)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->SetdefaultCaptureScope(m_ptr, scope.GetPtr());
#endif
	}
	
	bool CaptureManager::IsCapturing() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return m_table->isCapturing(m_ptr);
#else
		return false;
#endif
	}
}

MTLPP_END

