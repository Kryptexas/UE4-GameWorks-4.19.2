// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

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
#include "VulkanRHIPrivate.h"
#include "ArrayView.h"

class FVulkanComputePipeline;

// Common Pipeline state
class FVulkanCommonPipelineState : public VulkanRHI::FDeviceChild
{
public:
	FVulkanCommonPipelineState(FVulkanDevice* InDevice)
		: VulkanRHI::FDeviceChild(InDevice)
#if !VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
		, DSRingBuffer(InDevice)
#endif
	{
	}

protected:
#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
	inline void Bind(VkCommandBuffer CmdBuffer, VkPipelineLayout PipelineLayout, VkPipelineBindPoint BindPoint)
	{
		VulkanRHI::vkCmdBindDescriptorSets(CmdBuffer,
			BindPoint,
			PipelineLayout,
			0, DescriptorSetHandles.Num(), DescriptorSetHandles.GetData(),
			0, nullptr);
	}
#endif
	FVulkanDescriptorSetWriteContainer DSWriteContainer;
#if !VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
	FOLDVulkanDescriptorSetRingBuffer DSRingBuffer;
#endif

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
	FVulkanTypedDescriptorPoolSet* CurrentDescriptorPoolSet = nullptr;
	TArray<VkDescriptorSet> DescriptorSetHandles;
#endif
};


class FVulkanComputePipelineState : public FVulkanCommonPipelineState
{
public:
	FVulkanComputePipelineState(FVulkanDevice* InDevice, FVulkanComputePipeline* InComputePipeline);
	~FVulkanComputePipelineState()
	{
		ComputePipeline->Release();
	}

	void Reset()
	{
		PackedUniformBuffersDirty = PackedUniformBuffersMask;
#if VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
#else
		DSRingBuffer.Reset();
#endif
		DSWriter.ResetDirty();
	}

	inline void SetStorageBuffer(uint32 BindPoint, VkBuffer Buffer, uint32 Offset, uint32 Size, VkBufferUsageFlags UsageFlags)
	{
		check((UsageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) == VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		DSWriter.WriteStorageBuffer(BindPoint, Buffer, Offset, Size);
	}

	inline void SetUAVTexelBufferViewState(uint32 BindPoint, FVulkanBufferView* View)
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
			(TextureBase->Surface.GetFullAspectMask() & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) != 0 ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	inline void SetSRVBufferViewState(uint32 BindPoint, FVulkanBufferView* View)
	{
		check(View && (View->Flags & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT) == VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT);
		DSWriter.WriteUniformTexelBuffer(BindPoint, View);
	}

	inline void SetSRVTextureView(uint32 BindPoint, const FVulkanTextureView& TextureView, VkImageLayout Layout)
	{
		ensure(Layout == VK_IMAGE_LAYOUT_GENERAL);
		DSWriter.WriteImage(BindPoint, TextureView.View, Layout);
	}

	inline void SetSamplerState(uint32 BindPoint, FVulkanSamplerState* Sampler)
	{
		check(Sampler);
		DSWriter.WriteSampler(BindPoint, Sampler->Sampler);
	}

	inline void SetShaderParameter(uint32 BufferIndex, uint32 ByteOffset, uint32 NumBytes, const void* NewValue)
	{
		PackedUniformBuffers.SetPackedGlobalParameter(BufferIndex, ByteOffset, NumBytes, NewValue, PackedUniformBuffersDirty);
	}

	inline void SetUniformBufferConstantData(uint32 BindPoint, const TArray<uint8>& ConstantData)
	{
		PackedUniformBuffers.SetEmulatedUniformBufferIntoPacked(BindPoint, ConstantData, PackedUniformBuffersDirty);
	}

	inline void SetUniformBuffer(uint32 BindPoint, const FVulkanUniformBuffer* UniformBuffer)
	{
		if ((UniformBuffersWithDataMask & (1ULL << (uint64)BindPoint)) != 0)
		{
			DSWriter.WriteUniformBuffer(BindPoint, UniformBuffer->GetHandle(), UniformBuffer->GetOffset(), UniformBuffer->GetSize());
		}
	}

#if VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
	TArrayView<VkDescriptorSet> UpdateDescriptorSets(FVulkanCommandListContext* CmdListContext, FVulkanCmdBuffer* CmdBuffer, FVulkanGlobalUniformPool* GlobalUniformPool);

	inline void BindDescriptorSets(VkCommandBuffer CmdBuffer, const TArrayView<VkDescriptorSet>& DescriptorSets)
	{
		VulkanRHI::vkCmdBindDescriptorSets(CmdBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			ComputePipeline->GetLayout().GetPipelineLayout(),
			0, DescriptorSets.Num(), DescriptorSets.GetData(),
			0, nullptr);
	}
#else
	bool UpdateDescriptorSets(FVulkanCommandListContext* CmdListContext, FVulkanCmdBuffer* CmdBuffer, FVulkanGlobalUniformPool* GlobalUniformPool);

	inline void BindDescriptorSets(VkCommandBuffer CmdBuffer)
	{
#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
		Bind(CmdBuffer, ComputePipeline->GetLayout().GetPipelineLayout(), VK_PIPELINE_BIND_POINT_COMPUTE);
#else
		check(DSRingBuffer.CurrDescriptorSets);
		DSRingBuffer.CurrDescriptorSets->Bind(CmdBuffer, ComputePipeline->GetLayout().GetPipelineLayout(), VK_PIPELINE_BIND_POINT_COMPUTE);
#endif
	}
#endif

protected:
	FPackedUniformBuffers PackedUniformBuffers;
	uint64 PackedUniformBuffersMask;
	uint64 PackedUniformBuffersDirty;
	FVulkanDescriptorSetWriter DSWriter;
	uint64 UniformBuffersWithDataMask;

	FVulkanComputePipeline* ComputePipeline;

	void CreateDescriptorWriteInfos();

	friend class FVulkanPendingComputeState;
	friend class FVulkanCommandListContext;
};

class FVulkanGfxPipelineState : public FVulkanCommonPipelineState
{
public:
	FVulkanGfxPipelineState(FVulkanDevice* InDevice, FVulkanGraphicsPipelineState* InGfxPipeline, FVulkanBoundShaderState* InBSS);
	~FVulkanGfxPipelineState()
	{
		GfxPipeline->Release();
		BSS->Release();
	}

