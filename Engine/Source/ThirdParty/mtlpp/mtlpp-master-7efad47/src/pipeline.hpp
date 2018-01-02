// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once


#include "declare.hpp"
#include "device.hpp"

MTLPP_BEGIN

namespace mtlpp
{
	enum class Mutability
	{
		Default   = 0,
		Mutable   = 1,
		Immutable = 2,
	}
	MTLPP_AVAILABLE(10_13, 11_0);
	
	class PipelineBufferDescriptor : public ns::Object<MTLPipelineBufferDescriptor*>
	{
	public:
		PipelineBufferDescriptor();
		PipelineBufferDescriptor(MTLPipelineBufferDescriptor* h) : ns::Object<MTLPipelineBufferDescriptor*>(h) {}
		
		void SetMutability(Mutability m);
		Mutability GetMutability() const;
	}
	MTLPP_AVAILABLE(10_13, 11_0);
}

MTLPP_END
