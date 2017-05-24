// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanShaders.cpp: Vulkan shader RHI implementation.
=============================================================================*/

#include "VulkanRHIPrivate.h"
#include "VulkanPendingState.h"
#include "VulkanContext.h"
#include "GlobalShader.h"
#include "Serialization/MemoryReader.h"

static TAutoConsoleVariable<int32> GStripGLSL(
	TEXT("r.Vulkan.StripGlsl"),
	1,
	TEXT("1 to remove glsl source (default)\n")\
	TEXT("0 to keep glsl source in each shader for debugging"),
	ECVF_ReadOnly | ECVF_RenderThreadSafe
);


void FVulkanShader::Create(EShaderFrequency Frequency, const TArray<uint8>& InShaderCode)
{
	check(Device);

	FMemoryReader Ar(InShaderCode, true);

	Ar << CodeHeader;

	TArray<ANSICHAR> DebugNameArray;
	Ar << DebugNameArray;
	DebugName = ANSI_TO_TCHAR(DebugNameArray.GetData());

	TArray<uint8> Spirv;
	Ar << Spirv;

	Ar << GlslSource;
	if (GStripGLSL.GetValueOnAnyThread() == 1)
	{
		GlslSource.Empty(0);
	}

	int32 CodeOffset = Ar.Tell();

	VkShaderModuleCreateInfo ModuleCreateInfo;
	FMemory::Memzero(ModuleCreateInfo);
	ModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	ModuleCreateInfo.pNext = nullptr;
	ModuleCreateInfo.flags = 0;

	check(Code == nullptr)
	ModuleCreateInfo.codeSize = Spirv.Num();
	Code = (uint32*)FMemory::Malloc(ModuleCreateInfo.codeSize);
	FMemory::Memcpy(Code, Spirv.GetData(), ModuleCreateInfo.codeSize);

	check(Code != nullptr);
	CodeSize = ModuleCreateInfo.codeSize;
	ModuleCreateInfo.pCode = Code;

	VERIFYVULKANRESULT(VulkanRHI::vkCreateShaderModule(Device->GetInstanceHandle(), &ModuleCreateInfo, nullptr, &ShaderModule));

	if (Frequency == SF_Compute && CodeHeader.NEWDescriptorInfo.DescriptorTypes.Num() == 0)
	{
		UE_LOG(LogVulkanRHI, Warning, TEXT("Compute shader %s %s has no resources!"), *CodeHeader.ShaderName, *DebugName);
	}
}

