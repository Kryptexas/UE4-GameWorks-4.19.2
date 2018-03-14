/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLRenderCommandEncoder.h>
#include <Metal/MTLBuffer.h>
#include "render_command_encoder.hpp"
#include "buffer.hpp"
#include "depth_stencil.hpp"
#include "render_pipeline.hpp"
#include "sampler.hpp"
#include "texture.hpp"
#include "heap.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    void RenderCommandEncoder::SetRenderPipelineState(const RenderPipelineState& pipelineState)
    {
        Validate();
		m_table->Setrenderpipelinestate(m_ptr, pipelineState.GetPtr());
    }

    void RenderCommandEncoder::SetVertexData(const void* bytes, NSUInteger length, NSUInteger index)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 8_3)
		m_table->SetvertexbytesLengthAtindex(m_ptr, bytes, length, index);
#endif
    }

    void RenderCommandEncoder::SetVertexBuffer(const Buffer& buffer, NSUInteger offset, NSUInteger index)
    {
        Validate();
		m_table->SetvertexbufferOffsetAtindex(m_ptr, (id<MTLBuffer>)buffer.GetPtr(), offset, index);
    }
    void RenderCommandEncoder::SetVertexBufferOffset(NSUInteger offset, NSUInteger index)
    {
#if MTLPP_IS_AVAILABLE(10_11, 8_3)
        Validate();
		m_table->SetvertexbufferoffsetAtindex(m_ptr, offset, index);
#endif
    }

	void RenderCommandEncoder::SetVertexBuffers(const Buffer::Type* buffers, const uint64_t* offsets, const ns::Range& range)
    {
        Validate();

		m_table->SetvertexbuffersOffsetsWithrange(m_ptr, (const id<MTLBuffer>*)buffers, (NSUInteger const*)offsets, NSMakeRange(range.Location, range.Length));
    }

    void RenderCommandEncoder::SetVertexTexture(const Texture& texture, NSUInteger index)
    {
        Validate();
		m_table->SetvertextextureAtindex(m_ptr, (id<MTLTexture>)texture.GetPtr(), index);
    }


    void RenderCommandEncoder::SetVertexTextures(const Texture::Type* textures, const ns::Range& range)
    {
        Validate();

		m_table->SetvertextexturesWithrange(m_ptr, (const id<MTLTexture>*)textures, NSMakeRange(range.Location, range.Length));
    }

    void RenderCommandEncoder::SetVertexSamplerState(const SamplerState& sampler, NSUInteger index)
    {
        Validate();
		m_table->SetvertexsamplerstateAtindex(m_ptr, sampler.GetPtr(), index);
    }

    void RenderCommandEncoder::SetVertexSamplerStates(const SamplerState::Type* samplers, const ns::Range& range)
    {
        Validate();

		m_table->SetvertexsamplerstatesWithrange(m_ptr, samplers, NSMakeRange(range.Location, range.Length));
    }

    void RenderCommandEncoder::SetVertexSamplerState(const SamplerState& sampler, float lodMinClamp, float lodMaxClamp, NSUInteger index)
    {
        Validate();
		m_table->SetvertexsamplerstateLodminclampLodmaxclampAtindex(m_ptr, sampler.GetPtr(), lodMinClamp, lodMaxClamp, index);
    }

    void RenderCommandEncoder::SetVertexSamplerStates(const SamplerState::Type* samplers, const float* lodMinClamps, const float* lodMaxClamps, const ns::Range& range)
    {
        Validate();

		m_table->SetvertexsamplerstatesLodminclampsLodmaxclampsWithrange(m_ptr, samplers, lodMinClamps, lodMaxClamps, NSMakeRange(range.Location, range.Length));
    }

    void RenderCommandEncoder::SetViewport(const Viewport& viewport)
    {
        Validate();
		m_table->Setviewport(m_ptr, viewport);
    }
	
	void RenderCommandEncoder::SetViewports(const Viewport* viewports, NSUInteger count)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_13)
		m_table->SetviewportsCount(m_ptr, viewports, count);
