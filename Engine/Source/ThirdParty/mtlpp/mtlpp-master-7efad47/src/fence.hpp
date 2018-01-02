/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_Fence.hpp"
#include "ns.hpp"
#include "device.hpp"

MTLPP_BEGIN

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

MTLPP_END
