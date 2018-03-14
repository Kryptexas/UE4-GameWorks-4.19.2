/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLBuffer.h>
#include "buffer.hpp"
#include "texture.hpp"

MTLPP_BEGIN

namespace mtlpp
{
	Buffer::Buffer(ns::Protocol<id<MTLBuffer>>::type handle)
	: Resource((ns::Protocol<id<MTLResource>>::type)handle)
	{
	}
	
    NSUInteger Buffer::GetLength() const
    {
        Validate();
		return (NSUInteger)((IMPTable<id<MTLBuffer>, void>*)m_table)->Length((id<MTLBuffer>)m_ptr);
    }

    void* Buffer::GetContents()
    {
        Validate();
		return ((IMPTable<id<MTLBuffer>, void>*)m_table)->Contents((id<MTLBuffer>)m_ptr);
    }

    void Buffer::DidModify(const ns::Range& range)
    {
        Validate();
#if MTLPP_PLATFORM_MAC
		((IMPTable<id<MTLBuffer>, void>*)m_table)->DidModifyRange((id<MTLBuffer>)m_ptr, NSMakeRange(range.Location, range.Length));
#endif
    }

    Texture Buffer::NewTexture(const TextureDescriptor& descriptor, NSUInteger offset, NSUInteger bytesPerRow)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 8_0)
		MTLTextureDescriptor* mtlTextureDescriptor = (MTLTextureDescriptor*)descriptor.GetPtr();
		return ((IMPTable<id<MTLBuffer>, void>*)m_table)->NewTextureWithDescriptorOffsetBytesPerRow((id<MTLBuffer>)m_ptr, mtlTextureDescriptor, offset, bytesPerRow);
#else
        return nullptr;
#endif
    }

    void Buffer::AddDebugMarker(const ns::String& marker, const ns::Range& range)
    {
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
		((IMPTable<id<MTLBuffer>, void>*)m_table)->AddDebugMarkerRange((id<MTLBuffer>)m_ptr, marker.GetPtr(), NSMakeRange(range.Location, range.Length));
#endif
    }

    void Buffer::RemoveAllDebugMarkers()
    {
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
		((IMPTable<id<MTLBuffer>, void>*)m_table)->RemoveAllDebugMarkers((id<MTLBuffer>)m_ptr);
#endif
    }
}

MTLPP_END
