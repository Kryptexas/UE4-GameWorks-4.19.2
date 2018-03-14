// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#import <Metal/MTLRenderCommandEncoder.h>
#include "MTIRenderCommandEncoder.hpp"

MTLPP_BEGIN

INTERPOSE_PROTOCOL_REGISTER(MTIRenderCommandEncoderTrace, id<MTLRenderCommandEncoder>);

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, Setrenderpipelinestate,  void,   id <MTLRenderPipelineState> pipelineState)
{
	Original(Obj, Cmd, pipelineState);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetvertexbytesLengthAtindex,  void,   const void * bytes,NSUInteger length,NSUInteger index)
{
	Original(Obj, Cmd, bytes, length, index);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetvertexbufferOffsetAtindex,  void,    id <MTLBuffer> buffer,NSUInteger offset,NSUInteger index)
{
	Original(Obj, Cmd, buffer, offset, index);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetvertexbufferoffsetAtindex,  void,   NSUInteger offset,NSUInteger index)
{
	Original(Obj, Cmd, offset, index);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetvertexbuffersOffsetsWithrange,  void,   const id <MTLBuffer>  * buffers,const NSUInteger * offsets,NSRange range)
{
	Original(Obj, Cmd, buffers, offsets, range);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetvertextextureAtindex,  void,    id <MTLTexture> texture,NSUInteger index)
{
	Original(Obj, Cmd, texture, index);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetvertextexturesWithrange,  void,   const id <MTLTexture>  * textures,NSRange range)
{
	Original(Obj, Cmd, textures, range);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetvertexsamplerstateAtindex,  void,    id <MTLSamplerState> sampler,NSUInteger index)
{
	Original(Obj, Cmd, sampler, index);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetvertexsamplerstatesWithrange,  void,   const id <MTLSamplerState>  * samplers,NSRange range)
{
	Original(Obj, Cmd, samplers, range);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetvertexsamplerstateLodminclampLodmaxclampAtindex,  void,    id <MTLSamplerState> sampler,float lodMinClamp,float lodMaxClamp,NSUInteger index)
{
	Original(Obj, Cmd, sampler, lodMinClamp, lodMaxClamp, index);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetvertexsamplerstatesLodminclampsLodmaxclampsWithrange,  void,   const id <MTLSamplerState>  * samplers,const float * lodMinClamps,const float * lodMaxClamps,NSRange range)
{
	Original(Obj, Cmd, samplers, lodMinClamps, lodMaxClamps, range);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, Setviewport,  void,   MTLPPViewport viewport)
{
	Original(Obj, Cmd, viewport);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetviewportsCount,  void,   const MTLPPViewport * viewports,NSUInteger count)
{
	Original(Obj, Cmd, viewports, count);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, Setfrontfacingwinding,  void,   MTLWinding frontFacingWinding)
{
	Original(Obj, Cmd, frontFacingWinding);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, Setcullmode,  void,   MTLCullMode cullMode)
{
	Original(Obj, Cmd, cullMode);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, Setdepthclipmode,  void,   MTLDepthClipMode depthClipMode)
{
	Original(Obj, Cmd, depthClipMode);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetdepthbiasSlopescaleClamp,  void,   float depthBias,float slopeScale,float clamp)
{
	Original(Obj, Cmd, depthBias, slopeScale, clamp);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, Setscissorrect,  void,   MTLPPScissorRect rect)
{
	Original(Obj, Cmd, rect);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetscissorrectsCount,  void,   const MTLPPScissorRect * scissorRects,NSUInteger count)
{
	Original(Obj, Cmd, scissorRects, count);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, Settrianglefillmode,  void,   MTLTriangleFillMode fillMode)
{
	Original(Obj, Cmd, fillMode);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetfragmentbytesLengthAtindex,  void,   const void * bytes,NSUInteger length,NSUInteger index)
{
	Original(Obj, Cmd, bytes, length, index);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetfragmentbufferOffsetAtindex,  void,    id <MTLBuffer> buffer,NSUInteger offset,NSUInteger index)
{
	Original(Obj, Cmd, buffer, offset,index);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetfragmentbufferoffsetAtindex,  void,   NSUInteger offset,NSUInteger index)
{
	Original(Obj, Cmd, offset, index);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetfragmentbuffersOffsetsWithrange,  void,   const id <MTLBuffer>  * buffers,const NSUInteger * offsets,NSRange range)
{
	Original(Obj, Cmd, buffers, offsets, range);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetfragmenttextureAtindex,  void,    id <MTLTexture> texture,NSUInteger index)
{
	Original(Obj, Cmd, texture, index);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetfragmenttexturesWithrange,  void,   const id <MTLTexture>  * textures,NSRange range)
{
	Original(Obj, Cmd, textures, range);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetfragmentsamplerstateAtindex,  void,    id <MTLSamplerState> sampler,NSUInteger index)
{
	Original(Obj, Cmd, sampler, index);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetfragmentsamplerstatesWithrange,  void,   const id <MTLSamplerState>  * samplers,NSRange range)
{
	Original(Obj, Cmd, samplers, range);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetfragmentsamplerstateLodminclampLodmaxclampAtindex,  void,    id <MTLSamplerState> sampler,float lodMinClamp,float lodMaxClamp,NSUInteger index)
{
	Original(Obj, Cmd, sampler, lodMinClamp, lodMaxClamp, index);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetfragmentsamplerstatesLodminclampsLodmaxclampsWithrange,  void,   const id <MTLSamplerState>  * samplers,const float * lodMinClamps,const float * lodMaxClamps,NSRange range)
{
	Original(Obj, Cmd, samplers, lodMinClamps, lodMaxClamps, range);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetblendcolorredGreenBlueAlpha,  void,   float red,float green,float blue,float alpha)
{
	Original(Obj, Cmd, red, green, blue, alpha);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, Setdepthstencilstate,  void,    id <MTLDepthStencilState> depthStencilState)
{
	Original(Obj, Cmd, depthStencilState);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, Setstencilreferencevalue,  void,   uint32_t referenceValue)
{
	Original(Obj, Cmd, referenceValue);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetstencilfrontreferencevalueBackreferencevalue,  void,   uint32_t frontReferenceValue,uint32_t backReferenceValue)
{
	Original(Obj, Cmd, frontReferenceValue, backReferenceValue);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetvisibilityresultmodeOffset,  void,   MTLVisibilityResultMode mode,NSUInteger offset)
{
	Original(Obj, Cmd, mode, offset);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetcolorstoreactionAtindex,  void,   MTLStoreAction storeAction,NSUInteger colorAttachmentIndex)
{
	Original(Obj, Cmd, storeAction, colorAttachmentIndex);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, Setdepthstoreaction,  void,   MTLStoreAction storeAction)
{
	Original(Obj, Cmd, storeAction);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, Setstencilstoreaction,  void,   MTLStoreAction storeAction)
{
	Original(Obj, Cmd, storeAction);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SetcolorstoreactionoptionsAtindex,  void,   MTLStoreActionOptions storeActionOptions,NSUInteger colorAttachmentIndex)
{
	Original(Obj, Cmd, storeActionOptions,colorAttachmentIndex);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, Setdepthstoreactionoptions,  void,   MTLStoreActionOptions storeActionOptions)
{
	Original(Obj, Cmd, storeActionOptions);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, Setstencilstoreactionoptions,  void,   MTLStoreActionOptions storeActionOptions)
{
	Original(Obj, Cmd, storeActionOptions);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, DrawprimitivesVertexstartVertexcountInstancecount,  void,   MTLPrimitiveType primitiveType,NSUInteger vertexStart,NSUInteger vertexCount,NSUInteger instanceCount)
{
	Original(Obj, Cmd, primitiveType,vertexStart,vertexCount,instanceCount);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, DrawprimitivesVertexstartVertexcount,  void,   MTLPrimitiveType primitiveType,NSUInteger vertexStart,NSUInteger vertexCount)
{
	Original(Obj, Cmd, primitiveType,vertexStart,vertexCount);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, DrawindexedprimitivesIndexcountIndextypeIndexbufferIndexbufferoffsetInstancecount,  void,   MTLPrimitiveType primitiveType,NSUInteger indexCount,MTLIndexType indexType,id <MTLBuffer> indexBuffer,NSUInteger indexBufferOffset,NSUInteger instanceCount)
{
	Original(Obj, Cmd, primitiveType,indexCount,indexType,indexBuffer,indexBufferOffset,instanceCount);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, DrawindexedprimitivesIndexcountIndextypeIndexbufferIndexbufferoffset,  void,   MTLPrimitiveType primitiveType,NSUInteger indexCount,MTLIndexType indexType,id <MTLBuffer> indexBuffer,NSUInteger indexBufferOffset)
{
	Original(Obj, Cmd, primitiveType,indexCount,indexType,indexBuffer,indexBufferOffset);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, DrawprimitivesVertexstartVertexcountInstancecountBaseinstance,  void,   MTLPrimitiveType primitiveType,NSUInteger vertexStart,NSUInteger vertexCount,NSUInteger instanceCount,NSUInteger baseInstance)
{
	Original(Obj, Cmd, primitiveType,vertexStart,vertexCount,instanceCount,baseInstance);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, DrawindexedprimitivesIndexcountIndextypeIndexbufferIndexbufferoffsetInstancecountBasevertexBaseinstance,  void,   MTLPrimitiveType primitiveType,NSUInteger indexCount,MTLIndexType indexType,id <MTLBuffer> indexBuffer,NSUInteger indexBufferOffset,NSUInteger instanceCount,NSInteger baseVertex,NSUInteger baseInstance)
{
	Original(Obj, Cmd, primitiveType,indexCount,indexType,indexBuffer,indexBufferOffset,instanceCount,baseVertex,baseInstance);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, DrawprimitivesIndirectbufferIndirectbufferoffset,  void,   MTLPrimitiveType primitiveType,id <MTLBuffer> indirectBuffer,NSUInteger indirectBufferOffset)
{
	Original(Obj, Cmd, primitiveType,indirectBuffer,indirectBufferOffset);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, DrawindexedprimitivesIndextypeIndexbufferIndexbufferoffsetIndirectbufferIndirectbufferoffset,  void,   MTLPrimitiveType primitiveType,MTLIndexType indexType,id <MTLBuffer> indexBuffer,NSUInteger indexBufferOffset,id <MTLBuffer> indirectBuffer,NSUInteger indirectBufferOffset)
{
	Original(Obj, Cmd, primitiveType,indexType,indexBuffer,indexBufferOffset,indirectBuffer,indirectBufferOffset);
}

