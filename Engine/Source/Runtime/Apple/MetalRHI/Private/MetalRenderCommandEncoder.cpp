// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MetalRenderCommandEncoder.cpp: Metal command encoder wrapper.
=============================================================================*/

#include "MetalRHIPrivate.h"

#include "MetalRenderCommandEncoder.h"
#include "MetalCommandBuffer.h"

NS_ASSUME_NONNULL_BEGIN

@implementation FMetalDebugRenderCommandEncoder

@synthesize Inner;
@synthesize Buffer;
@synthesize Reflection;
@synthesize VertexSource;
@synthesize FragmentSource;

-(id)initWithEncoder:(id<MTLRenderCommandEncoder>)Encoder andCommandBuffer:(FMetalDebugCommandBuffer*)SourceBuffer
{
	id Self = [super init];
	if (Self)
	{
        Inner = [Encoder retain];
		Buffer = [SourceBuffer retain];
        Reflection = nil;
        VertexSource = nil;
        FragmentSource = nil;
	}
	return Self;
}

-(void)dealloc
{
	[Inner release];
	[Buffer release];
	[Reflection release];
	[VertexSource release];
	[FragmentSource release];
	[super dealloc];
}

-(id <MTLDevice>) device
{
	return Inner.device;
}

-(NSString *_Nullable)label
{
	return Inner.label;
}

-(void)setLabel:(NSString *_Nullable)Text
{
	Inner.label = Text;
}

- (void)endEncoding
{
    [Buffer endCommandEncoder];
    [Inner endEncoding];
}

- (void)insertDebugSignpost:(NSString *)string
{
    [Buffer insertDebugSignpost:string];
    [Inner insertDebugSignpost:string];
}

- (void)pushDebugGroup:(NSString *)string
{
    [Buffer pushDebugGroup:string];
    [Inner pushDebugGroup:string];
}

- (void)popDebugGroup
{
    [Buffer popDebugGroup];
    [Inner popDebugGroup];
}

- (void)setRenderPipelineState:(id <MTLRenderPipelineState>)pipelineState
{
    [Buffer setPipeline:pipelineState.label];
    [Buffer trackState:pipelineState];
    [Inner setRenderPipelineState:pipelineState];
}

- (void)setVertexBytes:(const void *)bytes length:(NSUInteger)length atIndex:(NSUInteger)index
{
    ShaderBuffers[EMetalShaderVertex].Buffers[index] = nil;
    ShaderBuffers[EMetalShaderVertex].Bytes[index] = bytes;
    ShaderBuffers[EMetalShaderVertex].Offsets[index] = length;
    
    [Inner setVertexBytes:bytes length:length atIndex:index];
}

- (void)setVertexBuffer:(nullable id <MTLBuffer>)buffer offset:(NSUInteger)offset atIndex:(NSUInteger)index
{
    ShaderBuffers[EMetalShaderVertex].Buffers[index] = buffer;
    ShaderBuffers[EMetalShaderVertex].Bytes[index] = nil;
    ShaderBuffers[EMetalShaderVertex].Offsets[index] = offset;
    
    [Buffer trackResource:buffer];
    [Inner setVertexBuffer:buffer offset:offset atIndex:index];
}

- (void)setVertexBufferOffset:(NSUInteger)offset atIndex:(NSUInteger)index
{
    ShaderBuffers[EMetalShaderVertex].Offsets[index] = offset;
    
    [Inner setVertexBufferOffset:offset atIndex:index];
}

- (void)setVertexBuffers:(const id <MTLBuffer> __nullable [])buffers offsets:(const NSUInteger [])offsets withRange:(NSRange)range
{
	for (uint32 i = 0; i < range.length; i++)
    {
        ShaderBuffers[EMetalShaderVertex].Buffers[i + range.location] = buffers[i];
        ShaderBuffers[EMetalShaderVertex].Bytes[i + range.location] = nil;
        ShaderBuffers[EMetalShaderVertex].Offsets[i + range.location] = offsets[i];
        
        [Buffer trackResource:buffers[i]];
    }
    [Inner setVertexBuffers:buffers offsets:offsets withRange:range];
}

