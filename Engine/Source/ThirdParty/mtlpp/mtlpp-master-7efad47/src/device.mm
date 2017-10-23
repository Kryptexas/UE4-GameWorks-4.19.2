/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include "device.hpp"
#include "buffer.hpp"
#include "command_queue.hpp"
#include "compute_pipeline.hpp"
#include "depth_stencil.hpp"
#include "render_pipeline.hpp"
#include "sampler.hpp"
#include "texture.hpp"
#include "heap.hpp"
#include "argument_encoder.hpp"
#include <Metal/MTLDevice.h>

namespace mtlpp
{
	ArgumentDescriptor::ArgumentDescriptor()
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
	: ns::Object<MTLArgumentDescriptor*>([MTLArgumentDescriptor new], false)
#endif
	{
	}
	
	DataType ArgumentDescriptor::GetDataType() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return (DataType)[(MTLArgumentDescriptor*)m_ptr dataType];
#else
		return 0;
#endif
	}
	
	uint32_t ArgumentDescriptor::GetIndex() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return [(MTLArgumentDescriptor*)m_ptr index];
#else
		return 0;
#endif
	}
	
	uint32_t ArgumentDescriptor::GetArrayLength() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return [(MTLArgumentDescriptor*)m_ptr arrayLength];
#else
		return 0;
#endif
	}
	
	ArgumentAccess ArgumentDescriptor::GetAccess() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return (ArgumentAccess)[(MTLArgumentDescriptor*)m_ptr access];
#else
		return 0;
#endif
	}
	
	TextureType ArgumentDescriptor::GetTextureType() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return (TextureType)[(MTLArgumentDescriptor*)m_ptr textureType];
#else
		return 0;
#endif
	}
	
	uint32_t ArgumentDescriptor::GetConstantBlockAlignment() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return [(MTLArgumentDescriptor*)m_ptr constantBlockAlignment];
#else
		return 0;
#endif
	}
	
    CompileOptions::CompileOptions() :
        ns::Object<MTLCompileOptions*>([[MTLCompileOptions alloc] init], false)
    {
    }

	ns::String Device::GetWasAddedNotification()
	{
#if MTLPP_IS_AVAILABLE_MAC(10_13)
		return MTLDeviceWasAddedNotification;
#else
		return ns::String();
#endif
	}
	
	ns::String Device::GetRemovalRequestedNotification()
	{
#if MTLPP_IS_AVAILABLE_MAC(10_13)
		return MTLDeviceRemovalRequestedNotification;
#else
		return ns::String();
#endif
	}
	
	ns::String Device::GetWasRemovedNotification()
	{
#if MTLPP_IS_AVAILABLE_MAC(10_13)
		return MTLDeviceWasRemovedNotification;
#else
		return ns::String();
#endif
	}
	
	ns::Array<ns::Ref<Device>> Device::CopyAllDevicesWithObserver(ns::Object<NSObject*> observer, std::function<void(const ns::Ref<Device>&, ns::String const&)> handler)
	{
#if MTLPP_IS_AVAILABLE_MAC(10_13)
		id<NSObject> obj = (id<NSObject>)observer.GetPtr();
		return MTLCopyAllDevicesWithObserver((id<NSObject>*)(obj ? &obj : nil), ^(id<MTLDevice>  _Nonnull device, MTLDeviceNotificationName  _Nonnull notifyName)
			{
			handler(ns::Ref<Device>(new Device(device)), ns::String(notifyName));
		});
#else
		return ns::Array<ns::Ref<Device>>();
#endif
	}
	
	void Device::RemoveDeviceObserver(ns::Object<NSObject*> observer)
	{
#if MTLPP_IS_AVAILABLE_MAC(10_13)
		if (observer)
		{
			MTLRemoveDeviceObserver((id<NSObject>)observer.GetPtr());
		}
#endif
	}
	
    ns::Ref<Device> Device::CreateSystemDefaultDevice()
    {
        return new Device(MTLCreateSystemDefaultDevice());
    }

    ns::Array<ns::Ref<Device>> Device::CopyAllDevices()
    {
#if MTLPP_PLATFORM_MAC
        return MTLCopyAllDevices();
#else
        return nullptr;
#endif
    }

    ns::String Device::GetName() const
    {
        Validate();
        return [(id<MTLDevice>)m_ptr name];
    }

    Size Device::GetMaxThreadsPerThreadgroup() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
        MTLSize mtlSize = [(id<MTLDevice>)m_ptr maxThreadsPerThreadgroup];
        return Size(uint32_t(mtlSize.width), uint32_t(mtlSize.height), uint32_t(mtlSize.depth));