#endif
	}

    void RenderCommandEncoder::SetFrontFacingWinding(Winding frontFacingWinding)
    {
        Validate();
		m_table->Setfrontfacingwinding(m_ptr, (MTLWinding)frontFacingWinding);
    }

    void RenderCommandEncoder::SetCullMode(CullMode cullMode)
    {
        Validate();
		m_table->Setcullmode(m_ptr, (MTLCullMode)cullMode);
    }

    void RenderCommandEncoder::SetDepthClipMode(DepthClipMode depthClipMode)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 11_0)
		m_table->Setdepthclipmode(m_ptr, (MTLDepthClipMode)depthClipMode);
#endif
    }

    void RenderCommandEncoder::SetDepthBias(float depthBias, float slopeScale, float clamp)
    {
        Validate();
		m_table->SetdepthbiasSlopescaleClamp(m_ptr, depthBias, slopeScale, clamp);
    }

    void RenderCommandEncoder::SetScissorRect(const ScissorRect& rect)
    {
        Validate();
		m_table->Setscissorrect(m_ptr, rect);
    }
	
	void RenderCommandEncoder::SetScissorRects(const ScissorRect* rect, NSUInteger count)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_13)
		m_table->SetscissorrectsCount(m_ptr, rect, count);
#endif
	}

    void RenderCommandEncoder::SetTriangleFillMode(TriangleFillMode fillMode)
    {
        Validate();
		m_table->Settrianglefillmode(m_ptr, (MTLTriangleFillMode)fillMode);
    }

    	void RenderCommandEncoder::SetFragmentData(const void* bytes, NSUInteger length, NSUInteger index)
	{
		Validate();
	#if MTLPP_IS_AVAILABLE(10_11, 8_3)
		m_table->SetfragmentbytesLengthAtindex(m_ptr, bytes, length, index);
	#endif
	}

	void RenderCommandEncoder::SetFragmentBuffer(const Buffer& buffer, NSUInteger offset, NSUInteger index)
	{
		Validate();
		m_table->SetfragmentbufferOffsetAtindex(m_ptr, (id<MTLBuffer>)buffer.GetPtr(), offset, index);
	}
	void RenderCommandEncoder::SetFragmentBufferOffset(NSUInteger offset, NSUInteger index)
	{
	#if MTLPP_IS_AVAILABLE(10_11, 8_3)
		Validate();
		m_table->SetfragmentbufferoffsetAtindex(m_ptr, offset, index);
	#endif
	}

	void RenderCommandEncoder::SetFragmentBuffers(const Buffer::Type* buffers, const uint64_t* offsets, const ns::Range& range)
	{
		Validate();

		m_table->SetfragmentbuffersOffsetsWithrange(m_ptr, (const id<MTLBuffer>*)buffers, (NSUInteger const*)offsets, NSMakeRange(range.Location, range.Length));
	}

	void RenderCommandEncoder::SetFragmentTexture(const Texture& texture, NSUInteger index)
	{
		Validate();
		m_table->SetfragmenttextureAtindex(m_ptr, (id<MTLTexture>)texture.GetPtr(), index);
	}


	void RenderCommandEncoder::SetFragmentTextures(const Texture::Type* textures, const ns::Range& range)
	{
		Validate();

		m_table->SetfragmenttexturesWithrange(m_ptr, (const id<MTLTexture>*)textures, NSMakeRange(range.Location, range.Length));
	}

	void RenderCommandEncoder::SetFragmentSamplerState(const SamplerState& sampler, NSUInteger index)
	{
		Validate();
		m_table->SetfragmentsamplerstateAtindex(m_ptr, sampler.GetPtr(), index);
	}

	void RenderCommandEncoder::SetFragmentSamplerStates(const SamplerState::Type* samplers, const ns::Range& range)
	{
		Validate();

		m_table->SetfragmentsamplerstatesWithrange(m_ptr, samplers, NSMakeRange(range.Location, range.Length));
	}

	void RenderCommandEncoder::SetFragmentSamplerState(const SamplerState& sampler, float lodMinClamp, float lodMaxClamp, NSUInteger index)
	{
		Validate();
		m_table->SetfragmentsamplerstateLodminclampLodmaxclampAtindex(m_ptr, sampler.GetPtr(), lodMinClamp, lodMaxClamp, index);
	}

	void RenderCommandEncoder::SetFragmentSamplerStates(const SamplerState::Type* samplers, const float* lodMinClamps, const float* lodMaxClamps, const ns::Range& range)
	{
		Validate();

		m_table->SetfragmentsamplerstatesLodminclampsLodmaxclampsWithrange(m_ptr, samplers, lodMinClamps, lodMaxClamps, NSMakeRange(range.Location, range.Length));
	}

    void RenderCommandEncoder::SetBlendColor(float red, float green, float blue, float alpha)
    {
        Validate();
		m_table->SetblendcolorredGreenBlueAlpha(m_ptr, red, green, blue, alpha);
    }

    void RenderCommandEncoder::SetDepthStencilState(const DepthStencilState& depthStencilState)
    {
        Validate();
		m_table->Setdepthstencilstate(m_ptr, depthStencilState.GetPtr());
    }

    void RenderCommandEncoder::SetStencilReferenceValue(uint32_t referenceValue)
    {
        Validate();
		m_table->Setstencilreferencevalue(m_ptr, referenceValue);
    }

    void RenderCommandEncoder::SetStencilReferenceValue(uint32_t frontReferenceValue, uint32_t backReferenceValue)
    {
        Validate();
		m_table->SetstencilfrontreferencevalueBackreferencevalue(m_ptr, frontReferenceValue, backReferenceValue);
    }

    void RenderCommandEncoder::SetVisibilityResultMode(VisibilityResultMode mode, NSUInteger offset)
    {
        Validate();
		m_table->SetvisibilityresultmodeOffset(m_ptr, (MTLVisibilityResultMode)mode, offset);
    }

    void RenderCommandEncoder::SetColorStoreAction(StoreAction storeAction, NSUInteger colorAttachmentIndex)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
		m_table->SetcolorstoreactionAtindex(m_ptr, (MTLStoreAction) storeAction, colorAttachmentIndex);
        [(id<MTLRenderCommandEncoder>)m_ptr setColorStoreAction:MTLStoreAction(storeAction) atIndex:colorAttachmentIndex];
