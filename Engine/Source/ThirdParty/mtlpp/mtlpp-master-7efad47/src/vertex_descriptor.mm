/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLVertexDescriptor.h>
#include "vertex_descriptor.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    VertexBufferLayoutDescriptor::VertexBufferLayoutDescriptor() :
        ns::Object<MTLVertexBufferLayoutDescriptor*>([[MTLVertexBufferLayoutDescriptor alloc] init], false)
    {
    }

    NSUInteger VertexBufferLayoutDescriptor::GetStride() const
    {
        Validate();
        return NSUInteger([(MTLVertexBufferLayoutDescriptor*)m_ptr stride]);
    }

    NSUInteger VertexBufferLayoutDescriptor::GetStepRate() const
    {
        Validate();
        return NSUInteger([(MTLVertexBufferLayoutDescriptor*)m_ptr stepRate]);
    }

    VertexStepFunction VertexBufferLayoutDescriptor::GetStepFunction() const
    {
        Validate();
        return VertexStepFunction([(MTLVertexBufferLayoutDescriptor*)m_ptr stepFunction]);
    }

    void VertexBufferLayoutDescriptor::SetStride(NSUInteger stride)
    {
        Validate();
        [(MTLVertexBufferLayoutDescriptor*)m_ptr setStride:stride];
    }

    void VertexBufferLayoutDescriptor::SetStepRate(NSUInteger stepRate)
    {
        Validate();
        [(MTLVertexBufferLayoutDescriptor*)m_ptr setStepRate:stepRate];
    }

    void VertexBufferLayoutDescriptor::SetStepFunction(VertexStepFunction stepFunction)
    {
        Validate();
        [(MTLVertexBufferLayoutDescriptor*)m_ptr setStepFunction:MTLVertexStepFunction(stepFunction)];
    }

    VertexAttributeDescriptor::VertexAttributeDescriptor() :
        ns::Object<MTLVertexAttributeDescriptor*>([[MTLVertexAttributeDescriptor alloc] init], false)
    {
    }

    VertexFormat VertexAttributeDescriptor::GetFormat() const
    {
        Validate();
        return VertexFormat([(MTLVertexAttributeDescriptor*)m_ptr format]);
    }

    NSUInteger VertexAttributeDescriptor::GetOffset() const
    {
        Validate();
        return NSUInteger([(MTLVertexAttributeDescriptor*)m_ptr offset]);
    }

    NSUInteger VertexAttributeDescriptor::GetBufferIndex() const
    {
        Validate();
        return NSUInteger([(MTLVertexAttributeDescriptor*)m_ptr bufferIndex]);
    }

    void VertexAttributeDescriptor::SetFormat(VertexFormat format)
    {
        Validate();
        [(MTLVertexAttributeDescriptor*)m_ptr setFormat:MTLVertexFormat(format)];
    }

    void VertexAttributeDescriptor::SetOffset(NSUInteger offset)
    {
        Validate();
        [(MTLVertexAttributeDescriptor*)m_ptr setOffset:offset];
    }

    void VertexAttributeDescriptor::SetBufferIndex(NSUInteger bufferIndex)
    {
        Validate();
        [(MTLVertexAttributeDescriptor*)m_ptr setBufferIndex:bufferIndex];
    }

    VertexDescriptor::VertexDescriptor() :
        ns::Object<MTLVertexDescriptor*>([[MTLVertexDescriptor alloc] init], false)
    {
    }

    ns::Array<VertexBufferLayoutDescriptor> VertexDescriptor::GetLayouts() const
    {
        Validate();
        return (NSArray*)[(MTLVertexDescriptor*)m_ptr layouts];
    }

    ns::Array<VertexAttributeDescriptor> VertexDescriptor::GetAttributes() const
    {
        Validate();
        return (NSArray*)[(MTLVertexDescriptor*)m_ptr attributes];
    }

    void VertexDescriptor::Reset()
    {
        Validate();
        [(MTLVertexDescriptor*)m_ptr reset];
    }
}

MTLPP_END
