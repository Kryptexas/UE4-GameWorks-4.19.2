// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MetalBlitCommandEncoder.cpp: Metal command encoder wrapper.
=============================================================================*/

#include "MetalRHIPrivate.h"

#include "MetalBlitCommandEncoder.h"
#include "MetalCommandBuffer.h"

@implementation FMetalDebugBlitCommandEncoder

@synthesize Inner;
@synthesize Buffer;

-(id)initWithEncoder:(id<MTLBlitCommandEncoder>)Encoder andCommandBuffer:(FMetalDebugCommandBuffer*)SourceBuffer
{
	id Self = [super init];
	if (Self)
	{
        Inner = [Encoder retain];
		Buffer = [SourceBuffer retain];
	}
	return Self;
}

-(void)dealloc
{
	[Inner release];
	[Buffer release];
	[super dealloc];
}

-(id <MTLDevice>) device
{
	return Inner.device;
}

-(NSString *)label
{
	return Inner.label;
}

-(void)setLabel:(NSString *)Text
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

#if PLATFORM_MAC
- (void)synchronizeResource:(id<MTLResource>)resource
{
    [Buffer blit:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    [Buffer trackResource:resource];
    [Inner synchronizeResource:(id<MTLResource>)resource];
}

- (void)synchronizeTexture:(id<MTLTexture>)texture slice:(NSUInteger)slice level:(NSUInteger)level
{
    [Buffer blit:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    [Buffer trackResource:texture];
    [Inner synchronizeTexture:(id<MTLTexture>)texture slice:(NSUInteger)slice level:(NSUInteger)level];
}
#endif

- (void)copyFromTexture:(id<MTLTexture>)sourceTexture sourceSlice:(NSUInteger)sourceSlice sourceLevel:(NSUInteger)sourceLevel sourceOrigin:(MTLOrigin)sourceOrigin sourceSize:(MTLSize)sourceSize toTexture:(id<MTLTexture>)destinationTexture destinationSlice:(NSUInteger)destinationSlice destinationLevel:(NSUInteger)destinationLevel destinationOrigin:(MTLOrigin)destinationOrigin
{
    [Buffer blit:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    [Buffer trackResource:sourceTexture];
    [Buffer trackResource:destinationTexture];
    
    [Inner copyFromTexture:(id<MTLTexture>)sourceTexture sourceSlice:(NSUInteger)sourceSlice sourceLevel:(NSUInteger)sourceLevel sourceOrigin:(MTLOrigin)sourceOrigin sourceSize:(MTLSize)sourceSize toTexture:(id<MTLTexture>)destinationTexture destinationSlice:(NSUInteger)destinationSlice destinationLevel:(NSUInteger)destinationLevel destinationOrigin:(MTLOrigin)destinationOrigin];
}

- (void)copyFromBuffer:(id<MTLBuffer>)sourceBuffer sourceOffset:(NSUInteger)sourceOffset sourceBytesPerRow:(NSUInteger)sourceBytesPerRow sourceBytesPerImage:(NSUInteger)sourceBytesPerImage sourceSize:(MTLSize)sourceSize toTexture:(id<MTLTexture>)destinationTexture destinationSlice:(NSUInteger)destinationSlice destinationLevel:(NSUInteger)destinationLevel destinationOrigin:(MTLOrigin)destinationOrigin
{
    [Buffer blit:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    [Buffer trackResource:sourceBuffer];
    [Buffer trackResource:destinationTexture];
    
    [Inner copyFromBuffer:(id<MTLBuffer>)sourceBuffer sourceOffset:(NSUInteger)sourceOffset sourceBytesPerRow:(NSUInteger)sourceBytesPerRow sourceBytesPerImage:(NSUInteger)sourceBytesPerImage sourceSize:(MTLSize)sourceSize toTexture:(id<MTLTexture>)destinationTexture destinationSlice:(NSUInteger)destinationSlice destinationLevel:(NSUInteger)destinationLevel destinationOrigin:(MTLOrigin)destinationOrigin];
}

- (void)copyFromBuffer:(id<MTLBuffer>)sourceBuffer sourceOffset:(NSUInteger)sourceOffset sourceBytesPerRow:(NSUInteger)sourceBytesPerRow sourceBytesPerImage:(NSUInteger)sourceBytesPerImage sourceSize:(MTLSize)sourceSize toTexture:(id<MTLTexture>)destinationTexture destinationSlice:(NSUInteger)destinationSlice destinationLevel:(NSUInteger)destinationLevel destinationOrigin:(MTLOrigin)destinationOrigin options:(MTLBlitOption)options
{
    [Buffer blit:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    [Buffer trackResource:sourceBuffer];
    [Buffer trackResource:destinationTexture];
    
    [Inner copyFromBuffer:(id<MTLBuffer>)sourceBuffer sourceOffset:(NSUInteger)sourceOffset sourceBytesPerRow:(NSUInteger)sourceBytesPerRow sourceBytesPerImage:(NSUInteger)sourceBytesPerImage sourceSize:(MTLSize)sourceSize toTexture:(id<MTLTexture>)destinationTexture destinationSlice:(NSUInteger)destinationSlice destinationLevel:(NSUInteger)destinationLevel destinationOrigin:(MTLOrigin)destinationOrigin options:(MTLBlitOption)options];
}

- (void)copyFromTexture:(id<MTLTexture>)sourceTexture sourceSlice:(NSUInteger)sourceSlice sourceLevel:(NSUInteger)sourceLevel sourceOrigin:(MTLOrigin)sourceOrigin sourceSize:(MTLSize)sourceSize toBuffer:(id<MTLBuffer>)destinationBuffer destinationOffset:(NSUInteger)destinationOffset destinationBytesPerRow:(NSUInteger)destinationBytesPerRow destinationBytesPerImage:(NSUInteger)destinationBytesPerImage
{
    [Buffer blit:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    [Buffer trackResource:sourceTexture];
    [Buffer trackResource:destinationBuffer];
    
    [Inner copyFromTexture:(id<MTLTexture>)sourceTexture sourceSlice:(NSUInteger)sourceSlice sourceLevel:(NSUInteger)sourceLevel sourceOrigin:(MTLOrigin)sourceOrigin sourceSize:(MTLSize)sourceSize toBuffer:(id<MTLBuffer>)destinationBuffer destinationOffset:(NSUInteger)destinationOffset destinationBytesPerRow:(NSUInteger)destinationBytesPerRow destinationBytesPerImage:(NSUInteger)destinationBytesPerImage];
}

- (void)copyFromTexture:(id<MTLTexture>)sourceTexture sourceSlice:(NSUInteger)sourceSlice sourceLevel:(NSUInteger)sourceLevel sourceOrigin:(MTLOrigin)sourceOrigin sourceSize:(MTLSize)sourceSize toBuffer:(id<MTLBuffer>)destinationBuffer destinationOffset:(NSUInteger)destinationOffset destinationBytesPerRow:(NSUInteger)destinationBytesPerRow destinationBytesPerImage:(NSUInteger)destinationBytesPerImage options:(MTLBlitOption)options
{
    [Buffer blit:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    [Buffer trackResource:sourceTexture];
    [Buffer trackResource:destinationBuffer];
    
    [Inner copyFromTexture:(id<MTLTexture>)sourceTexture sourceSlice:(NSUInteger)sourceSlice sourceLevel:(NSUInteger)sourceLevel sourceOrigin:(MTLOrigin)sourceOrigin sourceSize:(MTLSize)sourceSize toBuffer:(id<MTLBuffer>)destinationBuffer destinationOffset:(NSUInteger)destinationOffset destinationBytesPerRow:(NSUInteger)destinationBytesPerRow destinationBytesPerImage:(NSUInteger)destinationBytesPerImage options:(MTLBlitOption)options];
}

- (void)generateMipmapsForTexture:(id<MTLTexture>)texture
{
    [Buffer blit:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    [Buffer trackResource:texture];
    [Inner generateMipmapsForTexture:(id<MTLTexture>)texture];
}

- (void)fillBuffer:(id <MTLBuffer>)buffer range:(NSRange)range value:(uint8_t)value
{
    [Buffer blit:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    [Buffer trackResource:buffer];
    [Inner fillBuffer:(id <MTLBuffer>)buffer range:(NSRange)range value:(uint8_t)value];
}

- (void)copyFromBuffer:(id <MTLBuffer>)sourceBuffer sourceOffset:(NSUInteger)sourceOffset toBuffer:(id <MTLBuffer>)destinationBuffer destinationOffset:(NSUInteger)destinationOffset size:(NSUInteger)size
{
    [Buffer blit:[NSString stringWithFormat:@"%s", __PRETTY_FUNCTION__]];
    [Buffer trackResource:sourceBuffer];
    [Buffer trackResource:destinationBuffer];
    [Inner copyFromBuffer:(id <MTLBuffer>)sourceBuffer sourceOffset:(NSUInteger)sourceOffset toBuffer:(id <MTLBuffer>)destinationBuffer destinationOffset:(NSUInteger)destinationOffset size:(NSUInteger)size];
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

@end

