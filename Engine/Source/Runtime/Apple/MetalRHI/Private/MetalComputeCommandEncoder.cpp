// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MetalComputeCommandEncoder.cpp: Metal command encoder wrapper.
=============================================================================*/

#include "MetalRHIPrivate.h"

#include "MetalComputeCommandEncoder.h"
#include "MetalCommandBuffer.h"

NS_ASSUME_NONNULL_BEGIN

@implementation FMetalDebugComputeCommandEncoder

@synthesize Inner;
@synthesize Buffer;
@synthesize Reflection;
@synthesize Source;

-(id)initWithEncoder:(id<MTLComputeCommandEncoder>)Encoder andCommandBuffer:(FMetalDebugCommandBuffer*)SourceBuffer
{
	id Self = [super init];
	if (Self)
	{
        Inner = [Encoder retain];
		Buffer = [SourceBuffer retain];
        Reflection = nil;
        Source = nil;
	}
	return Self;
}

-(void)dealloc
{
	[Inner release];
	[Buffer release];
	[Reflection release];
	[Source release];
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

- (void)setComputePipelineState:(id <MTLComputePipelineState>)state
{
    //[Buffer pipeline:state.label];
    [Buffer trackState:state];
    [Inner setComputePipelineState:(id <MTLComputePipelineState>)state];
}

- (void)setBytes:(const void *)bytes length:(NSUInteger)length atIndex:(NSUInteger)index
{
	ShaderBuffers.Buffers[index] = nil;
	ShaderBuffers.Bytes[index] = bytes;
	ShaderBuffers.Offsets[index] = length;
	
    [Inner setBytes:bytes length:length atIndex:index];
}

- (void)setBuffer:(nullable id <MTLBuffer>)buffer offset:(NSUInteger)offset atIndex:(NSUInteger)index
{
	ShaderBuffers.Buffers[index] = buffer;
	ShaderBuffers.Bytes[index] = nil;
	ShaderBuffers.Offsets[index] = offset;
	
    [Buffer trackResource:buffer];
    [Inner setBuffer:buffer offset:offset atIndex:index];
}

- (void)setBufferOffset:(NSUInteger)offset atIndex:(NSUInteger)index
{
	ShaderBuffers.Offsets[index] = offset;
	
    [Inner setBufferOffset:offset atIndex:index];
}

- (void)setBuffers:(const id <MTLBuffer> __nullable [])buffers offsets:(const NSUInteger [])offsets withRange:(NSRange)range
{
	for (uint32 i = 0; i < range.length; i++)
	{
		ShaderBuffers.Buffers[i + range.location] = buffers[i];
		ShaderBuffers.Bytes[i + range.location] = nil;
		ShaderBuffers.Offsets[i + range.location] = offsets[i];
		
        [Buffer trackResource:buffers[i]];
    }
    [Inner setBuffers:buffers offsets:offsets withRange:range];
}

- (void)setTexture:(nullable id <MTLTexture>)texture atIndex:(NSUInteger)index
{
	ShaderTextures.Textures[index] = texture;
	
    [Buffer trackResource:texture];
    [Inner setTexture:texture atIndex:index];
}

- (void)setTextures:(const id <MTLTexture> __nullable [__nullable])textures withRange:(NSRange)range
{
    for (uint32 i = 0; i < range.length; i++)
	{
		ShaderTextures.Textures[i + range.location] = textures[i];
        [Buffer trackResource:textures[i]];
    }
    [Inner setTextures:textures withRange:range];
}

- (void)setSamplerState:(nullable id <MTLSamplerState>)sampler atIndex:(NSUInteger)index
{
	ShaderSamplers.Samplers[index] = sampler;
    [Buffer trackState:sampler];
    [Inner setSamplerState:sampler atIndex:index];
}

- (void)setSamplerStates:(const id <MTLSamplerState> __nullable [__nullable])samplers withRange:(NSRange)range
{
    for(uint32 i = 0; i < range.length; i++)
	{
		ShaderSamplers.Samplers[i + range.location] = samplers[i];
        [Buffer trackState:samplers[i]];
    }
    [Inner setSamplerStates:samplers withRange:range];
}

- (void)setSamplerState:(nullable id <MTLSamplerState>)sampler lodMinClamp:(float)lodMinClamp lodMaxClamp:(float)lodMaxClamp atIndex:(NSUInteger)index
{
	ShaderSamplers.Samplers[index] = sampler;
    [Buffer trackState:sampler];
    [Inner setSamplerState:sampler lodMinClamp:lodMinClamp lodMaxClamp:lodMaxClamp atIndex:index];
}

- (void)setSamplerStates:(const id <MTLSamplerState> __nullable [__nullable])samplers lodMinClamps:(const float [__nullable])lodMinClamps lodMaxClamps:(const float [__nullable])lodMaxClamps withRange:(NSRange)range
{
    for(uint32 i = 0; i < range.length; i++)
	{
		ShaderSamplers.Samplers[i + range.location] = samplers[i];
        [Buffer trackState:samplers[i]];
    }
    [Inner setSamplerStates:samplers lodMinClamps:lodMinClamps lodMaxClamps:lodMaxClamps withRange:range];
}

- (void)setThreadgroupMemoryLength:(NSUInteger)length atIndex:(NSUInteger)index
{
    [Inner setThreadgroupMemoryLength:(NSUInteger)length atIndex:(NSUInteger)index];
}

- (void)setStageInRegion:(MTLRegion)region
{
    [Inner setStageInRegion:(MTLRegion)region];
}

- (void)dispatchThreadgroups:(MTLSize)threadgroupsPerGrid threadsPerThreadgroup:(MTLSize)threadsPerThreadgroup
{
    [Buffer dispatch:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    [self validate];
    [Inner dispatchThreadgroups:(MTLSize)threadgroupsPerGrid threadsPerThreadgroup:(MTLSize)threadsPerThreadgroup];
}

- (void)dispatchThreadgroupsWithIndirectBuffer:(id <MTLBuffer>)indirectBuffer indirectBufferOffset:(NSUInteger)indirectBufferOffset threadsPerThreadgroup:(MTLSize)threadsPerThreadgroup
{
    [Buffer dispatch:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    [Buffer trackResource:indirectBuffer];
    
    [self validate];
    
    [Inner dispatchThreadgroupsWithIndirectBuffer:(id <MTLBuffer>)indirectBuffer indirectBufferOffset:(NSUInteger)indirectBufferOffset threadsPerThreadgroup:(MTLSize)threadsPerThreadgroup];
}

#if !PLATFORM_MAC
- (void)updateFence:(id <MTLFence>)fence
{
    [Inner updateFence:(id <MTLFence>)fence];
}

- (void)waitForFence:(id <MTLFence>)fence
{
    [Inner waitForFence:(id <MTLFence>)fence];
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

- (void)validate
{
    check(Reflection);
    
    bool bOK = true;
    
    NSArray<MTLArgument*>* Arguments = Reflection.arguments;
    for (uint32 i = 0; i < Arguments.count; i++)
    {
        MTLArgument* Arg = [Arguments objectAtIndex:i];
        check(Arg);
        switch(Arg.type)
        {
            case MTLArgumentTypeBuffer:
            {
                checkf(Arg.index < ML_MaxBuffers, TEXT("Metal buffer index exceeded!"));
                if ((ShaderBuffers.Buffers[Arg.index] == nil && ShaderBuffers.Bytes[Arg.index] == nil))
                {
                    UE_LOG(LogMetal, Warning, TEXT("Unbound buffer at Metal index %u which will crash the driver: %s"), (uint32)Arg.index, *FString([Arg description]));
                    bOK = false;
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
                if (ShaderTextures.Textures[Arg.index] == nil)
                {
                    UE_LOG(LogMetal, Warning, TEXT("Unbound texture at Metal index %u  which will crash the driver: %s"), (uint32)Arg.index, *FString([Arg description]));
                    bOK = false;
                }
                else if (ShaderTextures.Textures[Arg.index].textureType != Arg.textureType)
                {
                    UE_LOG(LogMetal, Warning, TEXT("Incorrect texture type bound at Metal index %u which will crash the driver: %s"), (uint32)Arg.index, *FString([Arg description]));
                    bOK = false;
                }
                break;
            }
            case MTLArgumentTypeSampler:
            {
                checkf(Arg.index < ML_MaxSamplers, TEXT("Metal sampler index exceeded!"));
                if (ShaderSamplers.Samplers[Arg.index] == nil)
                {
                    UE_LOG(LogMetal, Warning, TEXT("Unbound sampler at Metal index %u which will crash the driver: %s"), (uint32)Arg.index, *FString([Arg description]));
                    bOK = false;
                }
                break;
            }
            default:
                check(false);
                break;
        }
    }
    
    if (!bOK)
    {
        UE_LOG(LogMetal, Error, TEXT("Metal Validation failures for compute shader:\n%s"), Source ? *FString(Source) : TEXT("nil"));
    }
}

@end

NS_ASSUME_NONNULL_END
