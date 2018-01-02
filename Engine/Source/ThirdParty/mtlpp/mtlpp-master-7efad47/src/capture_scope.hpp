// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once


#include "declare.hpp"
#include "imp_CaptureScope.hpp"
#include "ns.hpp"

MTLPP_BEGIN

namespace mtlpp
{
	class Device;
	class CommandQueue;
	
	class CaptureScope : public ns::Object<ns::Protocol<id<MTLCaptureScope>>::type>
	{
	public:
		CaptureScope() { }
		CaptureScope(ns::Protocol<id<MTLCaptureScope>>::type handle) : ns::Object<ns::Protocol<id<MTLCaptureScope>>::type>(handle) { }
		
		void BeginScope();
		void EndScope();

		ns::String   GetLabel() const;
		void SetLabel(const ns::String& label);
		
		Device GetDevice() const;
		CommandQueue GetCommandQueue() const;
	} MTLPP_AVAILABLE(10_13, 11_0);
	
}

MTLPP_END
