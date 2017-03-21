// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanStatePipeline.cpp: Vulkan pipeline state implementation.
=============================================================================*/

#include "VulkanRHIPrivate.h"
#include "VulkanPipelineState.h"
#include "VulkanResources.h"
#include "VulkanPipeline.h"
#include "VulkanContext.h"
#include "VulkanPendingState.h"
#include "VulkanPipeline.h"


FVulkanComputePipelineState::FVulkanComputePipelineState(FVulkanDevice* InDevice, FVulkanComputePipeline* InComputePipeline)
	: FVulkanCommonPipelineState(InDevice)
	, PackedUniformBufferStagingDirty(0)
	, ComputePipeline(InComputePipeline)
{
	//ShaderHash = ComputeShader->CodeHeader.SourceHash;

	PackedUniformBuffers.Init(&InComputePipeline->GetShaderCodeHeader(), PackedUniformBufferStagingDirty);

	CreateDescriptorWriteInfos();
}

void FVulkanComputePipelineState::CreateDescriptorWriteInfos()
{
	check(DSWriteContainer.DescriptorWrites.Num() == 0);

	const FVulkanCodeHeader& CodeHeader = ComputePipeline->GetShaderCodeHeader();

	DSWriteContainer.DescriptorWrites.AddZeroed(CodeHeader.NEWDescriptorInfo.DescriptorTypes.Num());
	DSWriteContainer.DescriptorImageInfo.AddZeroed(CodeHeader.NEWDescriptorInfo.NumImageInfos);
	DSWriteContainer.DescriptorBufferInfo.AddZeroed(CodeHeader.NEWDescriptorInfo.NumBufferInfos);

	VkSampler DefaultSampler = Device->GetDefaultSampler();
	for (int32 Index = 0; Index < DSWriteContainer.DescriptorImageInfo.Num(); ++Index)
	{
		// Texture.Load() still requires a default sampler...
		DSWriteContainer.DescriptorImageInfo[Index].sampler = DefaultSampler;
		DSWriteContainer.DescriptorImageInfo[Index].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	}

	VkWriteDescriptorSet* CurrentDescriptorWrite = DSWriteContainer.DescriptorWrites.GetData();
	VkDescriptorImageInfo* CurrentImageInfo = DSWriteContainer.DescriptorImageInfo.GetData();
	VkDescriptorBufferInfo* CurrentBufferInfo = DSWriteContainer.DescriptorBufferInfo.GetData();

	DSWriter.SetupDescriptorWrites(CodeHeader.NEWDescriptorInfo, CurrentDescriptorWrite, CurrentImageInfo, CurrentBufferInfo);
}

bool FVulkanComputePipelineState::UpdateDescriptorSets(FVulkanCommandListContext* CmdListContext, FVulkanCmdBuffer* CmdBuffer, FVulkanGlobalUniformPool* GlobalUniformPool)
{
#if VULKAN_ENABLE_AGGRESSIVE_STATS
	SCOPE_CYCLE_COUNTER(STAT_VulkanUpdateDescriptorSets);
#endif
	int32 WriteIndex = 0;

	DSRingBuffer.CurrDescriptorSets = DSRingBuffer.RequestDescriptorSets(CmdListContext, CmdBuffer, ComputePipeline->GetLayout());
	if (!DSRingBuffer.CurrDescriptorSets)
	{
		return false;
	}

	const FVulkanDescriptorSets::FDescriptorSetArray& DescriptorSetHandles = DSRingBuffer.CurrDescriptorSets->GetHandles();
	int32 DescriptorSetIndex = 0;

	FVulkanUniformBufferUploader* UniformBufferUploader = CmdListContext->GetUniformBufferUploader();
	uint8* CPURingBufferBase = (uint8*)UniformBufferUploader->GetCPUMappedPointer();

	const VkDescriptorSet DescriptorSet = DescriptorSetHandles[DescriptorSetIndex];
	++DescriptorSetIndex;

	bool bRequiresPackedUBUpdate = (PackedUniformBufferStagingDirty != 0);
	if (bRequiresPackedUBUpdate)
	{
		SCOPE_CYCLE_COUNTER(STAT_VulkanApplyDSUniformBuffers);
		UpdatePackedUniformBuffers(Device->GetLimits().minUniformBufferOffsetAlignment, ComputePipeline->GetShaderCodeHeader(), PackedUniformBuffers, DSWriter, UniformBufferUploader, CPURingBufferBase, PackedUniformBufferStagingDirty);
		PackedUniformBufferStagingDirty = 0;
	}

	bool bRequiresNonPackedUBUpdate = (DSWriter.DirtyMask != 0);
	if (!bRequiresNonPackedUBUpdate && !bRequiresPackedUBUpdate)
	{
		//#todo-rco: Skip this desc set writes and only call update for the modified ones!
		//continue;
		int x = 0;
	}

	DSWriter.SetDescriptorSet(DescriptorSet);

#if VULKAN_ENABLE_AGGRESSIVE_STATS
	INC_DWORD_STAT_BY(STAT_VulkanNumUpdateDescriptors, DSWriteContainer.DescriptorWrites.Num());
	INC_DWORD_STAT_BY(STAT_VulkanNumDescSets, DescriptorSetIndex);
#endif

	{
#if VULKAN_ENABLE_AGGRESSIVE_STATS
		SCOPE_CYCLE_COUNTER(STAT_VulkanVkUpdateDS);
#endif
		VulkanRHI::vkUpdateDescriptorSets(Device->GetInstanceHandle(), DSWriteContainer.DescriptorWrites.Num(), DSWriteContainer.DescriptorWrites.GetData(), 0, nullptr);
	}

	return true;
}
