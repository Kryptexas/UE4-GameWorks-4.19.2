// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MetalRenderCommandEncoder.cpp: Metal command encoder wrapper.
=============================================================================*/

#include "MetalRHIPrivate.h"

#include "MetalRenderCommandEncoder.h"
#include "MetalCommandBuffer.h"
#include "MetalFence.h"
#include "MetalRenderPipelineDesc.h"

NS_ASSUME_NONNULL_BEGIN

@implementation FMetalDebugRenderCommandEncoder

APPLE_PLATFORM_OBJECT_ALLOC_OVERRIDES(FMetalDebugRenderCommandEncoder)

@synthesize Inner;
@synthesize Buffer;
@synthesize Pipeline;

-(id)initWithEncoder:(id<MTLRenderCommandEncoder>)Encoder andCommandBuffer:(FMetalDebugCommandBuffer*)SourceBuffer
{
	id Self = [super init];
	if (Self)
	{
        Inner = [Encoder retain];
		Buffer = [SourceBuffer retain];
        Pipeline = nil;
	}
	return Self;
}

-(void)dealloc
{
	[Inner release];
	[Buffer release];
	[Pipeline release];
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
#if METAL_DEBUG_OPTIONS
	switch (Buffer->DebugLevel)
	{
		case EMetalDebugLevelLogOperations:
		{
			[Buffer setPipeline:pipelineState.label];
		}
		case EMetalDebugLevelTrackResources:
		{
			[Buffer trackState:pipelineState];
		}
		default:
		{
			break;
		}
	}
#endif
    [Inner setRenderPipelineState:pipelineState];
}

- (void)setVertexBytes:(const void *)bytes length:(NSUInteger)length atIndex:(NSUInteger)index
{
#if METAL_DEBUG_OPTIONS
	switch (Buffer->DebugLevel)
	{
		case EMetalDebugLevelValidation:
		{
			ShaderBuffers[EMetalShaderVertex].Buffers[index] = nil;
			ShaderBuffers[EMetalShaderVertex].Bytes[index] = bytes;
			ShaderBuffers[EMetalShaderVertex].Offsets[index] = length;
		}
		case EMetalDebugLevelFastValidation:
		{
			ResourceMask[EMetalShaderVertex].BufferMask = bytes ? (ResourceMask[EMetalShaderVertex].BufferMask | (1 << index)) : (ResourceMask[EMetalShaderVertex].BufferMask & ~(1 << index));
		}
		default:
		{
			break;
		}
	}
#endif
	
    [Inner setVertexBytes:bytes length:length atIndex:index];
}

- (void)setVertexBuffer:(nullable id <MTLBuffer>)buffer offset:(NSUInteger)offset atIndex:(NSUInteger)index
{
#if METAL_DEBUG_OPTIONS
	switch (Buffer->DebugLevel)
	{
		case EMetalDebugLevelValidation:
		{
			ShaderBuffers[EMetalShaderVertex].Buffers[index] = buffer;
			ShaderBuffers[EMetalShaderVertex].Bytes[index] = nil;
			ShaderBuffers[EMetalShaderVertex].Offsets[index] = offset;
		}
		case EMetalDebugLevelTrackResources:
		{
			[Buffer trackResource:buffer];
		}
		case EMetalDebugLevelFastValidation:
		{
			ResourceMask[EMetalShaderVertex].BufferMask = buffer ? (ResourceMask[EMetalShaderVertex].BufferMask | (1 << index)) : (ResourceMask[EMetalShaderVertex].BufferMask & ~(1 << index));
		}
		default:
		{
			break;
		}
	}
#endif
    [Inner setVertexBuffer:buffer offset:offset atIndex:index];
}

- (void)setVertexBufferOffset:(NSUInteger)offset atIndex:(NSUInteger)index
{
#if METAL_DEBUG_OPTIONS
	switch (Buffer->DebugLevel)
	{
		case EMetalDebugLevelValidation:
		{
			ShaderBuffers[EMetalShaderVertex].Offsets[index] = offset;
		}
		case EMetalDebugLevelFastValidation:
		{
			check(ResourceMask[EMetalShaderVertex].BufferMask & 1 << index);
		}
		default:
		{
			break;
		}
	}
#endif
	[Inner setVertexBufferOffset:offset atIndex:index];
}

- (void)setVertexBuffers:(const id <MTLBuffer> __nullable [])buffers offsets:(const NSUInteger [])offsets withRange:(NSRange)range
{
#if METAL_DEBUG_OPTIONS
	for (uint32 i = 0; i < range.length; i++)
	{
		switch (Buffer->DebugLevel)
		{
			case EMetalDebugLevelValidation:
			{
				ShaderBuffers[EMetalShaderVertex].Buffers[i + range.location] = buffers[i];
				ShaderBuffers[EMetalShaderVertex].Bytes[i + range.location] = nil;
				ShaderBuffers[EMetalShaderVertex].Offsets[i + range.location] = offsets[i];
			}
			case EMetalDebugLevelTrackResources:
			{
				[Buffer trackResource:buffers[i]];
			}
			case EMetalDebugLevelFastValidation:
			{
				ResourceMask[EMetalShaderVertex].BufferMask = buffers[i] ? (ResourceMask[EMetalShaderVertex].BufferMask | (1 << (i + range.location))) : (ResourceMask[EMetalShaderVertex].BufferMask & ~(1 << (i + range.location)));
			}
			default:
			{
				break;
			}
		}
    }
#endif
    [Inner setVertexBuffers:buffers offsets:offsets withRange:range];
}

