/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once

#include "command_encoder.hpp"
#include "device.hpp"
#ifdef __OBJC__
#import <Metal/MTLCommandEncoder.h>
#endif

namespace mtlpp
{
	template<typename T>
    Device CommandEncoder<T>::GetDevice() const
    {
        this->Validate();
#ifdef __OBJC__
        return [(id<MTLCommandEncoder>)this->m_ptr device];
#else
		return Device();
#endif
    }

	template<typename T>
    ns::String CommandEncoder<T>::GetLabel() const
    {
        this->Validate();
#ifdef __OBJC__
        return [(id<MTLCommandEncoder>)this->m_ptr label];
#else
		return ns::String();
#endif
    }

	template<typename T>
    void CommandEncoder<T>::SetLabel(const ns::String& label)
    {
        this->Validate();
#ifdef __OBJC__
        [(id<MTLCommandEncoder>)this->m_ptr setLabel:(NSString*)label.GetPtr()];
#endif
    }

	template<typename T>
    void CommandEncoder<T>::EndEncoding()
    {
        this->Validate();
#ifdef __OBJC__
        [(id<MTLCommandEncoder>)this->m_ptr endEncoding];
#endif
    }

	template<typename T>
    void CommandEncoder<T>::InsertDebugSignpost(const ns::String& string)
    {
       this->Validate();
#ifdef __OBJC__
        [(id<MTLCommandEncoder>)this->m_ptr insertDebugSignpost:(NSString*)string.GetPtr()];
#endif
    }

	template<typename T>
    void CommandEncoder<T>::PushDebugGroup(const ns::String& string)
    {
        this->Validate();
#ifdef __OBJC__
        [(id<MTLCommandEncoder>)this->m_ptr pushDebugGroup:(NSString*)string.GetPtr()];
#endif
    }

	template<typename T>
    void CommandEncoder<T>::PopDebugGroup()
    {
        this->Validate();
#ifdef __OBJC__
        [(id<MTLCommandEncoder>)this->m_ptr popDebugGroup];
#endif
    }
}