#if VULKAN_HAS_DEBUGGING_ENABLED
inline void ValidateBindingPoint(const FVulkanShader& InShader, const uint32 InBindingPoint, const uint8 InSubType)
{
#if 0
	const TArray<FVulkanShaderSerializedBindings::FBindMap>& BindingLayout = InShader.CodeHeader.SerializedBindings.Bindings;
	bool bFound = false;

	for (const auto& Binding : BindingLayout)
	{
		const bool bIsPackedUniform = InSubType == CrossCompiler::PACKED_TYPENAME_HIGHP
			|| InSubType == CrossCompiler::PACKED_TYPENAME_MEDIUMP
			|| InSubType == CrossCompiler::PACKED_TYPENAME_LOWP;

		if (Binding.EngineBindingIndex == InBindingPoint &&
			bIsPackedUniform ? (Binding.Type == EVulkanBindingType::PACKED_UNIFORM_BUFFER) : (Binding.SubType == InSubType)
			)
		{
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		FString SubTypeName = "UNDEFINED";

		switch (InSubType)
		{
		case CrossCompiler::PACKED_TYPENAME_HIGHP: SubTypeName = "HIGH PRECISION UNIFORM PACKED BUFFER";	break;
		case CrossCompiler::PACKED_TYPENAME_MEDIUMP: SubTypeName = "MEDIUM PRECISION UNIFORM PACKED BUFFER";	break;
		case CrossCompiler::PACKED_TYPENAME_LOWP: SubTypeName = "LOW PRECISION UNIFORM PACKED BUFFER";	break;
		case CrossCompiler::PACKED_TYPENAME_INT: SubTypeName = "INT UNIFORM PACKED BUFFER";				break;
		case CrossCompiler::PACKED_TYPENAME_UINT: SubTypeName = "UINT UNIFORM PACKED BUFFER";				break;
		case CrossCompiler::PACKED_TYPENAME_SAMPLER: SubTypeName = "SAMPLER";								break;
		case CrossCompiler::PACKED_TYPENAME_IMAGE: SubTypeName = "IMAGE";									break;
		default:
			break;
		}

		UE_LOG(LogVulkanRHI, Warning,
			TEXT("Setting '%s' resource for an unexpected binding slot UE:%d, for shader '%s'"),
			*SubTypeName, InBindingPoint, *InShader.DebugName);
	}
#endif
}
#endif // VULKAN_HAS_DEBUGGING_ENABLED

template<typename BaseResourceType, EShaderFrequency Frequency>
void TVulkanBaseShader<BaseResourceType, Frequency>::Create(const TArray<uint8>& InCode)
{
	FVulkanShader::Create(Frequency, InCode);
}


FVulkanShader::~FVulkanShader()
{
	check(Device);

	if (Code)
	{
		FMemory::Free(Code);
		Code = nullptr;
	}

	if (ShaderModule != VK_NULL_HANDLE)
	{
		Device->GetDeferredDeletionQueue().EnqueueResource(VulkanRHI::FDeferredDeletionQueue::EType::ShaderModule, ShaderModule);
		ShaderModule = VK_NULL_HANDLE;
	}
}

FVertexShaderRHIRef FVulkanDynamicRHI::RHICreateVertexShader(const TArray<uint8>& Code)
{
	FVulkanVertexShader* Shader = new FVulkanVertexShader(Device);
	Shader->Create(Code);
	return Shader;
}

FPixelShaderRHIRef FVulkanDynamicRHI::RHICreatePixelShader(const TArray<uint8>& Code)
{
	FVulkanPixelShader* Shader = new FVulkanPixelShader(Device);
	Shader->Create(Code);
	return Shader;
}

FHullShaderRHIRef FVulkanDynamicRHI::RHICreateHullShader(const TArray<uint8>& Code) 
{ 
	FVulkanHullShader* Shader = new FVulkanHullShader(Device);
	Shader->Create(Code);
	return Shader;
}

FDomainShaderRHIRef FVulkanDynamicRHI::RHICreateDomainShader(const TArray<uint8>& Code) 
{ 
	FVulkanDomainShader* Shader = new FVulkanDomainShader(Device);
	Shader->Create(Code);
	return Shader;
}

FGeometryShaderRHIRef FVulkanDynamicRHI::RHICreateGeometryShader(const TArray<uint8>& Code) 
{ 
	FVulkanGeometryShader* Shader = new FVulkanGeometryShader(Device);
	Shader->Create(Code);
	return Shader;
}

FGeometryShaderRHIRef FVulkanDynamicRHI::RHICreateGeometryShaderWithStreamOutput(const TArray<uint8>& Code, const FStreamOutElementList& ElementList,
	uint32 NumStrides, const uint32* Strides, int32 RasterizedStream)
{
	VULKAN_SIGNAL_UNIMPLEMENTED();
	return nullptr;
}

FComputeShaderRHIRef FVulkanDynamicRHI::RHICreateComputeShader(const TArray<uint8>& Code) 
{ 
	FVulkanComputeShader* Shader = new FVulkanComputeShader(Device);
	Shader->Create(Code);
	return Shader;
}


FVulkanLayout::FVulkanLayout(FVulkanDevice* InDevice)
	: VulkanRHI::FDeviceChild(InDevice)
	, DescriptorSetLayout(Device)
	, PipelineLayout(VK_NULL_HANDLE)
{
}

FVulkanLayout::~FVulkanLayout()
{
	if (PipelineLayout != VK_NULL_HANDLE)
	{
		Device->GetDeferredDeletionQueue().EnqueueResource(VulkanRHI::FDeferredDeletionQueue::EType::PipelineLayout, PipelineLayout);
		PipelineLayout = VK_NULL_HANDLE;
	}
}

void FVulkanLayout::Compile()
{
	check(PipelineLayout == VK_NULL_HANDLE);

	DescriptorSetLayout.Compile();

	const TArray<VkDescriptorSetLayout>& LayoutHandles = DescriptorSetLayout.GetHandles();

#if !VULKAN_KEEP_CREATE_INFO
	VkPipelineLayoutCreateInfo CreateInfo;
#endif
	FMemory::Memzero(CreateInfo);
	CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	CreateInfo.pNext = nullptr;
	CreateInfo.setLayoutCount = LayoutHandles.Num();
	CreateInfo.pSetLayouts = LayoutHandles.GetData();
	CreateInfo.pushConstantRangeCount = 0;
	CreateInfo.pPushConstantRanges = nullptr;

	VERIFYVULKANRESULT(VulkanRHI::vkCreatePipelineLayout(Device->GetInstanceHandle(), &CreateInfo, nullptr, &PipelineLayout));
}


FVulkanDescriptorSetRingBuffer::FVulkanDescriptorSetRingBuffer(FVulkanDevice* InDevice)
	: VulkanRHI::FDeviceChild(InDevice)
	, CurrDescriptorSets(nullptr)
{
}

FVulkanDescriptorSetRingBuffer::~FVulkanDescriptorSetRingBuffer()
	{
}

inline void FVulkanDescriptorSetWriter::SetupDescriptorWrites(const FNEWVulkanShaderDescriptorInfo& Info, VkWriteDescriptorSet* InWriteDescriptors, VkDescriptorImageInfo* InImageInfo, VkDescriptorBufferInfo* InBufferInfo)
{
	WriteDescriptors = InWriteDescriptors;
	NumWrites = Info.DescriptorTypes.Num();
	FullMask = ((uint64)1 << (uint64)Info.DescriptorTypes.Num()) - (uint64)1;

	BufferViewReferences.Empty(NumWrites);
	BufferViewReferences.AddDefaulted(NumWrites);

	checkf(Info.DescriptorTypes.Num() <= sizeof(FullMask) * 8, TEXT("Out of dirty mask bits! Need %d"), Info.DescriptorTypes.Num());

	MarkAllDirty();

	for (int32 Index = 0; Index < Info.DescriptorTypes.Num(); ++Index)
	{
		InWriteDescriptors->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		InWriteDescriptors->dstBinding = Index;
		InWriteDescriptors->descriptorCount = 1;
		InWriteDescriptors->descriptorType = Info.DescriptorTypes[Index];

		switch (Info.DescriptorTypes[Index])
		{
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
			InWriteDescriptors->pBufferInfo = InBufferInfo++;
			break;
		case VK_DESCRIPTOR_TYPE_SAMPLER:
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			InWriteDescriptors->pImageInfo = InImageInfo++;
			break;
		case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
		case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
			break;
		default:
			checkf(0, TEXT("Unsupported descriptor type %d"), Info.DescriptorTypes[Index]);
			break;
		}
		++InWriteDescriptors;
	}
}

void FVulkanLayout::AddBindingsForStage(VkShaderStageFlagBits StageFlags, EDescriptorSetStage DescSet, const FVulkanCodeHeader& CodeHeader)
{
	//#todo-rco: Mobile assumption!
	int32 DescriptorSetIndex = (int32)DescSet;

	VkDescriptorSetLayoutBinding Binding;
	FMemory::Memzero(Binding);
	Binding.descriptorCount = 1;
	Binding.stageFlags = StageFlags;
	for (int32 Index = 0; Index < CodeHeader.NEWDescriptorInfo.DescriptorTypes.Num(); ++Index)
	{
		Binding.binding = Index;
		Binding.descriptorType = CodeHeader.NEWDescriptorInfo.DescriptorTypes[Index];
		DescriptorSetLayout.AddDescriptor(DescriptorSetIndex, Binding, Index);
	}
}

/*
void FVulkanComputeShaderState::SetUniformBuffer(uint32 BindPoint, const FVulkanUniformBuffer* UniformBuffer)
{
	check(0);
#if 0
	uint32 VulkanBindingPoint = ComputeShader->GetBindingTable().UniformBufferBindingIndices[BindPoint];

	check(UniformBuffer && (UniformBuffer->GetBufferUsageFlags() & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT));

	VkDescriptorBufferInfo* BufferInfo = &DescriptorBufferInfo[BindPoint];
	BufferInfo->buffer = UniformBuffer->GetHandle();
	BufferInfo->range = UniformBuffer->GetSize();

#if VULKAN_ENABLE_RHI_DEBUGGING
	//DebugInfo.UBs[BindPoint] = UniformBuffer;
#endif
#endif
}

void FVulkanComputeShaderState::SetSRVTextureView(uint32 BindPoint, const FVulkanTextureView& TextureView)
{
	ensure(0);
#if 0
	uint32 VulkanBindingPoint = ComputeShader->GetBindingTable().CombinedSamplerBindingIndices[BindPoint];
	DescriptorImageInfo[VulkanBindingPoint].imageView = TextureView.View;
	//DescriptorSamplerImageInfoForStage[VulkanBindingPoint].imageLayout = (TextureView.BaseTexture->Surface.GetFullAspectMask() & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) != 0 ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;

	DirtySamplerStates = true;
#if VULKAN_ENABLE_RHI_DEBUGGING
	//DebugInfo.SRVIs[BindPoint] = &TextureView;
#endif
#endif
}

void FVulkanComputeShaderState::ResetState()
{
#if 0
	DirtyPackedUniformBufferStaging = DirtyPackedUniformBufferStagingMask;
#endif

	CurrDescriptorSets = nullptr;

	LastBoundPipeline = VK_NULL_HANDLE;
}

*/

FVulkanBoundShaderState::FVulkanBoundShaderState(
		FVulkanDevice* InDevice,
		FVertexDeclarationRHIParamRef InVertexDeclarationRHI,
		FVertexShaderRHIParamRef InVertexShaderRHI,
		FPixelShaderRHIParamRef InPixelShaderRHI,
		FHullShaderRHIParamRef InHullShaderRHI,
		FDomainShaderRHIParamRef InDomainShaderRHI,
		FGeometryShaderRHIParamRef InGeometryShaderRHI)
	: FVulkanDescriptorSetRingBuffer(InDevice)
	, CacheLink(InVertexDeclarationRHI,InVertexShaderRHI,InPixelShaderRHI,InHullShaderRHI,InDomainShaderRHI,InGeometryShaderRHI,this)
	, GlobalListLink(this)
	, bDirtyVertexStreams(true)
	, Layout(InDevice)
	, LastBoundPipeline(VK_NULL_HANDLE)
{
	static int32 sID = 0;
	ID = sID++;
	INC_DWORD_STAT(STAT_VulkanNumBoundShaderState);

	FMemory::Memzero(NEWPackedUniformBufferStagingDirty);

	FVulkanVertexDeclaration* InVertexDeclaration = ResourceCast(InVertexDeclarationRHI);
	FVulkanVertexShader* InVertexShader = ResourceCast(InVertexShaderRHI);
	FVulkanPixelShader* InPixelShader = ResourceCast(InPixelShaderRHI);
	FVulkanHullShader* InHullShader = ResourceCast(InHullShaderRHI);
	FVulkanDomainShader* InDomainShader = ResourceCast(InDomainShaderRHI);
	FVulkanGeometryShader* InGeometryShader = ResourceCast(InGeometryShaderRHI);

	// cache everything
	VertexDeclaration = InVertexDeclaration;
	VertexShader = InVertexShader;
	PixelShader = InPixelShader;
	HullShader = InHullShader;
	DomainShader = InDomainShader;
	GeometryShader = InGeometryShader;

	GlobalListLink.LinkHead(FVulkanPipelineStateCache::GetBSSList());

	// Setup working areas for the global uniforms
	check(VertexShader);
	ShaderHashes[SF_Vertex] = VertexShader->CodeHeader.SourceHash;
	NEWPackedUniformBufferStaging[SF_Vertex].Init(&VertexShader->CodeHeader, NEWPackedUniformBufferStagingDirty[SF_Vertex]);
	Layout.AddBindingsForStage(VK_SHADER_STAGE_VERTEX_BIT, EDescriptorSetStage::Vertex, VertexShader->CodeHeader);

	if (PixelShader)
	{
		ShaderHashes[SF_Pixel] = PixelShader->CodeHeader.SourceHash;
		NEWPackedUniformBufferStaging[SF_Pixel].Init(&PixelShader->CodeHeader, NEWPackedUniformBufferStagingDirty[SF_Pixel]);
		Layout.AddBindingsForStage(VK_SHADER_STAGE_FRAGMENT_BIT, EDescriptorSetStage::Pixel, PixelShader->CodeHeader);
	}
	if (GeometryShader)
	{
		ShaderHashes[SF_Geometry] = GeometryShader->CodeHeader.SourceHash;
		NEWPackedUniformBufferStaging[SF_Geometry].Init(&GeometryShader->CodeHeader, NEWPackedUniformBufferStagingDirty[SF_Geometry]);
		Layout.AddBindingsForStage(VK_SHADER_STAGE_GEOMETRY_BIT, EDescriptorSetStage::Geometry, GeometryShader->CodeHeader);
	}
	if (HullShader)
	{
		// Can't have Hull w/o Domain
		check(DomainShader);
		ShaderHashes[SF_Hull] = HullShader->CodeHeader.SourceHash;
		ShaderHashes[SF_Domain] = DomainShader->CodeHeader.SourceHash;
		NEWPackedUniformBufferStaging[SF_Hull].Init(&HullShader->CodeHeader, NEWPackedUniformBufferStagingDirty[SF_Hull]);
		NEWPackedUniformBufferStaging[SF_Domain].Init(&DomainShader->CodeHeader, NEWPackedUniformBufferStagingDirty[SF_Domain]);
		Layout.AddBindingsForStage(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, EDescriptorSetStage::Hull, HullShader->CodeHeader);
		Layout.AddBindingsForStage(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, EDescriptorSetStage::Domain, DomainShader->CodeHeader);
	}
	else
	{
		// Can't have Domain w/o Hull
		check(DomainShader == nullptr);
	}

	Layout.Compile();
	VertexInputStateInfo.Create(VertexDeclaration, GetShader(SF_Vertex).CodeHeader.SerializedBindings.InOutMask);

	CreateDescriptorWriteInfos();
}

FVulkanBoundShaderState::~FVulkanBoundShaderState()
{
	GlobalListLink.Unlink();

	for (int32 Index = 0; Index < DescriptorSetsEntries.Num(); ++Index)
	{
		delete DescriptorSetsEntries[Index];
	}
	DescriptorSetsEntries.Empty(0);

	// toss the pipeline states
	for (auto& Pair : PipelineCache)
	{
		// Reference is decremented inside the Destroy function
		Device->PipelineStateCache->DestroyPipeline(Pair.Value);
	}

	DEC_DWORD_STAT(STAT_VulkanNumBoundShaderState);
}

FVulkanGfxPipeline* FVulkanBoundShaderState::PrepareForDraw(FVulkanRenderPass* RenderPass, const FVulkanPipelineGraphicsKey& PipelineKey, uint32 VertexInputKey, const struct FVulkanGfxPipelineState& State)
{
	SCOPE_CYCLE_COUNTER(STAT_VulkanGetOrCreatePipeline);

	// have we made a matching state object yet?
	FVulkanGfxPipeline* Pipeline = PipelineCache.FindRef(PipelineKey);

	// make one if not
	if (Pipeline == nullptr)
	{
		// Try the device cache
		FVulkanGfxPipelineStateKey PipelineCreateInfo(PipelineKey, VertexInputKey, ShaderHashes);
		Pipeline = Device->PipelineStateCache->Find(PipelineCreateInfo);
		if (Pipeline)
		{
			// Add it to the local cache; manually control RefCount
			PipelineCache.Add(PipelineKey, Pipeline);
			Pipeline->AddRef();
		}
		else
		{
			// Create a new one
			Pipeline = new FVulkanGfxPipeline(Device);
			Device->PipelineStateCache->CreateAndAdd(RenderPass, PipelineCreateInfo, Pipeline, State, *this);

			// Add it to the local cache; manually control RefCount
			PipelineCache.Add(PipelineKey, Pipeline);
			Pipeline->AddRef();
/*
#if !UE_BUILD_SHIPPING
			if (Device->FrameCounter > 3)
			{
				FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Created a hitchy pipeline state for hash %llx%llx %x (this = %x) VS %s %x %d PS %s %x %d\n"), 
					PipelineKey.Key[0], PipelineKey.Key[1], VertexInputKey, this, *VertexShader->DebugName, (void*)VertexShader.GetReference(), VertexShader->GlslSource.Num(), 
					*PixelShader->DebugName, (void*)PixelShader.GetReference(), PixelShader->GlslSource.Num());
			}
#endif*/
		}
	}

	return Pipeline;
}


void FVulkanBoundShaderState::ResetState()
{
	static_assert(SF_Geometry + 1 == SF_Compute, "Loop assumes compute is after gfx stages!");
#if 0
	for(uint32 Stage = 0; Stage < SF_Compute; Stage++)
	{
		DirtyTextures[Stage] = true;
		DirtySamplerStates[Stage] = true;
		DirtySRVs[Stage] = true;
		DirtyPackedUniformBufferStaging[Stage] = DirtyPackedUniformBufferStagingMask[Stage];
	}
#endif

	FVulkanDescriptorSetRingBuffer::Reset();

	LastBoundPipeline = VK_NULL_HANDLE;
	bDirtyVertexStreams = true;
}

void FVulkanBoundShaderState::CreateDescriptorWriteInfos()
{
	check(DescriptorWrites.Num() == 0);

	static_assert(SF_Geometry + 1 == SF_Compute, "Loop assumes compute is after gfx stages!");

	for (uint32 Stage = 0; Stage < SF_Compute; Stage++)
	{
		FVulkanShader* StageShader = GetShaderPtr((EShaderFrequency)Stage);
		if (!StageShader)
		{
			continue;
		}

		DescriptorWrites.AddZeroed(StageShader->CodeHeader.NEWDescriptorInfo.DescriptorTypes.Num());
		DescriptorImageInfo.AddZeroed(StageShader->CodeHeader.NEWDescriptorInfo.NumImageInfos);
		DescriptorBufferInfo.AddZeroed(StageShader->CodeHeader.NEWDescriptorInfo.NumBufferInfos);
	}

	for (int32 Index = 0; Index < DescriptorImageInfo.Num(); ++Index)
	{
		// Texture.Load() still requires a default sampler...
		DescriptorImageInfo[Index].sampler = Device->GetDefaultSampler();
		DescriptorImageInfo[Index].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	}

	VkWriteDescriptorSet* CurrentDescriptorWrite = DescriptorWrites.GetData();
	VkDescriptorImageInfo* CurrentImageInfo = DescriptorImageInfo.GetData();
	VkDescriptorBufferInfo* CurrentBufferInfo = DescriptorBufferInfo.GetData();

	for (uint32 Stage = 0; Stage < SF_Compute; Stage++)
	{
		FVulkanShader* StageShader = GetShaderPtr((EShaderFrequency)Stage);
		if (!StageShader)
		{
			continue;
		}

		DescriptorWriteSet[Stage].SetupDescriptorWrites(StageShader->CodeHeader.NEWDescriptorInfo, CurrentDescriptorWrite, CurrentImageInfo, CurrentBufferInfo);
		CurrentDescriptorWrite += StageShader->CodeHeader.NEWDescriptorInfo.DescriptorTypes.Num();
		CurrentImageInfo += StageShader->CodeHeader.NEWDescriptorInfo.NumImageInfos;
		CurrentBufferInfo += StageShader->CodeHeader.NEWDescriptorInfo.NumBufferInfos;
	}
}

void FVulkanBoundShaderState::InternalBindVertexStreams(FVulkanCmdBuffer* Cmd, const void* InVertexStreams)
{
#if VULKAN_ENABLE_AGGRESSIVE_STATS
	SCOPE_CYCLE_COUNTER(STAT_VulkanBindVertexStreamsTime);
#endif
	check(VertexDeclaration);

	// Its possible to have no vertex buffers
	if (VertexInputStateInfo.AttributesNum == 0)
	{
		// However, we need to verify that there are also no bindings
		check(VertexInputStateInfo.BindingsNum == 0);
		return;
	}

	FOLDVulkanPendingGfxState::FVertexStream* Streams = (FOLDVulkanPendingGfxState::FVertexStream*)InVertexStreams;

	Tmp.VertexBuffers.Reset(0);
	Tmp.VertexOffsets.Reset(0);
	const VkVertexInputAttributeDescription* CurrAttribute = nullptr;
	for (uint32 BindingIndex = 0; BindingIndex < VertexInputStateInfo.BindingsNum; BindingIndex++)
	{
		const VkVertexInputBindingDescription& CurrBinding = VertexInputStateInfo.Bindings[BindingIndex];

		uint32 StreamIndex = VertexInputStateInfo.BindingToStream.FindChecked(BindingIndex);
		FOLDVulkanPendingGfxState::FVertexStream& CurrStream = Streams[StreamIndex];

		// Verify the vertex buffer is set
		if (!CurrStream.Stream && !CurrStream.Stream2 && CurrStream.Stream3 == VK_NULL_HANDLE)
		{
			// The attribute in stream index is probably compiled out
			#if VULKAN_HAS_DEBUGGING_ENABLED
				// Lets verify
				for (uint32 AttributeIndex = 0; AttributeIndex < VertexInputStateInfo.AttributesNum; AttributeIndex++)
				{
					if (VertexInputStateInfo.Attributes[AttributeIndex].binding == CurrBinding.binding)
					{
						UE_LOG(LogVulkanRHI, Warning, TEXT("Missing binding on location %d in '%s' vertex shader"),
							CurrBinding.binding,
							*GetShader(SF_Vertex).DebugName);
						ensure(0);
					}
				}
			#endif
			continue;
		}

		Tmp.VertexBuffers.Add(CurrStream.Stream
			? CurrStream.Stream->GetBufferHandle()
			: (CurrStream.Stream2
				? CurrStream.Stream2->GetHandle()
				: CurrStream.Stream3)
			);
		Tmp.VertexOffsets.Add(CurrStream.BufferOffset);
	}

	if(Tmp.VertexBuffers.Num() > 0)
	{
		// Bindings are expected to be in ascending order with no index gaps in between:
		// Correct:		0, 1, 2, 3
		// Incorrect:	1, 0, 2, 3
		// Incorrect:	0, 2, 3, 5
		// Reordering and creation of stream binding index is done in "GenerateVertexInputStateInfo()"
		VulkanRHI::vkCmdBindVertexBuffers(Cmd->GetHandle(), 0, Tmp.VertexBuffers.Num(), Tmp.VertexBuffers.GetData(), Tmp.VertexOffsets.GetData());
	}
}

void FVulkanBoundShaderState::SetSRV(EShaderFrequency Stage, uint32 TextureIndex, FVulkanShaderResourceView* SRV)
{
	if (SRV)
	{
		// make sure any dynamically backed SRV points to current memory
		SRV->UpdateView();
		if (SRV->BufferView)
		{
			checkf(SRV->BufferView != VK_NULL_HANDLE, TEXT("Empty SRV"));
			SetBufferViewState(Stage, TextureIndex, SRV->BufferView);
		}
		else
		{
			checkf(SRV->TextureView.View != VK_NULL_HANDLE, TEXT("Empty SRV"));
			SetTextureView(Stage, TextureIndex, SRV->TextureView);
		}
	}
	else
	{
		DescriptorWriteSet[Stage].ClearBufferView(TextureIndex);
	}
}

void FVulkanBoundShaderState::SetUniformBuffer(EShaderFrequency Stage, uint32 BindPoint, const FVulkanUniformBuffer* UniformBuffer)
{
	check(0);
#if 0
	FVulkanShader* Shader = GetShaderPtr(Stage);
	uint32 VulkanBindingPoint = Shader->GetBindingTable().UniformBufferBindingIndices[BindPoint];

	check(UniformBuffer && (UniformBuffer->GetBufferUsageFlags() & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT));

	VkDescriptorBufferInfo* BufferInfo = &DescriptorBufferInfoForStage[Stage][BindPoint];
	BufferInfo->buffer = UniformBuffer->GetHandle();
	BufferInfo->range = UniformBuffer->GetSize();

	//#todo-rco: Mark Dirty UB	
#if VULKAN_ENABLE_RHI_DEBUGGING
	DebugInfo.UBs[Stage][BindPoint] = UniformBuffer;
#endif
#endif
}

FVulkanDescriptorSetRingBuffer::FDescriptorSetsPair::~FDescriptorSetsPair()
{
	delete DescriptorSets;
}

inline FVulkanDescriptorSets* FVulkanDescriptorSetRingBuffer::RequestDescriptorSets(FVulkanCommandListContext* Context, FVulkanCmdBuffer* CmdBuffer, const FVulkanLayout& Layout)
{
	FDescriptorSetsEntry* FoundEntry = nullptr;
	for (FDescriptorSetsEntry* DescriptorSetsEntry : DescriptorSetsEntries)
	{
		if (DescriptorSetsEntry->CmdBuffer == CmdBuffer)
		{
			FoundEntry = DescriptorSetsEntry;
		}
	}

	if (!FoundEntry)
	{
		if (!Layout.HasDescriptors())
		{
			return nullptr;
		}

		FoundEntry = new FDescriptorSetsEntry(CmdBuffer);
		DescriptorSetsEntries.Add(FoundEntry);
	}

	const uint64 CmdBufferFenceSignaledCounter = CmdBuffer->GetFenceSignaledCounter();
	for (int32 Index = 0; Index < FoundEntry->Pairs.Num(); ++Index)
	{
		FDescriptorSetsPair& Entry = FoundEntry->Pairs[Index];
		if (Entry.FenceCounter < CmdBufferFenceSignaledCounter)
		{
			Entry.FenceCounter = CmdBufferFenceSignaledCounter;
			return Entry.DescriptorSets;
		}
	}

	FDescriptorSetsPair* NewEntry = new (FoundEntry->Pairs) FDescriptorSetsPair;
	NewEntry->DescriptorSets = new FVulkanDescriptorSets(Device, Layout.GetDescriptorSetsLayout(), Context);
	NewEntry->FenceCounter = CmdBufferFenceSignaledCounter;
	return NewEntry->DescriptorSets;
}


bool FVulkanBoundShaderState::UpdateDescriptorSets(FVulkanCommandListContext* CmdListContext, FVulkanCmdBuffer* CmdBuffer, FVulkanGlobalUniformPool* GlobalUniformPool)
{
#if VULKAN_ENABLE_AGGRESSIVE_STATS
	SCOPE_CYCLE_COUNTER(STAT_VulkanUpdateDescriptorSets);
#endif

	check(GlobalUniformPool);

	FOLDVulkanPendingGfxState& State = *CmdListContext->GetPendingGfxState();

	int32 WriteIndex = 0;

	CurrDescriptorSets = RequestDescriptorSets(CmdListContext, CmdBuffer, Layout);
	if (!CurrDescriptorSets)
	{
		return false;
	}

	const FVulkanDescriptorSets::FDescriptorSetArray& DescriptorSetHandles = CurrDescriptorSets->GetHandles();
	int32 DescriptorSetIndex = 0;

	FVulkanUniformBufferUploader* UniformBufferUploader = CmdListContext->GetUniformBufferUploader();
	uint8* CPURingBufferBase = (uint8*)UniformBufferUploader->GetCPUMappedPointer();

	//#todo-rco: Compute!
	static_assert(SF_Geometry + 1 == SF_Compute, "Loop assumes compute is after gfx stages!");
	for (uint32 Stage = 0; Stage < SF_Compute; Stage++)
	{
		FVulkanShader* StageShader = GetShaderPtr((EShaderFrequency)Stage);
		if (!StageShader)
		{
			continue;
		}

		if (StageShader->CodeHeader.NEWDescriptorInfo.DescriptorTypes.Num() == 0)
		{
			// Empty set, still has its own index
			++DescriptorSetIndex;
			continue;
		}

		const VkDescriptorSet DescriptorSet = DescriptorSetHandles[DescriptorSetIndex];
		++DescriptorSetIndex;

		bool bRequiresPackedUBUpdate = (NEWPackedUniformBufferStagingDirty[Stage] != 0);
		if (bRequiresPackedUBUpdate)
		{
			SCOPE_CYCLE_COUNTER(STAT_VulkanApplyDSUniformBuffers);
			UpdatePackedUniformBuffers(Device->GetLimits().minUniformBufferOffsetAlignment, StageShader->CodeHeader, NEWPackedUniformBufferStaging[Stage], DescriptorWriteSet[Stage], UniformBufferUploader, CPURingBufferBase, NEWPackedUniformBufferStagingDirty[Stage]);
			NEWPackedUniformBufferStagingDirty[Stage] = 0;
		}

		bool bRequiresNonPackedUBUpdate = (DescriptorWriteSet[Stage].DirtyMask != 0);
		if (!bRequiresNonPackedUBUpdate && !bRequiresPackedUBUpdate)
		{
			//#todo-rco: Skip this desc set writes and only call update for the modified ones!
			//continue;
			int x = 0;
		}

		DescriptorWriteSet[Stage].SetDescriptorSet(DescriptorSet);
	}

#if VULKAN_ENABLE_AGGRESSIVE_STATS
	INC_DWORD_STAT_BY(STAT_VulkanNumUpdateDescriptors, DescriptorWrites.Num());
	INC_DWORD_STAT_BY(STAT_VulkanNumDescSets, DescriptorSetIndex);
#endif

	{
#if VULKAN_ENABLE_AGGRESSIVE_STATS
		SCOPE_CYCLE_COUNTER(STAT_VulkanVkUpdateDS);
#endif
		VulkanRHI::vkUpdateDescriptorSets(Device->GetInstanceHandle(), DescriptorWrites.Num(), DescriptorWrites.GetData(), 0, nullptr);
	}

	return true;
}

FBoundShaderStateRHIRef FVulkanDynamicRHI::RHICreateBoundShaderState(
	FVertexDeclarationRHIParamRef VertexDeclarationRHI, 
	FVertexShaderRHIParamRef VertexShaderRHI, 
	FHullShaderRHIParamRef HullShaderRHI, 
	FDomainShaderRHIParamRef DomainShaderRHI, 
	FPixelShaderRHIParamRef PixelShaderRHI,
	FGeometryShaderRHIParamRef GeometryShaderRHI
	)
{
	// Check for an existing bound shader state which matches the parameters
	FCachedBoundShaderStateLink* CachedBoundShaderStateLink = GetCachedBoundShaderState(
		VertexDeclarationRHI,
		VertexShaderRHI,
		PixelShaderRHI,
		HullShaderRHI,
		DomainShaderRHI,
		GeometryShaderRHI
		);

	if (CachedBoundShaderStateLink)
	{
		// If we've already created a bound shader state with these parameters, reuse it.
		return CachedBoundShaderStateLink->BoundShaderState;
	}

	return new FVulkanBoundShaderState(Device, VertexDeclarationRHI,VertexShaderRHI,PixelShaderRHI,HullShaderRHI,DomainShaderRHI,GeometryShaderRHI);
}

FVulkanDescriptorPool* FVulkanCommandListContext::AllocateDescriptorSets(const VkDescriptorSetAllocateInfo& InDescriptorSetAllocateInfo, const FVulkanDescriptorSetsLayout& Layout, VkDescriptorSet* OutSets)
{
	FVulkanDescriptorPool* Pool = DescriptorPools.Last();
	VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo = InDescriptorSetAllocateInfo;
	VkResult Result = VK_ERROR_OUT_OF_DEVICE_MEMORY;

	int32 ValidationLayerEnabled = 0;
#if VULKAN_HAS_DEBUGGING_ENABLED
	extern TAutoConsoleVariable<int32> GValidationCvar;
	ValidationLayerEnabled = GValidationCvar->GetInt();
#endif
	// Only try to find if it will fit in the pool if we're in validation mode
	if (ValidationLayerEnabled == 0 || Pool->CanAllocate(Layout))
	{
		DescriptorSetAllocateInfo.descriptorPool = Pool->GetHandle();
		Result = VulkanRHI::vkAllocateDescriptorSets(Device->GetInstanceHandle(), &DescriptorSetAllocateInfo, OutSets);
	}

	if (Result < VK_SUCCESS)
	{
		if (Pool->IsEmpty())
		{
			VERIFYVULKANRESULT(Result);
		}
		else
		{
			// Spec says any negative value could be due to fragmentation, so create a new Pool. If it fails here then we really are out of memory!
			Pool = new FVulkanDescriptorPool(Device);
			DescriptorPools.Add(Pool);
			DescriptorSetAllocateInfo.descriptorPool = Pool->GetHandle();
			VERIFYVULKANRESULT_EXPANDED(VulkanRHI::vkAllocateDescriptorSets(Device->GetInstanceHandle(), &DescriptorSetAllocateInfo, OutSets));
		}
	}

	return Pool;
}
