/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_Resource.hpp"
#include "imp_Buffer.hpp"
#include "imp_Texture.hpp"
#include "ns.hpp"

MTLPP_BEGIN

namespace ue4
{
	template<>
	inline IMPTable<id<MTLResource>, void>* CreateIMPTable(id<MTLResource> handle)
	{
		if (handle)
		{
			static Protocol* textureProto = objc_getProtocol("MTLTexture");
			static Protocol* bufferProto = objc_getProtocol("MTLBuffer");
			static Class debugTexture = objc_lookUpClass("MTLDebugTexture");
			static Class debugBuffer = objc_lookUpClass("MTLDebugBuffer");
			Class cls = object_getClass(handle);
			if (cls == debugTexture || class_conformsToProtocol(cls, textureProto))
			{
				IMPTable<id<MTLTexture>, void>* table = imp_cache<id<MTLTexture>, void>::Register((id<MTLTexture>)handle);
				return (IMPTable<id<MTLResource>, void>*)table;
			}
			else if (cls == debugBuffer || class_conformsToProtocol(cls, bufferProto))
			{
				IMPTable<id<MTLBuffer>, void>* table = imp_cache<id<MTLBuffer>, void>::Register((id<MTLBuffer>)handle);
				return (IMPTable<id<MTLResource>, void>*)table;
			}
		}
		return nullptr;
	}
}

namespace mtlpp
{
    class Heap;

    static const NSUInteger ResourceCpuCacheModeShift        = 0;
    static const NSUInteger ResourceStorageModeShift         = 4;
    static const NSUInteger ResourceHazardTrackingModeShift  = 8;

    enum class PurgeableState
    {
        KeepCurrent = 1,
        NonVolatile = 2,
        Volatile    = 3,
        Empty       = 4,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class CpuCacheMode
    {
        DefaultCache  = 0,
        WriteCombined = 1,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class StorageMode
    {
        Shared                                = 0,
        Managed    MTLPP_AVAILABLE(10_11, NA) = 1,
        Private                               = 2,
        Memoryless MTLPP_AVAILABLE(NA, 10_0)  = 3,
    }
    MTLPP_AVAILABLE(10_11, 9_0);

    enum ResourceOptions
    {
        CpuCacheModeDefaultCache                                = NSUInteger(CpuCacheMode::DefaultCache)  << ResourceCpuCacheModeShift,
        CpuCacheModeWriteCombined                               = NSUInteger(CpuCacheMode::WriteCombined) << ResourceCpuCacheModeShift,

        StorageModeShared           MTLPP_AVAILABLE(10_11, 9_0) = NSUInteger(StorageMode::Shared)     << ResourceStorageModeShift,
        StorageModeManaged          MTLPP_AVAILABLE(10_11, NA)  = NSUInteger(StorageMode::Managed)    << ResourceStorageModeShift,
        StorageModePrivate          MTLPP_AVAILABLE(10_11, 9_0) = NSUInteger(StorageMode::Private)    << ResourceStorageModeShift,
        StorageModeMemoryless       MTLPP_AVAILABLE(NA, 10_0)   = NSUInteger(StorageMode::Memoryless) << ResourceStorageModeShift,

        HazardTrackingModeUntracked MTLPP_AVAILABLE(NA, 10_0)   = 0x1 << ResourceHazardTrackingModeShift,

        OptionCpuCacheModeDefault                               = CpuCacheModeDefaultCache,
        OptionCpuCacheModeWriteCombined                         = CpuCacheModeWriteCombined,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

	class Resource : public ns::Object<ns::Protocol<id<MTLResource>>::type>
    {
    public:
        Resource() { }
        Resource(ns::Protocol<id<MTLResource>>::type handle) : ns::Object<ns::Protocol<id<MTLResource>>::type>(handle) { }

        ns::String   GetLabel() const;
        CpuCacheMode GetCpuCacheMode() const;
        StorageMode  GetStorageMode() const MTLPP_AVAILABLE(10_11, 9_0);
        Heap         GetHeap() const MTLPP_AVAILABLE(10_13, 10_0);
        bool         IsAliasable() const MTLPP_AVAILABLE(10_13, 10_0);
		NSUInteger	 GetAllocatedSize() const MTLPP_AVAILABLE(10_13, 11_0);

        void SetLabel(const ns::String& label);

        PurgeableState SetPurgeableState(PurgeableState state);
        void MakeAliasable() const MTLPP_AVAILABLE(10_13, 10_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);
}

MTLPP_END