INTERPOSE_DEFINITION_VOID( MTIRenderCommandEncoderTrace, Texturebarrier,  void)
{
	Original(Obj, Cmd);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, UpdatefenceAfterstages,  void,   id <MTLFence> fence,MTLRenderStages stages)
{
	Original(Obj, Cmd, fence,stages);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, WaitforfenceBeforestages,  void,   id <MTLFence> fence,MTLRenderStages stages)
{
	Original(Obj, Cmd, fence,stages);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, SettessellationfactorbufferOffsetInstancestride,  void,    id <MTLBuffer> buffer,NSUInteger offset,NSUInteger instanceStride)
{
	Original(Obj, Cmd, buffer,offset,instanceStride);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, Settessellationfactorscale,  void,   float scale)
{
	Original(Obj, Cmd, scale);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, DrawpatchesPatchstartPatchcountPatchindexbufferPatchindexbufferoffsetInstancecountBaseinstance,  void,   NSUInteger numberOfPatchControlPoints,NSUInteger patchStart,NSUInteger patchCount, id <MTLBuffer> patchIndexBuffer,NSUInteger patchIndexBufferOffset,NSUInteger instanceCount,NSUInteger baseInstance)
{
	Original(Obj, Cmd, numberOfPatchControlPoints,patchStart,patchCount,patchIndexBuffer,patchIndexBufferOffset,instanceCount,baseInstance);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, DrawpatchesPatchindexbufferPatchindexbufferoffsetIndirectbufferIndirectbufferoffset,  void,   NSUInteger numberOfPatchControlPoints, id <MTLBuffer> patchIndexBuffer,NSUInteger patchIndexBufferOffset,id <MTLBuffer> indirectBuffer,NSUInteger indirectBufferOffset)
{
	Original(Obj, Cmd, numberOfPatchControlPoints,patchIndexBuffer,patchIndexBufferOffset,indirectBuffer,indirectBufferOffset);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, DrawindexedpatchesPatchstartPatchcountPatchindexbufferPatchindexbufferoffsetControlpointindexbufferControlpointindexbufferoffsetInstancecountBaseinstance,  void,   NSUInteger numberOfPatchControlPoints,NSUInteger patchStart,NSUInteger patchCount, id <MTLBuffer> patchIndexBuffer,NSUInteger patchIndexBufferOffset,id <MTLBuffer> controlPointIndexBuffer,NSUInteger controlPointIndexBufferOffset,NSUInteger instanceCount,NSUInteger baseInstance)
{
	Original(Obj, Cmd, numberOfPatchControlPoints,patchStart,patchCount,patchIndexBuffer,patchIndexBufferOffset,controlPointIndexBuffer,controlPointIndexBufferOffset,instanceCount,baseInstance);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, DrawindexedpatchesPatchindexbufferPatchindexbufferoffsetControlpointindexbufferControlpointindexbufferoffsetIndirectbufferIndirectbufferoffset,  void,   NSUInteger numberOfPatchControlPoints, id <MTLBuffer> patchIndexBuffer,NSUInteger patchIndexBufferOffset,id <MTLBuffer> controlPointIndexBuffer,NSUInteger controlPointIndexBufferOffset,id <MTLBuffer> indirectBuffer,NSUInteger indirectBufferOffset)
{
	Original(Obj, Cmd, numberOfPatchControlPoints,patchIndexBuffer,patchIndexBufferOffset,controlPointIndexBuffer,controlPointIndexBufferOffset,indirectBuffer,indirectBufferOffset);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, UseresourceUsage,  void,   id <MTLResource> resource,MTLResourceUsage usage)
{
	Original(Obj, Cmd, resource, usage);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, UseresourcesCountUsage,  void,   const id <MTLResource> * resources,NSUInteger count,MTLResourceUsage usage)
{
	Original(Obj, Cmd, resources, count, usage);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, Useheap,  void,    id <MTLHeap> heap)
{
	Original(Obj, Cmd, heap);
}