- (void)setVertexTexture:(nullable id <MTLTexture>)texture atIndex:(NSUInteger)index
{
#if METAL_DEBUG_OPTIONS
	switch (Buffer->DebugLevel)
	{
		case EMetalDebugLevelValidation:
		{
			ShaderTextures[EMetalShaderVertex].Textures[index] = texture;
		}
		case EMetalDebugLevelTrackResources:
		{
			[Buffer trackResource:texture];
		}
		case EMetalDebugLevelFastValidation:
		{
			ResourceMask[EMetalShaderVertex].TextureMask = texture ? (ResourceMask[EMetalShaderVertex].TextureMask | (1 << (index))) : (ResourceMask[EMetalShaderVertex].TextureMask & ~(1 << (index)));
		}
		default:
		{
			break;
		}
	}
#endif
    [Inner setVertexTexture:texture atIndex:index];
}

- (void)setVertexTextures:(const id <MTLTexture> __nullable [__nullable])textures withRange:(NSRange)range
{
#if METAL_DEBUG_OPTIONS
    for (uint32 i = 0; i < range.length; i++)
    {
		switch (Buffer->DebugLevel)
		{
			case EMetalDebugLevelValidation:
			{
				ShaderTextures[EMetalShaderVertex].Textures[i + range.location] = textures[i];
			}
			case EMetalDebugLevelTrackResources:
			{
				[Buffer trackResource:textures[i]];
			}
			case EMetalDebugLevelFastValidation:
			{
				ResourceMask[EMetalShaderVertex].TextureMask = textures[i] ? (ResourceMask[EMetalShaderVertex].TextureMask | (1 << (i + range.location))) : (ResourceMask[EMetalShaderVertex].TextureMask & ~(1 << (i + range.location)));
			}
			default:
			{
				break;
			}
		}
    }
#endif
    [Inner setVertexTextures:textures withRange:range];
}

- (void)setVertexSamplerState:(nullable id <MTLSamplerState>)sampler atIndex:(NSUInteger)index
{
#if METAL_DEBUG_OPTIONS
	switch (Buffer->DebugLevel)
	{
		case EMetalDebugLevelValidation:
		{
			ShaderSamplers[EMetalShaderVertex].Samplers[index] = sampler;
		}
		case EMetalDebugLevelTrackResources:
		{
			[Buffer trackState:sampler];
		}
		case EMetalDebugLevelFastValidation:
		{
			ResourceMask[EMetalShaderVertex].SamplerMask = sampler ? (ResourceMask[EMetalShaderVertex].SamplerMask | (1 << (index))) : (ResourceMask[EMetalShaderVertex].SamplerMask & ~(1 << (index)));
		}
		default:
		{
			break;
		}
	}
#endif
    [Inner setVertexSamplerState:sampler atIndex:index];
}

- (void)setVertexSamplerStates:(const id <MTLSamplerState> __nullable [__nullable])samplers withRange:(NSRange)range
{
#if METAL_DEBUG_OPTIONS
    for(uint32 i = 0; i < range.length; i++)
    {
		switch (Buffer->DebugLevel)
		{
			case EMetalDebugLevelValidation:
			{
				ShaderSamplers[EMetalShaderVertex].Samplers[i + range.location] = samplers[i];
			}
			case EMetalDebugLevelTrackResources:
			{
				[Buffer trackState:samplers[i]];
			}
			case EMetalDebugLevelFastValidation:
			{
				ResourceMask[EMetalShaderVertex].SamplerMask = samplers[i] ? (ResourceMask[EMetalShaderVertex].SamplerMask | (1 << (i + range.location))) : (ResourceMask[EMetalShaderVertex].SamplerMask & ~(1 << (i + range.location)));
			}
			default:
			{
				break;
			}
		}
		
    }
#endif
    [Inner setVertexSamplerStates:samplers withRange:range];
}

- (void)setVertexSamplerState:(nullable id <MTLSamplerState>)sampler lodMinClamp:(float)lodMinClamp lodMaxClamp:(float)lodMaxClamp atIndex:(NSUInteger)index
{
#if METAL_DEBUG_OPTIONS
	switch (Buffer->DebugLevel)
	{
		case EMetalDebugLevelValidation:
		{
			ShaderSamplers[EMetalShaderVertex].Samplers[index] = sampler;
		}
		case EMetalDebugLevelTrackResources:
		{
			[Buffer trackState:sampler];
		}
		case EMetalDebugLevelFastValidation:
		{
			ResourceMask[EMetalShaderVertex].SamplerMask = sampler ? (ResourceMask[EMetalShaderVertex].SamplerMask | (1 << (index))) : (ResourceMask[EMetalShaderVertex].SamplerMask & ~(1 << (index)));
		}
		default:
		{
			break;
		}
	}
#endif
    [Inner setVertexSamplerState:sampler lodMinClamp:lodMinClamp lodMaxClamp:lodMaxClamp atIndex:index];
}

