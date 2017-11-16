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

static_assert(SF_Geometry + 1 == SF_Compute, "Assumes compute is after gfx stages!");

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
	VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo;
#endif
	FMemory::Memzero(PipelineLayoutCreateInfo);
	PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	//PipelineLayoutCreateInfo.pNext = nullptr;
	PipelineLayoutCreateInfo.setLayoutCount = LayoutHandles.Num();
	PipelineLayoutCreateInfo.pSetLayouts = LayoutHandles.GetData();
	//PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	//PipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	VERIFYVULKANRESULT(VulkanRHI::vkCreatePipelineLayout(Device->GetInstanceHandle(), &PipelineLayoutCreateInfo, nullptr, &PipelineLayout));
}


#if !VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
FOLDVulkanDescriptorSetRingBuffer::FOLDVulkanDescriptorSetRingBuffer(FVulkanDevice* InDevice)
	: VulkanRHI::FDeviceChild(InDevice)
	, CurrDescriptorSets(nullptr)
{
}

FOLDVulkanDescriptorSetRingBuffer::~FOLDVulkanDescriptorSetRingBuffer()
{
}
#endif

void FVulkanDescriptorSetWriter::SetupDescriptorWrites(const FNEWVulkanShaderDescriptorInfo& Info, VkWriteDescriptorSet* InWriteDescriptors, VkDescriptorImageInfo* InImageInfo, VkDescriptorBufferInfo* InBufferInfo)
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
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
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
			checkf(0, TEXT("Unsupported descriptor type %d"), (int32)Info.DescriptorTypes[Index]);
			break;
		}
		++InWriteDescriptors;
	}
}

void FVulkanDescriptorSetsLayoutInfo::AddBindingsForStage(VkShaderStageFlagBits StageFlags, EDescriptorSetStage DescSet, const FVulkanCodeHeader& CodeHeader)
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
		AddDescriptor(DescriptorSetIndex, Binding, Index);
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

#if !VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
FOLDVulkanDescriptorSetRingBuffer::FDescriptorSetsPair::~FDescriptorSetsPair()
{
	delete DescriptorSets;
}

FOLDVulkanDescriptorSets* FOLDVulkanDescriptorSetRingBuffer::RequestDescriptorSets(FVulkanCommandListContext* Context, FVulkanCmdBuffer* CmdBuffer, const FVulkanLayout& Layout)
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
	NewEntry->DescriptorSets = new FOLDVulkanDescriptorSets(Device, Layout.GetDescriptorSetsLayout(), Context);
	NewEntry->FenceCounter = CmdBufferFenceSignaledCounter;
	return NewEntry->DescriptorSets;
}
#endif

FVulkanBoundShaderState::FVulkanBoundShaderState(FVertexDeclarationRHIParamRef InVertexDeclarationRHI, FVertexShaderRHIParamRef InVertexShaderRHI,
	FPixelShaderRHIParamRef InPixelShaderRHI, FHullShaderRHIParamRef InHullShaderRHI,
	FDomainShaderRHIParamRef InDomainShaderRHI, FGeometryShaderRHIParamRef InGeometryShaderRHI)
	: CacheLink(InVertexDeclarationRHI, InVertexShaderRHI, InPixelShaderRHI, InHullShaderRHI, InDomainShaderRHI, InGeometryShaderRHI, this)
{
	CacheLink.AddToCache();
}

FVulkanBoundShaderState::~FVulkanBoundShaderState()
{
	CacheLink.RemoveFromCache();
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
	
	FBoundShaderStateRHIRef CachedBoundShaderState = GetCachedBoundShaderState_Threadsafe(
		VertexDeclarationRHI,
		VertexShaderRHI,
		PixelShaderRHI,
		HullShaderRHI,
		DomainShaderRHI,
		GeometryShaderRHI
	);
	if (CachedBoundShaderState.GetReference())
	{
		// If we've already created a bound shader state with these parameters, reuse it.
		return CachedBoundShaderState;
	}

	return new FVulkanBoundShaderState(VertexDeclarationRHI, VertexShaderRHI, PixelShaderRHI, HullShaderRHI, DomainShaderRHI, GeometryShaderRHI);
}

