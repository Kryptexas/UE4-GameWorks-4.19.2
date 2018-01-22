// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved..

/*=============================================================================
	VulkanPendingState.cpp: Private VulkanPendingState function definitions.
=============================================================================*/

#include "VulkanRHIPrivate.h"
#include "VulkanPendingState.h"
#include "VulkanPipeline.h"
#include "VulkanContext.h"

#if !VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
FOLDVulkanDescriptorPool::FOLDVulkanDescriptorPool(FVulkanDevice* InDevice, const FVulkanDescriptorSetsLayout& InLayout)
#else
FOLDVulkanDescriptorPool::FOLDVulkanDescriptorPool(FVulkanDevice* InDevice)
#endif
	: Device(InDevice)
	, MaxDescriptorSets(0)
	, NumAllocatedDescriptorSets(0)
	, PeakAllocatedDescriptorSets(0)
#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
	, Layout(InLayout)
#endif
	, DescriptorPool(VK_NULL_HANDLE)
{
#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
	INC_DWORD_STAT(STAT_VulkanNumDescPools);

	// Max number of descriptor sets layout allocations
	const uint32 MaxSetsAllocations = 256;

	// Descriptor sets number required to allocate the max number of descriptor sets layout.
	// When we're hashing pools with types usage ID the descriptor pool can be used for different layouts so the initial layout does not make much sense.
	// In the latter case we'll be probably overallocating the descriptor types but given the relatively small number of max allocations this should not have
	// a serious impact.
	MaxDescriptorSets = MaxSetsAllocations*(VULKAN_HASH_POOLS_WITH_TYPES_USAGE_ID ? 1 : Layout.GetLayouts().Num());
	TArray<VkDescriptorPoolSize, TFixedAllocator<VK_DESCRIPTOR_TYPE_RANGE_SIZE>> Types;
	for (uint32 TypeIndex = VK_DESCRIPTOR_TYPE_BEGIN_RANGE; TypeIndex < VK_DESCRIPTOR_TYPE_END_RANGE; ++TypeIndex)
	{
		VkDescriptorType DescriptorType =(VkDescriptorType)TypeIndex;
		uint32 NumTypesUsed = Layout.GetTypesUsed(DescriptorType);
		if (NumTypesUsed > 0)
		{
			VkDescriptorPoolSize* Type = new(Types) VkDescriptorPoolSize;
			FMemory::Memzero(*Type);
			Type->type = DescriptorType;
			Type->descriptorCount = NumTypesUsed * MaxSetsAllocations;
		}
	}
#else
	// Increased from 8192 to prevent Protostar crashing on Mali
	MaxDescriptorSets = 16384;

	const VkPhysicalDeviceLimits& Limits = Device->GetLimits();
	FMemory::Memzero(MaxAllocatedTypes);
	FMemory::Memzero(NumAllocatedTypes);
	FMemory::Memzero(PeakAllocatedTypes);

	//#todo-rco: Get some initial values
	uint32 LimitMaxUniformBuffers = 2048;
	uint32 LimitMaxSamplers = 1024;
	uint32 LimitMaxCombinedImageSamplers = 4096;
	uint32 LimitMaxUniformTexelBuffers = 512;
	uint32 LimitMaxStorageTexelBuffers = 512;
	uint32 LimitMaxStorageBuffers = 512;
	uint32 LimitMaxStorageImage = 512;

	TArray<VkDescriptorPoolSize> Types;
	VkDescriptorPoolSize* Type = new(Types) VkDescriptorPoolSize;
	FMemory::Memzero(*Type);
	Type->type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	Type->descriptorCount = LimitMaxUniformBuffers;

	Type = new(Types) VkDescriptorPoolSize;
	FMemory::Memzero(*Type);
	Type->type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	Type->descriptorCount = LimitMaxUniformBuffers;

	Type = new(Types) VkDescriptorPoolSize;
	FMemory::Memzero(*Type);
	Type->type = VK_DESCRIPTOR_TYPE_SAMPLER;
	Type->descriptorCount = LimitMaxSamplers;

	Type = new(Types) VkDescriptorPoolSize;
	FMemory::Memzero(*Type);
	Type->type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	Type->descriptorCount = LimitMaxCombinedImageSamplers;

	Type = new(Types) VkDescriptorPoolSize;
	FMemory::Memzero(*Type);
	Type->type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
	Type->descriptorCount = LimitMaxUniformTexelBuffers;

	Type = new(Types) VkDescriptorPoolSize;
	FMemory::Memzero(*Type);
	Type->type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
	Type->descriptorCount = LimitMaxStorageTexelBuffers;

	Type = new(Types) VkDescriptorPoolSize;
	FMemory::Memzero(*Type);
	Type->type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	Type->descriptorCount = LimitMaxStorageBuffers;

	Type = new(Types) VkDescriptorPoolSize;
	FMemory::Memzero(*Type);
	Type->type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	Type->descriptorCount = LimitMaxStorageImage;

	for (const VkDescriptorPoolSize& PoolSize : Types)
	{
		MaxAllocatedTypes[PoolSize.type] = PoolSize.descriptorCount;
	}
#endif

	VkDescriptorPoolCreateInfo PoolInfo;
	FMemory::Memzero(PoolInfo);
	PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	PoolInfo.poolSizeCount = Types.Num();
	PoolInfo.pPoolSizes = Types.GetData();
	PoolInfo.maxSets = MaxDescriptorSets;


	SCOPE_CYCLE_COUNTER(STAT_VulkanVkCreateDescriptorPool);
	VERIFYVULKANRESULT(VulkanRHI::vkCreateDescriptorPool(Device->GetInstanceHandle(), &PoolInfo, nullptr, &DescriptorPool));
}

