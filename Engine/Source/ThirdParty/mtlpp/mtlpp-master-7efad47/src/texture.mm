/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLTexture.h>
#include "texture.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    TextureDescriptor::TextureDescriptor() :
        ns::Object<MTLTextureDescriptor*>([[MTLTextureDescriptor alloc] init], false)
    {
    }

    TextureDescriptor TextureDescriptor::Texture2DDescriptor(PixelFormat pixelFormat, NSUInteger width, NSUInteger height, bool mipmapped)
    {
        return [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormat(pixelFormat)
                                                                                              width:width
                                                                                             height:height
                                                                                          mipmapped:mipmapped];
    }

    TextureDescriptor TextureDescriptor::TextureCubeDescriptor(PixelFormat pixelFormat, NSUInteger size, bool mipmapped)
    {
        return [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:MTLPixelFormat(pixelFormat)
                                                                                                 size:size
                                                                                            mipmapped:mipmapped];
    }

    TextureType TextureDescriptor::GetTextureType() const
    {
        Validate();
        return TextureType([(MTLTextureDescriptor*)m_ptr textureType]);
    }

    PixelFormat TextureDescriptor::GetPixelFormat() const
    {
        Validate();
        return PixelFormat([(MTLTextureDescriptor*)m_ptr pixelFormat]);
    }

    NSUInteger TextureDescriptor::GetWidth() const
    {
        Validate();
        return NSUInteger([(MTLTextureDescriptor*)m_ptr width]);
    }

    NSUInteger TextureDescriptor::GetHeight() const
    {
        Validate();
        return NSUInteger([(MTLTextureDescriptor*)m_ptr height]);
    }

    NSUInteger TextureDescriptor::GetDepth() const
    {
        Validate();
        return NSUInteger([(MTLTextureDescriptor*)m_ptr depth]);
    }

    NSUInteger TextureDescriptor::GetMipmapLevelCount() const
    {
        Validate();
        return NSUInteger([(MTLTextureDescriptor*)m_ptr mipmapLevelCount]);
    }

    NSUInteger TextureDescriptor::GetSampleCount() const
    {
        Validate();
        return NSUInteger([(MTLTextureDescriptor*)m_ptr sampleCount]);
    }

    NSUInteger TextureDescriptor::GetArrayLength() const
    {
        Validate();
        return NSUInteger([(MTLTextureDescriptor*)m_ptr arrayLength]);
    }

    ResourceOptions TextureDescriptor::GetResourceOptions() const
    {
        Validate();
        return ResourceOptions([(MTLTextureDescriptor*)m_ptr resourceOptions]);
    }

    CpuCacheMode TextureDescriptor::GetCpuCacheMode() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
        return CpuCacheMode([(MTLTextureDescriptor*)m_ptr cpuCacheMode]);
#else
        return CpuCacheMode(0);
#endif
    }

    StorageMode TextureDescriptor::GetStorageMode() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
        return StorageMode([(MTLTextureDescriptor*)m_ptr storageMode]);
#else
        return StorageMode(0);
#endif
    }

    TextureUsage TextureDescriptor::GetUsage() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
        return TextureUsage([(MTLTextureDescriptor*)m_ptr usage]);
#else
        return TextureUsage(0);
#endif
    }

    void TextureDescriptor::SetTextureType(TextureType textureType)
    {
        Validate();
        [(MTLTextureDescriptor*)m_ptr setTextureType:MTLTextureType(textureType)];
    }

    void TextureDescriptor::SetPixelFormat(PixelFormat pixelFormat)
    {
        Validate();
        [(MTLTextureDescriptor*)m_ptr setPixelFormat:MTLPixelFormat(pixelFormat)];
    }

    void TextureDescriptor::SetWidth(NSUInteger width)
    {
        Validate();
        [(MTLTextureDescriptor*)m_ptr setWidth:width];
    }

    void TextureDescriptor::SetHeight(NSUInteger height)
    {
        Validate();
        [(MTLTextureDescriptor*)m_ptr setHeight:height];
    }

    void TextureDescriptor::SetDepth(NSUInteger depth)
    {
        Validate();
        [(MTLTextureDescriptor*)m_ptr setDepth:depth];
    }

    void TextureDescriptor::SetMipmapLevelCount(NSUInteger mipmapLevelCount)
    {
        Validate();
        [(MTLTextureDescriptor*)m_ptr setMipmapLevelCount:mipmapLevelCount];
    }

    void TextureDescriptor::SetSampleCount(NSUInteger sampleCount)
    {
        Validate();
        [(MTLTextureDescriptor*)m_ptr setSampleCount:sampleCount];
    }

    void TextureDescriptor::SetArrayLength(NSUInteger arrayLength)
    {
        Validate();
        [(MTLTextureDescriptor*)m_ptr setArrayLength:arrayLength];
    }

    void TextureDescriptor::SetResourceOptions(ResourceOptions resourceOptions)
    {
        Validate();
        [(MTLTextureDescriptor*)m_ptr setResourceOptions:MTLResourceOptions(resourceOptions)];
    }

    void TextureDescriptor::SetCpuCacheMode(CpuCacheMode cpuCacheMode)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
        [(MTLTextureDescriptor*)m_ptr setCpuCacheMode:MTLCPUCacheMode(cpuCacheMode)];