#endif
    }

    void RenderCommandEncoder::SetDepthStoreAction(StoreAction storeAction)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
		m_table->Setdepthstoreaction(m_ptr, (MTLStoreAction)storeAction);
#endif
    }

    void RenderCommandEncoder::SetStencilStoreAction(StoreAction storeAction)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
		m_table->Setstencilstoreaction(m_ptr, (MTLStoreAction)storeAction);
#endif
    }
	
	void RenderCommandEncoder::SetColorStoreActionOptions(StoreActionOptions storeAction, NSUInteger colorAttachmentIndex)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->SetcolorstoreactionoptionsAtindex(m_ptr, (MTLStoreActionOptions)storeAction, colorAttachmentIndex);
#endif
	}
	
	void RenderCommandEncoder::SetDepthStoreActionOptions(StoreActionOptions storeAction)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->Setdepthstoreactionoptions(m_ptr, (MTLStoreActionOptions)storeAction);
#endif
	}
	
	void RenderCommandEncoder::SetStencilStoreActionOptions(StoreActionOptions storeAction)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->Setstencilstoreactionoptions(m_ptr, (MTLStoreActionOptions)storeAction);
#endif
	}

    void RenderCommandEncoder::Draw(PrimitiveType primitiveType, NSUInteger vertexStart, NSUInteger vertexCount)
    {
        Validate();
		m_table->DrawprimitivesVertexstartVertexcount(m_ptr, (MTLPrimitiveType)primitiveType, vertexStart, vertexCount);
    }

    void RenderCommandEncoder::Draw(PrimitiveType primitiveType, NSUInteger vertexStart, NSUInteger vertexCount, NSUInteger instanceCount)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
		m_table->DrawprimitivesVertexstartVertexcountInstancecount(m_ptr, (MTLPrimitiveType)primitiveType, vertexStart, vertexCount, instanceCount);