FOLDVulkanDescriptorPool::~FOLDVulkanDescriptorPool()
{
#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
	DEC_DWORD_STAT(STAT_VulkanNumDescPools);
#endif

	if (DescriptorPool != VK_NULL_HANDLE)
	{
		VulkanRHI::vkDestroyDescriptorPool(Device->GetInstanceHandle(), DescriptorPool, nullptr);
		DescriptorPool = VK_NULL_HANDLE;
	}
}

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
void FOLDVulkanDescriptorPool::TrackAddUsage(const FVulkanDescriptorSetsLayout& InLayout)
#else
void FOLDVulkanDescriptorPool::TrackAddUsage(const FVulkanDescriptorSetsLayout& Layout)
#endif
{
	// Check and increment our current type usage
	for (uint32 TypeIndex = VK_DESCRIPTOR_TYPE_BEGIN_RANGE; TypeIndex < VK_DESCRIPTOR_TYPE_END_RANGE; ++TypeIndex)
	{
#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
		check(Layout.GetTypesUsed((VkDescriptorType)TypeIndex) == InLayout.GetTypesUsed((VkDescriptorType)TypeIndex));
#else
		NumAllocatedTypes[TypeIndex] +=	(int32)Layout.GetTypesUsed((VkDescriptorType)TypeIndex);
		PeakAllocatedTypes[TypeIndex] = FMath::Max(PeakAllocatedTypes[TypeIndex], NumAllocatedTypes[TypeIndex]);
#endif
	}

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
	NumAllocatedDescriptorSets += InLayout.GetLayouts().Num();
#else
	NumAllocatedDescriptorSets += Layout.GetLayouts().Num();
#endif
	PeakAllocatedDescriptorSets = FMath::Max(NumAllocatedDescriptorSets, PeakAllocatedDescriptorSets);
}

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
void FOLDVulkanDescriptorPool::TrackRemoveUsage(const FVulkanDescriptorSetsLayout& InLayout)
#else
void FOLDVulkanDescriptorPool::TrackRemoveUsage(const FVulkanDescriptorSetsLayout& Layout)
#endif
{
	for (uint32 TypeIndex = VK_DESCRIPTOR_TYPE_BEGIN_RANGE; TypeIndex < VK_DESCRIPTOR_TYPE_END_RANGE; ++TypeIndex)
	{
#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
		check(Layout.GetTypesUsed((VkDescriptorType)TypeIndex) == InLayout.GetTypesUsed((VkDescriptorType)TypeIndex));
#else
		NumAllocatedTypes[TypeIndex] -=	(int32)Layout.GetTypesUsed((VkDescriptorType)TypeIndex);
		check(NumAllocatedTypes[TypeIndex] >= 0);
#endif
	}

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
	NumAllocatedDescriptorSets -= InLayout.GetLayouts().Num();
#else
	NumAllocatedDescriptorSets -= Layout.GetLayouts().Num();
#endif
}
#endif

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
void FOLDVulkanDescriptorPool::Reset()
{
	if (DescriptorPool != VK_NULL_HANDLE)
	{
		VERIFYVULKANRESULT(VulkanRHI::vkResetDescriptorPool(Device->GetInstanceHandle(), DescriptorPool, 0));
	}

	NumAllocatedDescriptorSets = 0;
}

