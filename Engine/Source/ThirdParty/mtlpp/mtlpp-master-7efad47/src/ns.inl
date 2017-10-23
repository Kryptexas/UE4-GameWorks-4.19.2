/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include "ns.hpp"
#ifdef __OBJC__
#import <CoreFoundation/CFBase.h>
#import <Foundation/NSString.h>
#import <Foundation/NSError.h>
#import <Foundation/NSArray.h>
#import <cstring>
#endif

namespace ns
{
	template<typename T>
    Object<T>::Object() :
        m_ptr(nullptr),
		RefCount(0)
    {
    }

	template<typename T>
    Object<T>::Object(const T handle, bool const retain) :
	m_ptr(handle),
	RefCount(0)
    {
#ifdef __OBJC__
		if (m_ptr && retain)
			CFRetain(m_ptr);
#endif
    }

	template<typename T>
    Object<T>::Object(const Object& rhs) :
	m_ptr(rhs.m_ptr),
	RefCount(0)
    {
#ifdef __OBJC__
		if (m_ptr)
            CFRetain(m_ptr);
#endif
    }

#if MTLPP_CONFIG_RVALUE_REFERENCES
	template<typename T>
    Object<T>::Object(Object&& rhs) :
	m_ptr(rhs.m_ptr),
	RefCount(0)
    {
        rhs.m_ptr = nullptr;
    }
#endif

	template<typename T>
    Object<T>::~Object()
    {
#ifdef __OBJC__
        if (m_ptr)
            CFRelease(m_ptr);
#endif
    }

	template<typename T>
    Object<T>& Object<T>::operator=(const Object& rhs)
    {
        if (rhs.m_ptr == m_ptr)
            return *this;
#ifdef __OBJC__
        if (rhs.m_ptr)
            CFRetain(rhs.m_ptr);
        if (m_ptr)
            CFRelease(m_ptr);
#endif
        m_ptr = rhs.m_ptr;
        return *this;
    }

#if MTLPP_CONFIG_RVALUE_REFERENCES
	template<typename T>
    Object<T>& Object<T>::operator=(Object&& rhs)
    {
        if (rhs.m_ptr == m_ptr)
            return *this;
#ifdef __OBJC__
        if (m_ptr)
            CFRelease(m_ptr);
#endif
        m_ptr = rhs.m_ptr;
        rhs.m_ptr = nullptr;
        return *this;
    }
#endif
}