#endif
    }

    void RenderCommandEncoder::Draw(PrimitiveType primitiveType, NSUInteger vertexStart, NSUInteger vertexCount, NSUInteger instanceCount, NSUInteger baseInstance)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
		m_table->DrawprimitivesVertexstartVertexcountInstancecountBaseinstance(m_ptr, (MTLPrimitiveType)primitiveType, vertexStart, vertexCount, instanceCount, baseInstance);
#endif
    }

    void RenderCommandEncoder::Draw(PrimitiveType primitiveType, Buffer indirectBuffer, NSUInteger indirectBufferOffset)
    {
        Validate();
		m_table->DrawprimitivesIndirectbufferIndirectbufferoffset(m_ptr, MTLPrimitiveType(primitiveType), (id<MTLBuffer>)indirectBuffer.GetPtr(), indirectBufferOffset);
    }

    void RenderCommandEncoder::DrawIndexed(PrimitiveType primitiveType, NSUInteger indexCount, IndexType indexType, const Buffer& indexBuffer, NSUInteger indexBufferOffset)
    {
        Validate();
		m_table->DrawindexedprimitivesIndexcountIndextypeIndexbufferIndexbufferoffset(m_ptr,MTLPrimitiveType(primitiveType), indexCount, (MTLIndexType)indexType, (id<MTLBuffer>)indexBuffer.GetPtr(), indexBufferOffset);
    }

    void RenderCommandEncoder::DrawIndexed(PrimitiveType primitiveType, NSUInteger indexCount, IndexType indexType, const Buffer& indexBuffer, NSUInteger indexBufferOffset, NSUInteger instanceCount)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
		m_table->DrawindexedprimitivesIndexcountIndextypeIndexbufferIndexbufferoffsetInstancecount(m_ptr,MTLPrimitiveType(primitiveType), indexCount, (MTLIndexType)indexType, (id<MTLBuffer>)indexBuffer.GetPtr(), indexBufferOffset, instanceCount);
#endif
    }

    void RenderCommandEncoder::DrawIndexed(PrimitiveType primitiveType, NSUInteger indexCount, IndexType indexType, const Buffer& indexBuffer, NSUInteger indexBufferOffset, NSUInteger instanceCount, NSUInteger baseVertex, NSUInteger baseInstance)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
		m_table->DrawindexedprimitivesIndexcountIndextypeIndexbufferIndexbufferoffsetInstancecountBasevertexBaseinstance(m_ptr,MTLPrimitiveType(primitiveType), indexCount, (MTLIndexType)indexType, (id<MTLBuffer>)indexBuffer.GetPtr(), indexBufferOffset, instanceCount, baseVertex, baseInstance);
#endif
    }

    void RenderCommandEncoder::DrawIndexed(PrimitiveType primitiveType, IndexType indexType, const Buffer& indexBuffer, NSUInteger indexBufferOffset, const Buffer& indirectBuffer, NSUInteger indirectBufferOffset)
    {
        Validate();
		m_table->DrawindexedprimitivesIndextypeIndexbufferIndexbufferoffsetIndirectbufferIndirectbufferoffset(m_ptr, MTLPrimitiveType(primitiveType), MTLIndexType(indexType), (id<MTLBuffer>)indexBuffer.GetPtr(), indexBufferOffset, (id<MTLBuffer>)indirectBuffer.GetPtr(), indirectBufferOffset);
    }

    void RenderCommandEncoder::TextureBarrier()
    {
        Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_11)
		m_table->Texturebarrier(m_ptr);
#endif
    }

    void RenderCommandEncoder::UpdateFence(const Fence& fence, RenderStages afterStages)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
		m_table->UpdatefenceAfterstages(m_ptr, fence.GetPtr(), (MTLRenderStages)afterStages);
#endif
    }

    void RenderCommandEncoder::WaitForFence(const Fence& fence, RenderStages beforeStages)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
		m_table->WaitforfenceBeforestages(m_ptr, fence.GetPtr(), (MTLRenderStages)beforeStages);