bool FOLDVulkanDescriptorPool::AllocateDescriptorSets(const VkDescriptorSetAllocateInfo& InDescriptorSetAllocateInfo, VkDescriptorSet* OutSets)
{
	VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo = InDescriptorSetAllocateInfo;
	DescriptorSetAllocateInfo.descriptorPool = DescriptorPool;

	return VK_SUCCESS == VulkanRHI::vkAllocateDescriptorSets(Device->GetInstanceHandle(), &DescriptorSetAllocateInfo, OutSets);
}

FVulkanTypedDescriptorPoolSet::~FVulkanTypedDescriptorPoolSet()
{
	for (auto Pool = PoolListHead; Pool;)
	{
		auto Next = Pool->Next;

		delete Pool->Element;
		delete Pool;

		Pool = Next;
	}
}

FOLDVulkanDescriptorPool* FVulkanTypedDescriptorPoolSet::PushNewPool()
{
	auto* NewPool = new FOLDVulkanDescriptorPool(Device, Layout);

	if (PoolListCurrent)
	{
		PoolListCurrent->Next = new FPoolList(NewPool);
		PoolListCurrent = PoolListCurrent->Next;
	}
	else
	{
		PoolListCurrent = PoolListHead = new FPoolList(NewPool);
	}

	return NewPool;
}

FOLDVulkanDescriptorPool* FVulkanTypedDescriptorPoolSet::GetFreePool(bool bForceNewPool)
{
	// Likely this
	if (!bForceNewPool)
	{
		return PoolListCurrent->Element;
	}

	if (PoolListCurrent->Next)
	{
		PoolListCurrent = PoolListCurrent->Next;
		return PoolListCurrent->Element;
	}

	return PushNewPool();
}

bool FVulkanTypedDescriptorPoolSet::AllocateDescriptorSets(const FVulkanDescriptorSetsLayout& InLayout, VkDescriptorSet* OutSets)
{
	const TArray<VkDescriptorSetLayout>& LayoutHandles = InLayout.GetHandles();

	if (LayoutHandles.Num() > 0)
	{
		auto Pool = PoolListCurrent->Element;
		while (!Pool->AllocateDescriptorSets(InLayout.GetAllocateInfo(), OutSets))
		{
			Pool = GetFreePool(true);
		}

#if VULKAN_ENABLE_AGGRESSIVE_STATS
		INC_DWORD_STAT_BY(STAT_VulkanNumDescSetsTotal, LayoutHandles.Num());
		Pool->TrackAddUsage(InLayout);
#endif

		return true;
	}

	return true;
}

void FVulkanTypedDescriptorPoolSet::Reset()
{
	for (auto Pool = PoolListHead; Pool; Pool = Pool->Next)
	{
		Pool->Element->Reset();
	}

	PoolListCurrent = PoolListHead;
}

FVulkanDescriptorPoolSet::~FVulkanDescriptorPoolSet()
{
	for (auto TypedDescriptorPools : DescriptorPools)
	{
		delete TypedDescriptorPools.Value;
	}

	DescriptorPools.Reset();
}