#else
        return Size(0, 0, 0);
#endif
    }

    bool Device::IsLowPower() const
    {
        Validate();
#if MTLPP_PLATFORM_MAC
        return [(id<MTLDevice>)m_ptr isLowPower];
#else
        return false;
#endif
    }

    bool Device::IsHeadless() const
    {
        Validate();
#if MTLPP_PLATFORM_MAC
        return [(id<MTLDevice>)m_ptr isHeadless];
#else
        return false;
#endif
    }

    uint64_t Device::GetRecommendedMaxWorkingSetSize() const
    {
#if MTLPP_PLATFORM_MAC
		if(@available(macOS 10.12, *))
			return [(id<MTLDevice>)m_ptr recommendedMaxWorkingSetSize];
		else
#endif
        return 0;
    }

    bool Device::IsDepth24Stencil8PixelFormatSupported() const
    {
        Validate();
#if MTLPP_PLATFORM_MAC
        return [(id<MTLDevice>)m_ptr isDepth24Stencil8PixelFormatSupported];
#else
        return true;
#endif
    }
	
	uint64_t Device::GetRegistryID() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return [(id<MTLDevice>)m_ptr registryID];
#else
		return 0;
#endif
	}
	
	ReadWriteTextureTier Device::GetReadWriteTextureSupport() const
	{
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return (ReadWriteTextureTier)[(id<MTLDevice>)m_ptr readWriteTextureSupport];
#else
		return 0;
#endif
	}
	
	ArgumentBuffersTier Device::GetArgumentsBufferSupport() const
	{
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return (ArgumentBuffersTier)[(id<MTLDevice>)m_ptr argumentBuffersSupport];
#else
		return 0;
#endif
	}
	
	bool Device::AreRasterOrderGroupsSupported() const
	{
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return [(id<MTLDevice>)m_ptr areRasterOrderGroupsSupported];
#else
		return false;
#endif
	}
	
	uint64_t Device::GetCurrentAllocatedSize() const
	{
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return [(id<MTLDevice>)m_ptr currentAllocatedSize];
#else
		return 0;
#endif
	}

    CommandQueue Device::NewCommandQueue()
    {
        Validate();
        return [(id<MTLDevice>)m_ptr newCommandQueue];
    }

    CommandQueue Device::NewCommandQueue(uint32_t maxCommandBufferCount)
    {
        Validate();
        return [(id<MTLDevice>)m_ptr newCommandQueueWithMaxCommandBufferCount:maxCommandBufferCount];
    }

    SizeAndAlign Device::HeapTextureSizeAndAlign(const TextureDescriptor& desc)
    {
		if (@available(macOS 10.13, iOS 10.0, *))
		{
			MTLSizeAndAlign mtlSizeAndAlign = [(id<MTLDevice>)m_ptr heapTextureSizeAndAlignWithDescriptor:(MTLTextureDescriptor*)desc.GetPtr()];
			return SizeAndAlign{ uint32_t(mtlSizeAndAlign.size), uint32_t(mtlSizeAndAlign.align) };
		}
        return SizeAndAlign{0, 0};
    }

    SizeAndAlign Device::HeapBufferSizeAndAlign(uint32_t length, ResourceOptions options)
    {
		if (@available(macOS 10.13, iOS 10.0, *))
		{
			MTLSizeAndAlign mtlSizeAndAlign = [(id<MTLDevice>)m_ptr heapBufferSizeAndAlignWithLength:length options:MTLResourceOptions(options)];
			return SizeAndAlign{ uint32_t(mtlSizeAndAlign.size), uint32_t(mtlSizeAndAlign.align) };
		}
        return SizeAndAlign{0, 0};
    }

    Heap Device::NewHeap(const HeapDescriptor& descriptor)
    {
		if (@available(macOS 10.13, iOS 10.0, *))
		{
			return [(id<MTLDevice>)m_ptr newHeapWithDescriptor:(MTLHeapDescriptor*)descriptor.GetPtr()];
		}
		return nullptr;
    }

    Buffer Device::NewBuffer(uint32_t length, ResourceOptions options)
    {
        Validate();
        return [(id<MTLDevice>)m_ptr newBufferWithLength:length options:MTLResourceOptions(options)];
    }

    Buffer Device::NewBuffer(const void* pointer, uint32_t length, ResourceOptions options)
    {
        Validate();
        return [(id<MTLDevice>)m_ptr newBufferWithBytes:pointer length:length options:MTLResourceOptions(options)];
    }


    Buffer Device::NewBuffer(void* pointer, uint32_t length, ResourceOptions options, std::function<void (void* pointer, uint32_t length)> deallocator)
    {
        Validate();
        return [(id<MTLDevice>)m_ptr newBufferWithBytesNoCopy:pointer
                                                                             length:length
                                                                            options:MTLResourceOptions(options)
                                                                        deallocator:^(void* pointer, NSUInteger length) { deallocator(pointer, uint32_t(length)); }];
    }

    DepthStencilState Device::NewDepthStencilState(const DepthStencilDescriptor& descriptor)
    {
        Validate();
        return [(id<MTLDevice>)m_ptr newDepthStencilStateWithDescriptor:(MTLDepthStencilDescriptor*)descriptor.GetPtr()];
    }

    Texture Device::NewTexture(const TextureDescriptor& descriptor)
    {
        Validate();
        return [(id<MTLDevice>)m_ptr newTextureWithDescriptor:(MTLTextureDescriptor*)descriptor.GetPtr()];
    }
	
	Texture Device::NewTextureWithDescriptor(const TextureDescriptor& descriptor, ns::IOSurface& iosurface, uint32_t plane)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_11)
		return [(id<MTLDevice>)m_ptr newTextureWithDescriptor:(MTLTextureDescriptor*)descriptor.GetPtr() iosurface:(IOSurfaceRef)iosurface.GetPtr() plane:plane];
