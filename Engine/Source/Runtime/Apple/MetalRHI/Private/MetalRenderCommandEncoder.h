// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MetalDebugCommandEncoder.h"

NS_ASSUME_NONNULL_BEGIN

@interface FMetalDebugRenderCommandEncoder : NSObject<MTLRenderCommandEncoder>
{
    @private
#pragma mark - Private Member Variables -
    FMetalDebugBufferBindings ShaderBuffers[EMetalShaderRenderNum];
    FMetalDebugTextureBindings ShaderTextures[EMetalShaderRenderNum];
    FMetalDebugSamplerBindings ShaderSamplers[EMetalShaderRenderNum];
}

/** The wrapped native command-encoder for which we collect debug information. */
@property (readonly, retain) id<MTLRenderCommandEncoder> Inner;
@property (readonly, retain) FMetalDebugCommandBuffer* Buffer;
@property (nonatomic, retain) MTLRenderPipelineReflection* Reflection;
@property (nonatomic, retain) NSString* VertexSource;
@property (nonatomic, retain) NSString* FragmentSource;

/** Initialise the wrapper with the provided command-buffer. */
-(id)initWithEncoder:(id<MTLRenderCommandEncoder>)Encoder andCommandBuffer:(FMetalDebugCommandBuffer*)Buffer;

/** Validates the pipeline/binding state */
-(bool)validateFunctionBindings:(EMetalShaderFrequency)Frequency;
-(void)validate;

@end
NS_ASSUME_NONNULL_END

#if METAL_DEBUG_OPTIONS
#define METAL_SET_RENDER_REFLECTION(Encoder, InReflection, InVertex, InFragment)                            \
    if (GetMetalDeviceContext().GetCommandQueue().GetRuntimeDebuggingLevel() >= EMetalDebugLevelValidation) \
    {                                                                                                       \
        ((FMetalDebugRenderCommandEncoder*)Encoder).Reflection = InReflection;                              \
        ((FMetalDebugRenderCommandEncoder*)Encoder).VertexSource = InVertex;                                \
        ((FMetalDebugRenderCommandEncoder*)Encoder).FragmentSource = InFragment;                            \
    }
#else
#define METAL_SET_RENDER_REFLECTION(Encoder, Reflection, VertexSource, FragmentSource)
#endif

