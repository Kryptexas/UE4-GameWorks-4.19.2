/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_Device.hpp"
#include "types.hpp"
#include "pixel_format.hpp"
#include "resource.hpp"
#include "library.hpp"

MTLPP_BEGIN

namespace mtlpp
{
	class ArgumentEncoder;
    class CommandQueue;
    class Device;
    class Buffer;
    class DepthStencilState;
    class Function;
    class Library;
    class Texture;
    class SamplerState;
    class RenderPipelineState;
    class ComputePipelineState;
    class Heap;
    class Fence;

    class SamplerDescriptor;
    class RenderPipelineColorAttachmentDescriptor;
    class DepthStencilDescriptor;
    class TextureDescriptor;
    class CompileOptions;
    class RenderPipelineDescriptor;
    class RenderPassDescriptor;
    class AutoReleasedRenderPipelineReflection;
	class TileRenderPipelineDescriptor;
    class ComputePipelineDescriptor;
    class AutoReleasedComputePipelineReflection;
    class CommandQueueDescriptor;
    class HeapDescriptor;

    enum class FeatureSet
    {
        iOS_GPUFamily1_v1         MTLPP_AVAILABLE_IOS(8_0)   = 0,
        iOS_GPUFamily2_v1         MTLPP_AVAILABLE_IOS(8_0)   = 1,

        iOS_GPUFamily1_v2         MTLPP_AVAILABLE_IOS(8_0)   = 2,
        iOS_GPUFamily2_v2         MTLPP_AVAILABLE_IOS(8_0)   = 3,
        iOS_GPUFamily3_v1         MTLPP_AVAILABLE_IOS(9_0)   = 4,

        iOS_GPUFamily1_v3         MTLPP_AVAILABLE_IOS(10_0)  = 5,
        iOS_GPUFamily2_v3         MTLPP_AVAILABLE_IOS(10_0)  = 6,
        iOS_GPUFamily3_v2         MTLPP_AVAILABLE_IOS(10_0)  = 7,
		
		iOS_GPUFamily1_v4         MTLPP_AVAILABLE_IOS(11_0)  = 8,
		iOS_GPUFamily2_v4         MTLPP_AVAILABLE_IOS(11_0)  = 9,
		iOS_GPUFamily3_v3         MTLPP_AVAILABLE_IOS(11_0)  = 10,
		iOS_GPUFamily4_v1 		  MTLPP_AVAILABLE_IOS(11_0)  = 11,

        macOS_GPUFamily1_v1         MTLPP_AVAILABLE_MAC(8_0)   = 10000,

        macOS_GPUFamily1_v2         MTLPP_AVAILABLE_MAC(10_12) = 10001,
        macOS_ReadWriteTextureTier2 MTLPP_AVAILABLE_MAC(10_12) = 10002,

		macOS_GPUFamily1_v3         MTLPP_AVAILABLE_MAC(10_13) = 10003,
		
        tvOS_GPUFamily1_v1        MTLPP_AVAILABLE_TVOS(9_0)  = 30000,

        tvOS_GPUFamily1_v2        MTLPP_AVAILABLE_TVOS(10_0) = 30001,
		
		tvOS_GPUFamily1_v3        MTLPP_AVAILABLE_TVOS(11_0) = 30002,
		tvOS_GPUFamily2_v1 		MTLPP_AVAILABLE_TVOS(11_0) = 30003,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class PipelineOption
    {
        None           = 0,
        ArgumentInfo   = 1 << 0,
        BufferTypeInfo = 1 << 1,
    }
    MTLPP_AVAILABLE(10_11, 8_0);
	
	enum class ReadWriteTextureTier
	{
		None  = 0,
		Tier1 = 1,
		Tier2 = 2,
	}
	MTLPP_AVAILABLE(10_13, 11_0);
	
	enum class ArgumentBuffersTier
	{
		Tier1 = 0,
		Tier2 = 1,
	}
	MTLPP_AVAILABLE(10_13, 11_0);

    struct SizeAndAlign
    {
        NSUInteger Size;
        NSUInteger Align;
    };
	
	class ArgumentDescriptor : public ns::Object<MTLArgumentDescriptor*>
	{
	public:
		ArgumentDescriptor();
		ArgumentDescriptor(MTLArgumentDescriptor* handle) : ns::Object<MTLArgumentDescriptor*>(handle) {}
		
