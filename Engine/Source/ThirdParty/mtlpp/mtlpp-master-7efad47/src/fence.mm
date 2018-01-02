/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLFence.h>
#include "fence.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    Device Fence::GetDevice() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
		return m_table->Device(m_ptr);
#else
		return nullptr;
#endif
    }

    ns::String Fence::GetLabel() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
		return m_table->Label(m_ptr);
#else
		return ns::String();
#endif
    }

    void Fence::SetLabel(const ns::String& label)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
		m_table->SetLabel(m_ptr, label.GetPtr());
#endif
	}
}

MTLPP_END
