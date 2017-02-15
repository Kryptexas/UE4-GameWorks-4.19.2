// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MetalDebugCommandEncoder.h"

NS_ASSUME_NONNULL_BEGIN

@interface FMetalDebugParallelRenderCommandEncoder : NSObject<MTLParallelRenderCommandEncoder>
{
    @private
#pragma mark - Private Member Variables -
    FMetalDebugBufferBindings ShaderBuffers[EMetalShaderRenderNum];
    FMetalDebugTextureBindings ShaderTextures[EMetalShaderRenderNum];
    FMetalDebugSamplerBindings ShaderSamplers[EMetalShaderRenderNum];
}

/** The wrapped native command-encoder for which we collect debug information. */
@property (readonly, retain) id<MTLParallelRenderCommandEncoder> Inner;
@property (readonly, retain) FMetalDebugCommandBuffer* Buffer;
@property (readonly, retain) MTLRenderPassDescriptor* RenderPassDescriptor;

/** Initialise the wrapper with the provided command-buffer. */
-(id)initWithEncoder:(id<MTLParallelRenderCommandEncoder>)Encoder andCommandBuffer:(FMetalDebugCommandBuffer*)Buffer withDescriptor:(MTLRenderPassDescriptor *)renderPassDescriptor;

@end
NS_ASSUME_NONNULL_END
