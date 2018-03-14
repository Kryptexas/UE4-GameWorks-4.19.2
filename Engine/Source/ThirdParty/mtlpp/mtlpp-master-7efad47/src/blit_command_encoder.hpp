/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_BlitCommandEncoder.hpp"
#include "command_encoder.hpp"
#include "buffer.hpp"
#include "texture.hpp"
#include "fence.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    enum class BlitOption
    {
        None                                             = 0,
        DepthFromDepthStencil                            = 1 << 0,
        StencilFromDepthStencil                          = 1 << 1,
        RowLinearPVRTC          MTLPP_AVAILABLE_AX(9_0)  = 1 << 2,
    }
    MTLPP_AVAILABLE(10_11, 9_0);

    class BlitCommandEncoder : public CommandEncoder<ns::Protocol<id<MTLBlitCommandEncoder>>::type>
    {
    public:
        BlitCommandEncoder() { }
        BlitCommandEncoder(ns::Protocol<id<MTLBlitCommandEncoder>>::type handle) : CommandEncoder<ns::Protocol<id<MTLBlitCommandEncoder>>::type>(handle) { }

        void Synchronize(const Resource& resource) MTLPP_AVAILABLE_MAC(10_11);
        void Synchronize(const Texture& texture, NSUInteger slice, NSUInteger level) MTLPP_AVAILABLE_MAC(10_11);
        void Copy(const Texture& sourceTexture, NSUInteger sourceSlice, NSUInteger sourceLevel, const Origin& sourceOrigin, const Size& sourceSize, const Texture& destinationTexture, NSUInteger destinationSlice, NSUInteger destinationLevel, const Origin& destinationOrigin);
        void Copy(const Buffer& sourceBuffer, NSUInteger sourceOffset, NSUInteger sourceBytesPerRow, NSUInteger sourceBytesPerImage, const Size& sourceSize, const Texture& destinationTexture, NSUInteger destinationSlice, NSUInteger destinationLevel, const Origin& destinationOrigin);
        void Copy(const Buffer& sourceBuffer, NSUInteger sourceOffset, NSUInteger sourceBytesPerRow, NSUInteger sourceBytesPerImage, const Size& sourceSize, const Texture& destinationTexture, NSUInteger destinationSlice, NSUInteger destinationLevel, const Origin& destinationOrigin, BlitOption options) MTLPP_AVAILABLE(10_11, 9_0);
        void Copy(const Texture& sourceTexture, NSUInteger sourceSlice, NSUInteger sourceLevel, const Origin& sourceOrigin, const Size& sourceSize, const Buffer& destinationBuffer, NSUInteger destinationOffset, NSUInteger destinationBytesPerRow, NSUInteger destinationBytesPerImage);
        void Copy(const Texture& sourceTexture, NSUInteger sourceSlice, NSUInteger sourceLevel, const Origin& sourceOrigin, const Size& sourceSize, const Buffer& destinationBuffer, NSUInteger destinationOffset, NSUInteger destinationBytesPerRow, NSUInteger destinationBytesPerImage, BlitOption options) MTLPP_AVAILABLE(10_11, 9_0);
        void Copy(const Buffer& sourceBuffer, NSUInteger soruceOffset, const Buffer& destinationBuffer, NSUInteger destinationOffset, NSUInteger size);
        void GenerateMipmaps(const Texture& texture);
        void Fill(const Buffer& buffer, const ns::Range& range, uint8_t value);
        void UpdateFence(const Fence& fence) MTLPP_AVAILABLE(10_13, 10_0);
        void WaitForFence(const Fence& fence) MTLPP_AVAILABLE(10_13, 10_0);
    };
}

MTLPP_END
