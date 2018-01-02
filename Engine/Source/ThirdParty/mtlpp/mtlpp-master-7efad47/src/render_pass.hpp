/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "ns.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    class Texture;
    class Buffer;
	class SamplePosition;

    enum class LoadAction
    {
        DontCare = 0,
        Load     = 1,
        Clear    = 2,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class StoreAction
    {
        DontCare                                               = 0,
        Store                                                  = 1,
        MultisampleResolve                                     = 2,
        StoreAndMultisampleResolve MTLPP_AVAILABLE(10_12,10_0) = 3,
        Unknown                    MTLPP_AVAILABLE(10_12,10_0) = 4,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class MultisampleDepthResolveFilter
    {
        Sample0 = 0,
        Min     = 1,
        Max     = 2,
    }
    MTLPP_AVAILABLE_AX(9_0);

	enum class StoreActionOptions
	{
		None                  = 0,
		CustomSamplePositions = 1 << 0,
	}
	MTLPP_AVAILABLE(10_13, 11_0);
	
    struct ClearColor
    {
        ClearColor(double red, double green, double blue, double alpha) :
            Red(red),
            Green(green),
            Blue(blue),
            Alpha(alpha) { }

        double Red;
        double Green;
        double Blue;
        double Alpha;
    };

	template<typename T>
    class RenderPassAttachmentDescriptor : public ns::Object<T>
    {
    public:
        RenderPassAttachmentDescriptor();
        RenderPassAttachmentDescriptor(T handle) : ns::Object<T>(handle) { }

        Texture     GetTexture() const;
        NSUInteger    GetLevel() const;
        NSUInteger    GetSlice() const;
        NSUInteger    GetDepthPlane() const;
        Texture     GetResolveTexture() const;
        NSUInteger    GetResolveLevel() const;
        NSUInteger    GetResolveSlice() const;
        NSUInteger    GetResolveDepthPlane() const;
        LoadAction  GetLoadAction() const;
        StoreAction GetStoreAction() const;
		StoreActionOptions GetStoreActionOptions() const MTLPP_AVAILABLE(10_13, 11_0);

        void SetTexture(const Texture& texture);
        void SetLevel(NSUInteger level);
        void SetSlice(NSUInteger slice);
        void SetDepthPlane(NSUInteger depthPlane);
        void SetResolveTexture(const Texture& texture);
        void SetResolveLevel(NSUInteger resolveLevel);
        void SetResolveSlice(NSUInteger resolveSlice);
        void SetResolveDepthPlane(NSUInteger resolveDepthPlane);
        void SetLoadAction(LoadAction loadAction);
        void SetStoreAction(StoreAction storeAction);
		void SetStoreActionOptions(StoreActionOptions options) MTLPP_AVAILABLE(10_13, 11_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    class RenderPassColorAttachmentDescriptor : public RenderPassAttachmentDescriptor<MTLRenderPassColorAttachmentDescriptor*>
    {
    public:
        RenderPassColorAttachmentDescriptor();
        RenderPassColorAttachmentDescriptor(MTLRenderPassColorAttachmentDescriptor* handle) : RenderPassAttachmentDescriptor<MTLRenderPassColorAttachmentDescriptor*>(handle) { }

        ClearColor GetClearColor() const;

        void SetClearColor(const ClearColor& clearColor);
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    class RenderPassDepthAttachmentDescriptor : public RenderPassAttachmentDescriptor<MTLRenderPassDepthAttachmentDescriptor*>
    {
    public:
        RenderPassDepthAttachmentDescriptor();
        RenderPassDepthAttachmentDescriptor(MTLRenderPassDepthAttachmentDescriptor* handle) : RenderPassAttachmentDescriptor<MTLRenderPassDepthAttachmentDescriptor*>(handle) { }

        double                        GetClearDepth() const;
        MultisampleDepthResolveFilter GetDepthResolveFilter() const MTLPP_AVAILABLE_AX(9_0);

        void SetClearDepth(double clearDepth);
        void SetDepthResolveFilter(MultisampleDepthResolveFilter depthResolveFilter) MTLPP_AVAILABLE_AX(9_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    class RenderPassStencilAttachmentDescriptor : public RenderPassAttachmentDescriptor<MTLRenderPassStencilAttachmentDescriptor*>
    {
    public:
        RenderPassStencilAttachmentDescriptor();
        RenderPassStencilAttachmentDescriptor(MTLRenderPassStencilAttachmentDescriptor* handle) : RenderPassAttachmentDescriptor<MTLRenderPassStencilAttachmentDescriptor*>(handle) { }

        NSUInteger GetClearStencil() const;

        void SetClearStencil(uint32_t clearStencil);
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    class RenderPassDescriptor : public ns::Object<MTLRenderPassDescriptor*>
    {
    public:
        RenderPassDescriptor();
        RenderPassDescriptor(MTLRenderPassDescriptor* handle) : ns::Object<MTLRenderPassDescriptor*>(handle) { }

        ns::Array<RenderPassColorAttachmentDescriptor> GetColorAttachments() const;
        RenderPassDepthAttachmentDescriptor   GetDepthAttachment() const;
        RenderPassStencilAttachmentDescriptor GetStencilAttachment() const;
        Buffer                                GetVisibilityResultBuffer() const;
        NSUInteger                              GetRenderTargetArrayLength() const MTLPP_AVAILABLE_MAC(10_11);

        void SetDepthAttachment(const RenderPassDepthAttachmentDescriptor& depthAttachment);
        void SetStencilAttachment(const RenderPassStencilAttachmentDescriptor& stencilAttachment);
        void SetVisibilityResultBuffer(const Buffer& visibilityResultBuffer);
        void SetRenderTargetArrayLength(NSUInteger renderTargetArrayLength) MTLPP_AVAILABLE_MAC(10_11);
		
		NSUInteger GetImageblockSampleLength() const MTLPP_AVAILABLE_IOS(11_0);
		NSUInteger GetThreadgroupMemoryLength() const MTLPP_AVAILABLE_IOS(11_0);
		NSUInteger GetTileWidth() const MTLPP_AVAILABLE_IOS(11_0);
		NSUInteger GetTileHeight() const MTLPP_AVAILABLE_IOS(11_0);
		NSUInteger GetDefaultRasterSampleCount() const MTLPP_AVAILABLE_IOS(11_0);
		NSUInteger GetRenderTargetWidth() const MTLPP_AVAILABLE_IOS(11_0);
		NSUInteger GetRenderTargetHeight() const MTLPP_AVAILABLE_IOS(11_0);
		
		void SetImageblockSampleLength(NSUInteger Val) MTLPP_AVAILABLE_IOS(11_0);
		void SetThreadgroupMemoryLength(NSUInteger Val) MTLPP_AVAILABLE_IOS(11_0);
		void SetTileWidth(NSUInteger Val) MTLPP_AVAILABLE_IOS(11_0);
		void SetTileHeight(NSUInteger Val) MTLPP_AVAILABLE_IOS(11_0);
		void SetDefaultRasterSampleCount(NSUInteger Val) MTLPP_AVAILABLE_IOS(11_0);
		void SetRenderTargetWidth(NSUInteger Val) MTLPP_AVAILABLE_IOS(11_0);
		void SetRenderTargetHeight(NSUInteger Val) MTLPP_AVAILABLE_IOS(11_0);
		
		void SetSamplePositions(SamplePosition const* positions, NSUInteger count) MTLPP_AVAILABLE(10_13, 11_0);
		NSUInteger GetSamplePositions(SamplePosition const* positions, NSUInteger count) MTLPP_AVAILABLE(10_13, 11_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);
}

MTLPP_END
