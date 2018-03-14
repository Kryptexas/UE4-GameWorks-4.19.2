/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLStageInputOutputDescriptor.h>
#include "stage_input_output_descriptor.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    BufferLayoutDescriptor::BufferLayoutDescriptor() :
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        ns::Object<MTLBufferLayoutDescriptor*>([[MTLBufferLayoutDescriptor alloc] init], false)
#else
        ns::Object<MTLBufferLayoutDescriptor*>(nullptr)
#endif
    {
    }

    NSUInteger BufferLayoutDescriptor::GetStride() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return NSUInteger([(MTLBufferLayoutDescriptor*)m_ptr stride]);
#else
        return 0;
#endif
    }

    StepFunction BufferLayoutDescriptor::GetStepFunction() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return StepFunction([(MTLBufferLayoutDescriptor*)m_ptr stepFunction]);
#else
        return StepFunction(0);
#endif
    }

    NSUInteger BufferLayoutDescriptor::GetStepRate() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return NSUInteger([(MTLBufferLayoutDescriptor*)m_ptr stepRate]);
#else
        return 0;
#endif
    }

    void BufferLayoutDescriptor::SetStride(NSUInteger stride)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        [(MTLBufferLayoutDescriptor*)m_ptr setStride:stride];
#endif
    }

    void BufferLayoutDescriptor::SetStepFunction(StepFunction stepFunction)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        [(MTLBufferLayoutDescriptor*)m_ptr setStepFunction:MTLStepFunction(stepFunction)];
#endif
    }

    void BufferLayoutDescriptor::SetStepRate(NSUInteger stepRate)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        [(MTLBufferLayoutDescriptor*)m_ptr setStepRate:stepRate];
#endif
    }

    AttributeDescriptor::AttributeDescriptor() :
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        ns::Object<MTLAttributeDescriptor*>([[MTLAttributeDescriptor alloc] init], false)
#else
        ns::Object<MTLAttributeDescriptor*>(nullptr)
#endif
    {
    }

    AttributeFormat AttributeDescriptor::GetFormat() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return AttributeFormat([(MTLAttributeDescriptor*)m_ptr format]);
#else
        return AttributeFormat(0);
#endif
    }

    NSUInteger AttributeDescriptor::GetOffset() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return NSUInteger([(MTLAttributeDescriptor*)m_ptr offset]);
#else
        return 0;
#endif
    }

    NSUInteger AttributeDescriptor::GetBufferIndex() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return NSUInteger([(MTLAttributeDescriptor*)m_ptr bufferIndex]);
#else
        return 0;
#endif
    }

    void AttributeDescriptor::SetFormat(AttributeFormat format)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        [(MTLAttributeDescriptor*)m_ptr setFormat:MTLAttributeFormat(format)];
#endif
    }

    void AttributeDescriptor::SetOffset(NSUInteger offset)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        [(MTLAttributeDescriptor*)m_ptr setOffset:offset];
#endif
    }

    void AttributeDescriptor::SetBufferIndex(NSUInteger bufferIndex)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        [(MTLAttributeDescriptor*)m_ptr setBufferIndex:bufferIndex];
#endif
    }

    StageInputOutputDescriptor::StageInputOutputDescriptor() :
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        ns::Object<MTLStageInputOutputDescriptor*>([[MTLStageInputOutputDescriptor alloc] init], false)
#else
        ns::Object<MTLStageInputOutputDescriptor*>(nullptr)
#endif
    {
    }

    ns::Array<BufferLayoutDescriptor> StageInputOutputDescriptor::GetLayouts() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return (NSArray<MTLBufferLayoutDescriptor*>*)[(MTLStageInputOutputDescriptor*)m_ptr layouts];
#else
        return nullptr;
#endif
    }

    ns::Array<AttributeDescriptor> StageInputOutputDescriptor::GetAttributes() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return (NSArray<MTLAttributeDescriptor*>*)[(MTLStageInputOutputDescriptor*)m_ptr attributes];
#else
        return nullptr;
#endif
    }

    IndexType StageInputOutputDescriptor::GetIndexType() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return IndexType([(MTLStageInputOutputDescriptor*)m_ptr indexType]);
#else
        return IndexType(0);
#endif
    }

    NSUInteger StageInputOutputDescriptor::GetIndexBufferIndex() const
   {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return NSUInteger([(MTLStageInputOutputDescriptor*)m_ptr indexBufferIndex]);
#else
        return 0;
#endif
    }

   void StageInputOutputDescriptor::SetIndexType(IndexType indexType)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        [(MTLStageInputOutputDescriptor*)m_ptr setIndexType:MTLIndexType(indexType)];
#endif
    }

    void StageInputOutputDescriptor::SetIndexBufferIndex(NSUInteger indexBufferIndex)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        [(MTLStageInputOutputDescriptor*)m_ptr setIndexBufferIndex:indexBufferIndex];
#endif
    }

    void StageInputOutputDescriptor::Reset()
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        [(MTLStageInputOutputDescriptor*)m_ptr reset];
#endif
    }
}


MTLPP_END