#if !VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
FOLDVulkanDescriptorPool* FVulkanCommandListContext::AllocateDescriptorSets(const VkDescriptorSetAllocateInfo& InDescriptorSetAllocateInfo, const FVulkanDescriptorSetsLayout& Layout, VkDescriptorSet* OutSets)
{
	FOLDVulkanDescriptorPool* Pool = DescriptorPools.Last();
	VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo = InDescriptorSetAllocateInfo;
	VkResult Result = VK_ERROR_OUT_OF_DEVICE_MEMORY;

	if (Pool->CanAllocate(Layout))
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
			Pool = new FOLDVulkanDescriptorPool(Device);
			DescriptorPools.Add(Pool);
			DescriptorSetAllocateInfo.descriptorPool = Pool->GetHandle();
			VERIFYVULKANRESULT_EXPANDED(VulkanRHI::vkAllocateDescriptorSets(Device->GetInstanceHandle(), &DescriptorSetAllocateInfo, OutSets));
		}
	}

	return Pool;
}
#endif

#if VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
FVulkanPipelineDescriptorSetAllocator::~FVulkanPipelineDescriptorSetAllocator()
{
	ensure(UsedPools.Num() == 0);
	ensure(FreePools.Num() == 0);
	ensure(CurrentPool == nullptr);
}

void FVulkanPipelineDescriptorSetAllocator::Destroy(FVulkanDevice* Device)
{
	Reset();
	static bool bFirst = true;
	if (UsedPools.Num() > 0)
	{
		if (bFirst)
		{
			UE_LOG(LogVulkanRHI, Warning, TEXT("Descriptor Pools still in use!"));
			bFirst = false;
		}

		FScopeLock ScopeLock(&CS);
		for (int32 Index = 0; Index < UsedPools.Num(); ++Index)
		{
			UsedPools[Index]->Destroy(Device);
			delete UsedPools[Index];
		}
		UsedPools.Reset(0);
	}

	{
		FScopeLock ScopeLock(&CS);
		for (int32 Index = 0; Index < FreePools.Num(); ++Index)
		{
			FreePools[Index]->Destroy(Device);
			delete FreePools[Index];
		}
		FreePools.Reset(0);
	}

	CurrentPool = nullptr;
}

void FVulkanPipelineDescriptorSetAllocator::InitLayout(const FVulkanLayout& InLayout, uint32 InNumAllocationsPerPool)
{
	check(InNumAllocationsPerPool > 0);
	NumAllocationsPerPool = InNumAllocationsPerPool;
	const FVulkanDescriptorSetsLayout& DSLayout = InLayout.GetDescriptorSetsLayout();
	const uint32* LayoutTypes = DSLayout.GetLayoutTypes();
	for (int32 Index = 0; Index < VK_DESCRIPTOR_TYPE_RANGE_SIZE; ++Index)
	{
		if (LayoutTypes[Index] > 0)
		{
			VkDescriptorPoolSize* Type = new(CreateInfoTypes) VkDescriptorPoolSize;
			FMemory::Memzero(*Type);
			Type->type = (VkDescriptorType)Index;
			Type->descriptorCount = LayoutTypes[Index] * InNumAllocationsPerPool;
		}
	}

	uint32 MaxDescriptorSets = DSLayout.GetHandles().Num() * InNumAllocationsPerPool;

	FMemory::Memzero(CreateInfo);
	CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	CreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	CreateInfo.poolSizeCount = CreateInfoTypes.Num();
	CreateInfo.pPoolSizes = CreateInfoTypes.GetData();
	CreateInfo.maxSets = MaxDescriptorSets;
}

void FVulkanPipelineDescriptorSetAllocator::Reset()
{
	FScopeLock ScopeLock(&CS);
	for (int32 Index = UsedPools.Num() - 1; Index >= 0; --Index)
	{
		FPool* Pool = UsedPools[Index];
		if (Pool != CurrentPool)
		{
			if (Pool->ProcessFences())
			{
				UsedPools.RemoveAtSwap(Index, 1, false);
				FreePools.Add(Pool);
			}
		}
	}
}


FVulkanDescriptorSetArray* FVulkanPipelineDescriptorSetAllocator::Allocate(FVulkanCommandListContext* CmdListContext, FVulkanCmdBuffer* CmdBuffer, const FVulkanLayout* InLayout)
{
	FVulkanDescriptorSetArray* OutSets = nullptr;

	if (CreateInfo.poolSizeCount == 0)
	{
		return nullptr;
	}

	FScopeLock ScopeLock(&CS);

	// Try the currently used pool
	if (CurrentPool)
	{
		bool bFull = false;
		if (CurrentPool->TryAllocate(CmdListContext, CmdBuffer, bFull, OutSets))
		{
			if (bFull)
			{
				UsedPools.Push(CurrentPool);
			}

			return OutSets;
		}
	}

	// Try one of the freed pools
	if (FreePools.Num() > 0)
	{
		CurrentPool = FreePools.Pop(false);
		bool bFull = false;
		if (CurrentPool->TryAllocate(CmdListContext, CmdBuffer, bFull, OutSets))
		{
			if (bFull)
			{
				UsedPools.Push(CurrentPool);
			}

			return OutSets;
		}
		else
		{
			checkf(0, TEXT("Internal error: Can't use free pool just acquired!"));
		}
	}

	// No pools, so make a new one
	CurrentPool = new FPool(CmdListContext->GetDevice(), InLayout, &CreateInfo, NumAllocationsPerPool);
	bool bFull = false;
	if (CurrentPool->TryAllocate(CmdListContext, CmdBuffer, bFull, OutSets))
	{
		if (bFull)
		{
			UsedPools.Push(CurrentPool);
		}

		return OutSets;
	}
	else
	{
		checkf(0, TEXT("Internal error: Can't use new pool!"));
	}

	return OutSets;
}


