// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once


#include "declare.hpp"
#include "imp_ArgumentEncoder.hpp"
#include "ns.hpp"

MTLPP_BEGIN

namespace mtlpp
{
	class Device;
	class Buffer;
	class Texture;
	class SamplerState;
	
	class ArgumentEncoder : public ns::Object<ns::Protocol<id<MTLArgumentEncoder>>::type>
	{
	public:
		ArgumentEncoder() { }
		ArgumentEncoder(ns::Protocol<id<MTLArgumentEncoder>>::type handle) : ns::Object<ns::Protocol<id<MTLArgumentEncoder>>::type>(handle) { }
		
		Device     GetDevice() const;
		ns::String GetLabel() const;
		NSUInteger GetEncodedLength() const;
		NSUInteger GetAlignment() const;
		void* GetConstantDataAtIndex(NSUInteger index) const;
		
		void SetLabel(const ns::String& label);
		
		void SetArgumentBuffer(const Buffer& buffer, NSUInteger offset);
		void SetArgumentBuffer(const Buffer& buffer, NSUInteger offset, NSUInteger index);
		
		void SetBuffer(const Buffer& buffer, NSUInteger offset, NSUInteger index);
		void SetBuffers(const Buffer* buffers, const NSUInteger* offsets, const ns::Range& range);
		void SetTexture(const Texture& texture, NSUInteger index);
		void SetTextures(const Texture* textures, const ns::Range& range);
		void SetSamplerState(const SamplerState& sampler, NSUInteger index);
		void SetSamplerStates(const SamplerState* samplers, const ns::Range& range);

		ArgumentEncoder NewArgumentEncoderForBufferAtIndex(NSUInteger index) MTLPP_AVAILABLE_MAC(10_13);
	}
	MTLPP_AVAILABLE(10_13, 11_0);
}

MTLPP_END
