/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLRenderPass.h>
#include "render_pass.hpp"
#include "texture.hpp"

MTLPP_BEGIN

namespace mtlpp
{
	template<typename T>
    RenderPassAttachmentDescriptor<T>::RenderPassAttachmentDescriptor()
    {
    }

	template<typename T>
    Texture RenderPassAttachmentDescriptor<T>::GetTexture() const
    {
        this->Validate();
        return [this->m_ptr texture];
    }

	template<typename T>
    NSUInteger RenderPassAttachmentDescriptor<T>::GetLevel() const
    {
       this-> Validate();
        return NSUInteger([this->m_ptr level]);
    }

	template<typename T>
    NSUInteger RenderPassAttachmentDescriptor<T>::GetSlice() const
    {
        this->Validate();
        return NSUInteger([this->m_ptr slice]);
    }

	template<typename T>
    NSUInteger RenderPassAttachmentDescriptor<T>::GetDepthPlane() const
    {
        this->Validate();
        return NSUInteger([this->m_ptr depthPlane]);
    }

	template<typename T>
    Texture RenderPassAttachmentDescriptor<T>::GetResolveTexture() const
    {
        this->Validate();
        return [this->m_ptr resolveTexture];
    }

	template<typename T>
    NSUInteger RenderPassAttachmentDescriptor<T>::GetResolveLevel() const
    {
        this->Validate();
        return NSUInteger([this->m_ptr resolveLevel]);
    }

	template<typename T>
    NSUInteger RenderPassAttachmentDescriptor<T>::GetResolveSlice() const
    {
        this->Validate();
        return NSUInteger([this->m_ptr resolveSlice]);
    }

	template<typename T>
    NSUInteger RenderPassAttachmentDescriptor<T>::GetResolveDepthPlane() const
    {
        this->Validate();
        return NSUInteger([this->m_ptr resolveDepthPlane]);
    }

	template<typename T>
    LoadAction RenderPassAttachmentDescriptor<T>::GetLoadAction() const
    {
        this->Validate();
        return LoadAction([this->m_ptr loadAction]);
    }

	template<typename T>
    StoreAction RenderPassAttachmentDescriptor<T>::GetStoreAction() const
    {
        this->Validate();
        return StoreAction([this->m_ptr storeAction]);
    }
	
	template<typename T>
	StoreActionOptions RenderPassAttachmentDescriptor<T>::GetStoreActionOptions() const
	{
		this->Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return StoreActionOptions([this->m_ptr storeActionOptions]);
#else
		return 0;
#endif
	}

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetTexture(const Texture& texture)
    {
        this->Validate();
        [this->m_ptr setTexture:(id<MTLTexture>)texture.GetPtr()];
    }

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetLevel(NSUInteger level)
    {
        this->Validate();
        [this->m_ptr setLevel:level];
    }

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetSlice(NSUInteger slice)
    {
        this->Validate();
        [this->m_ptr setSlice:slice];
    }

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetDepthPlane(NSUInteger depthPlane)
    {
        this->Validate();
        [this->m_ptr setDepthPlane:depthPlane];
    }

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetResolveTexture(const Texture& texture)
    {
        this->Validate();
        [this->m_ptr setResolveTexture:(id<MTLTexture>)texture.GetPtr()];
    }

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetResolveLevel(NSUInteger resolveLevel)
    {
        this->Validate();
        [this->m_ptr setResolveLevel:resolveLevel];
    }

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetResolveSlice(NSUInteger resolveSlice)
    {
        this->Validate();
        [this->m_ptr setResolveSlice:resolveSlice];
    }

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetResolveDepthPlane(NSUInteger resolveDepthPlane)
    {
        this->Validate();
        [this->m_ptr setResolveDepthPlane:resolveDepthPlane];
    }

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetLoadAction(LoadAction loadAction)
    {
        this->Validate();
        [this->m_ptr setLoadAction:MTLLoadAction(loadAction)];
    }

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetStoreAction(StoreAction storeAction)
    {
        this->Validate();
        [this->m_ptr setStoreAction:MTLStoreAction(storeAction)];
    }
	
	template<typename T>
	void RenderPassAttachmentDescriptor<T>::SetStoreActionOptions(StoreActionOptions options)
	{
		this->Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		[this->m_ptr setStoreActionOptions:(MTLStoreActionOptions)options];
#endif
	}

    RenderPassColorAttachmentDescriptor::RenderPassColorAttachmentDescriptor() :
        RenderPassAttachmentDescriptor([[MTLRenderPassColorAttachmentDescriptor alloc] init])
    {
    }