		DataType GetDataType() const;
		NSUInteger GetIndex() const;
		NSUInteger GetArrayLength() const;
		ArgumentAccess GetAccess() const;
		TextureType GetTextureType() const;
		NSUInteger GetConstantBlockAlignment() const;
	}
	MTLPP_AVAILABLE(10_13, 11_0);
	
	typedef std::function<void(const ns::Ref<Device>&, ns::String const&)> DeviceHandler;
	typedef std::function<void (void* pointer, NSUInteger length)> BufferDeallocHandler;
	typedef std::function<void(const Library&, const ns::AutoReleasedError&)> LibraryHandler;
	typedef std::function<void(const RenderPipelineState&, const ns::AutoReleasedError&)> RenderPipelineStateHandler;
	typedef std::function<void(const RenderPipelineState&, const AutoReleasedRenderPipelineReflection&, const ns::AutoReleasedError&)> RenderPipelineStateReflectionHandler;
	typedef std::function<void(const ComputePipelineState&, const ns::AutoReleasedError&)> ComputePipelineStateHandler;
	typedef std::function<void(const ComputePipelineState&, const AutoReleasedComputePipelineReflection&, const ns::AutoReleasedError&)> ComputePipelineStateReflectionHandler;

	class Device : public ns::Object<ns::Protocol<id<MTLDevice>>::type>
    {
    public:
        Device() { }
        Device(ns::Protocol<id<MTLDevice>>::type handle, bool const bRetain = true) : ns::Object<ns::Protocol<id<MTLDevice>>::type>(handle, bRetain) { }

		static ns::String GetWasAddedNotification() MTLPP_AVAILABLE_MAC(10_13);
		static ns::String GetRemovalRequestedNotification() MTLPP_AVAILABLE_MAC(10_13);
		static ns::String GetWasRemovedNotification() MTLPP_AVAILABLE_MAC(10_13);
		
		static ns::Array<ns::Ref<Device>> CopyAllDevicesWithObserver(ns::Object<id <NSObject>> observer, DeviceHandler handler) MTLPP_AVAILABLE_MAC(10_13);
		static void RemoveDeviceObserver(ns::Object<id <NSObject>> observer) MTLPP_AVAILABLE_MAC(10_13);
		
        static ns::Ref<Device> CreateSystemDefaultDevice() MTLPP_AVAILABLE(10_11, 8_0);
        static ns::Array<ns::Ref<Device>> CopyAllDevices() MTLPP_AVAILABLE(10_11, NA);

        ns::String GetName() const;
        Size       GetMaxThreadsPerThreadgroup() const MTLPP_AVAILABLE(10_11, 9_0);
        bool       IsLowPower() const MTLPP_AVAILABLE_MAC(10_11);
        bool       IsHeadless() const MTLPP_AVAILABLE_MAC(10_11);
		bool       IsRemovable() const MTLPP_AVAILABLE_MAC(10_13);
        uint64_t   GetRecommendedMaxWorkingSetSize() const MTLPP_AVAILABLE_MAC(10_12);
        bool       IsDepth24Stencil8PixelFormatSupported() const MTLPP_AVAILABLE_MAC(10_11);
		
		uint64_t GetRegistryID() const MTLPP_AVAILABLE(10_13, 11_0);
		
		ReadWriteTextureTier GetReadWriteTextureSupport() const MTLPP_AVAILABLE(10_13, 11_0);
		ArgumentBuffersTier GetArgumentsBufferSupport() const MTLPP_AVAILABLE(10_13, 11_0);
		
		bool AreRasterOrderGroupsSupported() const MTLPP_AVAILABLE(10_13, 11_0);
		
		uint64_t GetCurrentAllocatedSize() const MTLPP_AVAILABLE(10_13, 11_0);