#endif
    }

    void TextureDescriptor::SetStorageMode(StorageMode storageMode)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
        [(MTLTextureDescriptor*)m_ptr setStorageMode:MTLStorageMode(storageMode)];
#endif
    }

    void TextureDescriptor::SetUsage(TextureUsage usage)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
        [(MTLTextureDescriptor*)m_ptr setUsage:MTLTextureUsage(usage)];
#endif
    }

    Resource Texture::GetRootResource() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 8_0)
#   if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return nullptr;
#   else
		return ((IMPTable<id<MTLTexture>, void>*)m_table)->Rootresource((id<MTLTexture>)m_ptr);
#   endif
#else
        return nullptr;
#endif
    }

    Texture Texture::GetParentTexture() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
		return ((IMPTable<id<MTLTexture>, void>*)m_table)->Parenttexture((id<MTLTexture>)m_ptr);
#else
        return nullptr;
#endif
    }

    NSUInteger Texture::GetParentRelativeLevel() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
		return ((IMPTable<id<MTLTexture>, void>*)m_table)->Parentrelativelevel((id<MTLTexture>)m_ptr);
#else
        return 0;
#endif

    }

    NSUInteger Texture::GetParentRelativeSlice() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
		return ((IMPTable<id<MTLTexture>, void>*)m_table)->Parentrelativeslice((id<MTLTexture>)m_ptr);
#else
        return 0;
#endif

    }

    Buffer Texture::GetBuffer() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 9_0)
		return ((IMPTable<id<MTLTexture>, void>*)m_table)->Buffer((id<MTLTexture>)m_ptr);
#else
        return nullptr;
#endif

    }

    NSUInteger Texture::GetBufferOffset() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 9_0)
		return ((IMPTable<id<MTLTexture>, void>*)m_table)->Bufferoffset((id<MTLTexture>)m_ptr);
#else
        return 0;
#endif

    }

    NSUInteger Texture::GetBufferBytesPerRow() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 9_0)
		return ((IMPTable<id<MTLTexture>, void>*)m_table)->Bufferbytesperrow((id<MTLTexture>)m_ptr);
#else
        return 0;
#endif

    }
	
	ns::IOSurface Texture::GetIOSurface() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_11)
		return ((IMPTable<id<MTLTexture>, void>*)m_table)->Iosurface((id<MTLTexture>)m_ptr);
#else
		return ns::IOSurface();
#endif
	}

    NSUInteger Texture::GetIOSurfacePlane() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_11)
		return ((IMPTable<id<MTLTexture>, void>*)m_table)->Iosurfaceplane((id<MTLTexture>)m_ptr);
#else
        return 0;