#else
		return Texture();
#endif
	}

    SamplerState Device::NewSamplerState(const SamplerDescriptor& descriptor)
    {
        Validate();
        return [(id<MTLDevice>)m_ptr newSamplerStateWithDescriptor:(MTLSamplerDescriptor*)descriptor.GetPtr()];
    }

    Library Device::NewDefaultLibrary()
    {
        Validate();
        return [(id<MTLDevice>)m_ptr newDefaultLibrary];
    }

    Library Device::NewLibrary(const ns::String& filepath, ns::Error* error)
    {
        Validate();
        NSError** nsError = error ? (NSError**)error->GetInnerPtr() : nullptr;
        return [(id<MTLDevice>)m_ptr newLibraryWithFile:(NSString*)filepath.GetPtr() error:nsError];
    }
	
	Library Device::NewDefaultLibraryWithBundle(const ns::Bundle& bundle, ns::Error* error)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
		NSError** nsError = error ? (NSError**)error->GetInnerPtr() : nullptr;
		return [(id<MTLDevice>)m_ptr newDefaultLibraryWithBundle:(NSBundle*)bundle.GetPtr() error:nsError];
#else
		return Library();
#endif
	}

    Library Device::NewLibrary(const char* source, const CompileOptions& options, ns::Error* error)
    {
        Validate();
        NSString* nsSource = [NSString stringWithUTF8String:source];
        NSError** nsError = error ? (NSError**)error->GetInnerPtr() : nullptr;
        return [(id<MTLDevice>)m_ptr newLibraryWithSource:nsSource
                                                                        options:(MTLCompileOptions*)options.GetPtr()
                                                                          error:nsError];
    }
	
	Library Device::NewLibrary(ns::URL const& url, ns::Error* error)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		NSError** nsError = error ? (NSError**)error->GetInnerPtr() : nullptr;
		return [(id<MTLDevice>)m_ptr newLibraryWithURL:(NSURL*)url.GetPtr() error:nsError];
#else
		return Library();
