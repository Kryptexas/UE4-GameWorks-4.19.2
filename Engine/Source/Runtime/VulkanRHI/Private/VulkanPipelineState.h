// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanPipelineState.h: Vulkan pipeline state definitions.
=============================================================================*/

#pragma once

#include "VulkanConfiguration.h"
#include "VulkanMemory.h"
#include "VulkanCommandBuffer.h"
#include "VulkanDescriptorSets.h"
#include "VulkanGlobalUniformBuffer.h"
#include "VulkanPipeline.h"

class FVulkanComputePipeline;

// Common Pipeline state
class FVulkanCommonPipelineState : public VulkanRHI::FDeviceChild
{
public:
	FVulkanCommonPipelineState(FVulkanDevice* InDevice)
		: VulkanRHI::FDeviceChild(InDevice)
		, DSRingBuffer(InDevice)
	{
	}

protected:
	FVulkanDescriptorSetWriteContainer DSWriteContainer;
	FVulkanDescriptorSetRingBuffer DSRingBuffer;
};


class FVulkanComputePipelineState : public FVulkanCommonPipelineState
{
public:
	FVulkanComputePipelineState(FVulkanDevice* InDevice, FVulkanComputePipeline* InComputePipeline);

	inline void SetUAVBufferViewState(uint32 BindPoint, FVulkanBufferView* View)
	{
		check(View && (View->Flags & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT) == VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT);
		DSWriter.WriteStorageTexelBuffer(BindPoint, View);
	}

	inline void SetUAVTextureView(uint32 BindPoint, const FVulkanTextureView& TextureView)
	{
		DSWriter.WriteStorageImage(BindPoint, TextureView.View, VK_IMAGE_LAYOUT_GENERAL);
	}

	inline void SetTexture(uint32 BindPoint, const FVulkanTextureBase* TextureBase)
	{
		check(TextureBase);
		DSWriter.WriteImage(BindPoint, TextureBase->PartialView->View,
			(TextureBase->Surface.GetFullAspectMask() & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) != 0 ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL);
	}

	inline void SetSRVBufferViewState(uint32 BindPoint, FVulkanBufferView* View)
	{
		check(View && (View->Flags & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT) == VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT);
		DSWriter.WriteUniformTexelBuffer(BindPoint, View);
	}

	inline void SetSRVTextureView(uint32 BindPoint, const FVulkanTextureView& TextureView)
	{
		ensure(0);
	}

	inline void SetSamplerState(uint32 BindPoint, FVulkanSamplerState* Sampler)
	{
		check(Sampler);
		DSWriter.WriteSampler(BindPoint, Sampler->Sampler);
	}

	inline void SetShaderParameter(uint32 BufferIndex, uint32 ByteOffset, uint32 NumBytes, const void* NewValue)
	{
		PackedUniformBuffers.SetPackedGlobalParameter(BufferIndex, ByteOffset, NumBytes, NewValue, PackedUniformBufferStagingDirty);
	}

	inline void SetUniformBufferConstantData(uint32 BindPoint, const TArray<uint8>& ConstantData)
	{
		PackedUniformBuffers.SetEmulatedUniformBufferIntoPacked(BindPoint, ConstantData, PackedUniformBufferStagingDirty);
	}

	inline void SetUniformBuffer(uint32 BindPoint, const FVulkanUniformBuffer* UniformBuffer)
	{
		//#todo-rco
		ensure(0);
	}

	bool UpdateDescriptorSets(FVulkanCommandListContext* CmdListContext, FVulkanCmdBuffer* CmdBuffer, FVulkanGlobalUniformPool* GlobalUniformPool);

	inline void BindDescriptorSets(VkCommandBuffer CmdBuffer)
	{
		check(DSRingBuffer.CurrDescriptorSets);
		DSRingBuffer.CurrDescriptorSets->Bind(CmdBuffer, ComputePipeline->GetLayout().GetPipelineLayout(), VK_PIPELINE_BIND_POINT_COMPUTE);
	}

protected:
	FPackedUniformBuffers PackedUniformBuffers;
	uint64 PackedUniformBufferStagingDirty;
	FVulkanDescriptorSetWriter DSWriter;

	FVulkanComputePipeline* ComputePipeline;

	void CreateDescriptorWriteInfos();

	friend class FVulkanPendingComputeState;
	friend class FVulkanCommandListContext;
};


static inline void UpdatePackedUniformBuffers(VkDeviceSize UBOffsetAlignment, const FVulkanCodeHeader& CodeHeader, FPackedUniformBuffers& PackedUniformBuffers,
	FVulkanDescriptorSetWriter& DescriptorWriteSet, FVulkanUniformBufferUploader* UniformBufferUploader, uint8* CPURingBufferBase, uint64 RemainingPackedUniformsMask)
{
	int32 PackedUBIndex = 0;
	while (RemainingPackedUniformsMask)
	{
		if (RemainingPackedUniformsMask & 1)
		{
			const FPackedUniformBuffers::FPackedBuffer& StagedUniformBuffer = PackedUniformBuffers.GetBuffer(PackedUBIndex);
			int32 BindingIndex = CodeHeader.NEWPackedUBToVulkanBindingIndices[PackedUBIndex].VulkanBindingIndex;

			const int32 UBSize = StagedUniformBuffer.Num();

			// get offset into the RingBufferBase pointer
			uint64 RingBufferOffset = UniformBufferUploader->AllocateMemory(UBSize, UBOffsetAlignment);

			// get location in the ring buffer to use
			FMemory::Memcpy(CPURingBufferBase + RingBufferOffset, StagedUniformBuffer.GetData(), UBSize);

			DescriptorWriteSet.WriteUniformBuffer(BindingIndex, UniformBufferUploader->GetCPUBufferHandle(), RingBufferOffset + UniformBufferUploader->GetCPUBufferOffset(), UBSize);
		}
		RemainingPackedUniformsMask = RemainingPackedUniformsMask >> 1;
		++PackedUBIndex;
	}
}