- (void)setVertexSamplerStates:(const id <MTLSamplerState> __nullable [__nullable])samplers lodMinClamps:(const float [__nullable])lodMinClamps lodMaxClamps:(const float [__nullable])lodMaxClamps withRange:(NSRange)range
{
#if METAL_DEBUG_OPTIONS
    for(uint32 i = 0; i < range.length; i++)
    {
		switch (Buffer->DebugLevel)
		{
			case EMetalDebugLevelValidation:
			{
				ShaderSamplers[EMetalShaderVertex].Samplers[i + range.location] = samplers[i];
			}
			case EMetalDebugLevelTrackResources:
			{
				[Buffer trackState:samplers[i]];
			}
			case EMetalDebugLevelFastValidation:
			{
				ResourceMask[EMetalShaderVertex].SamplerMask = samplers[i] ? (ResourceMask[EMetalShaderVertex].SamplerMask | (1 << (i + range.location))) : (ResourceMask[EMetalShaderVertex].SamplerMask & ~(1 << (i + range.location)));
			}
			default:
			{
				break;
			}
		}
		
    }
#endif
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
#if METAL_DEBUG_OPTIONS
	switch (Buffer->DebugLevel)
	{
		case EMetalDebugLevelValidation:
		{
			ShaderBuffers[EMetalShaderFragment].Buffers[index] = nil;
			ShaderBuffers[EMetalShaderFragment].Bytes[index] = bytes;
			ShaderBuffers[EMetalShaderFragment].Offsets[index] = length;
		}
		case EMetalDebugLevelFastValidation:
		{
			ResourceMask[EMetalShaderFragment].BufferMask = bytes ? (ResourceMask[EMetalShaderFragment].BufferMask | (1 << (index))) : (ResourceMask[EMetalShaderFragment].BufferMask & ~(1 << (index)));
		}
		default:
		{
			break;
		}
	}
#endif
    [Inner setFragmentBytes:(const void *)bytes length:(NSUInteger)length atIndex:(NSUInteger)index];
}

- (void)setFragmentBuffer:(nullable id <MTLBuffer>)buffer offset:(NSUInteger)offset atIndex:(NSUInteger)index
{
#if METAL_DEBUG_OPTIONS
	switch (Buffer->DebugLevel)
	{
		case EMetalDebugLevelValidation:
		{
			ShaderBuffers[EMetalShaderFragment].Buffers[index] = buffer;
			ShaderBuffers[EMetalShaderFragment].Bytes[index] = nil;
			ShaderBuffers[EMetalShaderFragment].Offsets[index] = offset;
		}
		case EMetalDebugLevelTrackResources:
		{
			[Buffer trackResource:buffer];
		}
		case EMetalDebugLevelFastValidation:
		{
			ResourceMask[EMetalShaderFragment].BufferMask = buffer ? (ResourceMask[EMetalShaderFragment].BufferMask | (1 << (index))) : (ResourceMask[EMetalShaderFragment].BufferMask & ~(1 << (index)));
		}
		default:
		{
			break;
		}
	}
#endif
    [Inner setFragmentBuffer:buffer offset:offset atIndex:index];
}

- (void)setFragmentBufferOffset:(NSUInteger)offset atIndex:(NSUInteger)index
{
#if METAL_DEBUG_OPTIONS
	switch (Buffer->DebugLevel)
	{
		case EMetalDebugLevelValidation:
		{
			ShaderBuffers[EMetalShaderFragment].Offsets[index] = offset;
		}
		case EMetalDebugLevelFastValidation:
		{
			check(ResourceMask[EMetalShaderFragment].BufferMask & (1 << (index)));
		}
		default:
		{
			break;
		}
	}
#endif
    [Inner setFragmentBufferOffset:(NSUInteger)offset atIndex:(NSUInteger)index];
}

- (void)setFragmentBuffers:(const id <MTLBuffer> __nullable [])buffers offsets:(const NSUInteger [])offset withRange:(NSRange)range
{
#if METAL_DEBUG_OPTIONS
    for (uint32 i = 0; i < range.length; i++)
    {
		switch (Buffer->DebugLevel)
		{
			case EMetalDebugLevelValidation:
			{
				ShaderBuffers[EMetalShaderFragment].Buffers[i + range.location] = buffers[i];
				ShaderBuffers[EMetalShaderFragment].Bytes[i + range.location] = nil;
				ShaderBuffers[EMetalShaderFragment].Offsets[i + range.location] = offset[i];
			}
			case EMetalDebugLevelTrackResources:
			{
				[Buffer trackResource:buffers[i]];
			}
			case EMetalDebugLevelFastValidation:
			{
				ResourceMask[EMetalShaderFragment].BufferMask = buffers[i] ? (ResourceMask[EMetalShaderFragment].BufferMask | (1 << (i + range.location))) : (ResourceMask[EMetalShaderFragment].BufferMask & ~(1 << (i + range.location)));
			}
			default:
			{
				break;
			}
		}
    }
#endif
    [Inner setFragmentBuffers:buffers offsets:offset withRange:range];
}