#endif
	}

    void Device::NewLibrary(const char* source, const CompileOptions& options, std::function<void(const Library&, const ns::Error&)> completionHandler)
    {
        Validate();
        NSString* nsSource = [NSString stringWithUTF8String:source];
        [(id<MTLDevice>)m_ptr newLibraryWithSource:nsSource
                                                    options:(MTLCompileOptions*)options.GetPtr()
                                          completionHandler:^(id <MTLLibrary> library, NSError * error) {
                                                completionHandler(
                                                    library,
                                                    error);
                                          }];
    }

    RenderPipelineState Device::NewRenderPipelineState(const RenderPipelineDescriptor& descriptor, ns::Error* error)
    {
        Validate();
        NSError** nsError = error ? (NSError**)error->GetInnerPtr() : nullptr;
        return [(id<MTLDevice>)m_ptr newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor*)descriptor.GetPtr()
                                                                                          error:nsError];
    }

    RenderPipelineState Device::NewRenderPipelineState(const RenderPipelineDescriptor& descriptor, PipelineOption options, RenderPipelineReflection* outReflection, ns::Error* error)
    {
        Validate();
        NSError** nsError = error ? (NSError**)error->GetInnerPtr() : nullptr;
        MTLRenderPipelineReflection* mtlReflection = outReflection ? (MTLRenderPipelineReflection*)outReflection->GetPtr() : nullptr;
        return [(id<MTLDevice>)m_ptr newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor*)descriptor.GetPtr()
                                                                                        options:MTLPipelineOption(options)
                                                                                     reflection:&mtlReflection
                                                                                          error:nsError];
    }

    void Device::NewRenderPipelineState(const RenderPipelineDescriptor& descriptor, std::function<void(const RenderPipelineState&, const ns::Error&)> completionHandler)
    {
        Validate();
        [(id<MTLDevice>)m_ptr newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor*)descriptor.GetPtr()
                                                          completionHandler:^(id <MTLRenderPipelineState> renderPipelineState, NSError * error) {
                                                              completionHandler(
                                                                  renderPipelineState,
                                                                  error
                                                              );
                                                          }];
    }

    void Device::NewRenderPipelineState(const RenderPipelineDescriptor& descriptor, PipelineOption options, std::function<void(const RenderPipelineState&, const RenderPipelineReflection&, const ns::Error&)> completionHandler)
    {
        Validate();
        [(id<MTLDevice>)m_ptr newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor*)descriptor.GetPtr()
                                                                    options:MTLPipelineOption(options)
                                                          completionHandler:^(id <MTLRenderPipelineState> renderPipelineState, MTLRenderPipelineReflection * reflection, NSError * error) {
                                                              completionHandler(
                                                                  renderPipelineState,
                                                                  reflection,
                                                                  error
                                                              );
                                                          }];
    }

    ComputePipelineState Device::NewComputePipelineState(const Function& computeFunction, ns::Error* error)
    {
        Validate();
        NSError** nsError = error ? (NSError**)error->GetInnerPtr() : nullptr;
        return [(id<MTLDevice>)m_ptr newComputePipelineStateWithFunction:(id<MTLFunction>)computeFunction.GetPtr()
                                                                                         error:nsError];
    }

    ComputePipelineState Device::NewComputePipelineState(const Function& computeFunction, PipelineOption options, ComputePipelineReflection& outReflection, ns::Error* error)
    {
        Validate();
		NSError** nsError = error ? (NSError**)error->GetInnerPtr() : nullptr;
		MTLComputePipelineReflection* mtlReflection = outReflection ? (MTLComputePipelineReflection*)outReflection.GetPtr() : nullptr;
		return [(id<MTLDevice>)m_ptr newComputePipelineStateWithFunction:(id<MTLFunction>)computeFunction.GetPtr()
																  options:MTLPipelineOption(options)
															   reflection:&mtlReflection
																	error:nsError];
    }

    void Device::NewComputePipelineState(const Function& computeFunction, std::function<void(const ComputePipelineState&, const ns::Error&)> completionHandler)
    {
        Validate();
        [(id<MTLDevice>)m_ptr newComputePipelineStateWithFunction:(id<MTLFunction>)computeFunction.GetPtr()
                                                         completionHandler:^(id <MTLComputePipelineState> computePipelineState, NSError * error) {
                                                             completionHandler(
                                                                 computePipelineState,
                                                                 error
                                                             );
                                                         }];
    }

    void Device::NewComputePipelineState(const Function& computeFunction, PipelineOption options, std::function<void(const ComputePipelineState&, const ComputePipelineReflection&, const ns::Error&)> completionHandler)
    {
        Validate();
        [(id<MTLDevice>)m_ptr newComputePipelineStateWithFunction:(id<MTLFunction>)computeFunction.GetPtr()
                                                                   options:MTLPipelineOption(options)
                                                         completionHandler:^(id <MTLComputePipelineState> computePipelineState, MTLComputePipelineReflection * reflection, NSError * error) {
                                                             completionHandler(
                                                                 computePipelineState,
                                                                 reflection,
                                                                 error
                                                             );
                                                         }];
    }

    ComputePipelineState Device::NewComputePipelineState(const ComputePipelineDescriptor& descriptor, PipelineOption options, ComputePipelineReflection* outReflection, ns::Error* error)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
        NSError** nsError = error ? (NSError**)error->GetInnerPtr() : nullptr;
        MTLComputePipelineReflection* mtlReflection = outReflection ? (MTLComputePipelineReflection*)outReflection->GetPtr() : nullptr;
        return [(id<MTLDevice>)m_ptr newComputePipelineStateWithDescriptor:(MTLComputePipelineDescriptor*)descriptor.GetPtr()
                                                                                         options:MTLPipelineOption(options)
                                                                                      reflection:&mtlReflection
                                                                                           error:nsError];