    ClearColor RenderPassColorAttachmentDescriptor::GetClearColor() const
    {
        Validate();
        MTLClearColor mtlClearColor = [(MTLRenderPassColorAttachmentDescriptor*)m_ptr clearColor];
        return ClearColor(mtlClearColor.red, mtlClearColor.green, mtlClearColor.blue, mtlClearColor.alpha);
    }

    void RenderPassColorAttachmentDescriptor::SetClearColor(const ClearColor& clearColor)
    {
        Validate();
        MTLClearColor mtlClearColor = { clearColor.Red, clearColor.Green, clearColor.Blue, clearColor.Alpha };
        [(MTLRenderPassColorAttachmentDescriptor*)m_ptr setClearColor:mtlClearColor];
    }

    RenderPassDepthAttachmentDescriptor::RenderPassDepthAttachmentDescriptor() :
        RenderPassAttachmentDescriptor([[MTLRenderPassDepthAttachmentDescriptor alloc] init])
    {
    }

    double RenderPassDepthAttachmentDescriptor::GetClearDepth() const
    {
        Validate();
        return [(MTLRenderPassDepthAttachmentDescriptor*)m_ptr clearDepth];
    }

    MultisampleDepthResolveFilter RenderPassDepthAttachmentDescriptor::GetDepthResolveFilter() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE_AX(9_0)
        return MultisampleDepthResolveFilter([(MTLRenderPassDepthAttachmentDescriptor*)m_ptr depthResolveFilter]);
#else
        return MultisampleDepthResolveFilter(0);
#endif
    }

    void RenderPassDepthAttachmentDescriptor::SetClearDepth(double clearDepth)
    {
        Validate();
        [(MTLRenderPassDepthAttachmentDescriptor*)m_ptr setClearDepth:clearDepth];
    }

    void RenderPassDepthAttachmentDescriptor::SetDepthResolveFilter(MultisampleDepthResolveFilter depthResolveFilter)
    {
        Validate();
#if MTLPP_IS_AVAILABLE_AX(9_0)
        [(MTLRenderPassDepthAttachmentDescriptor*)m_ptr setDepthResolveFilter:MTLMultisampleDepthResolveFilter(depthResolveFilter)];
#endif
    }

    RenderPassStencilAttachmentDescriptor::RenderPassStencilAttachmentDescriptor() :
        RenderPassAttachmentDescriptor([[MTLRenderPassStencilAttachmentDescriptor alloc] init])
    {
    }

    NSUInteger RenderPassStencilAttachmentDescriptor::GetClearStencil() const
    {
        Validate();
        return NSUInteger([(MTLRenderPassStencilAttachmentDescriptor*)m_ptr clearStencil]);
    }

    void RenderPassStencilAttachmentDescriptor::SetClearStencil(uint32_t clearStencil)
    {
        Validate();
        [(MTLRenderPassStencilAttachmentDescriptor*)m_ptr setClearStencil:clearStencil];
    }

    RenderPassDescriptor::RenderPassDescriptor() :
        ns::Object<MTLRenderPassDescriptor*>([[MTLRenderPassDescriptor alloc] init], false)
    {
    }

    ns::Array<RenderPassColorAttachmentDescriptor> RenderPassDescriptor::GetColorAttachments() const
    {
        Validate();
		return (NSArray<RenderPassColorAttachmentDescriptor::Type>*)[(MTLRenderPassDescriptor*)m_ptr colorAttachments];
    }

    RenderPassDepthAttachmentDescriptor RenderPassDescriptor::GetDepthAttachment() const
    {
        Validate();
        return [(MTLRenderPassDescriptor*)m_ptr depthAttachment];
    }

    RenderPassStencilAttachmentDescriptor RenderPassDescriptor::GetStencilAttachment() const
    {
        Validate();
        return [(MTLRenderPassDescriptor*)m_ptr stencilAttachment];
    }

    Buffer RenderPassDescriptor::GetVisibilityResultBuffer() const
    {
        Validate();
        return [(MTLRenderPassDescriptor*)m_ptr visibilityResultBuffer];
    }

    NSUInteger RenderPassDescriptor::GetRenderTargetArrayLength() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_11)
        return NSUInteger([(MTLRenderPassDescriptor*)m_ptr renderTargetArrayLength]);
#else
        return 0;
