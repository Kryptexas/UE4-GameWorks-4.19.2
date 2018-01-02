// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#import <Metal/MTLBlitCommandEncoder.h>
#include "MTIBlitCommandEncoder.hpp"

MTLPP_BEGIN

INTERPOSE_PROTOCOL_REGISTER(MTIBlitCommandEncoderTrace, id<MTLBlitCommandEncoder>);

INTERPOSE_DEFINITION(MTIBlitCommandEncoderTrace, SynchronizeResource, void, id<MTLResource> Res)
{
	Original(Obj, Cmd, Res);
}
INTERPOSE_DEFINITION(MTIBlitCommandEncoderTrace, SynchronizeTextureSliceLevel, void, id<MTLTexture> Res, NSUInteger Slice, NSUInteger Level)
{
	Original(Obj, Cmd, Res, Slice, Level);
}
INTERPOSE_DEFINITION(MTIBlitCommandEncoderTrace, CopyFromTexturesourceSlicesourceLevelsourceOriginsourceSizetoTexturedestinationSlicedestinationLeveldestinationOrigin, void, id<MTLTexture> tex, NSUInteger slice, NSUInteger level, MTLPPOrigin origin, MTLPPSize size, id<MTLTexture> dst, NSUInteger dstslice, NSUInteger dstlevel, MTLPPOrigin dstorigin)
{
	Original(Obj, Cmd, tex, slice, level, origin, size, dst, dstslice, dstlevel, dstorigin);
}
INTERPOSE_DEFINITION(MTIBlitCommandEncoderTrace, CopyFromBuffersourceOffsetsourceBytesPerRowsourceBytesPerImagesourceSizetoTexturedestinationSlicedestinationLeveldestinationOrigin, void, id<MTLBuffer> buffer, NSUInteger offset, NSUInteger bpr, NSUInteger bpi, MTLPPSize size, id<MTLTexture> dst, NSUInteger slice, NSUInteger level, MTLPPOrigin origin)
{
	Original(Obj, Cmd, buffer, offset, bpr, bpi, size, dst, slice, level, origin);
}
INTERPOSE_DEFINITION(MTIBlitCommandEncoderTrace, CopyFromBuffersourceOffsetsourceBytesPerRowsourceBytesPerImagesourceSizetoTexturedestinationSlicedestinationLeveldestinationOriginoptions, void, id<MTLBuffer> buffer, NSUInteger offset, NSUInteger bpr, NSUInteger bpi, MTLPPSize size, id<MTLTexture> dst, NSUInteger slice, NSUInteger level, MTLPPOrigin origin, MTLBlitOption Opts)
{
	Original(Obj, Cmd, buffer, offset, bpr, bpi, size, dst, slice, level, origin, Opts);
}
INTERPOSE_DEFINITION(MTIBlitCommandEncoderTrace, CopyFromTexturesourceSlicesourceLevelsourceOriginsourceSizetoBufferdestinationOffsetdestinationBytesPerRowdestinationBytesPerImage, void, id<MTLTexture> tex, NSUInteger slice, NSUInteger level, MTLPPOrigin origin, MTLPPSize size, id<MTLBuffer> dst, NSUInteger offset, NSUInteger bpr, NSUInteger bpi)
{
	Original(Obj, Cmd, tex, slice, level, origin, size, dst, offset, bpr, bpi);
}
INTERPOSE_DEFINITION(MTIBlitCommandEncoderTrace, CopyFromTexturesourceSlicesourceLevelsourceOriginsourceSizetoBufferdestinationOffsetdestinationBytesPerRowdestinationBytesPerImageoptions, void, id<MTLTexture> Tex, NSUInteger slice, NSUInteger level, MTLPPOrigin origin, MTLPPSize size, id<MTLBuffer> dest, NSUInteger offset, NSUInteger bpr, NSUInteger bpi, MTLBlitOption Opts)
{
	Original(Obj, Cmd, Tex, slice, level, origin, size, dest, offset, bpr, bpi, Opts);
}
INTERPOSE_DEFINITION(MTIBlitCommandEncoderTrace, GenerateMipmapsForTexture, void, id<MTLTexture> Tex)
{
	Original(Obj, Cmd, Tex);
}
INTERPOSE_DEFINITION(MTIBlitCommandEncoderTrace, FillBufferRangeValue, void, id <MTLBuffer> Buffer, NSRange Range, uint8_t Val)
{
	Original(Obj, Cmd, Buffer, Range, Val);
}
INTERPOSE_DEFINITION(MTIBlitCommandEncoderTrace, CopyFromBufferSourceOffsetToBufferDestinationOffsetSize, void, id <MTLBuffer> Buffer, NSUInteger Offset, id <MTLBuffer> Dest,  NSUInteger DstOffset, NSUInteger Size)
{
	Original(Obj, Cmd, Buffer, Offset, Dest, DstOffset, Size);
}
INTERPOSE_DEFINITION(MTIBlitCommandEncoderTrace, UpdateFence, void, id <MTLFence> Fence)
{
	Original(Obj, Cmd, Fence);
}
INTERPOSE_DEFINITION(MTIBlitCommandEncoderTrace, WaitForFence, void, id <MTLFence> Fence)
{
	Original(Obj, Cmd, Fence);
}

MTLPP_END