- (void)setFragmentTexture:(nullable id <MTLTexture>)texture atIndex:(NSUInteger)index
{
#if METAL_DEBUG_OPTIONS
	switch (Buffer->DebugLevel)
	{
		case EMetalDebugLevelValidation:
		{
			ShaderTextures[EMetalShaderFragment].Textures[index] = texture;
		}
		case EMetalDebugLevelTrackResources:
		{
			[Buffer trackResource:texture];
		}
		case EMetalDebugLevelFastValidation:
		{
			ResourceMask[EMetalShaderFragment].TextureMask = texture ? (ResourceMask[EMetalShaderFragment].TextureMask | (1 << (index))) : (ResourceMask[EMetalShaderFragment].TextureMask & ~(1 << (index)));
		}
		default:
		{
			break;
		}
	}
#endif
    [Inner setFragmentTexture:texture atIndex:index];
}

- (void)setFragmentTextures:(const id <MTLTexture> __nullable [__nullable])textures withRange:(NSRange)range
{
#if METAL_DEBUG_OPTIONS
    for (uint32 i = 0; i < range.length; i++)
    {
		switch (Buffer->DebugLevel)
		{
			case EMetalDebugLevelValidation:
			{
				ShaderTextures[EMetalShaderFragment].Textures[i + range.location] = textures[i];
			}
			case EMetalDebugLevelTrackResources:
			{
				[Buffer trackResource:textures[i]];
			}
			case EMetalDebugLevelFastValidation:
			{
				ResourceMask[EMetalShaderFragment].TextureMask = textures[i] ? (ResourceMask[EMetalShaderFragment].TextureMask | (1 << (i + range.location))) : (ResourceMask[EMetalShaderFragment].TextureMask & ~(1 << (i + range.location)));
			}
			default:
			{
				break;
			}
		}
		
    }
#endif
    [Inner setFragmentTextures:textures withRange:(NSRange)range];
}

- (void)setFragmentSamplerState:(nullable id <MTLSamplerState>)sampler atIndex:(NSUInteger)index
{
#if METAL_DEBUG_OPTIONS
	switch (Buffer->DebugLevel)
	{
		case EMetalDebugLevelValidation:
		{
			ShaderSamplers[EMetalShaderFragment].Samplers[index] = sampler;
		}
		case EMetalDebugLevelTrackResources:
		{
			[Buffer trackState:sampler];
		}
		case EMetalDebugLevelFastValidation:
		{
			ResourceMask[EMetalShaderFragment].SamplerMask = sampler ? (ResourceMask[EMetalShaderFragment].SamplerMask | (1 << (index))) : (ResourceMask[EMetalShaderFragment].SamplerMask & ~(1 << (index)));
		}
		default:
		{
			break;
		}
	}
#endif
    [Inner setFragmentSamplerState:sampler atIndex:(NSUInteger)index];
}

- (void)setFragmentSamplerStates:(const id <MTLSamplerState> __nullable [__nullable])samplers withRange:(NSRange)range
{
#if METAL_DEBUG_OPTIONS
	for(uint32 i = 0; i < range.length; i++)
    {
		switch (Buffer->DebugLevel)
		{
			case EMetalDebugLevelValidation:
			{
				ShaderSamplers[EMetalShaderFragment].Samplers[i + range.location] = samplers[i];
			}
			case EMetalDebugLevelTrackResources:
			{
				[Buffer trackState:samplers[i]];
			}
			case EMetalDebugLevelFastValidation:
			{
				ResourceMask[EMetalShaderFragment].SamplerMask = samplers[i] ? (ResourceMask[EMetalShaderFragment].SamplerMask | (1 << (i + range.location))) : (ResourceMask[EMetalShaderFragment].SamplerMask & ~(1 << (i + range.location)));
			}
			default:
			{
				break;
			}
		}
		
    }
#endif
    [Inner setFragmentSamplerStates:samplers withRange:(NSRange)range];
}

- (void)setFragmentSamplerState:(nullable id <MTLSamplerState>)sampler lodMinClamp:(float)lodMinClamp lodMaxClamp:(float)lodMaxClamp atIndex:(NSUInteger)index
{
#if METAL_DEBUG_OPTIONS
	switch (Buffer->DebugLevel)
	{
		case EMetalDebugLevelValidation:
		{
			ShaderSamplers[EMetalShaderFragment].Samplers[index] = sampler;
		}
		case EMetalDebugLevelTrackResources:
		{
			[Buffer trackState:sampler];
		}
		case EMetalDebugLevelFastValidation:
		{
			ResourceMask[EMetalShaderFragment].SamplerMask = sampler ? (ResourceMask[EMetalShaderFragment].SamplerMask | (1 << (index))) : (ResourceMask[EMetalShaderFragment].SamplerMask & ~(1 << (index)));
		}
		default:
		{
			break;
		}
	}
#endif
    [Inner setFragmentSamplerState:sampler lodMinClamp:(float)lodMinClamp lodMaxClamp:(float)lodMaxClamp atIndex:(NSUInteger)index];
}

