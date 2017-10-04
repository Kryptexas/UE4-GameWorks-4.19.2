/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include "buffer.hpp"
#include "texture.hpp"
#include <Metal/MTLBuffer.h>

namespace mtlpp
{
    uint32_t Buffer::GetLength() const
    {
        Validate();
        return uint32_t([(id<MTLBuffer>)m_ptr length]);
    }

    void* Buffer::GetContents()
    {
        Validate();
        return [(id<MTLBuffer>)m_ptr contents];
    }

    void Buffer::DidModify(const ns::Range& range)
    {
        Validate();
#if MTLPP_PLATFORM_MAC
        [(id<MTLBuffer>)m_ptr didModifyRange:NSMakeRange(range.Location, range.Length)];
#endif
    }

    Texture Buffer::NewTexture(const TextureDescriptor& descriptor, uint32_t offset, uint32_t bytesPerRow)
    {
        Validate();
		if (@available(macOS 10.13, iOS 8.0, *))
		{
			MTLTextureDescriptor* mtlTextureDescriptor = (MTLTextureDescriptor*)descriptor.GetPtr();
			return [(id<MTLBuffer>)m_ptr newTextureWithDescriptor:mtlTextureDescriptor offset:offset bytesPerRow:bytesPerRow];
		}
        return nullptr;
    }

    void Buffer::AddDebugMarker(const ns::String& marker, const ns::Range& range)
    {
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        [(id<MTLBuffer>)m_ptr addDebugMarker:(NSString*)marker.GetPtr() range:NSMakeRange(range.Location, range.Length)];
#endif
    }

    void Buffer::RemoveAllDebugMarkers()
    {
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        [(id<MTLBuffer>)m_ptr removeAllDebugMarkers];
#endif
    }
}
