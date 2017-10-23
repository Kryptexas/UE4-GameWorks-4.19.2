/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include "fence.hpp"
#include <Metal/MTLFence.h>

namespace mtlpp
{
    Device Fence::GetDevice() const
    {
        Validate();
		if(@available(macOS 10.13, iOS 10.0, *))
			return [(id<MTLFence>)m_ptr device];
		else
			return nullptr;
    }

    ns::String Fence::GetLabel() const
    {
        Validate();
		if(@available(macOS 10.13, iOS 10.0, *))
			return [(id<MTLFence>)m_ptr label];
		else
			return ns::String();
    }

    void Fence::SetLabel(const ns::String& label)
    {
        Validate();
		if(@available(macOS 10.13, iOS 10.0, *))
			[(id<MTLFence>)m_ptr setLabel:(NSString*)label.GetPtr()];
    }
}
