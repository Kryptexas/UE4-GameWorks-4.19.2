/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once

#include "defines.hpp"
#include "ns.hpp"
#include "device.hpp"

MTLPP_PROTOCOL(MTLFence);

namespace mtlpp
{
    class Fence : public ns::Object<ns::Protocol<id<MTLFence>>::type>
    {
    public:
        Fence() { }
        Fence(ns::Protocol<id<MTLFence>>::type handle) : ns::Object<ns::Protocol<id<MTLFence>>::type>(handle) { }

        Device    GetDevice() const;
        ns::String GetLabel() const;

        void SetLabel(const ns::String& label);
    }
    MTLPP_AVAILABLE(10_13, 10_0);
}