#endif
    }

    TextureType Texture::GetTextureType() const
    {
        Validate();
		return (TextureType)((IMPTable<id<MTLTexture>, void>*)m_table)->Texturetype((id<MTLTexture>)m_ptr);
    }

    PixelFormat Texture::GetPixelFormat() const
    {
        Validate();
		return (PixelFormat)((IMPTable<id<MTLTexture>, void>*)m_table)->Pixelformat((id<MTLTexture>)m_ptr);
    }

    NSUInteger Texture::GetWidth() const
    {
        Validate();
		return ((IMPTable<id<MTLTexture>, void>*)m_table)->Width((id<MTLTexture>)m_ptr);    }

    NSUInteger Texture::GetHeight() const
    {
        Validate();
		return ((IMPTable<id<MTLTexture>, void>*)m_table)->Height((id<MTLTexture>)m_ptr);
    }

    NSUInteger Texture::GetDepth() const
    {
        Validate();
		return ((IMPTable<id<MTLTexture>, void>*)m_table)->Depth((id<MTLTexture>)m_ptr);
    }

    NSUInteger Texture::GetMipmapLevelCount() const
    {
        Validate();
		return ((IMPTable<id<MTLTexture>, void>*)m_table)->Mipmaplevelcount((id<MTLTexture>)m_ptr);
    }

    NSUInteger Texture::GetSampleCount() const
    {
        Validate();
		return ((IMPTable<id<MTLTexture>, void>*)m_table)->Samplecount((id<MTLTexture>)m_ptr);
    }

    NSUInteger Texture::GetArrayLength() const
    {
        Validate();
		return ((IMPTable<id<MTLTexture>, void>*)m_table)->Arraylength((id<MTLTexture>)m_ptr);
    }

    TextureUsage Texture::GetUsage() const
    {
        Validate();
		return TextureUsage(((IMPTable<id<MTLTexture>, void>*)m_table)->Usage((id<MTLTexture>)m_ptr));
    }

    bool Texture::IsFrameBufferOnly() const
    {
        Validate();
		return ((IMPTable<id<MTLTexture>, void>*)m_table)->Isframebufferonly((id<MTLTexture>)m_ptr);
    }

    void Texture::GetBytes(void* pixelBytes, NSUInteger bytesPerRow, NSUInteger bytesPerImage, const Region& fromRegion, NSUInteger mipmapLevel, NSUInteger slice)
    {
        Validate();
		((IMPTable<id<MTLTexture>, void>*)m_table)->Getbytesbytesperrowbytesperimagefromregionmipmaplevelslice((id<MTLTexture>)m_ptr, pixelBytes, bytesPerRow, bytesPerImage, fromRegion, mipmapLevel, slice);
    }

    void Texture::Replace(const Region& region, NSUInteger mipmapLevel, NSUInteger slice, void* pixelBytes, NSUInteger bytesPerRow, NSUInteger bytesPerImage)
    {
        Validate();
		((IMPTable<id<MTLTexture>, void>*)m_table)->Replaceregionmipmaplevelslicewithbytesbytesperrowbytesperimage((id<MTLTexture>)m_ptr, region, mipmapLevel, slice, pixelBytes, bytesPerRow, bytesPerImage);
    }

    void Texture::GetBytes(void* pixelBytes, NSUInteger bytesPerRow, const Region& fromRegion, NSUInteger mipmapLevel)
    {
        Validate();
		((IMPTable<id<MTLTexture>, void>*)m_table)->Getbytesbytesperrowfromregionmipmaplevel((id<MTLTexture>)m_ptr, pixelBytes, bytesPerRow, fromRegion, mipmapLevel);
    }

    void Texture::Replace(const Region& region, NSUInteger mipmapLevel, void* pixelBytes, NSUInteger bytesPerRow)
    {
        Validate();
		((IMPTable<id<MTLTexture>, void>*)m_table)->Replaceregionmipmaplevelwithbytesbytesperrow((id<MTLTexture>)m_ptr, region, mipmapLevel, pixelBytes, bytesPerRow);
    }

    Texture Texture::NewTextureView(PixelFormat pixelFormat)
    {
        Validate();
		return ((IMPTable<id<MTLTexture>, void>*)m_table)->Newtextureviewwithpixelformat((id<MTLTexture>)m_ptr, MTLPixelFormat(pixelFormat));
    }

    Texture Texture::NewTextureView(PixelFormat pixelFormat, TextureType textureType, const ns::Range& mipmapLevelRange, const ns::Range& sliceRange)
    {
        Validate();
		return ((IMPTable<id<MTLTexture>, void>*)m_table)->Newtextureviewwithpixelformattexturetypelevelsslices((id<MTLTexture>)m_ptr, MTLPixelFormat(pixelFormat), MTLTextureType(textureType), NSMakeRange(mipmapLevelRange.Location, mipmapLevelRange.Length), NSMakeRange(sliceRange.Location, sliceRange.Length));
    }
	
	Texture::Texture(ns::Protocol<id<MTLTexture>>::type handle)
	: Resource((ns::Protocol<id<MTLResource>>::type)handle)
	{
	}
}

MTLPP_END
