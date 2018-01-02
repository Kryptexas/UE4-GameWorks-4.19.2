/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLDepthStencil.h>
#include "depth_stencil.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    StencilDescriptor::StencilDescriptor() :
        ns::Object<MTLStencilDescriptor*>([[MTLStencilDescriptor alloc] init], false)
    {
    }

    CompareFunction StencilDescriptor::GetStencilCompareFunction() const
    {
        Validate();
        return CompareFunction([(MTLStencilDescriptor*)m_ptr stencilCompareFunction]);
    }

    StencilOperation StencilDescriptor::GetStencilFailureOperation() const
    {
        Validate();
        return StencilOperation([(MTLStencilDescriptor*)m_ptr stencilFailureOperation]);
    }

    StencilOperation StencilDescriptor::GetDepthFailureOperation() const
    {
        Validate();
        return StencilOperation([(MTLStencilDescriptor*)m_ptr depthFailureOperation]);
    }

    StencilOperation StencilDescriptor::GetDepthStencilPassOperation() const
    {
        Validate();
        return StencilOperation([(MTLStencilDescriptor*)m_ptr depthStencilPassOperation]);
    }

    uint32_t StencilDescriptor::GetReadMask() const
    {
        Validate();
        return ([(MTLStencilDescriptor*)m_ptr readMask]);
    }

    uint32_t StencilDescriptor::GetWriteMask() const
    {
        Validate();
        return ([(MTLStencilDescriptor*)m_ptr writeMask]);
    }

    void StencilDescriptor::SetStencilCompareFunction(CompareFunction stencilCompareFunction)
    {
        Validate();
        [(MTLStencilDescriptor*)m_ptr setStencilCompareFunction:MTLCompareFunction(stencilCompareFunction)];
    }

    void StencilDescriptor::SetStencilFailureOperation(StencilOperation stencilFailureOperation)
    {
        Validate();
        [(MTLStencilDescriptor*)m_ptr setStencilFailureOperation:MTLStencilOperation(stencilFailureOperation)];
    }

    void StencilDescriptor::SetDepthFailureOperation(StencilOperation depthFailureOperation)
    {
        Validate();
        [(MTLStencilDescriptor*)m_ptr setDepthFailureOperation:MTLStencilOperation(depthFailureOperation)];
    }

    void StencilDescriptor::SetDepthStencilPassOperation(StencilOperation depthStencilPassOperation)
    {
        Validate();
        [(MTLStencilDescriptor*)m_ptr setDepthStencilPassOperation:MTLStencilOperation(depthStencilPassOperation)];
    }

    void StencilDescriptor::SetReadMask(uint32_t readMask)
    {
        Validate();
        [(MTLStencilDescriptor*)m_ptr setReadMask:readMask];
    }

    void StencilDescriptor::SetWriteMask(uint32_t writeMask)
    {
        Validate();
        [(MTLStencilDescriptor*)m_ptr setWriteMask:writeMask];
    }

    DepthStencilDescriptor::DepthStencilDescriptor() :
        ns::Object<MTLDepthStencilDescriptor*>([[MTLDepthStencilDescriptor alloc] init], false)
    {
    }

    CompareFunction DepthStencilDescriptor::GetDepthCompareFunction() const
    {
        Validate();
        return CompareFunction([(MTLDepthStencilDescriptor*)m_ptr depthCompareFunction]);
    }

    bool DepthStencilDescriptor::IsDepthWriteEnabled() const
    {
        Validate();
        return [(MTLDepthStencilDescriptor*)m_ptr isDepthWriteEnabled];
    }

    StencilDescriptor DepthStencilDescriptor::GetFrontFaceStencil() const
    {
        Validate();
        return [(MTLDepthStencilDescriptor*)m_ptr frontFaceStencil];
    }

    StencilDescriptor DepthStencilDescriptor::GetBackFaceStencil() const
    {
        Validate();
        return [(MTLDepthStencilDescriptor*)m_ptr backFaceStencil];
    }

    ns::String DepthStencilDescriptor::GetLabel() const
    {
        Validate();
        return [(MTLDepthStencilDescriptor*)m_ptr label];
    }

    void DepthStencilDescriptor::SetDepthCompareFunction(CompareFunction depthCompareFunction) const
    {
        Validate();
        [(MTLDepthStencilDescriptor*)m_ptr setDepthCompareFunction:MTLCompareFunction(depthCompareFunction)];
    }

    void DepthStencilDescriptor::SetDepthWriteEnabled(bool depthWriteEnabled) const
    {
        Validate();
        [(MTLDepthStencilDescriptor*)m_ptr setDepthWriteEnabled:depthWriteEnabled];
    }

    void DepthStencilDescriptor::SetFrontFaceStencil(const StencilDescriptor& frontFaceStencil) const
    {
        Validate();
        [(MTLDepthStencilDescriptor*)m_ptr setFrontFaceStencil:(MTLStencilDescriptor*)frontFaceStencil.GetPtr()];
    }

    void DepthStencilDescriptor::SetBackFaceStencil(const StencilDescriptor& backFaceStencil) const
    {
        Validate();
        [(MTLDepthStencilDescriptor*)m_ptr setBackFaceStencil:(MTLStencilDescriptor*)backFaceStencil.GetPtr()];
    }

    void DepthStencilDescriptor::SetLabel(const ns::String& label) const
    {
        Validate();
        [(MTLDepthStencilDescriptor*)m_ptr setLabel:(NSString*)label.GetPtr()];
    }

    ns::String DepthStencilState::GetLabel() const
    {
        Validate();
		return m_table->Label(m_ptr);
    }

    Device DepthStencilState::GetDevice() const
    {
        Validate();
		return m_table->Device(m_ptr);
    }
}

MTLPP_END
