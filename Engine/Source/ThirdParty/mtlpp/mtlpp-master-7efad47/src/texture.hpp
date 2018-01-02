/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_Texture.hpp"
#include "resource.hpp"
#include "buffer.hpp"
#include "types.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    enum class TextureType
    {
        Texture1D                                       = 0,
        Texture1DArray                                  = 1,
        Texture2D                                       = 2,
        Texture2DArray                                  = 3,
        Texture2DMultisample                            = 4,
        TextureCube                                     = 5,
        TextureCubeArray     MTLPP_AVAILABLE_MAC(10_11) = 6,
        Texture3D                                       = 7,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class TextureUsage
    {
        Unknown         = 0x0000,
        ShaderRead      = 0x0001,
        ShaderWrite     = 0x0002,
        RenderTarget    = 0x0004,
        PixelFormatView = 0x0010,
    }
    MTLPP_AVAILABLE(10_11, 9_0);


    class TextureDescriptor : public ns::Object<MTLTextureDescriptor*>
    {
    public:
        TextureDescriptor();
        TextureDescriptor(MTLTextureDescriptor* handle) : ns::Object<MTLTextureDescriptor*>(handle) { }

        static TextureDescriptor Texture2DDescriptor(PixelFormat pixelFormat, NSUInteger width, NSUInteger height, bool mipmapped);
        static TextureDescriptor TextureCubeDescriptor(PixelFormat pixelFormat, NSUInteger size, bool mipmapped);

        TextureType     GetTextureType() const;
        PixelFormat     GetPixelFormat() const;
        NSUInteger        GetWidth() const;
        NSUInteger        GetHeight() const;
        NSUInteger        GetDepth() const;
        NSUInteger        GetMipmapLevelCount() const;
        NSUInteger        GetSampleCount() const;
        NSUInteger        GetArrayLength() const;
        ResourceOptions GetResourceOptions() const;
        CpuCacheMode    GetCpuCacheMode() const MTLPP_AVAILABLE(10_11, 9_0);
        StorageMode     GetStorageMode() const MTLPP_AVAILABLE(10_11, 9_0);
        TextureUsage    GetUsage() const MTLPP_AVAILABLE(10_11, 9_0);

        void SetTextureType(TextureType textureType);
        void SetPixelFormat(PixelFormat pixelFormat);
        void SetWidth(NSUInteger width);
        void SetHeight(NSUInteger height);
        void SetDepth(NSUInteger depth);
        void SetMipmapLevelCount(NSUInteger mipmapLevelCount);
        void SetSampleCount(NSUInteger sampleCount);
        void SetArrayLength(NSUInteger arrayLength);
        void SetResourceOptions(ResourceOptions resourceOptions);
        void SetCpuCacheMode(CpuCacheMode cpuCacheMode) MTLPP_AVAILABLE(10_11, 9_0);
        void SetStorageMode(StorageMode storageMode) MTLPP_AVAILABLE(10_11, 9_0);
        void SetUsage(TextureUsage usage) MTLPP_AVAILABLE(10_11, 9_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    class Texture : public Resource
    {
    public:
        Texture() { }
		Texture(ns::Protocol<id<MTLTexture>>::type handle);
		
		inline const ns::Protocol<id<MTLTexture>>::type GetPtr() const { return (ns::Protocol<id<MTLTexture>>::type)m_ptr; }
		inline ns::Protocol<id<MTLTexture>>::type* GetInnerPtr() { return (ns::Protocol<id<MTLTexture>>::type*)&m_ptr; }
		operator ns::Protocol<id<MTLTexture>>::type() const { return (ns::Protocol<id<MTLTexture>>::type)m_ptr; }

        Resource     GetRootResource() const MTLPP_DEPRECATED(10_11, 10_12, 8_0, 10_0);
        Texture      GetParentTexture() const MTLPP_AVAILABLE(10_11, 9_0);
        NSUInteger     GetParentRelativeLevel() const MTLPP_AVAILABLE(10_11, 9_0);
        NSUInteger     GetParentRelativeSlice() const MTLPP_AVAILABLE(10_11, 9_0);
        Buffer       GetBuffer() const MTLPP_AVAILABLE(10_12, 9_0);
        NSUInteger     GetBufferOffset() const MTLPP_AVAILABLE(10_12, 9_0);
        NSUInteger     GetBufferBytesPerRow() const MTLPP_AVAILABLE(10_12, 9_0);
		ns::IOSurface GetIOSurface() const MTLPP_AVAILABLE(10_11, NA);
        NSUInteger     GetIOSurfacePlane() const MTLPP_AVAILABLE(10_11, NA);
        TextureType  GetTextureType() const;
        PixelFormat  GetPixelFormat() const;
        NSUInteger     GetWidth() const;
        NSUInteger     GetHeight() const;
        NSUInteger     GetDepth() const;
        NSUInteger     GetMipmapLevelCount() const;
        NSUInteger     GetSampleCount() const;
        NSUInteger     GetArrayLength() const;
        TextureUsage GetUsage() const;
        bool         IsFrameBufferOnly() const;

        void GetBytes(void* pixelBytes, NSUInteger bytesPerRow, NSUInteger bytesPerImage, const Region& fromRegion, NSUInteger mipmapLevel, NSUInteger slice);
        void Replace(const Region& region, NSUInteger mipmapLevel, NSUInteger slice, void* pixelBytes, NSUInteger bytesPerRow, NSUInteger bytesPerImage);
        void GetBytes(void* pixelBytes, NSUInteger bytesPerRow, const Region& fromRegion, NSUInteger mipmapLevel);
        void Replace(const Region& region, NSUInteger mipmapLevel, void* pixelBytes, NSUInteger bytesPerRow);
        Texture NewTextureView(PixelFormat pixelFormat);
        Texture NewTextureView(PixelFormat pixelFormat, TextureType textureType, const ns::Range& mipmapLevelRange, const ns::Range& sliceRange) MTLPP_AVAILABLE(10_11, 9_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);
}

MTLPP_END