INTERPOSE_DEFINITION( MTIRenderCommandEncoderTrace, UseheapsCount,  void,   const id <MTLHeap> * heaps, NSUInteger count)
{
	Original(Obj, Cmd, heaps, count);
}

INTERPOSE_DEFINITION(MTIRenderCommandEncoderTrace, SetTilebytesLengthAtindex,	void, const void * b,NSUInteger l,NSUInteger i)
{
	Original(Obj, Cmd, b,l,i);
}
INTERPOSE_DEFINITION(MTIRenderCommandEncoderTrace, SetTilebufferOffsetAtindex,	void,id <MTLBuffer> b,NSUInteger o,NSUInteger i)
{
	Original(Obj, Cmd, b,o,i);
}
INTERPOSE_DEFINITION(MTIRenderCommandEncoderTrace, SetTilebufferoffsetAtindex,	void, NSUInteger o,NSUInteger i)
{
	Original(Obj, Cmd, o,i);
}
INTERPOSE_DEFINITION(MTIRenderCommandEncoderTrace, SetTilebuffersOffsetsWithrange,	void, const id <MTLBuffer> * b,const NSUInteger * o, NSRange r)
{
	Original(Obj, Cmd, b,o,r);
}
INTERPOSE_DEFINITION(MTIRenderCommandEncoderTrace, SetTiletextureAtindex,	void,id <MTLTexture> t,NSUInteger i)
{
	Original(Obj, Cmd, t,i);
}
INTERPOSE_DEFINITION(MTIRenderCommandEncoderTrace, SetTiletexturesWithrange,	void, const id <MTLTexture> * t,NSRange r)
{
	Original(Obj, Cmd, t,r);
}
INTERPOSE_DEFINITION(MTIRenderCommandEncoderTrace, SetTilesamplerstateAtindex,	void,id <MTLSamplerState> s,NSUInteger i)
{
	Original(Obj, Cmd, s,i);
}
INTERPOSE_DEFINITION(MTIRenderCommandEncoderTrace, SetTilesamplerstatesWithrange,	void, const id <MTLSamplerState> * s,NSRange r)
{
	Original(Obj, Cmd, s,r);
}
INTERPOSE_DEFINITION(MTIRenderCommandEncoderTrace, SetTilesamplerstateLodminclampLodmaxclampAtindex,	void,id <MTLSamplerState> s,float l,float x,NSUInteger i)
{
	Original(Obj, Cmd, s,l,x,i);
}
INTERPOSE_DEFINITION(MTIRenderCommandEncoderTrace, SetTilesamplerstatesLodminclampsLodmaxclampsWithrange,	void, const id <MTLSamplerState> * s,const float * l,const float * x,NSRange r)
{
	Original(Obj, Cmd, s,l,x,r);
}
INTERPOSE_DEFINITION(MTIRenderCommandEncoderTrace, dispatchThreadsPerTile,	void, MTLPPSize s)
{
	Original(Obj, Cmd, s);
}
INTERPOSE_DEFINITION(MTIRenderCommandEncoderTrace, setThreadgroupMemoryLength,	void, NSUInteger t, NSUInteger l, NSUInteger i)
{
	Original(Obj, Cmd, t,l,i);
}

MTLPP_END
