/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_Buffer.hpp"
#include "pixel_format.hpp"
#include "resource.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    class Texture;
    class TextureDescriptor;

    class Buffer : public Resource
    {
    public:
        Buffer() { }
		Buffer(ns::Protocol<id<MTLBuffer>>::type handle);

		inline const ns::Protocol<id<MTLBuffer>>::type GetPtr() const { return (ns::Protocol<id<MTLBuffer>>::type)m_ptr; }
		inline ns::Protocol<id<MTLBuffer>>::type* GetInnerPtr() { return (ns::Protocol<id<MTLBuffer>>::type*)&m_ptr; }
		operator ns::Protocol<id<MTLBuffer>>::type() const { return (ns::Protocol<id<MTLBuffer>>::type)m_ptr; }
		
        NSUInteger GetLength() const;
        void*    GetContents();
        void     DidModify(const ns::Range& range) MTLPP_AVAILABLE_MAC(10_11);
        Texture  NewTexture(const TextureDescriptor& descriptor, NSUInteger offset, NSUInteger bytesPerRow) MTLPP_AVAILABLE(10_13, 8_0);
        void     AddDebugMarker(const ns::String& marker, const ns::Range& range) MTLPP_AVAILABLE(10_12, 10_0);
        void     RemoveAllDebugMarkers() MTLPP_AVAILABLE(10_12, 10_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);
}

MTLPP_END