FVulkanTypedDescriptorPoolSet* FVulkanDescriptorPoolSet::AcquirePoolSet(const FVulkanDescriptorSetsLayout& Layout)
{
	const uint32 Hash = VULKAN_HASH_POOLS_WITH_TYPES_USAGE_ID ? Layout.GetTypesUsageID() : GetTypeHash(Layout);

	FVulkanTypedDescriptorPoolSet* TypedDescriptorPoolSet = DescriptorPools.FindRef(Hash);

	if (TypedDescriptorPoolSet == nullptr)
	{
		TypedDescriptorPoolSet = new FVulkanTypedDescriptorPoolSet(Device, this, Layout);
		DescriptorPools.Add(Hash, TypedDescriptorPoolSet);
	}

	return TypedDescriptorPoolSet;
}

void FVulkanDescriptorPoolSet::Reset()
{
	for (auto TypedDescriptorPools : DescriptorPools)
	{
		TypedDescriptorPools.Value->Reset();
	}
}

FVulkanDescriptorPoolsManager::~FVulkanDescriptorPoolsManager()
{
	for (auto PoolSet : PoolSets)
	{
		delete PoolSet;
	}

	PoolSets.Reset();
}

FVulkanDescriptorPoolSet& FVulkanDescriptorPoolsManager::AcquirePoolSet()
{
	FScopeLock ScopeLock(&CS);

	for (auto PoolSet : PoolSets)
	{
		if (PoolSet->IsUnused())
		{
			PoolSet->SetUsed(true);
			PoolSet->Reset();
			return *PoolSet;
		}
	}

	auto PoolSet = new FVulkanDescriptorPoolSet(Device);
	PoolSets.Add(PoolSet);

	return *PoolSet;
}

void FVulkanDescriptorPoolsManager::GC()
{
	FScopeLock ScopeLock(&CS);

	// Pool sets are forward allocated - iterate from the back to increase the chance of finding an unused one
	for (int32 Index = PoolSets.Num() - 1; Index >= 0; Index--)
	{
		auto PoolSet = PoolSets[Index];
		if (PoolSet->IsUnused() && GFrameNumberRenderThread - PoolSet->GetLastFrameUsed() > NUM_FRAMES_TO_WAIT_BEFORE_RELEASING_TO_OS)
		{
			PoolSets.RemoveAtSwap(Index, 1, true);

			if (AsyncDeletionTask)
			{
				if (!AsyncDeletionTask->IsDone())
				{
					AsyncDeletionTask->EnsureCompletion();
				}

				AsyncDeletionTask->GetTask().SetPoolSet(PoolSet);
			}
			else
			{
				AsyncDeletionTask = new FAsyncTask<FVulkanAsyncPoolSetDeletionWorker>(PoolSet);
			}

			AsyncDeletionTask->StartBackgroundTask();

			break;
		}
	}
}
#endif



FVulkanPendingComputeState::~FVulkanPendingComputeState()
{
	for (auto Pair : PipelineStates)
	{
		FVulkanComputePipelineState* State = Pair.Value;
		delete State;
	}
}


void FVulkanPendingComputeState::SetSRV(uint32 BindIndex, FVulkanShaderResourceView* SRV)
{
	if (SRV)
	{
		// make sure any dynamically backed SRV points to current memory
		SRV->UpdateView();
		if (SRV->BufferViews.Num() != 0)
		{
			FVulkanBufferView* BufferView = SRV->GetBufferView();
			checkf(BufferView->View != VK_NULL_HANDLE, TEXT("Empty SRV"));
			CurrentState->SetSRVBufferViewState(BindIndex, BufferView);
		}
		else if (SRV->SourceStructuredBuffer)
		{
			CurrentState->SetStorageBuffer(BindIndex, SRV->SourceStructuredBuffer->GetHandle(), SRV->SourceStructuredBuffer->GetOffset(), SRV->SourceStructuredBuffer->GetSize(), SRV->SourceStructuredBuffer->GetBufferUsageFlags());
		}
		else
		{
			checkf(SRV->TextureView.View != VK_NULL_HANDLE, TEXT("Empty SRV"));
			VkImageLayout Layout = Context.FindLayout(SRV->TextureView.Image);
			CurrentState->SetSRVTextureView(BindIndex, SRV->TextureView, Layout);
		}
	}
	else
	{
		//CurrentState->SetSRVBufferViewState(BindIndex, nullptr);
	}
}