        CommandQueue NewCommandQueue();
        CommandQueue NewCommandQueue(NSUInteger maxCommandBufferCount);
        SizeAndAlign HeapTextureSizeAndAlign(const TextureDescriptor& desc) MTLPP_AVAILABLE(10_13, 10_0);
        SizeAndAlign HeapBufferSizeAndAlign(NSUInteger length, ResourceOptions options) MTLPP_AVAILABLE(10_13, 10_0);
        Heap NewHeap(const HeapDescriptor& descriptor) MTLPP_AVAILABLE(10_13, 10_0);
        Buffer NewBuffer(NSUInteger length, ResourceOptions options);
        Buffer NewBuffer(const void* pointer, NSUInteger length, ResourceOptions options);
        Buffer NewBuffer(void* pointer, NSUInteger length, ResourceOptions options, BufferDeallocHandler deallocator);
        DepthStencilState NewDepthStencilState(const DepthStencilDescriptor& descriptor);
        Texture NewTexture(const TextureDescriptor& descriptor);
		Texture NewTextureWithDescriptor(const TextureDescriptor& descriptor, ns::IOSurface& iosurface, NSUInteger plane) MTLPP_AVAILABLE(10_11, NA);
        SamplerState NewSamplerState(const SamplerDescriptor& descriptor);
        Library NewDefaultLibrary();
		Library NewDefaultLibraryWithBundle(const ns::Bundle& bundle, ns::AutoReleasedError* error) MTLPP_AVAILABLE(10_12, 10_0);
        Library NewLibrary(const ns::String& filepath, ns::AutoReleasedError* error);
        Library NewLibrary(const char* source, const CompileOptions& options, ns::AutoReleasedError* error);
		Library NewLibrary(ns::URL const& url, ns::AutoReleasedError* error) MTLPP_AVAILABLE(10_13, 11_0);
        void NewLibrary(const char* source, const CompileOptions& options, LibraryHandler completionHandler);
        RenderPipelineState NewRenderPipelineState(const RenderPipelineDescriptor& descriptor, ns::AutoReleasedError* error);
        RenderPipelineState NewRenderPipelineState(const RenderPipelineDescriptor& descriptor, PipelineOption options, AutoReleasedRenderPipelineReflection* outReflection, ns::AutoReleasedError* error);
        void NewRenderPipelineState(const RenderPipelineDescriptor& descriptor, RenderPipelineStateHandler completionHandler);
        void NewRenderPipelineState(const RenderPipelineDescriptor& descriptor, PipelineOption options, RenderPipelineStateReflectionHandler completionHandler);
        ComputePipelineState NewComputePipelineState(const Function& computeFunction, ns::AutoReleasedError* error);
        ComputePipelineState NewComputePipelineState(const Function& computeFunction, PipelineOption options, AutoReleasedComputePipelineReflection& outReflection, ns::AutoReleasedError* error);
        void NewComputePipelineState(const Function& computeFunction, ComputePipelineStateHandler completionHandler);
        void NewComputePipelineState(const Function& computeFunction, PipelineOption options, ComputePipelineStateReflectionHandler completionHandler);
        ComputePipelineState NewComputePipelineState(const ComputePipelineDescriptor& descriptor, PipelineOption options, AutoReleasedComputePipelineReflection* outReflection, ns::AutoReleasedError* error);
        void NewComputePipelineState(const ComputePipelineDescriptor& descriptor, PipelineOption options, ComputePipelineStateReflectionHandler completionHandler) MTLPP_AVAILABLE(10_11, 9_0);
        Fence NewFence() MTLPP_AVAILABLE(NA, 10_0);
        bool SupportsFeatureSet(FeatureSet featureSet) const;
        bool SupportsTextureSampleCount(NSUInteger sampleCount) const MTLPP_AVAILABLE(10_11, 9_0);
		NSUInteger GetMinimumLinearTextureAlignmentForPixelFormat(PixelFormat format) const MTLPP_AVAILABLE(10_13, 11_0);
		NSUInteger GetMaxThreadgroupMemoryLength() const MTLPP_AVAILABLE(10_13, 11_0);
		bool AreProgrammableSamplePositionsSupported() const MTLPP_AVAILABLE(10_13, 11_0);
		void GetDefaultSamplePositions(SamplePosition* positions, NSUInteger count) MTLPP_AVAILABLE(10_13, 11_0);
		ArgumentEncoder NewArgumentEncoderWithArguments(ns::Array<ArgumentDescriptor> const& arguments) MTLPP_AVAILABLE(10_13, 11_0);
		RenderPipelineState NewRenderPipelineState(const TileRenderPipelineDescriptor& descriptor, PipelineOption options, AutoReleasedRenderPipelineReflection* outReflection, ns::AutoReleasedError* error) MTLPP_AVAILABLE_IOS(11_0);
		void NewRenderPipelineState(const TileRenderPipelineDescriptor& descriptor, PipelineOption options, RenderPipelineStateReflectionHandler completionHandler) MTLPP_AVAILABLE_IOS(11_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);
}

MTLPP_END
