/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLResource.h>
#include "resource.hpp"
#include "heap.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    ns::String Resource::GetLabel() const
    {
        Validate();
		return ((IMPTableResource<Resource::Type>*)m_table)->Label(m_ptr);
    }

    CpuCacheMode Resource::GetCpuCacheMode() const
    {
        Validate();
		return CpuCacheMode(((IMPTableResource<Resource::Type>*)m_table)->CpuCacheMode(m_ptr));
    }

    StorageMode Resource::GetStorageMode() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
        return StorageMode(((IMPTableResource<Resource::Type>*)m_table)->StorageMode(m_ptr));
#else
        return StorageMode(0);
#endif
    }

    Heap Resource::GetHeap() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
			return ((IMPTableResource<Resource::Type>*)m_table)->Heap(m_ptr);
#else
			return nullptr;
#endif
    }

    bool Resource::IsAliasable() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
			return ((IMPTableResource<Resource::Type>*)m_table)->IsAliasable(m_ptr);
#else
			return false;
#endif
    }
	
	NSUInteger Resource::GetAllocatedSize() const
	{
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
		return ((IMPTableResource<Resource::Type>*)m_table)->AllocatedSize(m_ptr);
#else
		return 0;
#endif
	}

    void Resource::SetLabel(const ns::String& label)
    {
        Validate();
		((IMPTableResource<Resource::Type>*)m_table)->SetLabel(m_ptr, label.GetPtr());
    }

    PurgeableState Resource::SetPurgeableState(PurgeableState state)
    {
        Validate();
		return (PurgeableState)((IMPTableResource<Resource::Type>*)m_table)->SetPurgeableState(m_ptr, MTLPurgeableState(state));
    }

    void Resource::MakeAliasable() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
			((IMPTableResource<Resource::Type>*)m_table)->MakeAliasable(m_ptr);
#endif
    }
}

MTLPP_END
