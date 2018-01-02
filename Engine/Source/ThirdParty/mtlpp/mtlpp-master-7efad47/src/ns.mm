/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include "ns.hpp"

MTLPP_BEGIN

namespace ns
{
    NSUInteger ArrayBase::GetSize() const
    {
		ns::Object<NSArray<id<NSObject>>*>::Validate();
        return NSUInteger([ns::Object<NSArray<id<NSObject>>*>::m_ptr count]);
    }

    void* ArrayBase::GetItem(NSUInteger index) const
    {
        ns::Object<NSArray<id<NSObject>>*>::Validate();
        return (void*)[ns::Object<NSArray<id<NSObject>>*>::m_ptr objectAtIndexedSubscript:index];
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

    NSUInteger String::GetLength() const
    {
        Validate();
        return NSUInteger([(NSString*)m_ptr length]);
    }

    AutoReleasedError::AutoReleasedError()
    {
    }

    String AutoReleasedError::GetDomain() const
    {
        Validate();
        return [(NSError*)m_ptr domain];
    }

    NSUInteger AutoReleasedError::GetCode() const
    {
        Validate();
        return NSUInteger([(NSError*)m_ptr code]);
    }

    //@property (readonly, copy) NSDictionary *userInfo;

    String AutoReleasedError::GetLocalizedDescription() const
    {
        Validate();
        return [(NSError*)m_ptr localizedDescription];
    }

    String AutoReleasedError::GetLocalizedFailureReason() const
    {
        Validate();
        return [(NSError*)m_ptr localizedFailureReason];
    }

    String AutoReleasedError::GetLocalizedRecoverySuggestion() const
    {
        Validate();
        return [(NSError*)m_ptr localizedRecoverySuggestion];
    }

    Array<String> AutoReleasedError::GetLocalizedRecoveryOptions() const
    {
        Validate();
        return (NSArray<NSString*>*)[(NSError*)m_ptr localizedRecoveryOptions];
    }

    //@property (nullable, readonly, strong) id recoveryAttempter;

    String AutoReleasedError::GetHelpAnchor() const
    {
        Validate();
        return [(NSError*)m_ptr helpAnchor];
    }
	
	AutoReleasedError& AutoReleasedError::operator=(NSError* const handle)
	{
		m_ptr = handle;
		return *this;
	}

	
	Error::Error(const AutoReleasedError& rhs)
	{
		operator=(rhs);
	}
	
#if MTLPP_CONFIG_RVALUE_REFERENCES
	Error::Error(const AutoReleasedError&& rhs)
	{
		operator=(rhs);
	}
#endif
	
	Error& Error::operator=(const AutoReleasedError& rhs)
	{
		if (m_ptr != rhs.m_ptr)
		{
			if(rhs.m_ptr)
			{
				rhs.m_table->Retain(rhs.m_ptr);
			}
			if(m_ptr)
			{
				m_table->Release(m_ptr);
			}
			m_ptr = rhs.m_ptr;
			m_table = rhs.m_table;
		}
		return *this;
	}
	
#if MTLPP_CONFIG_RVALUE_REFERENCES
	Error& Error::operator=(AutoReleasedError&& rhs)
	{
		if (m_ptr != rhs.m_ptr)
		{
			if(rhs.m_ptr)
			{
				rhs.m_table->Retain(rhs.m_ptr);
			}
			if(m_ptr)
			{
				m_table->Release(m_ptr);
			}
			m_ptr = rhs.m_ptr;
			m_table = rhs.m_table;
			rhs.m_ptr = nullptr;
			rhs.m_table = nullptr;
		}
		return *this;
	}
#endif
	
	Error::Error()
	{
		
	}
	
	String Error::GetDomain() const
	{
		Validate();
		return [(NSError*)m_ptr domain];
	}
	
	NSUInteger Error::GetCode() const
	{
		Validate();
		return NSUInteger([(NSError*)m_ptr code]);
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
		return (NSArray<NSString*>*)[(NSError*)m_ptr localizedRecoveryOptions];
	}
	
	//@property (nullable, readonly, strong) id recoveryAttempter;
	
	String Error::GetHelpAnchor() const
	{
		Validate();
		return [(NSError*)m_ptr helpAnchor];
	}
}

MTLPP_END