void FVulkanPendingComputeState::SetUAV(uint32 UAVIndex, FVulkanUnorderedAccessView* UAV)
{
	if (UAV)
	{
		// make sure any dynamically backed UAV points to current memory
		UAV->UpdateView();
		if (UAV->SourceStructuredBuffer)
		{
			CurrentState->SetStorageBuffer(UAVIndex, UAV->SourceStructuredBuffer->GetHandle(), UAV->SourceStructuredBuffer->GetOffset(), UAV->SourceStructuredBuffer->GetSize(), UAV->SourceStructuredBuffer->GetBufferUsageFlags());
		}
		else if (UAV->BufferView)
		{
			CurrentState->SetUAVTexelBufferViewState(UAVIndex, UAV->BufferView);
		}
		else if (UAV->SourceTexture)
		{
			VkImageLayout Layout = Context.FindOrAddLayout(UAV->TextureView.Image, VK_IMAGE_LAYOUT_UNDEFINED);
			if (Layout != VK_IMAGE_LAYOUT_GENERAL)
			{
				FVulkanTextureBase* VulkanTexture = GetVulkanTextureFromRHITexture(UAV->SourceTexture);
				FVulkanCmdBuffer* CmdBuffer = Context.GetCommandBufferManager()->GetActiveCmdBuffer();
				ensure(CmdBuffer->IsOutsideRenderPass());
				Context.GetTransitionState().TransitionResource(CmdBuffer, VulkanTexture->Surface, VulkanRHI::EImageLayoutBarrier::ComputeGeneralRW);
			}
			CurrentState->SetUAVTextureView(UAVIndex, UAV->TextureView);
		}
		else
		{
			ensure(0);
		}
	}
}


void FVulkanPendingComputeState::PrepareForDispatch(FVulkanCmdBuffer* InCmdBuffer)
{
	SCOPE_CYCLE_COUNTER(STAT_VulkanDispatchCallPrepareTime);

	check(CurrentState);
#if VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
	TArrayView<VkDescriptorSet> DescriptorSetHandles = CurrentState->UpdateDescriptorSets(&Context, InCmdBuffer, &GlobalUniformPool);

	VkCommandBuffer CmdBuffer = InCmdBuffer->GetHandle();

	{
		SCOPE_CYCLE_COUNTER(STAT_VulkanPipelineBind);
		CurrentPipeline->Bind(CmdBuffer);
		InCmdBuffer->SetDescriptorSetsFence(CurrentPipeline->GetLayout());
		if (DescriptorSetHandles.Num() > 0)
		{
			CurrentState->BindDescriptorSets(CmdBuffer, DescriptorSetHandles);
		}
	}
#else
	const bool bHasDescriptorSets = CurrentState->UpdateDescriptorSets(&Context, InCmdBuffer, &GlobalUniformPool);

	VkCommandBuffer CmdBuffer = InCmdBuffer->GetHandle();

	{
		//#todo-rco: Move this to SetComputePipeline()
		SCOPE_CYCLE_COUNTER(STAT_VulkanPipelineBind);
		CurrentPipeline->Bind(CmdBuffer);
		if (bHasDescriptorSets)
		{
			CurrentState->BindDescriptorSets(CmdBuffer);
		}
	}
#endif
}

FVulkanPendingGfxState::~FVulkanPendingGfxState()
{
	for (auto Pair : PipelineStates)
	{
		FVulkanGfxPipelineState* State = Pair.Value;
		delete State;
	}
}