- (void)setVertexTexture:(nullable id <MTLTexture>)texture atIndex:(NSUInteger)index
{
    ShaderTextures[EMetalShaderVertex].Textures[index] = texture;
    [Buffer trackResource:texture];
    [Inner setVertexTexture:texture atIndex:index];
}

- (void)setVertexTextures:(const id <MTLTexture> __nullable [__nullable])textures withRange:(NSRange)range
{
    for (uint32 i = 0; i < range.length; i++)
    {
        ShaderTextures[EMetalShaderVertex].Textures[i + range.location] = textures[i];
        [Buffer trackResource:textures[i]];
    }
    [Inner setVertexTextures:textures withRange:range];
}

- (void)setVertexSamplerState:(nullable id <MTLSamplerState>)sampler atIndex:(NSUInteger)index
{
    ShaderSamplers[EMetalShaderVertex].Samplers[index] = sampler;
    [Buffer trackState:sampler];
    [Inner setVertexSamplerState:sampler atIndex:index];
}

- (void)setVertexSamplerStates:(const id <MTLSamplerState> __nullable [__nullable])samplers withRange:(NSRange)range
{
    for(uint32 i = 0; i < range.length; i++)
    {
        ShaderSamplers[EMetalShaderVertex].Samplers[i + range.location] = samplers[i];
        [Buffer trackState:samplers[i]];
    }
    [Inner setVertexSamplerStates:samplers withRange:range];
}

- (void)setVertexSamplerState:(nullable id <MTLSamplerState>)sampler lodMinClamp:(float)lodMinClamp lodMaxClamp:(float)lodMaxClamp atIndex:(NSUInteger)index
{
    ShaderSamplers[EMetalShaderVertex].Samplers[index] = sampler;
    [Buffer trackState:sampler];
    [Inner setVertexSamplerState:sampler lodMinClamp:lodMinClamp lodMaxClamp:lodMaxClamp atIndex:index];
}

- (void)setVertexSamplerStates:(const id <MTLSamplerState> __nullable [__nullable])samplers lodMinClamps:(const float [__nullable])lodMinClamps lodMaxClamps:(const float [__nullable])lodMaxClamps withRange:(NSRange)range
{
    for(uint32 i = 0; i < range.length; i++)
    {
        ShaderSamplers[EMetalShaderVertex].Samplers[i + range.location] = samplers[i];
        [Buffer trackState:samplers[i]];
    }
    [Inner setVertexSamplerStates:samplers lodMinClamps:lodMinClamps lodMaxClamps:lodMaxClamps withRange:range];
}

- (void)setViewport:(MTLViewport)viewport
{
    [Inner setViewport:viewport];
}

- (void)setFrontFacingWinding:(MTLWinding)frontFacingWinding
{
    [Inner setFrontFacingWinding:frontFacingWinding];
}

- (void)setCullMode:(MTLCullMode)cullMode
{
    [Inner setCullMode:cullMode];
}

#if PLATFORM_MAC
- (void)setDepthClipMode:(MTLDepthClipMode)depthClipMode
{
    [Inner setDepthClipMode:depthClipMode];
}
#endif

- (void)setDepthBias:(float)depthBias slopeScale:(float)slopeScale clamp:(float)clamp
{
    [Inner setDepthBias:depthBias slopeScale:slopeScale clamp:clamp];
}

- (void)setScissorRect:(MTLScissorRect)rect
{
    [Inner setScissorRect:rect];
}

- (void)setTriangleFillMode:(MTLTriangleFillMode)fillMode
{
    [Inner setTriangleFillMode:fillMode];
}

- (void)setFragmentBytes:(const void *)bytes length:(NSUInteger)length atIndex:(NSUInteger)index
{
    ShaderBuffers[EMetalShaderFragment].Buffers[index] = nil;
    ShaderBuffers[EMetalShaderFragment].Bytes[index] = bytes;
    ShaderBuffers[EMetalShaderFragment].Offsets[index] = length;
    
    [Inner setFragmentBytes:(const void *)bytes length:(NSUInteger)length atIndex:(NSUInteger)index];
}

