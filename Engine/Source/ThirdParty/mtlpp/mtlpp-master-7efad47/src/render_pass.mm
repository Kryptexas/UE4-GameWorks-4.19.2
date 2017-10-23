/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include "render_pass.hpp"
#include "texture.hpp"
#include <Metal/MTLRenderPass.h>

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
    uint32_t RenderPassAttachmentDescriptor<T>::GetLevel() const
    {
       this-> Validate();
        return uint32_t([this->m_ptr level]);
    }

	template<typename T>
    uint32_t RenderPassAttachmentDescriptor<T>::GetSlice() const
    {
        this->Validate();
        return uint32_t([this->m_ptr slice]);
    }

	template<typename T>
    uint32_t RenderPassAttachmentDescriptor<T>::GetDepthPlane() const
    {
        this->Validate();
        return uint32_t([this->m_ptr depthPlane]);
    }

	template<typename T>
    Texture RenderPassAttachmentDescriptor<T>::GetResolveTexture() const
    {
        this->Validate();
        return [this->m_ptr resolveTexture];
    }

	template<typename T>
    uint32_t RenderPassAttachmentDescriptor<T>::GetResolveLevel() const
    {
        this->Validate();
        return uint32_t([this->m_ptr resolveLevel]);
    }

	template<typename T>
    uint32_t RenderPassAttachmentDescriptor<T>::GetResolveSlice() const
    {
        this->Validate();
        return uint32_t([this->m_ptr resolveSlice]);
    }

	template<typename T>
    uint32_t RenderPassAttachmentDescriptor<T>::GetResolveDepthPlane() const
    {
        this->Validate();
        return uint32_t([this->m_ptr resolveDepthPlane]);
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
    void RenderPassAttachmentDescriptor<T>::SetLevel(uint32_t level)
    {
        this->Validate();
        [this->m_ptr setLevel:level];
    }

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetSlice(uint32_t slice)
    {
        this->Validate();
        [this->m_ptr setSlice:slice];
    }

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetDepthPlane(uint32_t depthPlane)
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
    void RenderPassAttachmentDescriptor<T>::SetResolveLevel(uint32_t resolveLevel)
    {
        this->Validate();
        [this->m_ptr setResolveLevel:resolveLevel];
    }

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetResolveSlice(uint32_t resolveSlice)
    {
        this->Validate();
        [this->m_ptr setResolveSlice:resolveSlice];
    }

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetResolveDepthPlane(uint32_t resolveDepthPlane)
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
#if MTLPP_PLATFORM_IOS
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
#if MTLPP_PLATFORM_IOS
        [(MTLRenderPassDepthAttachmentDescriptor*)m_ptr setDepthResolveFilter:MTLMultisampleDepthResolveFilter(depthResolveFilter)];
#endif
    }

    RenderPassStencilAttachmentDescriptor::RenderPassStencilAttachmentDescriptor() :
        RenderPassAttachmentDescriptor([[MTLRenderPassStencilAttachmentDescriptor alloc] init])
    {
    }

    uint32_t RenderPassStencilAttachmentDescriptor::GetClearStencil() const
    {
        Validate();
        return uint32_t([(MTLRenderPassStencilAttachmentDescriptor*)m_ptr clearStencil]);
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
        return (NSArray*)[(MTLRenderPassDescriptor*)m_ptr colorAttachments];
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

    uint32_t RenderPassDescriptor::GetRenderTargetArrayLength() const
    {
        Validate();
#if MTLPP_PLATFORM_MAC
        return uint32_t([(MTLRenderPassDescriptor*)m_ptr renderTargetArrayLength]);
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

    void RenderPassDescriptor::SetRenderTargetArrayLength(uint32_t renderTargetArrayLength)
    {
        Validate();
#if MTLPP_PLATFORM_MAC
        [(MTLRenderPassDescriptor*)m_ptr setRenderTargetArrayLength:renderTargetArrayLength];
#endif
    }
	
	void RenderPassDescriptor::SetSamplePositions(SamplePosition const* positions, uint32_t count)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		[(MTLRenderPassDescriptor*)m_ptr setSamplePositions:(const MTLSamplePosition *)positions count:count];
#endif
	}
	
	uint32_t RenderPassDescriptor::GetSamplePositions(SamplePosition const* positions, uint32_t count)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return [(MTLRenderPassDescriptor*)m_ptr getSamplePositions:(MTLSamplePosition *)positions count:count];
#else
		return 0;
#endif
	}
	
	uint32_t RenderPassDescriptor::GetImageblockSampleLength() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [(MTLRenderPassDescriptor*)m_ptr imageblockSampleLength];
#else
		return 0;
#endif
	}
	
	uint32_t RenderPassDescriptor::GetThreadgroupMemoryLength() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [(MTLRenderPassDescriptor*)m_ptr threadgroupMemoryLength];
#else
		return 0;
#endif
	}
	
	uint32_t RenderPassDescriptor::GetTileWidth() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [(MTLRenderPassDescriptor*)m_ptr tileWidth];
#else
		return 0;
#endif
	}
	
	uint32_t RenderPassDescriptor::GetTileHeight() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [(MTLRenderPassDescriptor*)m_ptr tileHeight];
#else
		return 0;
#endif
	}
	
	uint32_t RenderPassDescriptor::GetDefaultRasterSampleCount() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [(MTLRenderPassDescriptor*)m_ptr defaultRasterSampleCount];
#else
		return 0;
#endif
	}
	
	uint32_t RenderPassDescriptor::GetRenderTargetWidth() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [(MTLRenderPassDescriptor*)m_ptr renderTargetWidth];
#else
		return 0;
#endif
	}
	
	uint32_t RenderPassDescriptor::GetRenderTargetHeight() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return [(MTLRenderPassDescriptor*)m_ptr renderTargetHeight];
#else
		return 0;
#endif
	}
	
	void RenderPassDescriptor::SetImageblockSampleLength(uint32_t Val)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[(MTLRenderPassDescriptor*)m_ptr setImageblockSampleLength:Val];
#endif
	}
	
	void RenderPassDescriptor::SetThreadgroupMemoryLength(uint32_t Val)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[(MTLRenderPassDescriptor*)m_ptr setThreadgroupMemoryLength:Val];
#endif
	}
	
	void RenderPassDescriptor::SetTileWidth(uint32_t Val)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[(MTLRenderPassDescriptor*)m_ptr setTileWidth:Val];
#endif
	}
	
	void RenderPassDescriptor::SetTileHeight(uint32_t Val)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[(MTLRenderPassDescriptor*)m_ptr setRenderTargetHeight:Val];
#endif
	}
	
	void RenderPassDescriptor::SetDefaultRasterSampleCount(uint32_t Val)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[(MTLRenderPassDescriptor*)m_ptr setDefaultRasterSampleCount:Val];
#endif
	}
	
	void RenderPassDescriptor::SetRenderTargetWidth(uint32_t Val)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[(MTLRenderPassDescriptor*)m_ptr setRenderTargetWidth:Val];
#endif
	}
	
	void RenderPassDescriptor::SetRenderTargetHeight(uint32_t Val)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[(MTLRenderPassDescriptor*)m_ptr setRenderTargetHeight:Val];
#endif
	}
}