void FVulkanPendingGfxState::PrepareForDraw(FVulkanCmdBuffer* CmdBuffer, VkPrimitiveTopology Topology)
{
	SCOPE_CYCLE_COUNTER(STAT_VulkanDrawCallPrepareTime);

	ensure(Topology == UEToVulkanType(CurrentPipeline->PipelineStateInitializer.PrimitiveType));

#if VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
	const TArrayView<VkDescriptorSet> DescriptorSetHandles = CurrentState->UpdateDescriptorSets(&Context, CmdBuffer, &GlobalUniformPool);

	UpdateDynamicStates(CmdBuffer);

	if (DescriptorSetHandles.Num() > 0)
	{
		CurrentState->BindDescriptorSets(CmdBuffer->GetHandle(), DescriptorSetHandles);
	}
#else
	bool bHasDescriptorSets = CurrentState->UpdateDescriptorSets(&Context, CmdBuffer, &GlobalUniformPool);

	UpdateDynamicStates(CmdBuffer);

	if (bHasDescriptorSets)
	{
		CurrentState->BindDescriptorSets(CmdBuffer->GetHandle());
	}
#endif

	if (bDirtyVertexStreams)
	{
#if VULKAN_ENABLE_AGGRESSIVE_STATS
		SCOPE_CYCLE_COUNTER(STAT_VulkanBindVertexStreamsTime);
#endif
		// Its possible to have no vertex buffers
		const FVulkanVertexInputStateInfo& VertexInputStateInfo = CurrentPipeline->Pipeline->GetVertexInputState();
		if (VertexInputStateInfo.AttributesNum == 0)
		{
			// However, we need to verify that there are also no bindings
			check(VertexInputStateInfo.BindingsNum == 0);
			return;
		}

		TemporaryIA.VertexBuffers.Reset(0);
		TemporaryIA.VertexOffsets.Reset(0);
		const VkVertexInputAttributeDescription* CurrAttribute = nullptr;
		for (uint32 BindingIndex = 0; BindingIndex < VertexInputStateInfo.BindingsNum; BindingIndex++)
		{
			const VkVertexInputBindingDescription& CurrBinding = VertexInputStateInfo.Bindings[BindingIndex];

			uint32 StreamIndex = VertexInputStateInfo.BindingToStream.FindChecked(BindingIndex);
			FVulkanPendingGfxState::FVertexStream& CurrStream = PendingStreams[StreamIndex];

			// Verify the vertex buffer is set
			if (/*!CurrStream.Stream && */!CurrStream.Stream2 && CurrStream.Stream3 == VK_NULL_HANDLE)
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
							*CurrentBSS->GetShader(SF_Vertex)->GetDebugName());
						ensure(0);
					}
				}
#endif
				continue;
			}

			TemporaryIA.VertexBuffers.Add(/*CurrStream.Stream
								  ? CurrStream.Stream->GetBufferHandle()
								  : (*/CurrStream.Stream2
								  ? CurrStream.Stream2->GetHandle()
								  : CurrStream.Stream3//)
			);
			TemporaryIA.VertexOffsets.Add(CurrStream.BufferOffset);
		}

		if (TemporaryIA.VertexBuffers.Num() > 0)
		{
			// Bindings are expected to be in ascending order with no index gaps in between:
			// Correct:		0, 1, 2, 3
			// Incorrect:	1, 0, 2, 3
			// Incorrect:	0, 2, 3, 5
			// Reordering and creation of stream binding index is done in "GenerateVertexInputStateInfo()"
			VulkanRHI::vkCmdBindVertexBuffers(CmdBuffer->GetHandle(), 0, TemporaryIA.VertexBuffers.Num(), TemporaryIA.VertexBuffers.GetData(), TemporaryIA.VertexOffsets.GetData());
		}

		bDirtyVertexStreams = true;
	}
}

