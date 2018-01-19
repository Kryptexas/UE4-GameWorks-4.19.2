// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanCommandBuffer.cpp: Vulkan device RHI implementation.
=============================================================================*/

#include "VulkanRHIPrivate.h"
#include "VulkanContext.h"
#include "VulkanDescriptorSets.h"

static int32 GUseSingleQueue = 0;
static FAutoConsoleVariableRef CVarVulkanUseSingleQueue(
	TEXT("r.Vulkan.UseSingleQueue"),
	GUseSingleQueue,
	TEXT("Forces using the same queue for uploads and graphics.\n")
	TEXT(" 0: Uses multiple queues(default)\n")
	TEXT(" 1: Always uses the gfx queue for submissions"),
	ECVF_Default
);

static int32 GVulkanProfileCmdBuffers = 0;
static FAutoConsoleVariableRef CVarVulkanProfileCmdBuffers(
	TEXT("r.Vulkan.ProfileCmdBuffers"),
	GVulkanProfileCmdBuffers,
	TEXT("Insert GPU timing queries in every cmd buffer\n"),
	ECVF_Default
);

const uint32 GNumberOfFramesBeforeDeletingDescriptorPool = 300;

FVulkanCmdBuffer::FVulkanCmdBuffer(FVulkanDevice* InDevice, FVulkanCommandBufferPool* InCommandBufferPool)
	: bNeedsDynamicStateSet(true)
	, bHasPipeline(false)
	, bHasViewport(false)
	, bHasScissor(false)
	, bHasStencilRef(false)
	, CurrentStencilRef(0)
	, Device(InDevice)
	, CommandBufferHandle(VK_NULL_HANDLE)
	, State(EState::ReadyForBegin)
	, Fence(nullptr)
	, FenceSignaledCounter(0)
	, SubmittedFenceCounter(0)
	, CommandBufferPool(InCommandBufferPool)
	, Timing(nullptr)
	, LastValidTiming(0)
{
	FMemory::Memzero(CurrentViewport);
	FMemory::Memzero(CurrentScissor);
	
	VkCommandBufferAllocateInfo CreateCmdBufInfo;
	FMemory::Memzero(CreateCmdBufInfo);
	CreateCmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	CreateCmdBufInfo.pNext = nullptr;
	CreateCmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	CreateCmdBufInfo.commandBufferCount = 1;
	CreateCmdBufInfo.commandPool = CommandBufferPool->GetHandle();

	VERIFYVULKANRESULT(VulkanRHI::vkAllocateCommandBuffers(Device->GetInstanceHandle(), &CreateCmdBufInfo, &CommandBufferHandle));
	Fence = Device->GetFenceManager().AllocateFence();
}

FVulkanCmdBuffer::~FVulkanCmdBuffer()
{
	VulkanRHI::FFenceManager& FenceManager = Device->GetFenceManager();
	if (State == EState::Submitted)
	{
		// Wait 60ms
		uint64 WaitForCmdBufferInNanoSeconds = 60 * 1000 * 1000LL;
		FenceManager.WaitAndReleaseFence(Fence, WaitForCmdBufferInNanoSeconds);
	}
	else
	{
		// Just free the fence, CmdBuffer was not submitted
		FenceManager.ReleaseFence(Fence);
	}

	VulkanRHI::vkFreeCommandBuffers(Device->GetInstanceHandle(), CommandBufferPool->GetHandle(), 1, &CommandBufferHandle);
	CommandBufferHandle = VK_NULL_HANDLE;

	if (Timing)
	{
		Timing->Release();
	}
}