	inline void SetStorageBuffer(EShaderFrequency Stage, uint32 BindPoint, VkBuffer Buffer, uint32 Offset, uint32 Size, VkBufferUsageFlags UsageFlags)
	{
		check((UsageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) == VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		DSWriter[Stage].WriteStorageBuffer(BindPoint, Buffer, Offset, Size);
	}

	inline void SetUAVTexelBufferViewState(EShaderFrequency Stage, uint32 BindPoint, FVulkanBufferView* View)
	{
		check(View && (View->Flags & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT) == VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT);
		DSWriter[Stage].WriteStorageTexelBuffer(BindPoint, View);
	}

	inline void SetUAVTextureView(EShaderFrequency Stage, uint32 BindPoint, const FVulkanTextureView& TextureView, VkImageLayout Layout)
	{
		DSWriter[Stage].WriteStorageImage(BindPoint, TextureView.View, Layout);
	}

	inline void SetTexture(EShaderFrequency Stage, uint32 BindPoint, const FVulkanTextureBase* TextureBase)
	{
		check(TextureBase);
		DSWriter[Stage].WriteImage(BindPoint, TextureBase->PartialView->View,
			(TextureBase->Surface.GetFullAspectMask() & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) != 0 ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	inline void SetSRVBufferViewState(EShaderFrequency Stage, uint32 BindPoint, FVulkanBufferView* View)
	{
		check(View && (View->Flags & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT) == VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT);
		DSWriter[Stage].WriteUniformTexelBuffer(BindPoint, View);
	}

	inline void SetSRVTextureView(EShaderFrequency Stage, uint32 BindPoint, const FVulkanTextureView& TextureView, VkImageLayout Layout)
	{
		DSWriter[Stage].WriteImage(BindPoint, TextureView.View, Layout);
	}

	inline void SetSamplerState(EShaderFrequency Stage, uint32 BindPoint, FVulkanSamplerState* Sampler)
	{
		check(Sampler && Sampler->Sampler != VK_NULL_HANDLE);
		DSWriter[Stage].WriteSampler(BindPoint, Sampler->Sampler);
	}

	inline void SetShaderParameter(EShaderFrequency Stage, uint32 BufferIndex, uint32 ByteOffset, uint32 NumBytes, const void* NewValue)
	{
		PackedUniformBuffers[Stage].SetPackedGlobalParameter(BufferIndex, ByteOffset, NumBytes, NewValue, PackedUniformBuffersDirty[Stage]);
	}

	inline void SetUniformBufferConstantData(EShaderFrequency Stage, uint32 BindPoint, const TArray<uint8>& ConstantData)
	{
		PackedUniformBuffers[Stage].SetEmulatedUniformBufferIntoPacked(BindPoint, ConstantData, PackedUniformBuffersDirty[Stage]);
	}

	inline void SetUniformBuffer(EShaderFrequency Stage, uint32 BindPoint, const FVulkanUniformBuffer* UniformBuffer)
	{
		if ((UniformBuffersWithDataMask[Stage] & (1ULL << (uint64)BindPoint)) != 0)
		{
			DSWriter[Stage].WriteUniformBuffer(BindPoint, UniformBuffer->GetHandle(), UniformBuffer->GetOffset(), UniformBuffer->GetSize());
		}
	}

#if VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
	TArrayView<VkDescriptorSet> UpdateDescriptorSets(FVulkanCommandListContext* CmdListContext, FVulkanCmdBuffer* CmdBuffer, FVulkanGlobalUniformPool* GlobalUniformPool);

	inline void BindDescriptorSets(VkCommandBuffer CmdBuffer, const TArrayView<VkDescriptorSet>& DescriptorSets)
	{
		VulkanRHI::vkCmdBindDescriptorSets(CmdBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			GfxPipeline->Pipeline->GetLayout().GetPipelineLayout(),
			0, DescriptorSets.Num(), DescriptorSets.GetData(),
			0, nullptr);
	}
#else
	bool UpdateDescriptorSets(FVulkanCommandListContext* CmdListContext, FVulkanCmdBuffer* CmdBuffer, FVulkanGlobalUniformPool* GlobalUniformPool);

	inline void BindDescriptorSets(VkCommandBuffer CmdBuffer)
	{
#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
		Bind(CmdBuffer, GfxPipeline->Pipeline->GetLayout().GetPipelineLayout(), VK_PIPELINE_BIND_POINT_GRAPHICS);
#else
		check(DSRingBuffer.CurrDescriptorSets);
		DSRingBuffer.CurrDescriptorSets->Bind(CmdBuffer, GfxPipeline->Pipeline->GetLayout().GetPipelineLayout(), VK_PIPELINE_BIND_POINT_GRAPHICS);
#endif
	}
#endif

	void Reset()
	{
		FMemory::Memcpy(PackedUniformBuffersDirty, PackedUniformBuffersMask);
#if VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
#else
		DSRingBuffer.Reset();
#endif
		for (int32 Index = 0; Index < SF_Compute; ++Index)
		{
			DSWriter[Index].ResetDirty();
		}
	}

	inline void Verify()
	{
#if 0
		int32 TotalNumWrites = 0;
		int32 TotalNumBuffer = 0;
		int32 TotalNumImages = 0;

		for (int32 Index = 0; Index < SF_Compute; ++Index)
		{
			FVulkanShader* Shader = GfxPipeline->Shaders[Index];
			if (!Shader)
			{
				ensure(DSWriter[Index].NumWrites == 0);
				continue;
			}

			ensure(Shader->GetCodeHeader().NEWDescriptorInfo.DescriptorTypes.Num() == DSWriter[Index].NumWrites);
			TotalNumWrites += Shader->GetCodeHeader().NEWDescriptorInfo.DescriptorTypes.Num();
			TotalNumBuffer += Shader->GetCodeHeader().NEWDescriptorInfo.NumBufferInfos;
			TotalNumImages += Shader->GetCodeHeader().NEWDescriptorInfo.NumImageInfos;
		}
		ensure(TotalNumWrites == DSWriteContainer.DescriptorWrites.Num());
		ensure(TotalNumBuffer == DSWriteContainer.DescriptorBufferInfo.Num());
		ensure(TotalNumImages == DSWriteContainer.DescriptorImageInfo.Num());
#endif
	}

protected:
	FPackedUniformBuffers PackedUniformBuffers[SF_Compute];
	uint64 PackedUniformBuffersMask[SF_Compute];
	uint64 PackedUniformBuffersDirty[SF_Compute];
	uint64 UniformBuffersWithDataMask[SF_Compute];
	FVulkanDescriptorSetWriter DSWriter[SF_Compute];

	FVulkanGraphicsPipelineState* GfxPipeline;
	FVulkanBoundShaderState* BSS;
	int32 ID;

	void CreateDescriptorWriteInfos();

	friend class FVulkanPendingGfxState;
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