#endif
    }

    void RenderPassDescriptor::SetDepthAttachment(const RenderPassDepthAttachmentDescriptor& depthAttachment)
    {
        Validate();
        [(MTLRenderPassDescriptor*)m_ptr setDepthAttachment:(MTLRenderPassDepthAttachmentDescriptor*)depthAttachment.GetPtr()];
    }

    void RenderPassDescriptor::SetStencilAttachment(const RenderPassStencilAttachmentDescriptor& stencilAttachment)
    {
        Validate();
        [(MTLRenderPassDescriptor*)m_ptr setStencilAttachment:(MTLRenderPassStencilAttachmentDescriptor*)stencilAttachment.GetPtr()];
    }

    void RenderPassDescriptor::SetVisibilityResultBuffer(const Buffer& visibilityResultBuffer)
    {
        Validate();
        [(MTLRenderPassDescriptor*)m_ptr setVisibilityResultBuffer:(id<MTLBuffer>)visibilityResultBuffer.GetPtr()];
    }

    void RenderPassDescriptor::SetRenderTargetArrayLength(NSUInteger renderTargetArrayLength)
    {
        Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_11)
        [(MTLRenderPassDescriptor*)m_ptr setRenderTargetArrayLength:renderTargetArrayLength];
#endif
    }
	
	void RenderPassDescriptor::SetSamplePositions(SamplePosition const* positions, NSUInteger count)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		[(MTLRenderPassDescriptor*)m_ptr setSamplePositions:(const MTLSamplePosition *)positions count:count];
#endif
	}
	
	NSUInteger RenderPassDescriptor::GetSamplePositions(SamplePosition const* positions, NSUInteger count)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return [(MTLRenderPassDescriptor*)m_ptr getSamplePositions:(MTLSamplePosition *)positions count:count];
#else
		return 0;
#endif
	}
	
	NSUInteger RenderPassDescriptor::GetImageblockSampleLength() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [(MTLRenderPassDescriptor*)m_ptr imageblockSampleLength];
#else
		return 0;
#endif
	}
	
	NSUInteger RenderPassDescriptor::GetThreadgroupMemoryLength() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [(MTLRenderPassDescriptor*)m_ptr threadgroupMemoryLength];
#else
		return 0;
#endif
	}
	
	NSUInteger RenderPassDescriptor::GetTileWidth() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [(MTLRenderPassDescriptor*)m_ptr tileWidth];
#else
		return 0;
#endif
	}
	
	NSUInteger RenderPassDescriptor::GetTileHeight() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [(MTLRenderPassDescriptor*)m_ptr tileHeight];
#else
		return 0;
#endif
	}
	
	NSUInteger RenderPassDescriptor::GetDefaultRasterSampleCount() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [(MTLRenderPassDescriptor*)m_ptr defaultRasterSampleCount];
#else
		return 0;
#endif
	}
	
	NSUInteger RenderPassDescriptor::GetRenderTargetWidth() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [(MTLRenderPassDescriptor*)m_ptr renderTargetWidth];
#else
		return 0;
#endif
	}
	
	NSUInteger RenderPassDescriptor::GetRenderTargetHeight() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [(MTLRenderPassDescriptor*)m_ptr renderTargetHeight];
#else
		return 0;
#endif
	}
	
	void RenderPassDescriptor::SetImageblockSampleLength(NSUInteger Val)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[(MTLRenderPassDescriptor*)m_ptr setImageblockSampleLength:Val];
#endif
	}
	
	void RenderPassDescriptor::SetThreadgroupMemoryLength(NSUInteger Val)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[(MTLRenderPassDescriptor*)m_ptr setThreadgroupMemoryLength:Val];
#endif
	}
	
	void RenderPassDescriptor::SetTileWidth(NSUInteger Val)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[(MTLRenderPassDescriptor*)m_ptr setTileWidth:Val];
#endif
	}
	
	void RenderPassDescriptor::SetTileHeight(NSUInteger Val)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[(MTLRenderPassDescriptor*)m_ptr setRenderTargetHeight:Val];
#endif
	}
	
	void RenderPassDescriptor::SetDefaultRasterSampleCount(NSUInteger Val)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[(MTLRenderPassDescriptor*)m_ptr setDefaultRasterSampleCount:Val];
#endif
	}
	
	void RenderPassDescriptor::SetRenderTargetWidth(NSUInteger Val)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[(MTLRenderPassDescriptor*)m_ptr setRenderTargetWidth:Val];
#endif
	}
	
	void RenderPassDescriptor::SetRenderTargetHeight(NSUInteger Val)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[(MTLRenderPassDescriptor*)m_ptr setRenderTargetHeight:Val];
#endif
	}
}

MTLPP_END