- (void)setFragmentSamplerStates:(const id <MTLSamplerState> __nullable [__nullable])samplers lodMinClamps:(const float [__nullable])lodMinClamps lodMaxClamps:(const float [__nullable])lodMaxClamps withRange:(NSRange)range
{
#if METAL_DEBUG_OPTIONS
    for(uint32 i = 0; i < range.length; i++)
    {
		switch (Buffer->DebugLevel)
		{
			case EMetalDebugLevelValidation:
			{
				ShaderSamplers[EMetalShaderFragment].Samplers[i + range.location] = samplers[i];
			}
			case EMetalDebugLevelTrackResources:
			{
				[Buffer trackState:samplers[i]];
			}
			case EMetalDebugLevelFastValidation:
			{
				ResourceMask[EMetalShaderFragment].SamplerMask = samplers[i] ? (ResourceMask[EMetalShaderFragment].SamplerMask | (1 << (i + range.location))) : (ResourceMask[EMetalShaderFragment].SamplerMask & ~(1 << (i + range.location)));
			}
			default:
			{
				break;
			}
		}
    }
#endif
    [Inner setFragmentSamplerStates:samplers lodMinClamps:lodMinClamps lodMaxClamps:lodMaxClamps withRange:(NSRange)range];
}

- (void)setBlendColorRed:(float)red green:(float)green blue:(float)blue alpha:(float)alpha
{
    [Inner setBlendColorRed:red green:green blue:blue alpha:alpha];
}