- (void)setFragmentBuffer:(nullable id <MTLBuffer>)buffer offset:(NSUInteger)offset atIndex:(NSUInteger)index
{
    ShaderBuffers[EMetalShaderFragment].Buffers[index] = buffer;
    ShaderBuffers[EMetalShaderFragment].Bytes[index] = nil;
    ShaderBuffers[EMetalShaderFragment].Offsets[index] = offset;
    
    [Buffer trackResource:buffer];
    [Inner setFragmentBuffer:buffer offset:offset atIndex:index];
}

- (void)setFragmentBufferOffset:(NSUInteger)offset atIndex:(NSUInteger)index
{
    ShaderBuffers[EMetalShaderFragment].Offsets[index] = offset;
    
    [Inner setFragmentBufferOffset:(NSUInteger)offset atIndex:(NSUInteger)index];
}

- (void)setFragmentBuffers:(const id <MTLBuffer> __nullable [])buffers offsets:(const NSUInteger [])offset withRange:(NSRange)range
{
    for (uint32 i = 0; i < range.length; i++)
    {
        ShaderBuffers[EMetalShaderFragment].Buffers[i + range.location] = buffers[i];
        ShaderBuffers[EMetalShaderFragment].Bytes[i + range.location] = nil;
        ShaderBuffers[EMetalShaderFragment].Offsets[i + range.location] = offset[i];
        
        [Buffer trackResource:buffers[i]];
    }
    [Inner setFragmentBuffers:buffers offsets:offset withRange:range];
}

- (void)setFragmentTexture:(nullable id <MTLTexture>)texture atIndex:(NSUInteger)index
{
    ShaderTextures[EMetalShaderFragment].Textures[index] = texture;
    [Buffer trackResource:texture];
    [Inner setFragmentTexture:texture atIndex:index];
}

- (void)setFragmentTextures:(const id <MTLTexture> __nullable [__nullable])textures withRange:(NSRange)range
{
    for (uint32 i = 0; i < range.length; i++)
    {
        ShaderTextures[EMetalShaderFragment].Textures[i + range.location] = textures[i];
        [Buffer trackResource:textures[i]];
    }
    [Inner setFragmentTextures:textures withRange:(NSRange)range];
}

- (void)setFragmentSamplerState:(nullable id <MTLSamplerState>)sampler atIndex:(NSUInteger)index
{
    ShaderSamplers[EMetalShaderFragment].Samplers[index] = sampler;
    [Buffer trackState:sampler];
    [Inner setFragmentSamplerState:sampler atIndex:(NSUInteger)index];
}

- (void)setFragmentSamplerStates:(const id <MTLSamplerState> __nullable [__nullable])samplers withRange:(NSRange)range
{
	for(uint32 i = 0; i < range.length; i++)
    {
        ShaderSamplers[EMetalShaderFragment].Samplers[i + range.location] = samplers[i];
        [Buffer trackState:samplers[i]];
    }
    [Inner setFragmentSamplerStates:samplers withRange:(NSRange)range];
}

- (void)setFragmentSamplerState:(nullable id <MTLSamplerState>)sampler lodMinClamp:(float)lodMinClamp lodMaxClamp:(float)lodMaxClamp atIndex:(NSUInteger)index
{
    ShaderSamplers[EMetalShaderFragment].Samplers[index] = sampler;
    [Buffer trackState:sampler];
    [Inner setFragmentSamplerState:sampler lodMinClamp:(float)lodMinClamp lodMaxClamp:(float)lodMaxClamp atIndex:(NSUInteger)index];
}

