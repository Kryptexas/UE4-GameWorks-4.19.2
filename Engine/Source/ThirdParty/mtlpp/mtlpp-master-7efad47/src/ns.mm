/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include "ns.hpp"

namespace ns
{
    uint32_t ArrayBase::GetSize() const
    {
		ns::Object<NSArray*>::Validate();
        return uint32_t([ns::Object<NSArray*>::m_ptr count]);
    }

    void* ArrayBase::GetItem(uint32_t index) const
    {
        ns::Object<NSArray*>::Validate();
        return (void*)[ns::Object<NSArray*>::m_ptr objectAtIndexedSubscript:index];
    }

    String::String(const char* cstr) :
        Object<NSString*>([NSString stringWithUTF8String:cstr])
    {
    }

    const char* String::GetCStr() const
    {
        Validate();
        return [(NSString*)m_ptr cStringUsingEncoding:NSUTF8StringEncoding];
    }

    uint32_t String::GetLength() const
    {
        Validate();
        return uint32_t([(NSString*)m_ptr length]);
    }

    Error::Error()
    {

    }

    String Error::GetDomain() const
    {
        Validate();
        return [(NSError*)m_ptr domain];
    }

    uint32_t Error::GetCode() const
    {
        Validate();
        return uint32_t([(NSError*)m_ptr code]);
    }

    //@property (readonly, copy) NSDictionary *userInfo;

    String Error::GetLocalizedDescription() const
    {
        Validate();
        return [(NSError*)m_ptr localizedDescription];
    }

    String Error::GetLocalizedFailureReason() const
    {
        Validate();
        return [(NSError*)m_ptr localizedFailureReason];
    }

    String Error::GetLocalizedRecoverySuggestion() const
    {
        Validate();
        return [(NSError*)m_ptr localizedRecoverySuggestion];
    }

    Array<String> Error::GetLocalizedRecoveryOptions() const
    {
        Validate();
        return (NSArray*)[(NSError*)m_ptr localizedRecoveryOptions];
    }

    //@property (nullable, readonly, strong) id recoveryAttempter;

    String Error::GetHelpAnchor() const
    {
        Validate();
        return [(NSError*)m_ptr helpAnchor];
    }
}
