/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLHeap.h>
#include "heap.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    NSUInteger HeapDescriptor::GetSize() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
        return NSUInteger([(MTLHeapDescriptor*)m_ptr size]);
#else
        return 0;
#endif

    }

    StorageMode HeapDescriptor::GetStorageMode() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
        return StorageMode([(MTLHeapDescriptor*)m_ptr storageMode]);
#else
        return StorageMode(0);
#endif

    }

    CpuCacheMode HeapDescriptor::GetCpuCacheMode() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
        return CpuCacheMode([(MTLHeapDescriptor*)m_ptr cpuCacheMode]);
#else
        return CpuCacheMode(0);
#endif

    }

    void HeapDescriptor::SetSize(NSUInteger size) const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
        [(MTLHeapDescriptor*)m_ptr setSize:size];
#endif

    }

    void HeapDescriptor::SetStorageMode(StorageMode storageMode) const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
        [(MTLHeapDescriptor*)m_ptr setStorageMode:MTLStorageMode(storageMode)];
#endif

    }

    void HeapDescriptor::SetCpuCacheMode(CpuCacheMode cpuCacheMode) const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
        [(MTLHeapDescriptor*)m_ptr setCpuCacheMode:MTLCPUCacheMode(cpuCacheMode)];
#endif

    }

    ns::String Heap::GetLabel() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
		return m_table->Label(m_ptr);
#else
        return nullptr;
#endif

    }

    Device Heap::GetDevice() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
		return m_table->Device(m_ptr);
#else
        return nullptr;
#endif

    }

    StorageMode Heap::GetStorageMode() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
        return StorageMode(m_table->StorageMode(m_ptr));
#else
        return StorageMode(0);
#endif

    }

    CpuCacheMode Heap::GetCpuCacheMode() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
        return CpuCacheMode(m_table->CpuCacheMode(m_ptr));
#else
        return CpuCacheMode(0);
#endif

    }

    NSUInteger Heap::GetSize() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
        return m_table->Size(m_ptr);
#else
        return 0;
#endif

    }

    NSUInteger Heap::GetUsedSize() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
        return m_table->UsedSize(m_ptr);
#else
        return 0;
#endif

    }
	
	NSUInteger Heap::GetCurrentAllocatedSize() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return m_table->CurrentAllocatedSize(m_ptr);
#else
		return GetSize();
#endif
	}

    void Heap::SetLabel(const ns::String& label)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
		m_table->SetLabel(m_ptr, label.GetPtr());
#endif

    }

    NSUInteger Heap::MaxAvailableSizeWithAlignment(NSUInteger alignment)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
		return m_table->MaxAvailableSizeWithAlignment(m_ptr, alignment);
#else
        return 0;
#endif

    }

    Buffer Heap::NewBuffer(NSUInteger length, ResourceOptions options)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
		return m_table->NewBufferWithLength(m_ptr, length, MTLResourceOptions(options));
#else
        return nullptr;
#endif

    }

    Texture Heap::NewTexture(const TextureDescriptor& desc)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
		return m_table->NewTextureWithDescriptor(m_ptr, desc.GetPtr());
#else
        return nullptr;
#endif

    }

    PurgeableState Heap::SetPurgeableState(PurgeableState state)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
		return PurgeableState(m_table->SetPurgeableState(m_ptr, MTLPurgeableState(state)));
#else
        return PurgeableState(0);
#endif

    }
}

MTLPP_END
