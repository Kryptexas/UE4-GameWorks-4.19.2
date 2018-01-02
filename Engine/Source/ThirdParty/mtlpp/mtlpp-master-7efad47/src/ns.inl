/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include "ns.hpp"
#include "imp_cache.hpp"
#ifdef __OBJC__
#import <CoreFoundation/CFBase.h>
#import <Foundation/NSString.h>
#import <Foundation/NSError.h>
#import <Foundation/NSArray.h>
#import <cstring>
#endif

namespace ns
{
	template<typename T, bool bAutoReleased>
    Object<T, bAutoReleased>::Object() :
        m_ptr(nullptr),
		m_table(nullptr),
		RefCount(0)
    {
		m_table = ue4::CreateIMPTable((T)nullptr);
    }

	template<typename T, bool bAutoReleased>
    Object<T, bAutoReleased>::Object(const T handle, bool const retain, ITable* table) :
	m_ptr(handle),
	m_table(table),
	RefCount(0)
    {
		if (m_ptr)
		{
			if (!m_table)
			{
				m_table = ue4::CreateIMPTable(handle);
			}
			if (!bAutoReleased)
			{
				if (retain)
				{
					assert(m_table);
					m_table->Retain(m_ptr);
				}
			}
		}
    }

	template<typename T, bool bAutoReleased>
    Object<T, bAutoReleased>::Object(const Object& rhs) :
	m_ptr(rhs.m_ptr),
	m_table(rhs.m_table),
	RefCount(0)
    {
		if (!bAutoReleased && m_ptr)
		{
			assert(m_table);
			m_table->Retain(m_ptr);
		}
    }

#if MTLPP_CONFIG_RVALUE_REFERENCES
	template<typename T, bool bAutoReleased>
    Object<T, bAutoReleased>::Object(Object&& rhs) :
	m_ptr(rhs.m_ptr),
	m_table(rhs.m_table),
	RefCount(0)
    {
        rhs.m_ptr = nullptr;
		rhs.m_table = nullptr;
    }
#endif

	template<typename T, bool bAutoReleased>
    Object<T, bAutoReleased>::~Object()
    {
		if (!bAutoReleased && m_ptr)
		{
			assert(m_table);
			m_table->Release(m_ptr);
		}
    }

	template<typename T, bool bAutoReleased>
    Object<T, bAutoReleased>& Object<T, bAutoReleased>::operator=(const Object& rhs)
    {
        if (rhs.m_ptr == m_ptr && rhs.m_table == m_table)
            return *this;
        if (!bAutoReleased && rhs.m_ptr)
		{
			assert(rhs.m_table);
			rhs.m_table->Retain(rhs.m_ptr);
		}
        if (!bAutoReleased && m_ptr)
		{
			assert(m_table);
			m_table->Release(m_ptr);
		}
        m_ptr = rhs.m_ptr;
		m_table = rhs.m_table;
        return *this;
    }

#if MTLPP_CONFIG_RVALUE_REFERENCES
	template<typename T, bool bAutoReleased>
    Object<T, bAutoReleased>& Object<T, bAutoReleased>::operator=(Object&& rhs)
    {
        if (rhs.m_ptr == m_ptr && rhs.m_table == m_table)
            return *this;
        if (!bAutoReleased && m_ptr)
		{
			assert(m_table);
			m_table->Release(m_ptr);
		}
        m_ptr = rhs.m_ptr;
        rhs.m_ptr = nullptr;
		m_table = rhs.m_table;
		rhs.m_table = nullptr;
        return *this;
    }
#endif
}
