// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#import <Metal/MTLDevice.h>
#include "MTIDevice.hpp"
#include "MTIBuffer.hpp"
#include "MTITexture.hpp"
#include "MTICommandQueue.hpp"
#include "MTIArgumentEncoder.hpp"
#include "MTIHeap.hpp"
#include "MTILibrary.hpp"
#include "MTIRenderPipeline.hpp"
#include "MTIComputePipeline.hpp"
#include "MTISampler.hpp"
#include "MTIFence.hpp"
#include "MTIDepthStencil.hpp"

MTLPP_BEGIN

#define DYLD_INTERPOSE(_replacment,_replacee) \
__attribute__((used)) struct{ const void* replacment; const void* replacee; } _interpose_##_replacee \
__attribute__ ((section ("__DATA,__interpose"))) = { (const void*)(unsigned long)&_replacment, (const void*)(unsigned long)&_replacee };

INTERPOSE_PROTOCOL_REGISTER(MTIDeviceTrace, id<MTLDevice>);

MTL_EXTERN id <MTLDevice> __nullable MTLTCreateSystemDefaultDevice(void)
{
	return MTIDeviceTrace::Register(MTLCreateSystemDefaultDevice());
}
DYLD_INTERPOSE(MTLTCreateSystemDefaultDevice, MTLCreateSystemDefaultDevice)

MTL_EXTERN NSArray <id<MTLDevice>> *MTLTCopyAllDevices(void)
{
	NSArray <id<MTLDevice>> * Devices = MTLCopyAllDevices();
	NSMutableArray* NewDevices = [NSMutableArray new];
	for (id<MTLDevice> Device in Devices)
	{
		MTIDeviceTrace::Register(Device);
	}
	return NewDevices;
}
DYLD_INTERPOSE(MTLTCopyAllDevices, MTLCopyAllDevices)