void FVulkanPendingGfxState::InternalUpdateDynamicStates(FVulkanCmdBuffer* Cmd)
{
	bool bInCmdNeedsDynamicState = Cmd->bNeedsDynamicStateSet;

	bool bNeedsUpdateViewport = !Cmd->bHasViewport || (Cmd->bHasViewport && FMemory::Memcmp((const void*)&Cmd->CurrentViewport, (const void*)&Viewport, sizeof(VkViewport)) != 0);
	// Validate and update Viewport
	if (bNeedsUpdateViewport)
	{
		ensure(Viewport.width > 0 || Viewport.height > 0);
		VulkanRHI::vkCmdSetViewport(Cmd->GetHandle(), 0, 1, &Viewport);
		FMemory::Memcpy(Cmd->CurrentViewport, Viewport);
		Cmd->bHasViewport = true;
	}

	bool bNeedsUpdateScissor = !Cmd->bHasScissor || (Cmd->bHasScissor && FMemory::Memcmp((const void*)&Cmd->CurrentScissor, (const void*)&Scissor, sizeof(VkRect2D)) != 0);
	if (bNeedsUpdateScissor)
	{
		VulkanRHI::vkCmdSetScissor(Cmd->GetHandle(), 0, 1, &Scissor);
		FMemory::Memcpy(Cmd->CurrentScissor, Scissor);
		Cmd->bHasScissor = true;
	}

	bool bNeedsUpdateStencil = !Cmd->bHasStencilRef || (Cmd->bHasStencilRef && Cmd->CurrentStencilRef != StencilRef);
	if (bNeedsUpdateStencil)
	{
		VulkanRHI::vkCmdSetStencilReference(Cmd->GetHandle(), VK_STENCIL_FRONT_AND_BACK, StencilRef);
		Cmd->CurrentStencilRef = StencilRef;
		Cmd->bHasStencilRef = true;
	}

	Cmd->bNeedsDynamicStateSet = false;
}

void FVulkanPendingGfxState::SetSRV(EShaderFrequency Stage, uint32 BindIndex, FVulkanShaderResourceView* SRV)
{
	if (SRV)
	{
		// make sure any dynamically backed SRV points to current memory
		SRV->UpdateView();
		if (SRV->BufferViews.Num() != 0)
		{
			FVulkanBufferView* BufferView = SRV->GetBufferView();
			checkf(BufferView->View != VK_NULL_HANDLE, TEXT("Empty SRV"));
			CurrentState->SetSRVBufferViewState(Stage, BindIndex, BufferView);
		}
		else if (SRV->SourceStructuredBuffer)
		{
			CurrentState->SetStorageBuffer(Stage, BindIndex, SRV->SourceStructuredBuffer->GetHandle(), SRV->SourceStructuredBuffer->GetOffset(), SRV->SourceStructuredBuffer->GetSize(), SRV->SourceStructuredBuffer->GetBufferUsageFlags());
		}
		else
		{
			checkf(SRV->TextureView.View != VK_NULL_HANDLE, TEXT("Empty SRV"));
			VkImageLayout Layout = Context.FindLayout(SRV->TextureView.Image);
			CurrentState->SetSRVTextureView(Stage, BindIndex, SRV->TextureView, Layout);
		}
	}
	else
	{
		//CurrentState->SetSRVBufferViewState(Stage, BindIndex, nullptr);
	}
}

void FVulkanPendingGfxState::SetUAV(EShaderFrequency Stage, uint32 UAVIndex, FVulkanUnorderedAccessView* UAV)
{
	if (UAV)
	{
		// make sure any dynamically backed UAV points to current memory
		UAV->UpdateView();
		if (UAV->SourceStructuredBuffer)
		{
			CurrentState->SetStorageBuffer(Stage, UAVIndex, UAV->SourceStructuredBuffer->GetHandle(), UAV->SourceStructuredBuffer->GetOffset(), UAV->SourceStructuredBuffer->GetSize(), UAV->SourceStructuredBuffer->GetBufferUsageFlags());
		}
		else if (UAV->BufferView)
		{
			CurrentState->SetUAVTexelBufferViewState(Stage, UAVIndex, UAV->BufferView);
		}
		else if (UAV->SourceTexture)
		{
			VkImageLayout Layout = Context.FindLayout(UAV->TextureView.Image);
			CurrentState->SetUAVTextureView(Stage, UAVIndex, UAV->TextureView, Layout);
		}
		else
		{
			ensure(0);
		}
	}
}
