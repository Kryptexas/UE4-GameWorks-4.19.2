/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_CommandEncoder.hpp"
#include "ns.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    class Device;
	
	enum class ResourceUsage
	{
		Read   = 1 << 0,
		Write  = 1 << 1,
		Sample = 1 << 2
	}
	MTLPP_AVAILABLE(10_13, 11_0);

	template<typename T>
	class CommandEncoder : public ns::Object<T>
    {
    public:
        CommandEncoder() { }
        CommandEncoder(T handle) : ns::Object<T>(handle) { }

        Device     GetDevice() const;
        ns::String GetLabel() const;

        void SetLabel(const ns::String& label);

        void EndEncoding();
        void InsertDebugSignpost(const ns::String& string);
        void PushDebugGroup(const ns::String& string);
        void PopDebugGroup();
    }
    MTLPP_AVAILABLE(10_11, 8_0);
}

#include "command_encoder.mm"

MTLPP_END
