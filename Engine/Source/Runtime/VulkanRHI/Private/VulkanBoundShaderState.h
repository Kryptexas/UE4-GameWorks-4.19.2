// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanBoundShaderState.h: 
=============================================================================*/

#pragma once 

#include "VulkanResources.h"
#include "../Private/VulkanDescriptorSets.h"

/**
* Combined shader state and vertex definition for rendering geometry.
* Each unique instance consists of a vertex decl, vertex shader, and pixel shader.
*/
class FVulkanBoundShaderState : public FVulkanDescriptorSetRingBuffer, public FVulkanDescriptorSetWriteContainer, public FRHIBoundShaderState
{
public:
	FVulkanBoundShaderState(
		FVulkanDevice* InDevice,
		FVertexDeclarationRHIParamRef InVertexDeclarationRHI,
		FVertexShaderRHIParamRef InVertexShaderRHI,
		FPixelShaderRHIParamRef InPixelShaderRHI,
		FHullShaderRHIParamRef InHullShaderRHI,
		FDomainShaderRHIParamRef InDomainShaderRHI,
		FGeometryShaderRHIParamRef InGeometryShaderRHI);

	virtual ~FVulkanBoundShaderState();

	// Called when the shader is set by the engine as current pending state
	// After the shader is set, it is expected that all require states are provided/set by the engine
	void ResetState();

	const FVulkanLayout& GetLayout() const
	{
		return Layout;
	}

	class FVulkanGfxPipeline* PrepareForDraw(class FVulkanRenderPass* RenderPass, const struct FVulkanPipelineGraphicsKey& PipelineKey, uint32 VertexInputKey, const struct FVulkanGfxPipelineState& State);

	bool UpdateDescriptorSets(FVulkanCommandListContext* CmdListContext, FVulkanCmdBuffer* CmdBuffer, FVulkanGlobalUniformPool* GlobalUniformPool);

	void BindDescriptorSets(FVulkanCmdBuffer* Cmd);

	inline void BindVertexStreams(FVulkanCmdBuffer* Cmd, const void* VertexStreams)
	{
		if (bDirtyVertexStreams)
		{
			InternalBindVertexStreams(Cmd, VertexStreams);
			bDirtyVertexStreams = false;
		}
	}

	inline void MarkDirtyVertexStreams()
	{
		bDirtyVertexStreams = true;
	}

	inline const FVulkanVertexInputStateInfo& GetVertexInputStateInfo() const
	{
		check(VertexInputStateInfo.Info.sType == VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
		return VertexInputStateInfo;
	}

	inline FSHAHash const* GetShaderHashes() const
	{
		return ShaderHashes;
	}

	// Binding
	inline void SetShaderParameter(EShaderFrequency Stage, uint32 BufferIndex, uint32 ByteOffset, uint32 NumBytes, const void* NewValue)
	{
#if VULKAN_ENABLE_AGGRESSIVE_STATS
		SCOPE_CYCLE_COUNTER(STAT_VulkanSetShaderParamTime);
#endif
		NEWPackedUniformBufferStaging[Stage].SetPackedGlobalParameter(BufferIndex, ByteOffset, NumBytes, NewValue, NEWPackedUniformBufferStagingDirty[Stage]);
	}

	inline void SetTexture(EShaderFrequency Stage, uint32 BindPoint, const FVulkanTextureBase* VulkanTextureBase)
	{
		check(VulkanTextureBase);
		DescriptorWriteSet[Stage].WriteImage(BindPoint, VulkanTextureBase->PartialView->View,
			(VulkanTextureBase->Surface.GetFullAspectMask() & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) != 0 ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL);
	}

	inline void SetSamplerState(EShaderFrequency Stage, uint32 BindPoint, FVulkanSamplerState* Sampler)
	{
		check(Sampler);
		DescriptorWriteSet[Stage].WriteSampler(BindPoint, Sampler->Sampler);
	}

	inline void SetBufferViewState(EShaderFrequency Stage, uint32 BindPoint, FVulkanBufferView* View)
	{
		check(View && (View->Flags & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT) == VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT);
		DescriptorWriteSet[Stage].WriteUniformTexelBuffer(BindPoint, View);
	}

	inline void SetTextureView(EShaderFrequency Stage, uint32 BindPoint, const FVulkanTextureView& TextureView)
	{
		DescriptorWriteSet[Stage].WriteImage(BindPoint, TextureView.View, VK_IMAGE_LAYOUT_GENERAL);
	}

	inline void SetUniformBufferConstantData(EShaderFrequency Stage, uint32 BindPoint, const TArray<uint8>& ConstantData)
	{
		FVulkanShader* Shader = GetShaderPtr(Stage);
		NEWPackedUniformBufferStaging[Stage].SetEmulatedUniformBufferIntoPacked(BindPoint, ConstantData, NEWPackedUniformBufferStagingDirty[Stage]);
	}

	void SetUniformBuffer(EShaderFrequency Stage, uint32 BindPoint, const FVulkanUniformBuffer* UniformBuffer);

	void SetSRV(EShaderFrequency Stage, uint32 TextureIndex, FVulkanShaderResourceView* SRV);

	inline const FVulkanShader* GetShaderPtr(EShaderFrequency Stage) const
	{
		switch (Stage)
		{
		case SF_Vertex:		return VertexShader;
		case SF_Hull:		return HullShader;
		case SF_Domain:		return DomainShader;
		case SF_Pixel:		return PixelShader;
		case SF_Geometry:	return GeometryShader;
			//#todo-rco: Really should assert here...
		default:			return nullptr;
		}
	}

	// Has no internal verify.
	// If stage does not exists, returns a "nullptr".
	inline FVulkanShader* GetShaderPtr(EShaderFrequency Stage)
	{
		switch (Stage)
		{
		case SF_Vertex:		return VertexShader;
		case SF_Hull:		return HullShader;
		case SF_Domain:		return DomainShader;
		case SF_Pixel:		return PixelShader;
		case SF_Geometry:	return GeometryShader;
			//#todo-rco: Really should assert here...
		default:			return nullptr;
		}
	}

	inline bool HasShaderStage(EShaderFrequency Stage) const
	{
		return (GetShaderPtr(Stage) != nullptr);
	}

	// Verifies if the shader stage exists
	inline const FVulkanShader& GetShader(EShaderFrequency Stage) const
	{
		const FVulkanShader* Shader = GetShaderPtr(Stage);
		check(Shader);
		return *Shader;
	}

	inline void BindPipeline(VkCommandBuffer CmdBuffer, VkPipeline NewPipeline)
	{
		if (LastBoundPipeline != NewPipeline)
		{
			VulkanRHI::vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, NewPipeline);
			LastBoundPipeline = NewPipeline;
		}
	}

private:
	void InternalBindVertexStreams(FVulkanCmdBuffer* Cmd, const void* VertexStreams);
	FCachedBoundShaderStateLink CacheLink;

