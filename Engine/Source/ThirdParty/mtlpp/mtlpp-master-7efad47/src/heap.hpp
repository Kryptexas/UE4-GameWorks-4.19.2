/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_Heap.hpp"
#include "ns.hpp"
#include "device.hpp"
#include "resource.hpp"
#include "buffer.hpp"
#include "texture.hpp"
#include "types.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    class HeapDescriptor : public ns::Object<MTLHeapDescriptor*>
    {
    public:
        HeapDescriptor(MTLHeapDescriptor* handle) : ns::Object<MTLHeapDescriptor*>(handle) { }

        NSUInteger     GetSize() const;
        StorageMode  GetStorageMode() const;
        CpuCacheMode GetCpuCacheMode() const;

        void SetSize(NSUInteger size) const;
        void SetStorageMode(StorageMode storageMode) const;
        void SetCpuCacheMode(CpuCacheMode cpuCacheMode) const;
    }
    MTLPP_AVAILABLE(10_13, 10_0);

    class Heap : public ns::Object<ns::Protocol<id<MTLHeap>>::type>
    {
    public:
        Heap(ns::Protocol<id<MTLHeap>>::type handle) : ns::Object<ns::Protocol<id<MTLHeap>>::type>(handle) { }

        ns::String   GetLabel() const;
        Device       GetDevice() const;
        StorageMode  GetStorageMode() const;
        CpuCacheMode GetCpuCacheMode() const;
        NSUInteger     GetSize() const;
        NSUInteger     GetUsedSize() const;
		NSUInteger	 GetCurrentAllocatedSize() const MTLPP_AVAILABLE(10_13, 11_0);

        void SetLabel(const ns::String& label);

        NSUInteger MaxAvailableSizeWithAlignment(NSUInteger alignment);
        Buffer NewBuffer(NSUInteger length, ResourceOptions options);
        Texture NewTexture(const TextureDescriptor& desc);
        PurgeableState SetPurgeableState(PurgeableState state);
    }
    MTLPP_AVAILABLE(10_13, 10_0);
}

MTLPP_END
