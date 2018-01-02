// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include <Metal/MTLArgumentEncoder.h>
#include "argument_encoder.hpp"
#include "device.hpp"
#include "buffer.hpp"
#include "texture.hpp"
#include "sampler.hpp"

MTLPP_BEGIN

namespace mtlpp
{
	Device     ArgumentEncoder::GetDevice() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return m_table->Device(m_ptr);
#else
		return Device();
#endif
	}
	
	ns::String ArgumentEncoder::GetLabel() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return m_table->Label(m_ptr);
#else
		return ns::String();
#endif
	}
	
	NSUInteger ArgumentEncoder::GetEncodedLength() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return m_table->EncodedLength(m_ptr);
#else
		return 0;
#endif
	}

	NSUInteger ArgumentEncoder::GetAlignment() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return m_table->Alignment(m_ptr);
#else
		return 0;
#endif
	}
	
	void* ArgumentEncoder::GetConstantDataAtIndex(NSUInteger index) const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return m_table->ConstantDataAtIndex(m_ptr, index);
#else
		return nullptr;
#endif
	}
	
	void ArgumentEncoder::SetLabel(const ns::String& label)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->SetLabel(m_ptr, label.GetPtr());
#endif
	}
	
	void ArgumentEncoder::SetArgumentBuffer(const Buffer& buffer, NSUInteger offset)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->Setargumentbufferoffset(m_ptr, (id<MTLBuffer>)buffer.GetPtr(), offset);
#endif
	}
	
	void ArgumentEncoder::SetArgumentBuffer(const Buffer& buffer, NSUInteger offset, NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->Setargumentbufferstartoffsetarrayelement(m_ptr, (id<MTLBuffer>)buffer.GetPtr(), offset, index);
#endif
	}
	
	void ArgumentEncoder::SetBuffer(const Buffer& buffer, NSUInteger offset, NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->Setbufferoffsetatindex(m_ptr, (id<MTLBuffer>)buffer.GetPtr(), offset, index);
#endif
	}
	
	void ArgumentEncoder::SetBuffers(const Buffer* buffers, const NSUInteger* offsets, const ns::Range& range)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		id<MTLBuffer>* array = (id<MTLBuffer>*)alloca(range.Length * sizeof(id<MTLBuffer>));
		for (NSUInteger i = 0; i < range.Length; i++)
			array[i] = (id<MTLBuffer>)buffers[i].GetPtr();
		
		m_table->Setbuffersoffsetswithrange(m_ptr, array, (NSUInteger*)offsets, NSMakeRange(range.Location, range.Length));
#endif
	}
	
	void ArgumentEncoder::SetTexture(const Texture& texture, NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->Settextureatindex(m_ptr, (id<MTLTexture>)texture.GetPtr(), index);
#endif
	}
	
	void ArgumentEncoder::SetTextures(const Texture* textures, const ns::Range& range)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		id<MTLTexture>* array = (id<MTLTexture>*)alloca(range.Length * sizeof(id<MTLTexture>));
		for (NSUInteger i = 0; i < range.Length; i++)
			array[i] = (id<MTLTexture>)textures[i].GetPtr();

		m_table->Settextureswithrange(m_ptr, array, NSMakeRange(range.Location, range.Length));
#endif
	}
	
	void ArgumentEncoder::SetSamplerState(const SamplerState& sampler, NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->Setsamplerstateatindex(m_ptr, (id<MTLSamplerState>)sampler.GetPtr(), index);
#endif
	}
	
	void ArgumentEncoder::SetSamplerStates(const SamplerState* samplers, const ns::Range& range)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		id<MTLSamplerState>* array = (id<MTLSamplerState>*)alloca(range.Length * sizeof(id<MTLSamplerState>));
		for (NSUInteger i = 0; i < range.Length; i++)
			array[i] = (id<MTLSamplerState>)samplers[i].GetPtr();

		m_table->Setsamplerstateswithrange(m_ptr, array, NSMakeRange(range.Location, range.Length));
#endif
	}
	
	ArgumentEncoder ArgumentEncoder::NewArgumentEncoderForBufferAtIndex(NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_13)
		return m_table->NewArgumentEncoderForBufferAtIndex(m_ptr, index);
#else
		return ArgumentEncoder();
#endif
	}
}

MTLPP_END