	/** Cached vertex structure */
	TRefCountPtr<FVulkanVertexDeclaration> VertexDeclaration;

	friend class FVulkanPipelineStateCache;

	FSHAHash ShaderHashes[SF_NumFrequencies];

	TLinkedList<FVulkanBoundShaderState*> GlobalListLink;

	/** Cached shaders */
	TRefCountPtr<FVulkanVertexShader> VertexShader;
	TRefCountPtr<FVulkanPixelShader> PixelShader;
	TRefCountPtr<FVulkanHullShader> HullShader;
	TRefCountPtr<FVulkanDomainShader> DomainShader;
	TRefCountPtr<FVulkanGeometryShader> GeometryShader;

	// For debugging only
	int32 ID;

	bool bDirtyVertexStreams;

	FVulkanDescriptorSetWriter DescriptorWriteSet[SF_Compute];

	FPackedUniformBuffers NEWPackedUniformBufferStaging[SF_Compute];
	uint64 NEWPackedUniformBufferStagingDirty[SF_Compute];

#if VULKAN_ENABLE_RHI_DEBUGGING
	struct FDebugInfo
	{
		const FVulkanTextureBase* Textures[SF_Compute][32];
		const FVulkanSamplerState* SamplerStates[SF_Compute][32];
		union
		{
			const FVulkanTextureView* SRVIs[SF_Compute][32];
			const FVulkanBufferView* SRVBs[SF_Compute][32];
		};
		const FVulkanUniformBuffer* UBs[SF_Compute][32];
	};
	FDebugInfo DebugInfo;
#endif

	void CreateDescriptorWriteInfos();


	FVulkanLayout Layout;

	VkPipeline LastBoundPipeline;

	// Vertex input configuration
	FVulkanVertexInputStateInfo VertexInputStateInfo;

	// Members in Tmp are normally allocated each frame
	// To reduce the allocation, we reuse these array
	struct FTmp
	{
		TArray<VkBuffer> VertexBuffers;
		TArray<VkDeviceSize> VertexOffsets;
	} Tmp;

	// these are the cache pipeline state objects used in this BSS; RefCounts must be manually controlled for the FVulkanPipelines!
	TMap<FVulkanPipelineGraphicsKey, FVulkanGfxPipeline* > PipelineCache;
};

template<>
struct TVulkanResourceTraits<FRHIBoundShaderState>
{
	typedef FVulkanBoundShaderState TConcreteType;
};