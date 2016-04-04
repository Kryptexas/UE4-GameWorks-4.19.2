// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanIndexBuffer.cpp: Vulkan Index buffer RHI implementation.
=============================================================================*/

#include "VulkanRHIPrivate.h"
#include "VulkanDevice.h"
#include "VulkanContext.h"

/** Constructor */
#if VULKAN_USE_NEW_RESOURCE_MANAGEMENT

static TMap<FVulkanIndexBuffer*, VulkanRHI::FPendingBuffer2Lock> GPendingLockIBs;

FVulkanIndexBuffer::FVulkanIndexBuffer(FVulkanDevice* InDevice, VkBuffer InBuffer, VulkanRHI::FResourceAllocation* InResourceAllocation, uint32 InStride, uint32 InSize, uint32 InUsage)
	: FRHIIndexBuffer(InStride, InSize, InUsage)
	, IndexType(InStride == 4 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16)
{
	Buffer = new FVulkanBuffer2(InDevice, InBuffer, InResourceAllocation);
}

#else
FVulkanIndexBuffer::FVulkanIndexBuffer(FVulkanDevice& InDevice, uint32 InStride, uint32 InSize, uint32 InUsage)
	: FRHIIndexBuffer(InStride, InSize, InUsage)
	, FVulkanMultiBuffer(InDevice, InSize, (EBufferUsageFlags)InUsage, VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
	, IndexType(InStride == 4 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16)
{
}
#endif

FIndexBufferRHIRef FVulkanDynamicRHI::RHICreateIndexBuffer(uint32 Stride, uint32 Size, uint32 InUsage, FRHIResourceCreateInfo& CreateInfo)
{
#if VULKAN_USE_NEW_RESOURCE_MANAGEMENT
	check(Size > 0);

	VkBufferUsageFlags BufferUsageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	const bool bStatic = (InUsage & BUF_Static) != 0;
	const bool bDynamic = (InUsage & BUF_Dynamic) != 0;
	const bool bVolatile = (InUsage & BUF_Volatile) != 0;

	VkBuffer Buffer = VK_NULL_HANDLE;
	VulkanRHI::FResourceAllocation* ResourceAllocation = FVulkanBuffer2::CreateResourceAllocationAndBuffer(Device, Size, InUsage, BufferUsageFlags, Buffer);
	FVulkanIndexBuffer* NewBuffer = nullptr;
	if (bDynamic)
	{
		//#todo-rco: Allocate in the CPU staging heap
		NewBuffer = new FVulkanIndexBuffer(Device, Buffer, ResourceAllocation, Stride, Size, InUsage);

		if (CreateInfo.ResourceArray)
		{
			uint32 CopyDataSize = FMath::Min(Size, CreateInfo.ResourceArray->GetResourceDataSize());

			// Already mapped
			FMemory::Memcpy(ResourceAllocation->GetMappedPointer(), CreateInfo.ResourceArray->GetResourceData(), CopyDataSize);

			CreateInfo.ResourceArray->Discard();
		}
	}
#if 0
	else if (bVolatile)
	{
		check(0);
	}
#endif
	else
	{
		// Static buffer path
		ensure(bStatic);
		NewBuffer = new FVulkanIndexBuffer(Device, Buffer, ResourceAllocation, Stride, Size, InUsage);

		if (CreateInfo.ResourceArray)
		{
			uint32 CopyDataSize = FMath::Min(Size, CreateInfo.ResourceArray->GetResourceDataSize());
			void* Data = this->RHILockIndexBuffer(NewBuffer, 0, CopyDataSize, RLM_WriteOnly);
			FMemory::Memcpy(Data, CreateInfo.ResourceArray->GetResourceData(), CopyDataSize);
			this->RHIUnlockIndexBuffer(NewBuffer);

			CreateInfo.ResourceArray->Discard();
		}
	}

	return NewBuffer;
#else
	// make the RHI object, which will allocate memory
	FVulkanIndexBuffer* IndexBuffer = new FVulkanIndexBuffer(*Device, Stride, Size, InUsage);
	if (CreateInfo.ResourceArray)
	{
		check(IndexBuffer->GetSize() == CreateInfo.ResourceArray->GetResourceDataSize());
		auto* Buffer = IndexBuffer->Lock(RLM_WriteOnly, Size);

		// copy the contents of the given data into the buffer
		FMemory::Memcpy(Buffer, CreateInfo.ResourceArray->GetResourceData(), Size);

		IndexBuffer->Unlock();

		// Discard the resource array's contents.
		CreateInfo.ResourceArray->Discard();
	}
	return IndexBuffer;
#endif
}

void* FVulkanDynamicRHI::RHILockIndexBuffer(FIndexBufferRHIParamRef IndexBufferRHI, uint32 Offset, uint32 Size, EResourceLockMode LockMode)
{
	FVulkanIndexBuffer* IndexBuffer = ResourceCast(IndexBufferRHI);
#if VULKAN_USE_NEW_RESOURCE_MANAGEMENT
	void* Data = nullptr;

	const bool bIsDynamic = (IndexBuffer->GetUsage() & BUF_AnyDynamic) != 0;
	if (bIsDynamic)
	{
		Data = IndexBuffer->GetResourceAllocation()->GetMappedPointer();
	}
	else
	{
		if (LockMode == RLM_ReadOnly)
		{
			check(0);
		}
		else
		{
			check(LockMode == RLM_WriteOnly);
			auto* StagingBuffer = new VulkanRHI::FStagingBuffer2(Device, Size);
			VulkanRHI::FResourceAllocation* ResourceAllocation = Device->GetResourceHeapManager().AllocateCPUStagingResource(Size, StagingBuffer->GetAlignment(), __FILE__, __LINE__);
			StagingBuffer->BindAllocation(ResourceAllocation);

			{
				FScopeLock ScopeLock(&LockBufferCS);
				check(!GPendingLockIBs.Contains(IndexBuffer));

				VulkanRHI::FPendingBuffer2Lock PendingLock;
				PendingLock.ResourceAllocation = ResourceAllocation;
				PendingLock.Offset = Offset;
				PendingLock.Size = Size;
				PendingLock.StagingBuffer = StagingBuffer;

				GPendingLockIBs.Add(IndexBuffer, PendingLock);
			}

			Data = ResourceAllocation->GetMappedPointer();
		}
	}

	check(Data);
	return Data;
#else
	return IndexBuffer->Lock(LockMode, Size, Offset);
#endif
}

void FVulkanDynamicRHI::RHIUnlockIndexBuffer(FIndexBufferRHIParamRef IndexBufferRHI)
{
	FVulkanIndexBuffer* IndexBuffer = ResourceCast(IndexBufferRHI);
#if VULKAN_USE_NEW_RESOURCE_MANAGEMENT
	VulkanRHI::FResourceAllocation* ResourceAllocation = nullptr;
	uint32 LockSize = 0;
	uint32 LockOffset = 0;
	TRefCountPtr<VulkanRHI::FStagingBuffer2> StagingBuffer;

	const bool bIsDynamic = (IndexBuffer->GetUsage() & BUF_AnyDynamic) != 0;
	if (bIsDynamic)
	{
		//#todo-rco: Flush?
	}
	else
	{
		{
			FScopeLock ScopeLock(&LockBufferCS);
			VulkanRHI::FPendingBuffer2Lock PendingLock = GPendingLockIBs.FindAndRemoveChecked(IndexBuffer);
			ResourceAllocation = PendingLock.ResourceAllocation;
			LockSize = PendingLock.Size;
			LockOffset = PendingLock.Offset;
			StagingBuffer = PendingLock.StagingBuffer;
		}
#if VULKAN_USE_NEW_COMMAND_BUFFERS
		FVulkanCmdBuffer* Cmd = Device->GetImmediateContext().GetCommandBufferManager()->GetActiveCmdBuffer();
		ensure(Cmd->IsOutsideRenderPass());
		VkCommandBuffer CmdBuffer = Cmd->GetHandle();
#else
		VkCommandBuffer CmdBuffer = VK_NULL_HANDLE;
		FVulkanCmdBuffer* CmdObject = nullptr;

		CmdObject = Device->GetImmediateContext().GetCommandBufferManager()->Create();
		check(CmdObject);
		CmdObject->Begin();
		CmdBuffer = CmdObject->GetHandle();
#endif

		VkBufferCopy Region;
		FMemory::Memzero(Region);
		Region.size = LockSize;
		Region.dstOffset = LockOffset;
		Region.srcOffset = 0;
		vkCmdCopyBuffer(CmdBuffer, StagingBuffer.GetReference()->Buffer, IndexBuffer->GetBuffer()->GetBuffer(), 1, &Region);
		//UpdateBuffer(ResourceAllocation, IndexBuffer->GetBuffer(), LockSize, LockOffset);

#if VULKAN_USE_NEW_COMMAND_BUFFERS && VULKAN_USE_NEW_RESOURCE_MANAGEMENT
		Device->GetDeferredDeletionQueue().EnqueueResource(Cmd, StagingBuffer);
#else
		//if (!State)
		{
			check(CmdObject);
			//#todo-rco: TEMP!
			Device->GetQueue()->SubmitBlocking(CmdObject);
			Device->GetImmediateContext().GetCommandBufferManager()->Destroy(CmdObject);
		}
#endif
	}
#else
	IndexBuffer->Unlock();
#endif
}


FVulkanDynamicPooledBuffer::FPoolElement::FPoolElement() :
	NextFreeOffset(0),
	LastLockSize(0),
	Size(0)
{
}

inline FVulkanDynamicLockInfo FVulkanDynamicPooledBuffer::FPoolElement::Lock(VkDeviceSize InSize)
{
	check(LastLockSize == 0);
	int32 BufferIndex = GFrameNumberRenderThread % NUM_FRAMES;

	auto* Buffer = Buffers[BufferIndex];

	FVulkanDynamicLockInfo Info;
	Info.Buffer = Buffer;
	Info.Data = Buffer->Lock(InSize, NextFreeOffset);
	Info.Offset = NextFreeOffset;

	NextFreeOffset += InSize;
	LastLockSize = InSize;

	return Info;
}

void FVulkanDynamicPooledBuffer::FPoolElement::Unlock(FVulkanDynamicLockInfo LockInfo)
{
	check(LastLockSize != 0);

	int32 BufferIndex = GFrameNumberRenderThread % NUM_FRAMES;
	Buffers[BufferIndex]->Unlock();

	LastLockSize = 0;
}

FVulkanDynamicPooledBuffer::FVulkanDynamicPooledBuffer(FVulkanDevice& Device, uint32 NumPoolElements, const VkDeviceSize* PoolElements, VkBufferUsageFlagBits UsageFlags)
{
	for (uint32 PoolIndex = 0; PoolIndex < NumPoolElements; ++PoolIndex)
	{
		auto* Element = new(Elements) FPoolElement;
		VkDeviceSize Size = PoolElements[PoolIndex];
		Element->Size = Size;
		for (int32 Index = 0; Index < NUM_FRAMES; ++Index)
		{
			Element->Buffers[Index] = new FVulkanBuffer(Device, Size, UsageFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, false, __FILE__, __LINE__);
		}
	}
}

FVulkanDynamicPooledBuffer::~FVulkanDynamicPooledBuffer()
{
	for (auto& Element : Elements)
	{
		for (int32 Index = 0; Index < NUM_FRAMES; ++Index)
		{
			delete Element.Buffers[Index];
		}
	}

	Elements.Reset(0);
}

FVulkanDynamicLockInfo FVulkanDynamicPooledBuffer::Lock(VkDeviceSize InSize)
{
	check(InSize > 0);

	FPoolElement* Element = nullptr;
	int32 PoolIndex = 0;
	for ( ; PoolIndex < Elements.Num(); ++PoolIndex)
	{
		if (Elements[PoolIndex].CanLock(InSize))
		{
			Element = &Elements[PoolIndex];
			break;
		}
	}

	checkf(Element, TEXT("Out of memory allocating %d bytes from DynamicIndex buffer!"), InSize);
	FVulkanDynamicLockInfo Info = Element->Lock(InSize);
	Info.PoolElementIndex = PoolIndex;
	return Info;
}

void FVulkanDynamicPooledBuffer::EndFrame()
{
	for (int32 PoolIndex = 0; PoolIndex < Elements.Num(); ++PoolIndex)
	{
		Elements[PoolIndex].NextFreeOffset = 0;
		Elements[PoolIndex].LastLockSize = 0;
	}
}

static const VkDeviceSize GDynamicIndexBufferPoolSizes[] =
{
	1 * 1024,
	16 * 1024,
	64 * 1024,
	1024 * 1024,
};

FVulkanDynamicIndexBuffer::FVulkanDynamicIndexBuffer(FVulkanDevice& Device) :
	FVulkanDynamicPooledBuffer(Device, ARRAY_COUNT(GDynamicIndexBufferPoolSizes), GDynamicIndexBufferPoolSizes, VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
{
}

static const VkDeviceSize GDynamicVertexBufferPoolSizes[] =
{
	4 * 1024,
	64 * 1024,
	256 * 1024,
	1024 * 1024,
};

FVulkanDynamicVertexBuffer::FVulkanDynamicVertexBuffer(FVulkanDevice& Device) :
	FVulkanDynamicPooledBuffer(Device, ARRAY_COUNT(GDynamicVertexBufferPoolSizes), GDynamicVertexBufferPoolSizes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
{
}