#endif
    }

    void RenderCommandEncoder::SetTessellationFactorBuffer(const Buffer& buffer, NSUInteger offset, NSUInteger instanceStride)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
		m_table->SettessellationfactorbufferOffsetInstancestride(m_ptr, (id<MTLBuffer>)buffer.GetPtr(), offset, instanceStride);
#endif
    }

    void RenderCommandEncoder::SetTessellationFactorScale(float scale)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
		m_table->Settessellationfactorscale(m_ptr, scale);
#endif
    }

    void RenderCommandEncoder::DrawPatches(NSUInteger numberOfPatchControlPoints, NSUInteger patchStart, NSUInteger patchCount, const Buffer& patchIndexBuffer, NSUInteger patchIndexBufferOffset, NSUInteger instanceCount, NSUInteger baseInstance)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
		m_table->DrawpatchesPatchstartPatchcountPatchindexbufferPatchindexbufferoffsetInstancecountBaseinstance(m_ptr, numberOfPatchControlPoints, patchStart, patchCount, (id<MTLBuffer>)patchIndexBuffer.GetPtr(), patchIndexBufferOffset, instanceCount, baseInstance);
#endif
    }

    void RenderCommandEncoder::DrawPatches(NSUInteger numberOfPatchControlPoints, const Buffer& patchIndexBuffer, NSUInteger patchIndexBufferOffset, const Buffer& indirectBuffer, NSUInteger indirectBufferOffset)
    {
        Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_12)
		m_table->DrawpatchesPatchindexbufferPatchindexbufferoffsetIndirectbufferIndirectbufferoffset(m_ptr, numberOfPatchControlPoints, (id<MTLBuffer>)patchIndexBuffer.GetPtr(), patchIndexBufferOffset, (id<MTLBuffer>)indirectBuffer.GetPtr(), indirectBufferOffset);
#endif
    }

    void RenderCommandEncoder::DrawIndexedPatches(NSUInteger numberOfPatchControlPoints, NSUInteger patchStart, NSUInteger patchCount, const Buffer& patchIndexBuffer, NSUInteger patchIndexBufferOffset, const Buffer& controlPointIndexBuffer, NSUInteger controlPointIndexBufferOffset, NSUInteger instanceCount, NSUInteger baseInstance)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
		m_table->DrawindexedpatchesPatchstartPatchcountPatchindexbufferPatchindexbufferoffsetControlpointindexbufferControlpointindexbufferoffsetInstancecountBaseinstance(m_ptr, numberOfPatchControlPoints, patchStart, patchCount, (id<MTLBuffer>)patchIndexBuffer.GetPtr(), patchIndexBufferOffset, (id<MTLBuffer>)controlPointIndexBuffer.GetPtr(), controlPointIndexBufferOffset, instanceCount, baseInstance);
#endif
    }

    void RenderCommandEncoder::DrawIndexedPatches(NSUInteger numberOfPatchControlPoints, const Buffer& patchIndexBuffer, NSUInteger patchIndexBufferOffset, const Buffer& controlPointIndexBuffer, NSUInteger controlPointIndexBufferOffset, const Buffer& indirectBuffer, NSUInteger indirectBufferOffset)
    {
        Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_12)
		m_table->DrawindexedpatchesPatchindexbufferPatchindexbufferoffsetControlpointindexbufferControlpointindexbufferoffsetIndirectbufferIndirectbufferoffset(m_ptr, numberOfPatchControlPoints, (id<MTLBuffer>)patchIndexBuffer.GetPtr(), patchIndexBufferOffset, (id<MTLBuffer>)controlPointIndexBuffer.GetPtr(), controlPointIndexBufferOffset, (id<MTLBuffer>)indirectBuffer.GetPtr(), indirectBufferOffset);
#endif
    }
	
	void RenderCommandEncoder::UseResource(const Resource& resource, ResourceUsage usage)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->UseresourceUsage(m_ptr, resource.GetPtr(), (MTLResourceUsage)usage);
#endif
	}
	
	void RenderCommandEncoder::UseResources(const Resource::Type* resource, NSUInteger count, ResourceUsage usage)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->UseresourcesCountUsage(m_ptr, resource, count, (MTLResourceUsage)usage);