void FVulkanCmdBuffer::BeginRenderPass(const FVulkanRenderTargetLayout& Layout, FVulkanRenderPass* RenderPass, FVulkanFramebuffer* Framebuffer, const VkClearValue* AttachmentClearValues)
{
	check(IsOutsideRenderPass());

	VkRenderPassBeginInfo Info;
	FMemory::Memzero(Info);
	Info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	Info.renderPass = RenderPass->GetHandle();
	Info.framebuffer = Framebuffer->GetHandle();
	Info.renderArea.offset.x = 0;
	Info.renderArea.offset.y = 0;
	Info.renderArea.extent.width = Framebuffer->GetWidth();
	Info.renderArea.extent.height = Framebuffer->GetHeight();
	Info.clearValueCount = Layout.GetNumUsedClearValues();
	Info.pClearValues = AttachmentClearValues;

	VulkanRHI::vkCmdBeginRenderPass(CommandBufferHandle, &Info, VK_SUBPASS_CONTENTS_INLINE);

	State = EState::IsInsideRenderPass;
}

void FVulkanCmdBuffer::End()
{
	check(IsOutsideRenderPass());

	if (GVulkanProfileCmdBuffers)
	{
		if (Timing)
		{
			Timing->EndTiming(this);
			LastValidTiming = FenceSignaledCounter;
		}
	}

	VERIFYVULKANRESULT(VulkanRHI::vkEndCommandBuffer(GetHandle()));
	State = EState::HasEnded;
}

inline void FVulkanCmdBuffer::InitializeTimings(FVulkanCommandListContext* InContext)
{
	if (GVulkanProfileCmdBuffers && !Timing)
	{
		if (InContext)
		{
			Timing = new FVulkanGPUTiming(InContext, Device);
			Timing->Initialize();
		}
	}
}

void FVulkanCmdBuffer::Begin()
{
	check(State == EState::ReadyForBegin);

	VkCommandBufferBeginInfo CmdBufBeginInfo;
	FMemory::Memzero(CmdBufBeginInfo);
	CmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	CmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VERIFYVULKANRESULT(VulkanRHI::vkBeginCommandBuffer(CommandBufferHandle, &CmdBufBeginInfo));

	if (GVulkanProfileCmdBuffers)
	{
		InitializeTimings(&Device->GetImmediateContext());
		if (Timing)
		{
			Timing->StartTiming(this);
		}
	}

	bNeedsDynamicStateSet = true;
	State = EState::IsInsideBegin;
}