#else
        return nullptr;
#endif
    }

    void Device::NewComputePipelineState(const ComputePipelineDescriptor& descriptor, PipelineOption options, std::function<void(const ComputePipelineState&, const ComputePipelineReflection&, const ns::Error&)> completionHandler)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
        [(id<MTLDevice>)m_ptr newComputePipelineStateWithDescriptor:(MTLComputePipelineDescriptor*)descriptor.GetPtr()
                                                                     options:MTLPipelineOption(options)
                                                         completionHandler:^(id <MTLComputePipelineState> computePipelineState, MTLComputePipelineReflection * reflection, NSError * error)
                                                                    {
                                                                        completionHandler(
                                                                            computePipelineState,
                                                                            reflection,
                                                                            error);
                                                                    }];
#endif
    }

    Fence Device::NewFence()
    {
        Validate();
		if (@available(macOS 10.13, iOS 10.0, *))
			return [(id<MTLDevice>)m_ptr newFence];
		else
			return nullptr;
    }

    bool Device::SupportsFeatureSet(FeatureSet featureSet) const
    {
        Validate();
        return [(id<MTLDevice>)m_ptr supportsFeatureSet:MTLFeatureSet(featureSet)];
    }

    bool Device::SupportsTextureSampleCount(uint32_t sampleCount) const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
        return [(id<MTLDevice>)m_ptr supportsTextureSampleCount:sampleCount];
#else
        return true;
#endif
    }
	
	uint32_t Device::GetMinimumLinearTextureAlignmentForPixelFormat(PixelFormat format) const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return [(id<MTLDevice>)m_ptr minimumLinearTextureAlignmentForPixelFormat:(MTLPixelFormat)format];
#else
		return 0;
#endif
	}
	
	uint32_t Device::GetMaxThreadgroupMemoryLength() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return [(id<MTLDevice>)m_ptr maxThreadgroupMemoryLength];
#else
		return 0;
#endif
	}
	
	bool Device::AreProgrammableSamplePositionsSupported() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return [(id<MTLDevice>)m_ptr areProgrammableSamplePositionsSupported];
#else
		return false;
#endif
	}
	
	void Device::GetDefaultSamplePositions(SamplePosition* positions, uint32_t count)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		[(id<MTLDevice>)m_ptr getDefaultSamplePositions:(MTLSamplePosition *)positions count:count];
#endif
	}
	
	ArgumentEncoder Device::NewArgumentEncoderWithArguments(ns::Array<ArgumentDescriptor> const& arguments)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return [(id<MTLDevice>)m_ptr newArgumentEncoderWithArguments:(NSArray<MTLArgumentDescriptor *> *)arguments.GetPtr()];
#else
		return ArgumentEncoder();
#endif
	}
	
	RenderPipelineState Device::NewRenderPipelineState(const TileRenderPipelineDescriptor& descriptor, PipelineOption options, RenderPipelineReflection* outReflection, ns::Error* error)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		NSError** nsError = error ? (NSError**)error->GetInnerPtr() : nullptr;
		MTLRenderPipelineReflection* mtlReflection = outReflection ? (MTLRenderPipelineReflection*)outReflection->GetPtr() : nullptr;
		return [m_ptr newRenderPipelineStateWithTileDescriptor:descriptor.GetPtr() options:(MTLPipelineOption)options reflection:&mtlReflection error:nsError];
#else
		return RenderPipelineState();
#endif
	}
	
	void Device::NewRenderPipelineState(const TileRenderPipelineDescriptor& descriptor, PipelineOption options, std::function<void(const RenderPipelineState&, const RenderPipelineReflection&, const ns::Error&)> completionHandler)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		[(id<MTLDevice>)m_ptr newRenderPipelineStateWithTileDescriptor: descriptor.GetPtr()
														   options:MTLPipelineOption(options)
												 completionHandler:^(id <MTLRenderPipelineState> renderPipelineState, MTLRenderPipelineReflection * reflection, NSError * error) {
													 completionHandler(
																	   renderPipelineState,
																	   reflection,
																	   error
																	   );
												 }];
#endif
	}
}