- (void)setFragmentSamplerStates:(const id <MTLSamplerState> __nullable [__nullable])samplers lodMinClamps:(const float [__nullable])lodMinClamps lodMaxClamps:(const float [__nullable])lodMaxClamps withRange:(NSRange)range
{
    for(uint32 i = 0; i < range.length; i++)
    {
        ShaderSamplers[EMetalShaderFragment].Samplers[i + range.location] = samplers[i];
        [Buffer trackState:samplers[i]];
    }
    [Inner setFragmentSamplerStates:samplers lodMinClamps:lodMinClamps lodMaxClamps:lodMaxClamps withRange:(NSRange)range];
}

- (void)setBlendColorRed:(float)red green:(float)green blue:(float)blue alpha:(float)alpha
{
    [Inner setBlendColorRed:red green:green blue:blue alpha:alpha];
}

- (void)setDepthStencilState:(nullable id <MTLDepthStencilState>)depthStencilState
{
    [Buffer trackState:depthStencilState];
    [Inner setDepthStencilState:depthStencilState];
}

- (void)setStencilReferenceValue:(uint32_t)referenceValue
{
    [Inner setStencilReferenceValue:referenceValue];
}

- (void)setStencilFrontReferenceValue:(uint32_t)frontReferenceValue backReferenceValue:(uint32_t)backReferenceValue
{
    [Inner setStencilFrontReferenceValue:frontReferenceValue backReferenceValue:backReferenceValue];
}

- (void)setVisibilityResultMode:(MTLVisibilityResultMode)mode offset:(NSUInteger)offset
{
    [Inner setVisibilityResultMode:mode offset:offset];
}

- (void)setColorStoreAction:(MTLStoreAction)storeAction atIndex:(NSUInteger)colorAttachmentIndex
{
    [Inner setColorStoreAction:storeAction atIndex:colorAttachmentIndex];
}

- (void)setDepthStoreAction:(MTLStoreAction)storeAction
{
    [Inner setDepthStoreAction:storeAction];
}

- (void)setStencilStoreAction:(MTLStoreAction)storeAction
{
    [Inner setStencilStoreAction:storeAction];
}