id<MTLCommandQueue> MTIDeviceTrace::NewCommandQueueImpl(id Object, SEL Selector, Super::NewCommandQueueType::DefinedIMP Original)
{
	return MTICommandQueueTrace::Register(Original(Object, Selector));
}
id<MTLCommandQueue> MTIDeviceTrace::NewCommandQueueWithMaxCommandBufferCountImpl(id Object, SEL Selector, Super::NewCommandQueueWithMaxCommandBufferCountType::DefinedIMP Original, NSUInteger Num)
{
	return MTICommandQueueTrace::Register(Original(Object, Selector, Num));
}
id<MTLHeap> MTIDeviceTrace::NewHeapWithDescriptorImpl(id Object, SEL Selector, Super::NewHeapWithDescriptorType::DefinedIMP Original, MTLHeapDescriptor* Desc)
{
	return MTIHeapTrace::Register(Original(Object, Selector, Desc));
}
id<MTLBuffer> MTIDeviceTrace::NewBufferWithLengthImpl(id Object, SEL Selector, Super::NewBufferWithLengthType::DefinedIMP Original, NSUInteger Len, MTLResourceOptions Opt)
{
	return MTIBufferTrace::Register(Original(Object, Selector, Len, Opt));
}
id<MTLBuffer> MTIDeviceTrace::NewBufferWithBytesImpl(id Object, SEL Selector, Super::NewBufferWithBytesType::DefinedIMP Original, const void* Ptr, NSUInteger Len, MTLResourceOptions Opt)
{
	return MTIBufferTrace::Register(Original(Object, Selector, Ptr, Len, Opt));
}
id<MTLBuffer> MTIDeviceTrace::NewBufferWithBytesNoCopyImpl(id Object, SEL Selector, Super::NewBufferWithBytesNoCopyType::DefinedIMP Original, const void* Ptr, NSUInteger Len, MTLResourceOptions Opt, void (^__nullable Block)(void *pointer, NSUInteger length))
{
	return MTIBufferTrace::Register(Original(Object, Selector, Ptr, Len, Opt, Block));
}
id<MTLDepthStencilState> MTIDeviceTrace::NewDepthStencilStateWithDescriptorImpl(id Object, SEL Selector, Super::NewDepthStencilStateWithDescriptorType::DefinedIMP Original, MTLDepthStencilDescriptor* Desc)
{
	return MTIDepthStencilStateTrace::Register(Original(Object, Selector, Desc));
}
id<MTLTexture> MTIDeviceTrace::NewTextureWithDescriptorImpl(id Object, SEL Selector, Super::NewTextureWithDescriptorType::DefinedIMP Original, MTLTextureDescriptor* Desc)
{
	return MTITextureTrace::Register(Original(Object, Selector, Desc));
}
id<MTLTexture> MTIDeviceTrace::NewTextureWithDescriptorIOSurfaceImpl(id Object, SEL Selector, Super::NewTextureWithDescriptorIOSurfaceType::DefinedIMP Original, MTLTextureDescriptor* Desc, IOSurfaceRef IO, NSUInteger Plane)
{
	return MTITextureTrace::Register(Original(Object, Selector, Desc, IO, Plane));
}
id<MTLSamplerState> MTIDeviceTrace::NewSamplerStateWithDescriptorImpl(id Object, SEL Selector, Super::NewSamplerStateWithDescriptorType::DefinedIMP Original, MTLSamplerDescriptor* Desc)
{
	return MTISamplerStateTrace::Register(Original(Object, Selector, Desc));
}
id<MTLLibrary> MTIDeviceTrace::NewDefaultLibraryImpl(id Object, SEL Selector, Super::NewDefaultLibraryType::DefinedIMP Original)
{
	return MTILibraryTrace::Register(Original(Object, Selector));
}
id<MTLLibrary> MTIDeviceTrace::NewDefaultLibraryWithBundleImpl(id Object, SEL Selector, Super::NewDefaultLibraryWithBundleType::DefinedIMP Original, NSBundle* Bndl, __autoreleasing NSError ** Err)
{
	return MTILibraryTrace::Register(Original(Object, Selector, Bndl, Err));
}
id<MTLLibrary> MTIDeviceTrace::NewLibraryWithFileImpl(id Object, SEL Selector, Super::NewLibraryWithFileType::DefinedIMP Original, NSString* Str, __autoreleasing NSError ** Err)
{
	return MTILibraryTrace::Register(Original(Object, Selector, Str, Err));
}
id<MTLLibrary> MTIDeviceTrace::NewLibraryWithURLImpl(id Object, SEL Selector, Super::NewLibraryWithURLType::DefinedIMP Original, NSURL* Url, __autoreleasing NSError ** Err)
{
	return MTILibraryTrace::Register(Original(Object, Selector, Url, Err));
}
id<MTLLibrary> MTIDeviceTrace::NewLibraryWithDataImpl(id Object, SEL Selector, Super::NewLibraryWithDataType::DefinedIMP Original, dispatch_data_t Data, __autoreleasing NSError ** Err)
{
	return MTILibraryTrace::Register(Original(Object, Selector, Data, Err));
}
id<MTLLibrary> MTIDeviceTrace::NewLibraryWithSourceOptionsErrorImpl(id Object, SEL Selector, Super::NewLibraryWithSourceOptionsErrorType::DefinedIMP Original, NSString* Src, MTLCompileOptions* Opts, NSError** Err)
{
	return MTILibraryTrace::Register(Original(Object, Selector, Src, Opts, Err));
}
void MTIDeviceTrace::NewLibraryWithSourceOptionsCompletionHandlerImpl(id Object, SEL Selector, Super::NewLibraryWithSourceOptionsCompletionHandlerType::DefinedIMP Original, NSString* Src, MTLCompileOptions* Opts, MTLNewLibraryCompletionHandler Handler)
{
	Original(Object, Selector, Src, Opts, ^(id <MTLLibrary> __nullable library, NSError * __nullable error)
	{
		Handler(MTILibraryTrace::Register(library), error);
	});
}
id<MTLRenderPipelineState> MTIDeviceTrace::NewRenderPipelineStateWithDescriptorErrorImpl(id Object, SEL Selector, Super::NewRenderPipelineStateWithDescriptorErrorType::DefinedIMP Original, MTLRenderPipelineDescriptor* Desc, NSError** Err)
{
	return MTIRenderPipelineStateTrace::Register(Original(Object, Selector, Desc, Err));
}
id<MTLRenderPipelineState> MTIDeviceTrace::NewRenderPipelineStateWithDescriptorOptionsReflectionErrorImpl(id Object, SEL Selector, Super::NewRenderPipelineStateWithDescriptorOptionsReflectionErrorType::DefinedIMP Original, MTLRenderPipelineDescriptor* Desc, MTLPipelineOption Opts, MTLAutoreleasedRenderPipelineReflection* Refl, NSError** Err)
{
	return MTIRenderPipelineStateTrace::Register(Original(Object, Selector, Desc, Opts, Refl, Err));
}
void MTIDeviceTrace::NewRenderPipelineStateWithDescriptorCompletionHandlerImpl(id Object, SEL Selector, Super::NewRenderPipelineStateWithDescriptorCompletionHandlerType::DefinedIMP Original, MTLRenderPipelineDescriptor* Desc, MTLNewRenderPipelineStateCompletionHandler Handler)
{
	Original(Object, Selector, Desc, ^(id <MTLRenderPipelineState> __nullable renderPipelineState, NSError * __nullable error)
			 {
				 Handler(MTIRenderPipelineStateTrace::Register(renderPipelineState), error);
			 });
}
void MTIDeviceTrace::NewRenderPipelineStateWithDescriptorOptionsCompletionHandlerImpl(id Object, SEL Selector, Super::NewRenderPipelineStateWithDescriptorOptionsCompletionHandlerType::DefinedIMP Original, MTLRenderPipelineDescriptor* Desc, MTLPipelineOption Opts, MTLNewRenderPipelineStateWithReflectionCompletionHandler Handler)
{
	Original(Object, Selector, Desc, Opts, ^(id <MTLRenderPipelineState> __nullable renderPipelineState, MTLRenderPipelineReflection * __nullable reflection, NSError * __nullable error)
	{
		Handler(MTIRenderPipelineStateTrace::Register(renderPipelineState), reflection, error);
	});
}
id<MTLComputePipelineState> MTIDeviceTrace::NewComputePipelineStateWithFunctionErrorImpl(id Object, SEL Selector, Super::NewComputePipelineStateWithFunctionErrorType::DefinedIMP Original, id<MTLFunction> Func, NSError** Err)
{
	return MTIComputePipelineStateTrace::Register(Original(Object, Selector, Func, Err));
}
id<MTLComputePipelineState> MTIDeviceTrace::NewComputePipelineStateWithFunctionOptionsReflectionErrorImpl(id Object, SEL Selector, Super::NewComputePipelineStateWithFunctionOptionsReflectionErrorType::DefinedIMP Original, id<MTLFunction> Func, MTLPipelineOption Opts, MTLAutoreleasedComputePipelineReflection * Refl, NSError** Err)
{
	return MTIComputePipelineStateTrace::Register(Original(Object, Selector, Func, Opts, Refl, Err));
}
void MTIDeviceTrace::NewComputePipelineStateWithFunctionCompletionHandlerImpl(id Object, SEL Selector, Super::NewComputePipelineStateWithFunctionCompletionHandlerType::DefinedIMP Original, id<MTLFunction>  Func, MTLNewComputePipelineStateCompletionHandler Handler)
{
	Original(Object, Selector, Func, ^(id <MTLComputePipelineState> __nullable computePipelineState, NSError * __nullable error){
		Handler(MTIComputePipelineStateTrace::Register(computePipelineState ), error);
	});
}
void MTIDeviceTrace::NewComputePipelineStateWithFunctionOptionsCompletionHandlerImpl(id Object, SEL Selector, Super::NewComputePipelineStateWithFunctionOptionsCompletionHandlerType::DefinedIMP Original, id<MTLFunction> Func, MTLPipelineOption Opts, MTLNewComputePipelineStateWithReflectionCompletionHandler Handler)
{
	Original(Object, Selector, Func, Opts, ^(id <MTLComputePipelineState> __nullable computePipelineState, MTLComputePipelineReflection * __nullable reflection, NSError * __nullable error){
		Handler(MTIComputePipelineStateTrace::Register(computePipelineState ), reflection, error);
	});
}
id<MTLComputePipelineState> MTIDeviceTrace::NewComputePipelineStateWithDescriptorOptionsReflectionErrorImpl(id Object, SEL Selector, Super::NewComputePipelineStateWithDescriptorOptionsReflectionErrorType::DefinedIMP Original, MTLComputePipelineDescriptor* Desc, MTLPipelineOption Opts, MTLAutoreleasedComputePipelineReflection * Refl, NSError** Err)
{
	return MTIComputePipelineStateTrace::Register(Original(Object, Selector, Desc, Opts, Refl, Err));
}
void MTIDeviceTrace::NewComputePipelineStateWithDescriptorOptionsCompletionHandlerImpl(id Object, SEL Selector, Super::NewComputePipelineStateWithDescriptorOptionsCompletionHandlerType::DefinedIMP Original, MTLComputePipelineDescriptor* Desc, MTLPipelineOption Opts, MTLNewComputePipelineStateWithReflectionCompletionHandler Handler)
{
	Original(Object, Selector, Desc, Opts, ^(id <MTLComputePipelineState> __nullable computePipelineState, MTLComputePipelineReflection * __nullable reflection, NSError * __nullable error){
		Handler(MTIComputePipelineStateTrace::Register(computePipelineState ), reflection, error);
	});
}
id<MTLFence> MTIDeviceTrace::NewFenceImpl(id Object, SEL Selector, Super::NewFenceType::DefinedIMP Original)
{
	return MTIFenceTrace::Register(Original(Object, Selector));
}
id<MTLArgumentEncoder> MTIDeviceTrace::NewArgumentEncoderWithArgumentsImpl(id Object, SEL Selector, Super::NewArgumentEncoderWithArgumentsType::DefinedIMP Original, NSArray <MTLArgumentDescriptor *> * Args)
{
	return MTIArgumentEncoderTrace::Register(Original(Object, Selector, Args));
}
INTERPOSE_DEFINITION(MTIDeviceTrace, getDefaultSamplePositionscount, void, MTLPPSamplePosition* s, NSUInteger c)
{
	Original(Obj, Cmd, s, c);
}

INTERPOSE_DEFINITION(MTIDeviceTrace, newRenderPipelineStateWithTileDescriptoroptionsreflectionerror, id <MTLRenderPipelineState>, MTLTileRenderPipelineDescriptor* d, MTLPipelineOption o, MTLAutoreleasedRenderPipelineReflection* r, NSError** e)
{
	return Original(Obj, Cmd, d, o, r, e);
}

INTERPOSE_DEFINITION(MTIDeviceTrace,  newRenderPipelineStateWithTileDescriptoroptionscompletionHandler, void, MTLTileRenderPipelineDescriptor* d, MTLPipelineOption o, MTLNewRenderPipelineStateWithReflectionCompletionHandler h)
{
	Original(Obj, Cmd, d, o, h);
}

MTLPP_END