#endif
	}
	
	void RenderCommandEncoder::UseHeap(const Heap& heap)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->Useheap(m_ptr, heap.GetPtr());
#endif
	}
	
	void RenderCommandEncoder::UseHeaps(const Heap::Type* heap, NSUInteger count)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->UseheapsCount(m_ptr, heap, count);
#endif
	}
	
	NSUInteger RenderCommandEncoder::GetTileWidth()
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return m_table->tileWidth(m_ptr);
#else
		return 0;
#endif
	}
	
	NSUInteger RenderCommandEncoder::GetTileHeight()
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		return m_table->tileHeight(m_ptr);
#else
		return 0;
#endif
	}
	
	void RenderCommandEncoder::SetTileData(const void* bytes, NSUInteger length, NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		m_table->SetTilebytesLengthAtindex(m_ptr, bytes, length, index);
#endif
	}
	
	void RenderCommandEncoder::SetTileBuffer(const Buffer& buffer, NSUInteger offset, NSUInteger index)
	{
		Validate();
		m_table->SetTilebufferOffsetAtindex(m_ptr, (id<MTLBuffer>)buffer.GetPtr(), offset, index);
	}
	void RenderCommandEncoder::SetTileBufferOffset(NSUInteger offset, NSUInteger index)
	{
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		Validate();
		m_table->SetTilebufferoffsetAtindex(m_ptr, offset, index);
#endif
	}
	
	void RenderCommandEncoder::SetTileBuffers(const Buffer::Type* buffers, const uint64_t* offsets, const ns::Range& range)
	{
		Validate();
		
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		m_table->SetTilebuffersOffsetsWithrange(m_ptr, (const id<MTLBuffer>*)buffers, (NSUInteger const*)offsets, NSMakeRange(range.Location, range.Length));
#endif
	}
	
	void RenderCommandEncoder::SetTileTexture(const Texture& texture, NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		m_table->SetTiletextureAtindex(m_ptr, (id<MTLTexture>)texture.GetPtr(), index);
#endif
	}
	
	
	void RenderCommandEncoder::SetTileTextures(const Texture::Type* textures, const ns::Range& range)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		m_table->SetTiletexturesWithrange(m_ptr, (const id<MTLTexture>*)textures, NSMakeRange(range.Location, range.Length));
#endif
	}
	
	void RenderCommandEncoder::SetTileSamplerState(const SamplerState& sampler, NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		m_table->SetTilesamplerstateAtindex(m_ptr, sampler.GetPtr(), index);
#endif
	}
	
	void RenderCommandEncoder::SetTileSamplerStates(const SamplerState::Type* samplers, const ns::Range& range)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		m_table->SetTilesamplerstatesWithrange(m_ptr, samplers, NSMakeRange(range.Location, range.Length));
#endif
	}
	
	void RenderCommandEncoder::SetTileSamplerState(const SamplerState& sampler, float lodMinClamp, float lodMaxClamp, NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		m_table->SetTilesamplerstateLodminclampLodmaxclampAtindex(m_ptr, sampler.GetPtr(), lodMinClamp, lodMaxClamp, index);
#endif
	}
	
	void RenderCommandEncoder::SetTileSamplerStates(const SamplerState::Type* samplers, const float* lodMinClamps, const float* lodMaxClamps, const ns::Range& range)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		m_table->SetTilesamplerstatesLodminclampsLodmaxclampsWithrange(m_ptr, samplers, lodMinClamps, lodMaxClamps, NSMakeRange(range.Location, range.Length));
#endif
	}
	
	void RenderCommandEncoder::DispatchThreadsPerTile(Size const& threadsPerTile)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		m_table->dispatchThreadsPerTile(m_ptr, threadsPerTile);
#endif
	}
	
	void RenderCommandEncoder::SetThreadgroupMemoryLength(NSUInteger length, NSUInteger offset, NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
		m_table->setThreadgroupMemoryLength(m_ptr, length, offset, index);
#endif
	}
}


MTLPP_END
