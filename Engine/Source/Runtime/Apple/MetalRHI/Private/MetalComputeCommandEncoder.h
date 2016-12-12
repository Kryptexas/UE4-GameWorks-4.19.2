// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MetalDebugCommandEncoder.h"

NS_ASSUME_NONNULL_BEGIN

@class FMetalDebugCommandBuffer;

@interface FMetalDebugComputeCommandEncoder : NSObject<MTLComputeCommandEncoder>
{
@private
#pragma mark - Private Member Variables -
    FMetalDebugBufferBindings ShaderBuffers;
    FMetalDebugTextureBindings ShaderTextures;
    FMetalDebugSamplerBindings ShaderSamplers;
}

/** The wrapped native command-encoder for which we collect debug information. */
@property (readonly, retain) id<MTLComputeCommandEncoder> Inner;
@property (readonly, retain) FMetalDebugCommandBuffer* Buffer;
@property (nonatomic, retain) MTLComputePipelineReflection* Reflection;
@property (nonatomic, retain) NSString* Source;

/** Initialise the wrapper with the provided command-buffer. */
-(id)initWithEncoder:(id<MTLComputeCommandEncoder>)Encoder andCommandBuffer:(FMetalDebugCommandBuffer*)Buffer;

/** Validates the pipeline/binding state */
-(void)validate;

@end
NS_ASSUME_NONNULL_END

#if METAL_DEBUG_OPTIONS
#define METAL_SET_COMPUTE_REFLECTION(Encoder, InReflection, InSource)                                       \
    if (GetMetalDeviceContext().GetCommandQueue().GetRuntimeDebuggingLevel() >= EMetalDebugLevelValidation) \
    {                                                                                                       \
        ((FMetalDebugComputeCommandEncoder*)Encoder).Reflection = InReflection;                             \
        ((FMetalDebugComputeCommandEncoder*)Encoder).Source = InSource;                                     \
    }
#else
#define METAL_SET_COMPUTE_REFLECTION(Encoder, Reflection, Source)
#endif
