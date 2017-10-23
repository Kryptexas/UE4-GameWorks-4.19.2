/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include "sampler.hpp"
#include <Metal/MTLSampler.h>

namespace mtlpp
{
    SamplerDescriptor::SamplerDescriptor() :
        ns::Object<MTLSamplerDescriptor*>([[MTLSamplerDescriptor alloc] init], false)
    {
    }

    SamplerMinMagFilter SamplerDescriptor::GetMinFilter() const
    {
        Validate();
        return SamplerMinMagFilter([(MTLSamplerDescriptor*)m_ptr minFilter]);
    }

    SamplerMinMagFilter SamplerDescriptor::GetMagFilter() const
    {
        Validate();
        return SamplerMinMagFilter([(MTLSamplerDescriptor*)m_ptr magFilter]);
    }

    SamplerMipFilter SamplerDescriptor::GetMipFilter() const
    {
        Validate();
        return SamplerMipFilter([(MTLSamplerDescriptor*)m_ptr mipFilter]);
    }

    uint32_t SamplerDescriptor::GetMaxAnisotropy() const
    {
        Validate();
        return uint32_t([(MTLSamplerDescriptor*)m_ptr maxAnisotropy]);
    }

    SamplerAddressMode SamplerDescriptor::GetSAddressMode() const
    {
        Validate();
        return SamplerAddressMode([(MTLSamplerDescriptor*)m_ptr sAddressMode]);
    }

    SamplerAddressMode SamplerDescriptor::GetTAddressMode() const
    {
        Validate();
        return SamplerAddressMode([(MTLSamplerDescriptor*)m_ptr tAddressMode]);
    }

    SamplerAddressMode SamplerDescriptor::GetRAddressMode() const
    {
        Validate();
        return SamplerAddressMode([(MTLSamplerDescriptor*)m_ptr rAddressMode]);
    }

    SamplerBorderColor SamplerDescriptor::GetBorderColor() const
    {
#if MTLPP_PLATFORM_MAC
		if (@available(macOS 10.12, *))
			return SamplerBorderColor([(MTLSamplerDescriptor*)m_ptr borderColor]);
		else
#endif
        return SamplerBorderColor(0);
    }

    bool SamplerDescriptor::IsNormalizedCoordinates() const
    {
        Validate();
        return [(MTLSamplerDescriptor*)m_ptr normalizedCoordinates];
    }

    float SamplerDescriptor::GetLodMinClamp() const
    {
        Validate();
        return [(MTLSamplerDescriptor*)m_ptr lodMinClamp];
    }

    float SamplerDescriptor::GetLodMaxClamp() const
    {
        Validate();
        return [(MTLSamplerDescriptor*)m_ptr lodMaxClamp];
    }

    CompareFunction SamplerDescriptor::GetCompareFunction() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
        return CompareFunction([(MTLSamplerDescriptor*)m_ptr compareFunction]);
#else
        return CompareFunction(0);
#endif
    }

    ns::String SamplerDescriptor::GetLabel() const
    {
        Validate();
        return [(MTLSamplerDescriptor*)m_ptr label];
    }
	
	bool SamplerDescriptor::SupportArgumentBuffers() const
	{
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return [(MTLSamplerDescriptor*)m_ptr supportArgumentBuffers];
#else
		return false;
#endif
	}

    void SamplerDescriptor::SetMinFilter(SamplerMinMagFilter minFilter)
    {
        Validate();
        [(MTLSamplerDescriptor*)m_ptr setMinFilter:MTLSamplerMinMagFilter(minFilter)];
    }

    void SamplerDescriptor::SetMagFilter(SamplerMinMagFilter magFilter)
    {
        Validate();
        [(MTLSamplerDescriptor*)m_ptr setMagFilter:MTLSamplerMinMagFilter(magFilter)];
    }

    void SamplerDescriptor::SetMipFilter(SamplerMipFilter mipFilter)
    {
        Validate();
        [(MTLSamplerDescriptor*)m_ptr setMipFilter:MTLSamplerMipFilter(mipFilter)];
    }

    void SamplerDescriptor::SetMaxAnisotropy(uint32_t maxAnisotropy)
    {
        Validate();
        [(MTLSamplerDescriptor*)m_ptr setMaxAnisotropy:maxAnisotropy];
    }

    void SamplerDescriptor::SetSAddressMode(SamplerAddressMode sAddressMode)
    {
        Validate();
        [(MTLSamplerDescriptor*)m_ptr setSAddressMode:MTLSamplerAddressMode(sAddressMode)];
    }

    void SamplerDescriptor::SetTAddressMode(SamplerAddressMode tAddressMode)
    {
        Validate();
        [(MTLSamplerDescriptor*)m_ptr setTAddressMode:MTLSamplerAddressMode(tAddressMode)];
    }

    void SamplerDescriptor::SetRAddressMode(SamplerAddressMode rAddressMode)
    {
        Validate();
        [(MTLSamplerDescriptor*)m_ptr setRAddressMode:MTLSamplerAddressMode(rAddressMode)];
    }

    void SamplerDescriptor::SetBorderColor(SamplerBorderColor borderColor)
    {
#if MTLPP_PLATFORM_MAC
		if (@available(macOS 10.12, *))
			[(MTLSamplerDescriptor*)m_ptr setBorderColor:MTLSamplerBorderColor(borderColor)];
#endif
    }

    void SamplerDescriptor::SetNormalizedCoordinates(bool normalizedCoordinates)
    {
        Validate();
        [(MTLSamplerDescriptor*)m_ptr setNormalizedCoordinates:normalizedCoordinates];
    }

    void SamplerDescriptor::SetLodMinClamp(float lodMinClamp)
    {
        Validate();
        [(MTLSamplerDescriptor*)m_ptr setLodMinClamp:lodMinClamp];
    }

    void SamplerDescriptor::SetLodMaxClamp(float lodMaxClamp)
    {
        Validate();
        [(MTLSamplerDescriptor*)m_ptr setLodMaxClamp:lodMaxClamp];
    }

    void SamplerDescriptor::SetCompareFunction(CompareFunction compareFunction)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
        [(MTLSamplerDescriptor*)m_ptr setCompareFunction:MTLCompareFunction(compareFunction)];
#endif
    }

    void SamplerDescriptor::SetLabel(const ns::String& label)
    {
        Validate();
        [(MTLSamplerDescriptor*)m_ptr setLabel:(NSString*)label.GetPtr()];
    }

    ns::String SamplerState::GetLabel() const
    {
        Validate();
        return [(id<MTLSamplerState>)m_ptr label];
    }

    Device SamplerState::GetDevice() const
    {
        Validate();
        return [(id<MTLSamplerState>)m_ptr device];
    }
}