void FVulkanCmdBuffer::RefreshFenceStatus()
{
	if (State == EState::Submitted)
	{
		VulkanRHI::FFenceManager* FenceMgr = Fence->GetOwner();
		if (FenceMgr->IsFenceSignaled(Fence))
		{
			State = EState::ReadyForBegin;
			bHasPipeline = false;
			bHasViewport = false;
			bHasScissor = false;
			bHasStencilRef = false;

			FMemory::Memzero(CurrentViewport);
			FMemory::Memzero(CurrentScissor);
			CurrentStencilRef = 0;

			VulkanRHI::vkResetCommandBuffer(CommandBufferHandle, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
#if VULKAN_REUSE_FENCES
			Fence->GetOwner()->ResetFence(Fence);
#else
			VulkanRHI::FFence* PrevFence = Fence;
			Fence = FenceMgr->AllocateFence();
			FenceMgr->ReleaseFence(PrevFence);
#endif
			++FenceSignaledCounter;

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
			if (CurrentDescriptorPoolSet)
			{
				CurrentDescriptorPoolSet->SetUsed(false);
				CurrentDescriptorPoolSet = nullptr;
			}
#endif

#if VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
			CommandBufferPool->ResetDescriptors(this);
#endif
		}
	}
	else
	{
		check(!Fence->IsSignaled());
	}
}

FVulkanCommandBufferPool::FVulkanCommandBufferPool(FVulkanDevice* InDevice)
	: Device(InDevice)
	, Handle(VK_NULL_HANDLE)
{
}

FVulkanCommandBufferPool::~FVulkanCommandBufferPool()
{
#if VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
	for (auto Pair : DSAllocators)
	{
		delete Pair.Value;
	}
	DSAllocators.Reset();
#endif

	for (int32 Index = 0; Index < CmdBuffers.Num(); ++Index)
	{
		FVulkanCmdBuffer* CmdBuffer = CmdBuffers[Index];
		delete CmdBuffer;
	}

	VulkanRHI::vkDestroyCommandPool(Device->GetInstanceHandle(), Handle, nullptr);
	Handle = VK_NULL_HANDLE;
}

void FVulkanCommandBufferPool::Create(uint32 QueueFamilyIndex)
{
	VkCommandPoolCreateInfo CmdPoolInfo;
	FMemory::Memzero(CmdPoolInfo);
	CmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	CmdPoolInfo.queueFamilyIndex =  QueueFamilyIndex;
	//#todo-rco: Should we use VK_COMMAND_POOL_CREATE_TRANSIENT_BIT?
	CmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VERIFYVULKANRESULT(VulkanRHI::vkCreateCommandPool(Device->GetInstanceHandle(), &CmdPoolInfo, nullptr, &Handle));
}

FVulkanCommandBufferManager::FVulkanCommandBufferManager(FVulkanDevice* InDevice, FVulkanCommandListContext* InContext)
	: Device(InDevice)
	, Pool(InDevice)
	, Queue(InContext->GetQueue())
	, ActiveCmdBuffer(nullptr)
	, UploadCmdBuffer(nullptr)
{
	check(Device);

	Pool.Create(Queue->GetFamilyIndex());

	ActiveCmdBuffer = Pool.Create();
	ActiveCmdBuffer->InitializeTimings(InContext);
	ActiveCmdBuffer->Begin();

	if (InContext->IsImmediate())
	{
		// Insert the Begin frame timestamp query. On EndDrawingViewport() we'll insert the End and immediately after a new Begin()
		InContext->WriteBeginTimestamp(ActiveCmdBuffer);

		// Flush the cmd buffer immediately to ensure a valid
		// 'Last submitted' cmd buffer exists at frame 0.
		SubmitActiveCmdBuffer(false);
		PrepareForNewActiveCommandBuffer();
	}
}

FVulkanCommandBufferManager::~FVulkanCommandBufferManager()
{
}

void FVulkanCommandBufferManager::WaitForCmdBuffer(FVulkanCmdBuffer* CmdBuffer, float TimeInSecondsToWait)
{
	check(CmdBuffer->IsSubmitted());
	bool bSuccess = Device->GetFenceManager().WaitForFence(CmdBuffer->Fence, (uint64)(TimeInSecondsToWait * 1e9));
	check(bSuccess);
	CmdBuffer->RefreshFenceStatus();
}


void FVulkanCommandBufferManager::SubmitUploadCmdBuffer(bool bWaitForFence)
{
	check(UploadCmdBuffer);
	if (!UploadCmdBuffer->IsSubmitted() && UploadCmdBuffer->HasBegun())
	{
		check(UploadCmdBuffer->IsOutsideRenderPass());
		UploadCmdBuffer->End();
		Queue->Submit(UploadCmdBuffer, nullptr, 0, nullptr);
	}

	if (bWaitForFence)
	{
		if (UploadCmdBuffer->IsSubmitted())
		{
			WaitForCmdBuffer(UploadCmdBuffer);
		}
	}

	UploadCmdBuffer = nullptr;
}

void FVulkanCommandBufferManager::SubmitActiveCmdBuffer(bool bWaitForFence)
{
	check(!UploadCmdBuffer);
	check(ActiveCmdBuffer);
	if (!ActiveCmdBuffer->IsSubmitted() && ActiveCmdBuffer->HasBegun())
	{
		if (!ActiveCmdBuffer->IsOutsideRenderPass())
		{
			UE_LOG(LogVulkanRHI, Warning, TEXT("Forcing EndRenderPass() for submission"));
			ActiveCmdBuffer->EndRenderPass();
		}
		ActiveCmdBuffer->End();
		Queue->Submit(ActiveCmdBuffer, nullptr, 0, nullptr);
	}

	if (bWaitForFence)
	{
		if (ActiveCmdBuffer->IsSubmitted())
		{
			WaitForCmdBuffer(ActiveCmdBuffer);
		}
	}

	ActiveCmdBuffer = nullptr;
}

FVulkanCmdBuffer* FVulkanCommandBufferPool::Create()
{
	check(Device);
	FVulkanCmdBuffer* CmdBuffer = new FVulkanCmdBuffer(Device, this);
	CmdBuffers.Add(CmdBuffer);
	check(CmdBuffer);
	return CmdBuffer;
}

void FVulkanCommandBufferPool::RefreshFenceStatus()
{
	for (int32 Index = 0; Index < CmdBuffers.Num(); ++Index)
	{
		FVulkanCmdBuffer* CmdBuffer = CmdBuffers[Index];
		CmdBuffer->RefreshFenceStatus();
	}
}

void FVulkanCommandBufferManager::PrepareForNewActiveCommandBuffer()
{
	check(!UploadCmdBuffer);

	for (int32 Index = 0; Index < Pool.CmdBuffers.Num(); ++Index)
	{
		FVulkanCmdBuffer* CmdBuffer = Pool.CmdBuffers[Index];
		CmdBuffer->RefreshFenceStatus();
		if (CmdBuffer->State == FVulkanCmdBuffer::EState::ReadyForBegin)
		{
			ActiveCmdBuffer = CmdBuffer;
			ActiveCmdBuffer->Begin();
			return;
		}
		else
		{
			check(CmdBuffer->State == FVulkanCmdBuffer::EState::Submitted);
		}
	}

	// All cmd buffers are being executed still
	ActiveCmdBuffer = Pool.Create();
	ActiveCmdBuffer->Begin();
}

uint32 FVulkanCommandBufferManager::CalculateGPUTime()
{
	uint32 Time = 0;
	for (int32 Index = 0; Index < Pool.CmdBuffers.Num(); ++Index)
	{
		FVulkanCmdBuffer* CmdBuffer = Pool.CmdBuffers[Index];
		if (CmdBuffer->HasValidTiming())
		{
			Time += CmdBuffer->Timing->GetTiming(false);
		}
	}
	return Time;
}

FVulkanCmdBuffer* FVulkanCommandBufferManager::GetUploadCmdBuffer()
{
	if (!UploadCmdBuffer)
	{
		for (int32 Index = 0; Index < Pool.CmdBuffers.Num(); ++Index)
		{
			FVulkanCmdBuffer* CmdBuffer = Pool.CmdBuffers[Index];
			CmdBuffer->RefreshFenceStatus();
			if (CmdBuffer->State == FVulkanCmdBuffer::EState::ReadyForBegin)
			{
				UploadCmdBuffer = CmdBuffer;
				UploadCmdBuffer->Begin();
				return UploadCmdBuffer;
			}
		}

		// All cmd buffers are being executed still
		UploadCmdBuffer = Pool.Create();
		UploadCmdBuffer->Begin();
	}

	return UploadCmdBuffer;
}

#if VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
TArrayView<VkDescriptorSet> FVulkanCommandBufferPool::AllocateDescriptorSets(FVulkanCmdBuffer* CmdBuffer, const FVulkanLayout& Layout)
{
	SCOPE_CYCLE_COUNTER(STAT_VulkanDescriptorSetAllocator);
	if (Layout.GetDescriptorSetsLayout().GetHandles().Num() == 0)
	{
		return TArrayView<VkDescriptorSet>();
	}

	VulkanRHI::FDescriptorSetsAllocator** Found = DSAllocators.Find(Layout.GetDescriptorSetLayoutHash());
	// Should not happen as SetDescriptorSetsFence should have been called beforehand!
	check(Found);
	VulkanRHI::FDescriptorSetsAllocator* Allocator = *Found;

	return Allocator->Allocate(Layout, 8);
}

static bool GDump = false;
void FVulkanCommandBufferPool::ResetDescriptors(FVulkanCmdBuffer* CmdBuffer)
{
	if (GDump)
	{
		int32 NumPools = 0;
		int32 UsedPools = 0;
		int32 FreePools = 0;
		for (auto Pair : DSAllocators)
		{
			VulkanRHI::FDescriptorSetsAllocator* Pool = Pair.Value;
			UsedPools += Pool->UsedEntries.Num();
			FreePools += Pool->FreeEntries.Num();
			NumPools += Pool->FreeEntries.Num() + Pool->UsedEntries.Num();
		}
		UE_LOG(LogVulkanRHI, Log, TEXT("*** Descriptor Sets: %d unique layouts, %d used pools, %d total pools, Waste %.2f%%"), DSAllocators.Num(), UsedPools, NumPools, (float)FreePools * 100.0f / (float)NumPools);
	}

	for (auto Pair : DSAllocators)
	{
		Pair.Value->Reset(CmdBuffer);
	}
}

void FVulkanCommandBufferPool::SetDescriptorSetsFence(FVulkanCmdBuffer* CmdBuffer, const FVulkanLayout& Layout)
{
	VulkanRHI::FDescriptorSetsAllocator** Found = DSAllocators.Find(Layout.GetDescriptorSetLayoutHash());
	VulkanRHI::FDescriptorSetsAllocator* Allocator = nullptr;
	if (Found)
	{
		Allocator = *Found;
	}
	else
	{
		Allocator = new VulkanRHI::FDescriptorSetsAllocator(Device, Layout, 8);
		DSAllocators.Add(Layout.GetDescriptorSetLayoutHash(), Allocator);
	}

	Allocator->SetFence(CmdBuffer, CmdBuffer->GetFenceSignaledCounter());
}

namespace VulkanRHI
{
	FDescriptorSetsAllocator::FDescriptorSetsAllocator(FVulkanDevice* InDevice, const FVulkanLayout& InLayout, uint32 InNumAllocationsPerPool)
		: VulkanRHI::FDeviceChild(InDevice)
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

	FDescriptorSetsAllocator::~FDescriptorSetsAllocator()
	{
		DEC_DWORD_STAT_BY(STAT_VulkanNumDescPools, UsedEntries.Num() + FreeEntries.Num());
		for (int32 Index = 0; Index < UsedEntries.Num(); ++Index)
		{
			FFencedPoolEntry& Entry = UsedEntries[Index];
			VulkanRHI::vkDestroyDescriptorPool(Device->GetInstanceHandle(), Entry.Pool, nullptr);
			delete [] Entry.Sets;
		}
		for (int32 Index = 0; Index < FreeEntries.Num(); ++Index)
		{
			FFreePoolEntry& Entry = FreeEntries[Index];
			VulkanRHI::vkDestroyDescriptorPool(Device->GetInstanceHandle(), Entry.Pool, nullptr);
			delete[] Entry.Sets;
		}
		FreeEntries.SetNum(0, false);
	}

	TArrayView<VkDescriptorSet> FDescriptorSetsAllocator::Allocate(const FVulkanLayout& Layout, uint32 InNumAllocations/*, FVulkanCmdBuffer* CmdBuffer*/)
	{
		int32 NumSets = Layout.GetDescriptorSetsLayout().GetHandles().Num();
		if (NumSets == 0)
		{
			TArrayView<VkDescriptorSet> Out;
			return Out;
		}

		if (UsedEntries.Num() > 0)
		{
			FFencedPoolEntry& Entry = UsedEntries.Last();
			if (Entry.NumUsedSets + NumSets < Entry.MaxSets)
			{
				TArrayView<VkDescriptorSet> Out(Entry.Sets + Entry.NumUsedSets, NumSets);
				Entry.NumUsedSets += NumSets;
				check(Entry.NumUsedSets <= Entry.MaxSets);
				Entry.CmdBuffer = CurrentCmdBuffer;
				Entry.Fence = CurrentFence;
				return Out;
			}
		}

		if (FreeEntries.Num() > 0)
		{
			FPoolEntry& Entry = FreeEntries.Last();

			TArrayView<VkDescriptorSet> Out(Entry.Sets, NumSets);
			FFencedPoolEntry* NewEntry = new(UsedEntries) FFencedPoolEntry(Entry);
			NewEntry->NumUsedSets = NumSets;
			NewEntry->CmdBuffer = CurrentCmdBuffer;
			NewEntry->Fence = CurrentFence;

			FreeEntries.Pop(false);

			return Out;
		}

		FFencedPoolEntry* Entry = CreatePool(Layout);
		TArrayView<VkDescriptorSet> Out(Entry->Sets, NumSets);
		Entry->NumUsedSets = NumSets;
		Entry->CmdBuffer = CurrentCmdBuffer;
		Entry->Fence = CurrentFence;
		return Out;
	}

	FDescriptorSetsAllocator::FFencedPoolEntry* FDescriptorSetsAllocator::CreatePool(const FVulkanLayout& Layout)
	{
		FFencedPoolEntry* Entry = new(UsedEntries) FFencedPoolEntry;

		{
			SCOPE_CYCLE_COUNTER(STAT_VulkanVkCreateDescriptorPool);
			INC_DWORD_STAT(STAT_VulkanNumDescPools);
			VERIFYVULKANRESULT(VulkanRHI::vkCreateDescriptorPool(Device->GetInstanceHandle(), &CreateInfo, nullptr, &Entry->Pool));
		}

		const TArray<VkDescriptorSetLayout>& LayoutHandles = Layout.GetDescriptorSetsLayout().GetHandles();
		int32 NumSets = LayoutHandles.Num();
		Entry->Sets = new VkDescriptorSet[NumAllocationsPerPool * NumSets];
		Entry->MaxSets = NumAllocationsPerPool * NumSets;

		VkDescriptorSetAllocateInfo AllocateInfo;
		FMemory::Memzero(AllocateInfo);
		AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		AllocateInfo.descriptorPool = Entry->Pool;
		AllocateInfo.descriptorSetCount = LayoutHandles.Num();
		AllocateInfo.pSetLayouts = LayoutHandles.GetData();
		for (uint32 Index = 0; Index < NumAllocationsPerPool; ++Index)
		{
			VERIFYVULKANRESULT(VulkanRHI::vkAllocateDescriptorSets(Device->GetInstanceHandle(), &AllocateInfo, Entry->Sets + Index * NumSets));
		}

		return Entry;
	}

	void FDescriptorSetsAllocator::Reset(FVulkanCmdBuffer* CmdBuffer)
	{
		SCOPE_CYCLE_COUNTER(STAT_VulkanDescriptorSetAllocator);
		uint32 CurrentFrame = GFrameNumberRenderThread;
		for (int32 Index = FreeEntries.Num() - 1; Index >= 0; --Index)
		{
			FFreePoolEntry& Entry = FreeEntries[Index];
			if (Entry.LastUsedFrame + GNumberOfFramesBeforeDeletingDescriptorPool < CurrentFrame)
			{
				DEC_DWORD_STAT(STAT_VulkanNumDescPools);
				VulkanRHI::vkDestroyDescriptorPool(Device->GetInstanceHandle(), Entry.Pool, nullptr);
				delete[] Entry.Sets;
				FreeEntries.RemoveAtSwap(Index, 1, false);
			}
		}

		for (int32 Index = UsedEntries.Num() - 1; Index >= 0; --Index)
		{
			FFencedPoolEntry& Entry = UsedEntries[Index];
			if (Entry.CmdBuffer == CmdBuffer && Entry.Fence < CmdBuffer->GetFenceSignaledCounter())
			{
				FFreePoolEntry* FreeEntry = new(FreeEntries) FFreePoolEntry(Entry);
				UsedEntries.RemoveAtSwap(Index, 1, false);
				FreeEntry->LastUsedFrame = CurrentFrame;
			}
		}
	}
}
#endif