- (void)setDepthStencilState:(nullable id <MTLDepthStencilState>)depthStencilState
{
#if METAL_DEBUG_OPTIONS
	if (Buffer->DebugLevel >= EMetalDebugLevelTrackResources)
	{
		[Buffer trackState:depthStencilState];
	}
#endif
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
#if METAL_DEBUG_OPTIONS
	switch(Buffer->DebugLevel)
	{
		case EMetalDebugLevelLogOperations:
		{
			[Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
		}
		case EMetalDebugLevelFastValidation:
		{
			[self validate];
		}
		default:
		{
			break;
		}
	}
#endif
    [Inner drawPrimitives:primitiveType vertexStart:vertexStart vertexCount:vertexCount instanceCount:instanceCount];
}

- (void)drawPrimitives:(MTLPrimitiveType)primitiveType vertexStart:(NSUInteger)vertexStart vertexCount:(NSUInteger)vertexCount
{
#if METAL_DEBUG_OPTIONS
	switch(Buffer->DebugLevel)
	{
		case EMetalDebugLevelLogOperations:
		{
			[Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
		}
		case EMetalDebugLevelFastValidation:
		{
			[self validate];
		}
		default:
		{
			break;
		}
	}
#endif
    [Inner drawPrimitives:primitiveType vertexStart:vertexStart vertexCount:vertexCount];
}

- (void)drawIndexedPrimitives:(MTLPrimitiveType)primitiveType indexCount:(NSUInteger)indexCount indexType:(MTLIndexType)indexType indexBuffer:(id <MTLBuffer>)indexBuffer indexBufferOffset:(NSUInteger)indexBufferOffset instanceCount:(NSUInteger)instanceCount
{
#if METAL_DEBUG_OPTIONS
	switch(Buffer->DebugLevel)
	{
		case EMetalDebugLevelLogOperations:
		{
			[Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
		}
		case EMetalDebugLevelTrackResources:
		{
			[Buffer trackResource:indexBuffer];
		}
		case EMetalDebugLevelFastValidation:
		{
			[self validate];
		}
		default:
		{
			break;
		}
	}
#endif
    [Inner drawIndexedPrimitives:(MTLPrimitiveType)primitiveType indexCount:(NSUInteger)indexCount indexType:(MTLIndexType)indexType indexBuffer:(id <MTLBuffer>)indexBuffer indexBufferOffset:(NSUInteger)indexBufferOffset instanceCount:(NSUInteger)instanceCount];
}

- (void)drawIndexedPrimitives:(MTLPrimitiveType)primitiveType indexCount:(NSUInteger)indexCount indexType:(MTLIndexType)indexType indexBuffer:(id <MTLBuffer>)indexBuffer indexBufferOffset:(NSUInteger)indexBufferOffset
{
#if METAL_DEBUG_OPTIONS
	switch(Buffer->DebugLevel)
	{
		case EMetalDebugLevelLogOperations:
		{
			[Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
		}
		case EMetalDebugLevelTrackResources:
		{
			[Buffer trackResource:indexBuffer];
		}
		case EMetalDebugLevelFastValidation:
		{
			[self validate];
		}
		default:
		{
			break;
		}
	}
#endif
    [Inner drawIndexedPrimitives:(MTLPrimitiveType)primitiveType indexCount:(NSUInteger)indexCount indexType:(MTLIndexType)indexType indexBuffer:(id <MTLBuffer>)indexBuffer indexBufferOffset:(NSUInteger)indexBufferOffset];
}

- (void)drawPrimitives:(MTLPrimitiveType)primitiveType vertexStart:(NSUInteger)vertexStart vertexCount:(NSUInteger)vertexCount instanceCount:(NSUInteger)instanceCount baseInstance:(NSUInteger)baseInstance
{
#if METAL_DEBUG_OPTIONS
	switch(Buffer->DebugLevel)
	{
		case EMetalDebugLevelLogOperations:
		{
			[Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
		}
		case EMetalDebugLevelFastValidation:
		{
			[self validate];
		}
		default:
		{
			break;
		}
	}
#endif
    [Inner drawPrimitives:(MTLPrimitiveType)primitiveType vertexStart:(NSUInteger)vertexStart vertexCount:(NSUInteger)vertexCount instanceCount:(NSUInteger)instanceCount baseInstance:(NSUInteger)baseInstance];
}

- (void)drawIndexedPrimitives:(MTLPrimitiveType)primitiveType indexCount:(NSUInteger)indexCount indexType:(MTLIndexType)indexType indexBuffer:(id <MTLBuffer>)indexBuffer indexBufferOffset:(NSUInteger)indexBufferOffset instanceCount:(NSUInteger)instanceCount baseVertex:(NSInteger)baseVertex baseInstance:(NSUInteger)baseInstance
{
#if METAL_DEBUG_OPTIONS
	switch(Buffer->DebugLevel)
	{
		case EMetalDebugLevelLogOperations:
		{
			[Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
		}
		case EMetalDebugLevelTrackResources:
		{
			[Buffer trackResource:indexBuffer];
		}
		case EMetalDebugLevelFastValidation:
		{
			[self validate];
		}
		default:
		{
			break;
		}
	}
#endif
    [Inner drawIndexedPrimitives:(MTLPrimitiveType)primitiveType indexCount:(NSUInteger)indexCount indexType:(MTLIndexType)indexType indexBuffer:(id <MTLBuffer>)indexBuffer indexBufferOffset:(NSUInteger)indexBufferOffset instanceCount:(NSUInteger)instanceCount baseVertex:(NSInteger)baseVertex baseInstance:(NSUInteger)baseInstance];
}

- (void)drawPrimitives:(MTLPrimitiveType)primitiveType indirectBuffer:(id <MTLBuffer>)indirectBuffer indirectBufferOffset:(NSUInteger)indirectBufferOffset
{
#if METAL_DEBUG_OPTIONS
	switch(Buffer->DebugLevel)
	{
		case EMetalDebugLevelLogOperations:
		{
			[Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
		}
		case EMetalDebugLevelTrackResources:
		{
			[Buffer trackResource:indirectBuffer];
		}
		case EMetalDebugLevelFastValidation:
		{
			[self validate];
		}
		default:
		{
			break;
		}
	}
#endif
    [Inner drawPrimitives:(MTLPrimitiveType)primitiveType indirectBuffer:(id <MTLBuffer>)indirectBuffer indirectBufferOffset:(NSUInteger)indirectBufferOffset];
}

- (void)drawIndexedPrimitives:(MTLPrimitiveType)primitiveType indexType:(MTLIndexType)indexType indexBuffer:(id <MTLBuffer>)indexBuffer indexBufferOffset:(NSUInteger)indexBufferOffset indirectBuffer:(id <MTLBuffer>)indirectBuffer indirectBufferOffset:(NSUInteger)indirectBufferOffset
{
#if METAL_DEBUG_OPTIONS
	switch(Buffer->DebugLevel)
	{
		case EMetalDebugLevelLogOperations:
		{
			[Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
		}
		case EMetalDebugLevelTrackResources:
		{
			[Buffer trackResource:indexBuffer];
			[Buffer trackResource:indirectBuffer];
		}
		case EMetalDebugLevelFastValidation:
		{
			[self validate];
		}
		default:
		{
			break;
		}
	}
#endif
    [Inner drawIndexedPrimitives:(MTLPrimitiveType)primitiveType indexType:(MTLIndexType)indexType indexBuffer:(id <MTLBuffer>)indexBuffer indexBufferOffset:(NSUInteger)indexBufferOffset indirectBuffer:(id <MTLBuffer>)indirectBuffer indirectBufferOffset:(NSUInteger)indirectBufferOffset];
}

#if PLATFORM_MAC
- (void)textureBarrier
{
	[Inner textureBarrier];
}

#endif

#if METAL_SUPPORTS_HEAPS
- (void)updateFence:(id <MTLFence>)fence afterStages:(MTLRenderStages)stages
{
#if METAL_DEBUG_OPTIONS
	if (fence && Buffer->DebugLevel >= EMetalDebugLevelValidation)
	{
		[self addUpdateFence:fence];
		[Inner updateFence:((FMetalDebugFence*)fence).Inner afterStages:(MTLRenderStages)stages];
	}
	else
#endif
	{
		[Inner updateFence:(id <MTLFence>)fence afterStages:(MTLRenderStages)stages];
	}
}

- (void)waitForFence:(id <MTLFence>)fence beforeStages:(MTLRenderStages)stages
{
#if METAL_DEBUG_OPTIONS
	if (fence && Buffer->DebugLevel >= EMetalDebugLevelValidation)
	{
		[self addWaitFence:fence];
		[Inner waitForFence:((FMetalDebugFence*)fence).Inner beforeStages:(MTLRenderStages)stages];
	}
	else
#endif
	{
		[Inner waitForFence:(id <MTLFence>)fence beforeStages:(MTLRenderStages)stages];
	}
}
#endif

-(void)setTessellationFactorBuffer:(nullable id <MTLBuffer>)buffer offset:(NSUInteger)offset instanceStride:(NSUInteger)instanceStride
{
#if METAL_DEBUG_OPTIONS
	if (Buffer->DebugLevel >= EMetalDebugLevelTrackResources)
	{
		[Buffer trackResource:buffer];
	}
#endif
    [Inner setTessellationFactorBuffer:buffer offset:(NSUInteger)offset instanceStride:(NSUInteger)instanceStride];
}

-(void)setTessellationFactorScale:(float)scale
{
    [Inner setTessellationFactorScale:(float)scale];
}

-(void)drawPatches:(NSUInteger)numberOfPatchControlPoints patchStart:(NSUInteger)patchStart patchCount:(NSUInteger)patchCount patchIndexBuffer:(nullable id <MTLBuffer>)patchIndexBuffer patchIndexBufferOffset:(NSUInteger)patchIndexBufferOffset instanceCount:(NSUInteger)instanceCount baseInstance:(NSUInteger)baseInstance
{
#if METAL_DEBUG_OPTIONS
	switch(Buffer->DebugLevel)
	{
		case EMetalDebugLevelLogOperations:
		{
			[Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
		}
		case EMetalDebugLevelTrackResources:
		{
			[Buffer trackResource:patchIndexBuffer];
		}
		case EMetalDebugLevelFastValidation:
		{
			[self validate];
		}
		default:
		{
			break;
		}
	}
#endif
    [Inner drawPatches:(NSUInteger)numberOfPatchControlPoints patchStart:(NSUInteger)patchStart patchCount:(NSUInteger)patchCount patchIndexBuffer:patchIndexBuffer patchIndexBufferOffset:(NSUInteger)patchIndexBufferOffset instanceCount:(NSUInteger)instanceCount baseInstance:(NSUInteger)baseInstance];
}

#if PLATFORM_MAC
-(void)drawPatches:(NSUInteger)numberOfPatchControlPoints patchIndexBuffer:(nullable id <MTLBuffer>)patchIndexBuffer patchIndexBufferOffset:(NSUInteger)patchIndexBufferOffset indirectBuffer:(id <MTLBuffer>)indirectBuffer indirectBufferOffset:(NSUInteger)indirectBufferOffset
{
#if METAL_DEBUG_OPTIONS
	switch(Buffer->DebugLevel)
	{
		case EMetalDebugLevelLogOperations:
		{
			[Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
		}
		case EMetalDebugLevelTrackResources:
		{
			[Buffer trackResource:patchIndexBuffer];
			[Buffer trackResource:indirectBuffer];
		}
		case EMetalDebugLevelFastValidation:
		{
			[self validate];
		}
		default:
		{
			break;
		}
	}
#endif
    [Inner drawPatches:(NSUInteger)numberOfPatchControlPoints patchIndexBuffer:patchIndexBuffer patchIndexBufferOffset:(NSUInteger)patchIndexBufferOffset indirectBuffer:(id <MTLBuffer>)indirectBuffer indirectBufferOffset:(NSUInteger)indirectBufferOffset];
}
#endif

-(void)drawIndexedPatches:(NSUInteger)numberOfPatchControlPoints patchStart:(NSUInteger)patchStart patchCount:(NSUInteger)patchCount patchIndexBuffer:(nullable id <MTLBuffer>)patchIndexBuffer patchIndexBufferOffset:(NSUInteger)patchIndexBufferOffset controlPointIndexBuffer:(id <MTLBuffer>)controlPointIndexBuffer controlPointIndexBufferOffset:(NSUInteger)controlPointIndexBufferOffset instanceCount:(NSUInteger)instanceCount baseInstance:(NSUInteger)baseInstance
{
#if METAL_DEBUG_OPTIONS
	switch(Buffer->DebugLevel)
	{
		case EMetalDebugLevelLogOperations:
		{
			[Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
		}
		case EMetalDebugLevelTrackResources:
		{
			[Buffer trackResource:patchIndexBuffer];
			[Buffer trackResource:controlPointIndexBuffer];
		}
		case EMetalDebugLevelFastValidation:
		{
			[self validate];
		}
		default:
		{
			break;
		}
	}
#endif
    [Inner drawIndexedPatches:(NSUInteger)numberOfPatchControlPoints patchStart:(NSUInteger)patchStart patchCount:(NSUInteger)patchCount patchIndexBuffer:patchIndexBuffer patchIndexBufferOffset:(NSUInteger)patchIndexBufferOffset controlPointIndexBuffer:(id <MTLBuffer>)controlPointIndexBuffer controlPointIndexBufferOffset:(NSUInteger)controlPointIndexBufferOffset instanceCount:(NSUInteger)instanceCount baseInstance:(NSUInteger)baseInstance];
}

#if PLATFORM_MAC
-(void)drawIndexedPatches:(NSUInteger)numberOfPatchControlPoints patchIndexBuffer:(nullable id <MTLBuffer>)patchIndexBuffer patchIndexBufferOffset:(NSUInteger)patchIndexBufferOffset controlPointIndexBuffer:(id <MTLBuffer>)controlPointIndexBuffer controlPointIndexBufferOffset:(NSUInteger)controlPointIndexBufferOffset indirectBuffer:(id <MTLBuffer>)indirectBuffer indirectBufferOffset:(NSUInteger)indirectBufferOffset
{
#if METAL_DEBUG_OPTIONS
	switch(Buffer->DebugLevel)
	{
		case EMetalDebugLevelLogOperations:
		{
			[Buffer draw:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
		}
		case EMetalDebugLevelTrackResources:
		{
			[Buffer trackResource:patchIndexBuffer];
			[Buffer trackResource:controlPointIndexBuffer];
			[Buffer trackResource:indirectBuffer];
		}
		case EMetalDebugLevelFastValidation:
		{
			[self validate];
		}
		default:
		{
			break;
		}
	}
#endif
	
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
	bool bOK = true;
#if METAL_DEBUG_OPTIONS
	switch (Buffer->DebugLevel)
	{
		case EMetalDebugLevelValidation:
		{
			check(Pipeline);

			MTLRenderPipelineReflection* Reflection = Pipeline.RenderPipelineReflection;
			check(Reflection);
			
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
							UE_LOG(LogMetal, Warning, TEXT("Incorrect texture type bound at Metal index %u which will crash the driver: %s\n%s"), (uint32)Arg.index, *FString([Arg description]), *FString([ShaderTextures[Frequency].Textures[Arg.index] description]));
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
			break;
		}
		case EMetalDebugLevelFastValidation:
		{
			check(Pipeline);
			
			FMetalTextureMask TextureMask = (ResourceMask[Frequency].TextureMask & Pipeline->ResourceMask[Frequency].TextureMask);
			if (TextureMask != Pipeline->ResourceMask[Frequency].TextureMask)
			{
				bOK = false;
				for (uint32 i = 0; i < ML_MaxTextures; i++)
				{
					if ((Pipeline->ResourceMask[Frequency].TextureMask & (1 < i)) && ((TextureMask & (1 < i)) != (Pipeline->ResourceMask[Frequency].TextureMask & (1 < i))))
					{
						UE_LOG(LogMetal, Warning, TEXT("Unbound texture at Metal index %u which will crash the driver"), i);
					}
				}
			}
			
			FMetalBufferMask BufferMask = (ResourceMask[Frequency].BufferMask & Pipeline->ResourceMask[Frequency].BufferMask);
			if (BufferMask != Pipeline->ResourceMask[Frequency].BufferMask)
			{
				bOK = false;
				for (uint32 i = 0; i < ML_MaxBuffers; i++)
				{
					if ((Pipeline->ResourceMask[Frequency].BufferMask & (1 < i)) && ((BufferMask & (1 < i)) != (Pipeline->ResourceMask[Frequency].BufferMask & (1 < i))))
					{
						UE_LOG(LogMetal, Warning, TEXT("Unbound buffer at Metal index %u which will crash the driver"), i);
					}
				}
			}
			
			FMetalSamplerMask SamplerMask = (ResourceMask[Frequency].SamplerMask & Pipeline->ResourceMask[Frequency].SamplerMask);
			if (SamplerMask != Pipeline->ResourceMask[Frequency].SamplerMask)
			{
				bOK = false;
				for (uint32 i = 0; i < ML_MaxSamplers; i++)
				{
					if ((Pipeline->ResourceMask[Frequency].SamplerMask & (1 < i)) && ((SamplerMask & (1 < i)) != (Pipeline->ResourceMask[Frequency].SamplerMask & (1 < i))))
					{
						UE_LOG(LogMetal, Warning, TEXT("Unbound sampler at Metal index %u which will crash the driver"), i);
					}
				}
			}
			
			break;
		}
		default:
		{
			break;
		}
	}
#endif
    return bOK;
}

- (void)validate
{
#if METAL_DEBUG_OPTIONS
    bool bOK = [self validateFunctionBindings:EMetalShaderVertex];
    if (!bOK)
    {
        UE_LOG(LogMetal, Error, TEXT("Metal Validation failures for vertex shader:\n%s"), Pipeline && Pipeline.VertexSource ? *FString(Pipeline.VertexSource) : TEXT("nil"));
    }
	
    bOK = [self validateFunctionBindings:EMetalShaderFragment];
    if (!bOK)
    {
        UE_LOG(LogMetal, Error, TEXT("Metal Validation failures for fragment shader:\n%s"), Pipeline && Pipeline.FragmentSource ? *FString(Pipeline.FragmentSource) : TEXT("nil"));
    }
#endif
}

-(id<MTLCommandEncoder>)commandEncoder
{
	return self;
}

@end

#if !METAL_SUPPORTS_HEAPS
@implementation FMetalDebugRenderCommandEncoder (MTLRenderCommandEncoderExtensions)
-(void) updateFence:(id <MTLFence>)fence afterStages:(MTLRenderStages)stages
{
#if METAL_DEBUG_OPTIONS
	[self addUpdateFence:fence];
#endif
}

-(void) waitForFence:(id <MTLFence>)fence beforeStages:(MTLRenderStages)stages
{
#if METAL_DEBUG_OPTIONS
	[self addWaitFence:fence];
#endif
}
@end
#endif

NS_ASSUME_NONNULL_END
