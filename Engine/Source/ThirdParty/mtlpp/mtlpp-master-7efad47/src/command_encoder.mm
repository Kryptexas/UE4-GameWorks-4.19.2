/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once

#include "command_encoder.hpp"
#include "device.hpp"

MTLPP_BEGIN

namespace mtlpp
{
	template<typename T>
    Device CommandEncoder<T>::GetDevice() const
    {
        this->Validate();
		return this->m_table->Device((id<MTLCommandEncoder>)this->m_ptr);
    }

	template<typename T>
    ns::String CommandEncoder<T>::GetLabel() const
    {
        this->Validate();
		return this->m_table->Label((id<MTLCommandEncoder>)this->m_ptr);
    }

	template<typename T>
    void CommandEncoder<T>::SetLabel(const ns::String& label)
    {
        this->Validate();
		this->m_table->SetLabel((id<MTLCommandEncoder>)this->m_ptr, label.GetPtr());
    }

	template<typename T>
    void CommandEncoder<T>::EndEncoding()
    {
        this->Validate();
		this->m_table->EndEncoding((id<MTLCommandEncoder>)this->m_ptr);
    }

	template<typename T>
    void CommandEncoder<T>::InsertDebugSignpost(const ns::String& string)
    {
    	this->Validate();
		this->m_table->InsertDebugSignpost((id<MTLCommandEncoder>)this->m_ptr, string.GetPtr());
    }

	template<typename T>
    void CommandEncoder<T>::PushDebugGroup(const ns::String& string)
    {
        this->Validate();
		this->m_table->PushDebugGroup((id<MTLCommandEncoder>)this->m_ptr, string.GetPtr());
    }

	template<typename T>
    void CommandEncoder<T>::PopDebugGroup()
    {
        this->Validate();
		this->m_table->PopDebugGroup((id<MTLCommandEncoder>)this->m_ptr);
    }
}

MTLPP_END