FVulkanPipelineDescriptorSetAllocator::FPool::FPool(FVulkanDevice* Device, const FVulkanLayout* InLayout, const VkDescriptorPoolCreateInfo* CreateInfo, uint32 InMaxAllocations)
	: Handle(VK_NULL_HANDLE)
{
	if (CreateInfo->poolSizeCount > 0)
	{
		{
			SCOPE_CYCLE_COUNTER(STAT_VulkanVkCreateDescriptorPool);
			INC_DWORD_STAT(STAT_VulkanDescriptorPools);
			VERIFYVULKANRESULT(VulkanRHI::vkCreateDescriptorPool(Device->GetInstanceHandle(), CreateInfo, GInstrumentedMemoryAllocator, &Handle));
		}

		const TArray<VkDescriptorSetLayout>& LayoutHandles = InLayout->GetDescriptorSetsLayout().GetHandles();

		Entries.AddDefaulted(InMaxAllocations);

		VkDescriptorSetAllocateInfo AllocateInfo;
		FMemory::Memzero(AllocateInfo);
		AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		AllocateInfo.descriptorPool = Handle;
		AllocateInfo.descriptorSetCount =  LayoutHandles.Num();
		AllocateInfo.pSetLayouts = LayoutHandles.GetData();
		for (uint32 Index = 0; Index < InMaxAllocations; ++Index)
		{
			Entries[Index].Allocation.AddUninitialized(LayoutHandles.Num());
			VERIFYVULKANRESULT(VulkanRHI::vkAllocateDescriptorSets(Device->GetInstanceHandle(), &AllocateInfo, Entries[Index].Allocation.GetData()));
		}
	}
}


FVulkanPipelineDescriptorSetAllocator::FPool::~FPool()
{
	ensure(Handle == VK_NULL_HANDLE);
}

void FVulkanPipelineDescriptorSetAllocator::FPool::Destroy(FVulkanDevice* Device)
{
	if (Handle != VK_NULL_HANDLE)
	{
		DEC_DWORD_STAT(STAT_VulkanDescriptorPools);
		VulkanRHI::vkDestroyDescriptorPool(Device->GetInstanceHandle(), Handle, nullptr);
		Handle = VK_NULL_HANDLE;
	}
}

bool FVulkanPipelineDescriptorSetAllocator::FPool::TryAllocate(FVulkanCommandListContext* CmdListContext, FVulkanCmdBuffer* CmdBuffer, bool& bOutIsFullAfterAllocation, FVulkanDescriptorSetArray*& OutSets)
{
	bOutIsFullAfterAllocation = false;

	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (!Entries[Index].CmdBuffer)
		{
			Entries[Index].CmdBuffer = CmdBuffer;
			Entries[Index].FenceCounter = CmdBuffer->GetFenceSignaledCounter();
			++UsedEntries;
			bOutIsFullAfterAllocation = UsedEntries == (uint32)Entries.Num();
			OutSets = &Entries[Index].Allocation;
			return true;
		}
	}

	return false;
}


bool FVulkanPipelineDescriptorSetAllocator::FPool::ProcessFences()
{
	if (UsedEntries == 0)
	{
		for (int32 Index = 0; Index < Entries.Num(); ++Index)
		{
			check(!Entries[Index].CmdBuffer);
		}
		return true;
	}

	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (Entries[Index].CmdBuffer)
		{
			if (Entries[Index].FenceCounter < Entries[Index].CmdBuffer->GetFenceSignaledCounter())
			{
				Entries[Index].CmdBuffer = nullptr;
				Entries[Index].FenceCounter = 0;
				--UsedEntries;
			}
		}
	}

	check(UsedEntries >= 0 && UsedEntries <= Entries.Num());
	return (UsedEntries == 0);
}
#endif
