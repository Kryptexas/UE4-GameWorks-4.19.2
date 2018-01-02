/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLBlitCommandEncoder.h>
#include "blit_command_encoder.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    void BlitCommandEncoder::Synchronize(const Resource& resource)
    {
        Validate();
#if MTLPP_PLATFORM_MAC
		m_table->SynchronizeResource(m_ptr, resource.GetPtr());
#endif
    }

    void BlitCommandEncoder::Synchronize(const Texture& texture, NSUInteger slice, NSUInteger level)
    {
        Validate();
#if MTLPP_PLATFORM_MAC
		m_table->SynchronizeTextureSliceLevel(m_ptr, (id<MTLTexture>)texture.GetPtr(), slice, level);
#endif
    }

    void BlitCommandEncoder::Copy(const Texture& sourceTexture, NSUInteger sourceSlice, NSUInteger sourceLevel, const Origin& sourceOrigin, const Size& sourceSize, const Texture& destinationTexture, NSUInteger destinationSlice, NSUInteger destinationLevel, const Origin& destinationOrigin)
    {
        Validate();
		
		m_table->CopyFromTexturesourceSlicesourceLevelsourceOriginsourceSizetoTexturedestinationSlicedestinationLeveldestinationOrigin(m_ptr, (id<MTLTexture>)sourceTexture.GetPtr(), sourceSlice, sourceLevel, sourceOrigin, sourceSize, (id<MTLTexture>)destinationTexture.GetPtr(), destinationSlice, destinationLevel, destinationOrigin);
    }

    void BlitCommandEncoder::Copy(const Buffer& sourceBuffer, NSUInteger sourceOffset, NSUInteger sourceBytesPerRow, NSUInteger sourceBytesPerImage, const Size& sourceSize, const Texture& destinationTexture, NSUInteger destinationSlice, NSUInteger destinationLevel, const Origin& destinationOrigin)
    {
        Validate();
		m_table->CopyFromBuffersourceOffsetsourceBytesPerRowsourceBytesPerImagesourceSizetoTexturedestinationSlicedestinationLeveldestinationOrigin(m_ptr, (id<MTLBuffer>)sourceBuffer.GetPtr(), sourceOffset, sourceBytesPerRow, sourceBytesPerImage, sourceSize, (id<MTLTexture>)destinationTexture.GetPtr(), destinationSlice, destinationLevel, destinationOrigin);
    }

    void BlitCommandEncoder::Copy(const Buffer& sourceBuffer, NSUInteger sourceOffset, NSUInteger sourceBytesPerRow, NSUInteger sourceBytesPerImage, const Size& sourceSize, const Texture& destinationTexture, NSUInteger destinationSlice, NSUInteger destinationLevel, const Origin& destinationOrigin, BlitOption options)
    {
        Validate();
		m_table->CopyFromBuffersourceOffsetsourceBytesPerRowsourceBytesPerImagesourceSizetoTexturedestinationSlicedestinationLeveldestinationOriginoptions(m_ptr, (id<MTLBuffer>)sourceBuffer.GetPtr(), sourceOffset, sourceBytesPerRow, sourceBytesPerImage, sourceSize, (id<MTLTexture>)destinationTexture.GetPtr(), destinationSlice, destinationLevel, destinationOrigin, MTLBlitOption(options));
    }

    void BlitCommandEncoder::Copy(const Texture& sourceTexture, NSUInteger sourceSlice, NSUInteger sourceLevel, const Origin& sourceOrigin, const Size& sourceSize, const Buffer& destinationBuffer, NSUInteger destinationOffset, NSUInteger destinationBytesPerRow, NSUInteger destinationBytesPerImage)
    {
        Validate();
		m_table->CopyFromTexturesourceSlicesourceLevelsourceOriginsourceSizetoBufferdestinationOffsetdestinationBytesPerRowdestinationBytesPerImage(m_ptr, (id<MTLTexture>)sourceTexture.GetPtr(), sourceSlice, sourceLevel, sourceOrigin, sourceSize, (id<MTLBuffer>)destinationBuffer.GetPtr(), destinationOffset, destinationBytesPerRow, destinationBytesPerImage);
    }

    void BlitCommandEncoder::Copy(const Texture& sourceTexture, NSUInteger sourceSlice, NSUInteger sourceLevel, const Origin& sourceOrigin, const Size& sourceSize, const Buffer& destinationBuffer, NSUInteger destinationOffset, NSUInteger destinationBytesPerRow, NSUInteger destinationBytesPerImage, BlitOption options)
    {
        Validate();
		m_table->CopyFromTexturesourceSlicesourceLevelsourceOriginsourceSizetoBufferdestinationOffsetdestinationBytesPerRowdestinationBytesPerImageoptions(m_ptr, (id<MTLTexture>)sourceTexture.GetPtr(), sourceSlice, sourceLevel, sourceOrigin, sourceSize, (id<MTLBuffer>)destinationBuffer.GetPtr(), destinationOffset, destinationBytesPerRow, destinationBytesPerImage, MTLBlitOption(options));
    }

    void BlitCommandEncoder::Copy(const Buffer& sourceBuffer, NSUInteger sourceOffset, const Buffer& destinationBuffer, NSUInteger destinationOffset, NSUInteger size)
    {
        Validate();
		m_table->CopyFromBufferSourceOffsetToBufferDestinationOffsetSize(m_ptr, (id<MTLBuffer>)sourceBuffer.GetPtr(), sourceOffset, (id<MTLBuffer>)destinationBuffer.GetPtr(), destinationOffset, size);
    }

    void BlitCommandEncoder::GenerateMipmaps(const Texture& texture)
    {
        Validate();
		m_table->GenerateMipmapsForTexture(m_ptr, (id<MTLTexture>)texture.GetPtr());
    }

    void BlitCommandEncoder::Fill(const Buffer& buffer, const ns::Range& range, uint8_t value)
    {
        Validate();
		m_table->FillBufferRangeValue(m_ptr, (id<MTLBuffer>)buffer.GetPtr(), NSMakeRange(range.Location, range.Length), value);
    }

    void BlitCommandEncoder::UpdateFence(const Fence& fence)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
		m_table->UpdateFence(m_ptr, fence.GetPtr());
#endif
    }

    void BlitCommandEncoder::WaitForFence(const Fence& fence)
    {
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
		m_table->WaitForFence(m_ptr, fence.GetPtr());
#endif
    }
}

MTLPP_END