- (void)drawPrimitives:(MTLPrimitiveType)primitiveType vertexStart:(NSUInteger)vertexStart vertexCount:(NSUInteger)vertexCount instanceCount:(NSUInteger)instanceCount
{
    [Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    
    [self validate];
    [Inner drawPrimitives:primitiveType vertexStart:vertexStart vertexCount:vertexCount instanceCount:instanceCount];
}

- (void)drawPrimitives:(MTLPrimitiveType)primitiveType vertexStart:(NSUInteger)vertexStart vertexCount:(NSUInteger)vertexCount
{
    [Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    
    [self validate];
    [Inner drawPrimitives:primitiveType vertexStart:vertexStart vertexCount:vertexCount];
}

- (void)drawIndexedPrimitives:(MTLPrimitiveType)primitiveType indexCount:(NSUInteger)indexCount indexType:(MTLIndexType)indexType indexBuffer:(id <MTLBuffer>)indexBuffer indexBufferOffset:(NSUInteger)indexBufferOffset instanceCount:(NSUInteger)instanceCount
{
    [Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    
    [self validate];
    [Buffer trackResource:indexBuffer];
    [Inner drawIndexedPrimitives:(MTLPrimitiveType)primitiveType indexCount:(NSUInteger)indexCount indexType:(MTLIndexType)indexType indexBuffer:(id <MTLBuffer>)indexBuffer indexBufferOffset:(NSUInteger)indexBufferOffset instanceCount:(NSUInteger)instanceCount];
}

- (void)drawIndexedPrimitives:(MTLPrimitiveType)primitiveType indexCount:(NSUInteger)indexCount indexType:(MTLIndexType)indexType indexBuffer:(id <MTLBuffer>)indexBuffer indexBufferOffset:(NSUInteger)indexBufferOffset
{
    [Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    
    [self validate];
    [Buffer trackResource:indexBuffer];
    [Inner drawIndexedPrimitives:(MTLPrimitiveType)primitiveType indexCount:(NSUInteger)indexCount indexType:(MTLIndexType)indexType indexBuffer:(id <MTLBuffer>)indexBuffer indexBufferOffset:(NSUInteger)indexBufferOffset];
}

- (void)drawPrimitives:(MTLPrimitiveType)primitiveType vertexStart:(NSUInteger)vertexStart vertexCount:(NSUInteger)vertexCount instanceCount:(NSUInteger)instanceCount baseInstance:(NSUInteger)baseInstance
{
    [Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    
    [self validate];
    [Inner drawPrimitives:(MTLPrimitiveType)primitiveType vertexStart:(NSUInteger)vertexStart vertexCount:(NSUInteger)vertexCount instanceCount:(NSUInteger)instanceCount baseInstance:(NSUInteger)baseInstance];
}

- (void)drawIndexedPrimitives:(MTLPrimitiveType)primitiveType indexCount:(NSUInteger)indexCount indexType:(MTLIndexType)indexType indexBuffer:(id <MTLBuffer>)indexBuffer indexBufferOffset:(NSUInteger)indexBufferOffset instanceCount:(NSUInteger)instanceCount baseVertex:(NSInteger)baseVertex baseInstance:(NSUInteger)baseInstance
{
    [Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    [Buffer trackResource:indexBuffer];
    
    [self validate];
    [Inner drawIndexedPrimitives:(MTLPrimitiveType)primitiveType indexCount:(NSUInteger)indexCount indexType:(MTLIndexType)indexType indexBuffer:(id <MTLBuffer>)indexBuffer indexBufferOffset:(NSUInteger)indexBufferOffset instanceCount:(NSUInteger)instanceCount baseVertex:(NSInteger)baseVertex baseInstance:(NSUInteger)baseInstance];
}

- (void)drawPrimitives:(MTLPrimitiveType)primitiveType indirectBuffer:(id <MTLBuffer>)indirectBuffer indirectBufferOffset:(NSUInteger)indirectBufferOffset
{
    [Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    [Buffer trackResource:indirectBuffer];
    
    [self validate];
    [Inner drawPrimitives:(MTLPrimitiveType)primitiveType indirectBuffer:(id <MTLBuffer>)indirectBuffer indirectBufferOffset:(NSUInteger)indirectBufferOffset];
}

- (void)drawIndexedPrimitives:(MTLPrimitiveType)primitiveType indexType:(MTLIndexType)indexType indexBuffer:(id <MTLBuffer>)indexBuffer indexBufferOffset:(NSUInteger)indexBufferOffset indirectBuffer:(id <MTLBuffer>)indirectBuffer indirectBufferOffset:(NSUInteger)indirectBufferOffset
{
    [Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    [Buffer trackResource:indexBuffer];
    [Buffer trackResource:indirectBuffer];
    
    [self validate];
    [Inner drawIndexedPrimitives:(MTLPrimitiveType)primitiveType indexType:(MTLIndexType)indexType indexBuffer:(id <MTLBuffer>)indexBuffer indexBufferOffset:(NSUInteger)indexBufferOffset indirectBuffer:(id <MTLBuffer>)indirectBuffer indirectBufferOffset:(NSUInteger)indirectBufferOffset];
}

#if PLATFORM_MAC
- (void)textureBarrier
{
	[Inner textureBarrier];
}

#else

- (void)updateFence:(id <MTLFence>)fence afterStages:(MTLRenderStages)stages
{
    [Inner updateFence:(id <MTLFence>)fence afterStages:(MTLRenderStages)stages];
}

- (void)waitForFence:(id <MTLFence>)fence beforeStages:(MTLRenderStages)stages
{
    [Inner waitForFence:(id <MTLFence>)fence beforeStages:(MTLRenderStages)stages];
}
#endif

-(void)setTessellationFactorBuffer:(nullable id <MTLBuffer>)buffer offset:(NSUInteger)offset instanceStride:(NSUInteger)instanceStride
{
    [Buffer trackResource:buffer];
    
    [Inner setTessellationFactorBuffer:buffer offset:(NSUInteger)offset instanceStride:(NSUInteger)instanceStride];
}

-(void)setTessellationFactorScale:(float)scale
{
    [Inner setTessellationFactorScale:(float)scale];
}

-(void)drawPatches:(NSUInteger)numberOfPatchControlPoints patchStart:(NSUInteger)patchStart patchCount:(NSUInteger)patchCount patchIndexBuffer:(nullable id <MTLBuffer>)patchIndexBuffer patchIndexBufferOffset:(NSUInteger)patchIndexBufferOffset instanceCount:(NSUInteger)instanceCount baseInstance:(NSUInteger)baseInstance
{
    [Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    [Buffer trackResource:patchIndexBuffer];
    
    [self validate];
    [Inner drawPatches:(NSUInteger)numberOfPatchControlPoints patchStart:(NSUInteger)patchStart patchCount:(NSUInteger)patchCount patchIndexBuffer:patchIndexBuffer patchIndexBufferOffset:(NSUInteger)patchIndexBufferOffset instanceCount:(NSUInteger)instanceCount baseInstance:(NSUInteger)baseInstance];
}

#if PLATFORM_MAC
-(void)drawPatches:(NSUInteger)numberOfPatchControlPoints patchIndexBuffer:(nullable id <MTLBuffer>)patchIndexBuffer patchIndexBufferOffset:(NSUInteger)patchIndexBufferOffset indirectBuffer:(id <MTLBuffer>)indirectBuffer indirectBufferOffset:(NSUInteger)indirectBufferOffset
{
    [Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    [Buffer trackResource:patchIndexBuffer];
    [Buffer trackResource:indirectBuffer];
    
    [self validate];
    [Inner drawPatches:(NSUInteger)numberOfPatchControlPoints patchIndexBuffer:patchIndexBuffer patchIndexBufferOffset:(NSUInteger)patchIndexBufferOffset indirectBuffer:(id <MTLBuffer>)indirectBuffer indirectBufferOffset:(NSUInteger)indirectBufferOffset];
}
#endif

-(void)drawIndexedPatches:(NSUInteger)numberOfPatchControlPoints patchStart:(NSUInteger)patchStart patchCount:(NSUInteger)patchCount patchIndexBuffer:(nullable id <MTLBuffer>)patchIndexBuffer patchIndexBufferOffset:(NSUInteger)patchIndexBufferOffset controlPointIndexBuffer:(id <MTLBuffer>)controlPointIndexBuffer controlPointIndexBufferOffset:(NSUInteger)controlPointIndexBufferOffset instanceCount:(NSUInteger)instanceCount baseInstance:(NSUInteger)baseInstance
{
    [Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    [Buffer trackResource:patchIndexBuffer];
    [Buffer trackResource:controlPointIndexBuffer];
    
    [self validate];
    [Inner drawIndexedPatches:(NSUInteger)numberOfPatchControlPoints patchStart:(NSUInteger)patchStart patchCount:(NSUInteger)patchCount patchIndexBuffer:patchIndexBuffer patchIndexBufferOffset:(NSUInteger)patchIndexBufferOffset controlPointIndexBuffer:(id <MTLBuffer>)controlPointIndexBuffer controlPointIndexBufferOffset:(NSUInteger)controlPointIndexBufferOffset instanceCount:(NSUInteger)instanceCount baseInstance:(NSUInteger)baseInstance];
}

#if PLATFORM_MAC
-(void)drawIndexedPatches:(NSUInteger)numberOfPatchControlPoints patchIndexBuffer:(nullable id <MTLBuffer>)patchIndexBuffer patchIndexBufferOffset:(NSUInteger)patchIndexBufferOffset controlPointIndexBuffer:(id <MTLBuffer>)controlPointIndexBuffer controlPointIndexBufferOffset:(NSUInteger)controlPointIndexBufferOffset indirectBuffer:(id <MTLBuffer>)indirectBuffer indirectBufferOffset:(NSUInteger)indirectBufferOffset
{
    [Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    
    [Buffer trackResource:patchIndexBuffer];
    [Buffer trackResource:controlPointIndexBuffer];
    [Buffer trackResource:indirectBuffer];
    
    [self validate];
    [Inner drawIndexedPatches:(NSUInteger)numberOfPatchControlPoints patchIndexBuffer:patchIndexBuffer patchIndexBufferOffset:(NSUInteger)patchIndexBufferOffset controlPointIndexBuffer:(id <MTLBuffer>)controlPointIndexBuffer controlPointIndexBufferOffset:(NSUInteger)controlPointIndexBufferOffset indirectBuffer:(id <MTLBuffer>)indirectBuffer indirectBufferOffset:(NSUInteger)indirectBufferOffset];
}
#endif

-(NSString*) description
{
	return [Inner description];
}

-(NSString*) debugDescription
{
	return [Inner debugDescription];
}

- (bool)validateFunctionBindings:(EMetalShaderFrequency)Frequency
{
    check(Reflection);
    
    bool bOK = true;
    
    NSArray<MTLArgument*>* Arguments = nil;
    switch(Frequency)
    {
        case EMetalShaderVertex:
        {
            Arguments = Reflection.vertexArguments;
            break;
        }
        case EMetalShaderFragment:
        {
            Arguments = Reflection.fragmentArguments;
            break;
        }
        default:
            check(false);
            break;
    }
    
    for (uint32 i = 0; i < Arguments.count; i++)
    {
        MTLArgument* Arg = [Arguments objectAtIndex:i];
        check(Arg);
        switch(Arg.type)
        {
            case MTLArgumentTypeBuffer:
            {
                checkf(Arg.index < ML_MaxBuffers, TEXT("Metal buffer index exceeded!"));
                if ((ShaderBuffers[Frequency].Buffers[Arg.index] == nil && ShaderBuffers[Frequency].Bytes[Arg.index] == nil))
                {
                    bOK = false;
					UE_LOG(LogMetal, Warning, TEXT("Unbound buffer at Metal index %u which will crash the driver: %s"), (uint32)Arg.index, *FString([Arg description]));
                }
                break;
            }
            case MTLArgumentTypeThreadgroupMemory:
            {
                break;
            }
            case MTLArgumentTypeTexture:
            {
                checkf(Arg.index < ML_MaxTextures, TEXT("Metal texture index exceeded!"));
                if (ShaderTextures[Frequency].Textures[Arg.index] == nil)
                {
                    bOK = false;
                    UE_LOG(LogMetal, Warning, TEXT("Unbound texture at Metal index %u which will crash the driver: %s"), (uint32)Arg.index, *FString([Arg description]));
                }
                else if (ShaderTextures[Frequency].Textures[Arg.index].textureType != Arg.textureType)
                {
                    bOK = false;
                    UE_LOG(LogMetal, Warning, TEXT("Incorrect texture type bound at Metal index %u which will crash the driver: %s"), (uint32)Arg.index, *FString([Arg description]));
                }
                break;
            }
            case MTLArgumentTypeSampler:
            {
                checkf(Arg.index < ML_MaxSamplers, TEXT("Metal sampler index exceeded!"));
                if (ShaderSamplers[Frequency].Samplers[Arg.index] == nil)
                {
                    bOK = false;
                    UE_LOG(LogMetal, Warning, TEXT("Unbound sampler at Metal index %u which will crash the driver: %s"), (uint32)Arg.index, *FString([Arg description]));
                }
                break;
            }
            default:
                check(false);
                break;
        }
    }
    
    return bOK;
}

- (void)validate
{
    bool bOK = [self validateFunctionBindings:EMetalShaderVertex];
    if (!bOK)
    {
        UE_LOG(LogMetal, Error, TEXT("Metal Validation failures for vertex shader:\n%s"), VertexSource ? *FString(VertexSource) : TEXT("nil"));
    }
    
    bOK = [self validateFunctionBindings:EMetalShaderFragment];
    if (!bOK)
    {
        UE_LOG(LogMetal, Error, TEXT("Metal Validation failures for fragment shader:\n%s"), FragmentSource ? *FString(FragmentSource) : TEXT("nil"));
    }
}

@end

NS_ASSUME_NONNULL_END
