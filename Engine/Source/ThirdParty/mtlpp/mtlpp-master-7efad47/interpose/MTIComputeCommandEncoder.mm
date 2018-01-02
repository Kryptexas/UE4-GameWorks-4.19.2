// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#import <Metal/MTLComputeCommandEncoder.h>
#include "MTIComputeCommandEncoder.hpp"
#include "MTIRenderCommandEncoder.hpp"

MTLPP_BEGIN

INTERPOSE_PROTOCOL_REGISTER(MTIComputeCommandEncoderTrace, id<MTLComputeCommandEncoder>);

INTERPOSE_DEFINITION( MTIComputeCommandEncoderTrace, Setcomputepipelinestate, void, id <MTLComputePipelineState> state)
{
	Original(Obj, Cmd, state);
}

INTERPOSE_DEFINITION( MTIComputeCommandEncoderTrace, Setbyteslengthatindex, void, const void * bytes, NSUInteger length, NSUInteger index)
{
	Original(Obj, Cmd, bytes, length, index);
}

INTERPOSE_DEFINITION( MTIComputeCommandEncoderTrace, Setbufferoffsetatindex, void,  id <MTLBuffer> buffer, NSUInteger offset, NSUInteger index)
{
	Original(Obj, Cmd, buffer, offset, index);
}

INTERPOSE_DEFINITION( MTIComputeCommandEncoderTrace, SetBufferOffsetatindex, void, NSUInteger offset, NSUInteger index)
{
	Original(Obj, Cmd, offset, index);
}

INTERPOSE_DEFINITION( MTIComputeCommandEncoderTrace, Setbuffersoffsetswithrange, void, const id <MTLBuffer>  * buffers, const NSUInteger * offsets, NSRange range)
{
	Original(Obj, Cmd, buffers, offsets, range);
}

INTERPOSE_DEFINITION( MTIComputeCommandEncoderTrace, Settextureatindex, void,  id <MTLTexture> texture, NSUInteger index)
{
	Original(Obj, Cmd, texture, index);
}

INTERPOSE_DEFINITION( MTIComputeCommandEncoderTrace, Settextureswithrange, void, const id <MTLTexture>  * textures, NSRange range)
{
	Original(Obj, Cmd, textures, range);
}

INTERPOSE_DEFINITION( MTIComputeCommandEncoderTrace, Setsamplerstateatindex, void,  id <MTLSamplerState> sampler, NSUInteger index)
{
	Original(Obj, Cmd, sampler, index);
}

INTERPOSE_DEFINITION( MTIComputeCommandEncoderTrace, Setsamplerstateswithrange, void, const id <MTLSamplerState>  * samplers, NSRange range)
{
	Original(Obj, Cmd, samplers, range);
}

INTERPOSE_DEFINITION( MTIComputeCommandEncoderTrace, Setsamplerstatelodminclamplodmaxclampatindex, void,  id <MTLSamplerState> sampler, float lodMinClamp, float lodMaxClamp, NSUInteger index)
{
	Original(Obj, Cmd, sampler, lodMinClamp, lodMaxClamp, index);
}

INTERPOSE_DEFINITION( MTIComputeCommandEncoderTrace, Setsamplerstateslodminclampslodmaxclampswithrange, void, const id <MTLSamplerState>  * samplers, const float * lodMinClamps, const float * lodMaxClamps, NSRange range)
{
	Original(Obj, Cmd, samplers, lodMinClamps, lodMaxClamps, range);
}

INTERPOSE_DEFINITION( MTIComputeCommandEncoderTrace, Setthreadgroupmemorylengthatindex, void, NSUInteger length, NSUInteger index)
{
	Original(Obj, Cmd, length, index);
}

INTERPOSE_DEFINITION( MTIComputeCommandEncoderTrace, Setstageinregion, void, MTLPPRegion region)
{
	Original(Obj, Cmd, region);
}

INTERPOSE_DEFINITION( MTIComputeCommandEncoderTrace, Dispatchthreadgroupsthreadsperthreadgroup, void, MTLPPSize threadgroupsPerGrid, MTLPPSize threadsPerThreadgroup)
{
	Original(Obj, Cmd, threadgroupsPerGrid, threadsPerThreadgroup);
}

INTERPOSE_DEFINITION( MTIComputeCommandEncoderTrace, Dispatchthreadgroupswithindirectbufferindirectbufferoffsetthreadsperthreadgroup, void, id <MTLBuffer> indirectBuffer, NSUInteger indirectBufferOffset, MTLPPSize threadsPerThreadgroup)
{
	Original(Obj, Cmd, indirectBuffer, indirectBufferOffset, threadsPerThreadgroup);
}

INTERPOSE_DEFINITION( MTIComputeCommandEncoderTrace, Dispatchthreadsthreadsperthreadgroup, void, MTLPPSize threadsPerGrid, MTLPPSize threadsPerThreadgroup)
{
	Original(Obj, Cmd, threadsPerGrid, threadsPerThreadgroup);
}

INTERPOSE_DEFINITION( MTIComputeCommandEncoderTrace, Updatefence, void, id <MTLFence> fence)
{
	Original(Obj, Cmd, fence);
}

INTERPOSE_DEFINITION( MTIComputeCommandEncoderTrace, Waitforfence, void, id <MTLFence> fence)
{
	Original(Obj, Cmd, fence);
}

INTERPOSE_DEFINITION( MTIComputeCommandEncoderTrace, Useresourceusage, void, id <MTLResource> resource, MTLResourceUsage usage)
{
	Original(Obj, Cmd, resource, usage);
}

INTERPOSE_DEFINITION( MTIComputeCommandEncoderTrace, Useresourcescountusage, void, const id <MTLResource> * resources, NSUInteger count, MTLResourceUsage usage)
{
	Original(Obj, Cmd, resources, count, usage);
}

INTERPOSE_DEFINITION( MTIComputeCommandEncoderTrace, Useheap, void, id <MTLHeap> heap)
{
	Original(Obj, Cmd, heap);
}

INTERPOSE_DEFINITION( MTIComputeCommandEncoderTrace, Useheapscount, void, const id <MTLHeap> * heaps, NSUInteger count)
{
	Original(Obj, Cmd, heaps, count);
}

INTERPOSE_DEFINITION(MTIComputeCommandEncoderTrace, SetImageblockWidthHeight, void, NSUInteger width, NSUInteger height)
{
	Original(Obj, Cmd, width, height);
}

MTLPP_END
