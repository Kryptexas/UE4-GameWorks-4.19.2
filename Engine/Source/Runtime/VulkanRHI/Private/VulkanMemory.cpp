// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanMemory.cpp: Vulkan memory RHI implementation.
=============================================================================*/

#include "VulkanRHIPrivate.h"
#include "VulkanMemory.h"

namespace VulkanRHI
{
	static FCriticalSection GBufferAllocationLock;
	static FCriticalSection GAllocationLock;
	static FCriticalSection GFenceLock;

	FDeviceMemoryManager::FDeviceMemoryManager() :
		DeviceHandle(VK_NULL_HANDLE),
		Device(nullptr),
		NumAllocations(0),
		PeakNumAllocations(0)
	{
		FMemory::Memzero(MemoryProperties);
	}

	FDeviceMemoryManager::~FDeviceMemoryManager()
	{
		Deinit();
	}

	void FDeviceMemoryManager::Init(FVulkanDevice* InDevice)
	{
		check(Device == nullptr);
		Device = InDevice;
		NumAllocations = 0;
		PeakNumAllocations = 0;

		DeviceHandle = Device->GetInstanceHandle();
		vkGetPhysicalDeviceMemoryProperties(InDevice->GetPhysicalHandle(), &MemoryProperties);

		const uint32 MaxAllocations = InDevice->GetLimits().maxMemoryAllocationCount;

		HeapInfos.AddDefaulted(MemoryProperties.memoryHeapCount);

		UE_LOG(LogVulkanRHI, Display, TEXT("%d Device Memory Heaps; Max memory allocations %d"), MemoryProperties.memoryHeapCount, MaxAllocations);
		for (uint32 Index = 0; Index < MemoryProperties.memoryHeapCount; ++Index)
		{
			bool bIsGPUHeap = ((MemoryProperties.memoryHeaps[Index].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) == VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);
			UE_LOG(LogVulkanRHI, Display, TEXT("%d: Flags 0x%x Size %llu (%.2f MB) %s"), 
				Index,
				MemoryProperties.memoryHeaps[Index].flags,
				MemoryProperties.memoryHeaps[Index].size,
				(float)((double)MemoryProperties.memoryHeaps[Index].size / 1024.0 / 1024.0),
				bIsGPUHeap ? TEXT("GPU") : TEXT(""));
			HeapInfos[Index].TotalSize = MemoryProperties.memoryHeaps[Index].size;
		}

		UE_LOG(LogVulkanRHI, Display, TEXT("%d Device Memory Types"), MemoryProperties.memoryTypeCount);
		for (uint32 Index = 0; Index < MemoryProperties.memoryTypeCount; ++Index)
		{
			auto GetFlagsString = [](VkMemoryPropertyFlags Flags)
				{
					FString String;
					if ((Flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
					{
						String += TEXT(" Local");
					}
					if ((Flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
					{
						String += TEXT(" HVisible");
					}
					if ((Flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
					{
						String += TEXT(" HCoherent");
					}
					if ((Flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) == VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
					{
						String += TEXT(" HCached");
					}
					if ((Flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) == VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
					{
						String += TEXT(" Lazy");
					}
					return String;
				};
			UE_LOG(LogVulkanRHI, Display, TEXT("%d: Flags 0x%x Heap %d %s"),
				Index,
				MemoryProperties.memoryTypes[Index].propertyFlags,
				MemoryProperties.memoryTypes[Index].heapIndex,
				*GetFlagsString(MemoryProperties.memoryTypes[Index].propertyFlags));
		}
	}

	void FDeviceMemoryManager::Deinit()
	{
		for (int32 Index = 0; Index < HeapInfos.Num(); ++Index)
		{
			checkf(HeapInfos[Index].Allocations.Num() == 0, TEXT("Found %d unfreed allocations!"), HeapInfos[Index].Allocations.Num());
		}
		NumAllocations = 0;
	}

	FDeviceMemoryAllocation* FDeviceMemoryManager::Alloc(VkDeviceSize AllocationSize, uint32 MemoryTypeIndex, const char* File, uint32 Line)
	{
		FScopeLock Lock(&GAllocationLock);

		check(AllocationSize > 0);
		check(MemoryTypeIndex < MemoryProperties.memoryTypeCount);

		VkMemoryAllocateInfo Info;
		FMemory::Memzero(Info);
		Info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		Info.allocationSize = AllocationSize;
		Info.memoryTypeIndex = MemoryTypeIndex;

		auto* NewAllocation = new FDeviceMemoryAllocation;
		NewAllocation->DeviceHandle = DeviceHandle;
		NewAllocation->Size = AllocationSize;
		NewAllocation->MemoryTypeIndex = MemoryTypeIndex;
		NewAllocation->bCanBeMapped = ((MemoryProperties.memoryTypes[MemoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		NewAllocation->bIsCoherent = ((MemoryProperties.memoryTypes[MemoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		NewAllocation->bIsCached = ((MemoryProperties.memoryTypes[MemoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) == VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
#if VULKAN_TRACK_MEMORY_USAGE
		NewAllocation->File = File;
		NewAllocation->Line = Line;
#endif
		++NumAllocations;
		PeakNumAllocations = FMath::Max(NumAllocations, PeakNumAllocations);
		if (NumAllocations == Device->GetLimits().maxMemoryAllocationCount)
		{
			UE_LOG(LogVulkanRHI, Warning, TEXT("Hit Maximum # of allocations (%d) reported by device!"), NumAllocations);
		}
		VERIFYVULKANRESULT(vkAllocateMemory(DeviceHandle, &Info, nullptr, &NewAllocation->Handle));

		uint32 HeapIndex = MemoryProperties.memoryTypes[MemoryTypeIndex].heapIndex;
		HeapInfos[HeapIndex].Allocations.Add(NewAllocation);
		HeapInfos[HeapIndex].UsedSize += AllocationSize;

		return NewAllocation;
	}

	void FDeviceMemoryManager::Free(FDeviceMemoryAllocation*& Allocation)
	{
		FScopeLock Lock(&GAllocationLock);

		check(Allocation);
		check(Allocation->Handle != VK_NULL_HANDLE);
		check(!Allocation->bFreedBySystem);
		vkFreeMemory(DeviceHandle, Allocation->Handle, nullptr);

		--NumAllocations;

		uint32 HeapIndex = MemoryProperties.memoryTypes[Allocation->MemoryTypeIndex].heapIndex;

		HeapInfos[HeapIndex].UsedSize -= Allocation->Size;
		HeapInfos[HeapIndex].Allocations.RemoveSwap(Allocation);
		Allocation->bFreedBySystem = true;
		delete Allocation;
		Allocation = nullptr;
	}

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	void FDeviceMemoryManager::DumpMemory()
	{
		UE_LOG(LogVulkanRHI, Display, TEXT("Device Memory: %d allocations on %d heaps"), NumAllocations, HeapInfos.Num());
		for (int32 Index = 0; Index < HeapInfos.Num(); ++Index)
		{
			auto& HeapInfo = HeapInfos[Index];
			UE_LOG(LogVulkanRHI, Display, TEXT("\tHeap %d, %d allocations"), Index, HeapInfo.Allocations.Num());
			uint64 TotalSize = 0;
			for (int32 SubIndex = 0; SubIndex < HeapInfo.Allocations.Num(); ++SubIndex)
			{
				FDeviceMemoryAllocation* Allocation = HeapInfo.Allocations[SubIndex];
				//UE_LOG(LogVulkanRHI, Display, TEXT("\t\tAllocation %d Size %d"), SubIndex, Allocation->Size);
				TotalSize += Allocation->Size;
			}
			UE_LOG(LogVulkanRHI, Display, TEXT("\t\tTotal Allocated %u"), TotalSize);
		}
		NumAllocations = 0;
	}
#endif
	FDeviceMemoryAllocation::~FDeviceMemoryAllocation()
	{
		checkf(bFreedBySystem, TEXT("Memory has to released calling FDeviceMemory::Free()!"));
	}

	void* FDeviceMemoryAllocation::Map(VkDeviceSize InSize, VkDeviceSize Offset)
	{
		check(bCanBeMapped);
		check(!MappedPointer);
		check(InSize + Offset <= Size);

		VERIFYVULKANRESULT(vkMapMemory(DeviceHandle, Handle, Offset, InSize, 0, &MappedPointer));
		return MappedPointer;
	}

	void FDeviceMemoryAllocation::Unmap()
	{
		check(MappedPointer);
		vkUnmapMemory(DeviceHandle, Handle);
		MappedPointer = nullptr;
	}


	FResourceAllocation::FResourceAllocation(FResourceHeapPage* InOwner, FDeviceMemoryAllocation* InDeviceMemoryAllocation, uint32 InSize, uint32 InOffset, char* InFile, uint32 InLine)
		: Owner(InOwner)
		, DeviceMemoryAllocation(InDeviceMemoryAllocation)
		, Size(InSize)
		, Offset(InOffset)
#if VULKAN_TRACK_MEMORY_USAGE
		, File(InFile)
		, Line(InLine)
#endif
	{
	}

	FResourceAllocation::~FResourceAllocation()
	{
		Owner->ReleaseAllocation(this);
	}

	void* FResourceAllocation::GetMappedPointer()
	{
		check(DeviceMemoryAllocation->CanBeMapped());
		check(DeviceMemoryAllocation->IsMapped());
		return (uint8*)DeviceMemoryAllocation->GetMappedPointer() + Offset;
	}


	FResourceHeapPage::~FResourceHeapPage()
	{
		check(!DeviceMemoryAllocation);
	}

	FResourceAllocation* FResourceHeapPage::TryAllocate(uint32 Size, uint32 Alignment, char* File, uint32 Line)
	{
		for (int32 Index = 0; Index < FreeList.Num(); ++Index)
		{
			FPair& Entry = FreeList[Index];
			uint32 NewOffset = Align(Entry.Offset, Alignment);
			uint32 OffsetAdjustment = NewOffset - Entry.Offset;
			if (OffsetAdjustment + Size <= Entry.Size)
			{
				FScopeLock ScopeLock(&GAllocationLock);
				uint32 NewFreeOffset = NewOffset + Size;
				if (OffsetAdjustment + Size < Entry.Size)
				{
					// Modify current Entry
					Entry.Offset = NewFreeOffset;
					Entry.Size -= Size;
				}
				else
				{
					check(OffsetAdjustment + Size == Entry.Size);
					// Remove the complete entry
					FreeList.RemoveAtSwap(Index, 1, false);
				}

				UsedSize += Size;

				auto* NewResourceAllocation = new FResourceAllocation(this, DeviceMemoryAllocation, Size, NewOffset, File, Line);
				ResourceAllocations.Add(NewResourceAllocation);

				PeakNumAllocations = FMath::Max(PeakNumAllocations, ResourceAllocations.Num());
				return NewResourceAllocation;
			}
		}

		return nullptr;
	}

	void FResourceHeapPage::ReleaseAllocation(FResourceAllocation* Allocation)
	{
		{
			FScopeLock ScopeLock(&GAllocationLock);
			ResourceAllocations.RemoveSingleSwap(Allocation, false);

			FPair NewFree;
			NewFree.Offset = Allocation->Offset;
			NewFree.Size = Allocation->Size;
			FreeList.Add(NewFree);
		}

		UsedSize -= Allocation->Size;
		check(UsedSize >= 0);

		if (JoinFreeBlocks())
		{
			Owner->FreePage(this);
		}
	}

	bool FResourceHeapPage::JoinFreeBlocks()
	{
		FScopeLock ScopeLock(&GAllocationLock);
		if (FreeList.Num() > 1)
		{
			FreeList.Sort();

			for (int32 Index = FreeList.Num() - 1; Index > 0; --Index)
			{
				auto& Current = FreeList[Index];
				auto& Prev = FreeList[Index - 1];
				if (Prev.Offset + Prev.Size == Current.Offset)
				{
					Prev.Size += Current.Size;
					FreeList.RemoveAt(Index, 1, false);
				}
			}
		}

		if (FreeList.Num() == 1)
		{
			if (ResourceAllocations.Num() == 0)
			{
				check(UsedSize == 0);
				checkf(FreeList[0].Offset == 0 && FreeList[0].Size == MaxSize, TEXT("Memory leak, should have %d free, only have %d; missing %d bytes"), MaxSize, FreeList[0].Size, MaxSize - FreeList[0].Size);
				return true;
			}
		}

		return false;
	}

	FResourceHeap::FResourceHeap(FResourceHeapManager* InOwner, uint32 InMemoryTypeIndex, uint32 InPageSize)
		: Owner(InOwner)
		, MemoryTypeIndex(InMemoryTypeIndex)
		, DefaultPageSize(InPageSize)
		, PeakPageSize(0)
		, UsedMemory(0)
	{
	}

	FResourceHeap::~FResourceHeap()
	{
		FScopeLock ScopeLock(&CriticalSection);

		for (int32 Index = UsedPages.Num() - 1; Index >= 0; --Index)
		{
			FResourceHeapPage* Page = UsedPages[Index];
			if (Page->JoinFreeBlocks())
			{
				Owner->GetParent()->GetMemoryManager().Free(Page->DeviceMemoryAllocation);
				delete Page;
			}
			else
			{
				check(0);
			}
		}

		check(UsedPages.Num() == 0);
		for (int32 Index = 0; Index < FreePages.Num(); ++Index)
		{
			FResourceHeapPage* Page = FreePages[Index];
			Owner->GetParent()->GetMemoryManager().Free(Page->DeviceMemoryAllocation);
			delete Page;
		}
	}

	void FResourceHeap::FreePage(FResourceHeapPage* InPage)
	{
		FScopeLock ScopeLock(&CriticalSection);
		check(InPage->JoinFreeBlocks());
		UsedPages.RemoveSingleSwap(InPage, false);
		FreePages.Add(InPage);

		//#todo-rco: Give free pages back to the OS after N frames?
	}

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	void FResourceHeap::DumpMemory()
	{
		UE_LOG(LogVulkanRHI, Display, TEXT("%d Free Pages, %d Used Pages, Peak Allocation Size on a Page %d"), FreePages.Num(), UsedPages.Num(), PeakPageSize);
		uint64 UsedMemory = 0;
		uint64 FreeMemory = 0;
		uint32 NumSubAllocations = 0;
		for (int32 Index = 0; Index < UsedPages.Num(); ++Index)
		{
			UsedMemory += UsedPages[Index]->UsedSize;
			NumSubAllocations += UsedPages[Index]->ResourceAllocations.Num();
		}

		for (int32 Index = 0; Index < FreePages.Num(); ++Index)
		{
			FreeMemory += FreePages[Index]->MaxSize;
		}

		UE_LOG(LogVulkanRHI, Display, TEXT("\tUsed Memory %d in %d Suballocations, Free Memory %d"), UsedMemory, NumSubAllocations, FreeMemory);
	}
#endif

	FResourceAllocation* FResourceHeap::AllocateResource(uint32 Size, uint32 Alignment, bool bMapAllocation, char* File, uint32 Line)
	{
		FScopeLock ScopeLock(&CriticalSection);

		if (Size < DefaultPageSize)
		{
			// Check Used pages to see if we can fit this in
			for (int32 Index = 0; Index < UsedPages.Num(); ++Index)
			{
				FResourceHeapPage* Page = UsedPages[Index];
				FResourceAllocation* ResourceAllocation = Page->TryAllocate(Size, Alignment, File, Line);
				if (ResourceAllocation)
				{
					return ResourceAllocation;
				}
			}
		}

		for (int32 Index = 0; Index < FreePages.Num(); ++Index)
		{
			FResourceHeapPage* Page = FreePages[Index];
			FResourceAllocation* ResourceAllocation = Page->TryAllocate(Size, Alignment, File, Line);
			if (ResourceAllocation)
			{
				FreePages.RemoveSingleSwap(Page, false);
				UsedPages.Add(Page);
				return ResourceAllocation;
			}
		}

		uint32 AllocationSize = FMath::Max(Size, DefaultPageSize);
		FDeviceMemoryAllocation* DeviceMemoryAllocation = Owner->GetParent()->GetMemoryManager().Alloc(AllocationSize, MemoryTypeIndex, __FILE__, __LINE__);
		auto* NewPage = new FResourceHeapPage(this, DeviceMemoryAllocation, AllocationSize);
		UsedPages.Add(NewPage);

		UsedMemory += AllocationSize;

		PeakPageSize = FMath::Max(PeakPageSize, AllocationSize);

		if (bMapAllocation)
		{
			DeviceMemoryAllocation->Map(AllocationSize, 0);
		}

		return NewPage->Allocate(Size, Alignment, File, Line);
	}

	FResourceHeapManager::FResourceHeapManager(FVulkanDevice* InDevice)
		: FDeviceChild(InDevice)
		, GPUOnlyHeap(nullptr)
	{
	}

	FResourceHeapManager::~FResourceHeapManager()
	{
		Deinit();
	}

	void FResourceHeapManager::Init()
	{
		auto& MemoryManager = Device->GetMemoryManager();
		const uint32 TypeBits = (1 << MemoryManager.GetNumMemoryTypes()) - 1;

		auto& MemoryProperties = MemoryManager.GetMemoryProperties();

		//#todo-rco: Hack! Figure out a better way for page size
		float PercentageHeapForGPU = 0.75;

		{
			uint32 TypeIndex = 0;
			VERIFYVULKANRESULT(MemoryManager.GetMemoryTypeFromProperties(TypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &TypeIndex));

			uint32 PageSize = (uint32)((uint64)(MemoryProperties.memoryHeaps[MemoryProperties.memoryTypes[TypeIndex].heapIndex].size * PercentageHeapForGPU) / (uint64)Device->GetLimits().maxMemoryAllocationCount);
			GPUOnlyHeap = new FResourceHeap(this, TypeIndex, PageSize);
		}

		{
			uint32 TypeIndex = 0;
			VERIFYVULKANRESULT(MemoryManager.GetMemoryTypeFromProperties(TypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &TypeIndex));

			uint32 PageSize = (uint32)((uint64)(MemoryProperties.memoryHeaps[MemoryProperties.memoryTypes[TypeIndex].heapIndex].size * (1.0f - PercentageHeapForGPU)) / (uint64)Device->GetLimits().maxMemoryAllocationCount);
			CPUStagingHeap = new FResourceHeap(this, TypeIndex, PageSize);
		}
	}

	void FResourceHeapManager::Deinit()
	{
		delete GPUOnlyHeap;
		GPUOnlyHeap = nullptr;

		delete CPUStagingHeap;
		CPUStagingHeap = nullptr;
	}

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	void FResourceHeapManager::DumpMemory()
	{
		UE_LOG(LogVulkanRHI, Display, TEXT("GPU Only Heap"));
		GPUOnlyHeap->DumpMemory();
		UE_LOG(LogVulkanRHI, Display, TEXT("CPU Staging Heap"));
		CPUStagingHeap->DumpMemory();
	}
#endif


	FStagingResource::~FStagingResource()
	{
		checkf(!Allocation, TEXT("Staging Resource not released!"));
	}

	void FStagingImage::Destroy(FDeviceMemoryManager& Memory, VkDevice DeviceHandle)
	{
		check(Allocation);

		vkDestroyImage(DeviceHandle, Image, nullptr);

		Memory.Free(Allocation);
	}

	void FStagingBuffer::Destroy(FDeviceMemoryManager& Memory, VkDevice DeviceHandle)
	{
		check(Allocation);

		vkDestroyBuffer(DeviceHandle, Buffer, nullptr);

		Memory.Free(Allocation);
	}

	void FStagingManager::Init(FVulkanDevice* InDevice, FVulkanQueue* InQueue)
	{
		Device = InDevice;
		Queue = InQueue;
	}

	void FStagingManager::Deinit()
	{
		check(UsedStagingImages.Num() == 0);
		auto& DeviceMemory = Device->GetMemoryManager();
		for (int32 Index = 0; Index < FreeStagingImages.Num(); ++Index)
		{
			FreeStagingImages[Index]->Destroy(DeviceMemory, Device->GetInstanceHandle());
			delete FreeStagingImages[Index];
		}

		FreeStagingImages.Empty(0);

		check(UsedStagingBuffers.Num() == 0);
		for (int32 Index = 0; Index < FreeStagingBuffers.Num(); ++Index)
		{
			FreeStagingBuffers[Index]->Destroy(DeviceMemory, Device->GetInstanceHandle());
			delete FreeStagingBuffers[Index];
		}

		FreeStagingBuffers.Empty(0);
	}

	FStagingImage* FStagingManager::AcquireImage(VkFormat Format, uint32 Width, uint32 Height, uint32 Depth)
	{
		//#todo-rco: Better locking!
		{
			FScopeLock Lock(&GAllocationLock);

			for (int32 Index = 0; Index < FreeStagingImages.Num(); ++Index)
			{
				auto* FreeImage = FreeStagingImages[Index];
				if (FreeImage->Format == Format && FreeImage->Width == Width && FreeImage->Height == Height && FreeImage->Depth == Depth)
				{
					FreeStagingImages.RemoveAtSwap(Index, 1, false);
					UsedStagingImages.Add(FreeImage);
					return FreeImage;
				}
			}
		}

		auto* StagingImage = new FStagingImage(Format, Width, Height, Depth);

		VkExtent3D StagingExtent;
		FMemory::Memzero(StagingExtent);
		StagingExtent.width = Width;
		StagingExtent.height = Height;
		StagingExtent.depth = Depth;

		VkImageCreateInfo StagingImageCreateInfo;
		FMemory::Memzero(StagingImageCreateInfo);
		StagingImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		StagingImageCreateInfo.imageType = Depth > 1 ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
		StagingImageCreateInfo.format = Format;
		StagingImageCreateInfo.extent = StagingExtent;
		StagingImageCreateInfo.mipLevels = 1;
		StagingImageCreateInfo.arrayLayers = 1;
		StagingImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		StagingImageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
		StagingImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		VkDevice VulkanDevice = Device->GetInstanceHandle();

		VERIFYVULKANRESULT(vkCreateImage(VulkanDevice, &StagingImageCreateInfo, nullptr, &StagingImage->Image));

		VkMemoryRequirements MemReqs;
		vkGetImageMemoryRequirements(VulkanDevice, StagingImage->Image, &MemReqs);

		StagingImage->Allocation = Device->GetMemoryManager().Alloc(MemReqs.size, MemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, __FILE__, __LINE__);

		VERIFYVULKANRESULT(vkBindImageMemory(VulkanDevice, StagingImage->Image, StagingImage->Allocation->GetHandle(), 0));

		{
			FScopeLock Lock(&GAllocationLock);
			UsedStagingImages.Add(StagingImage);
		}
		return StagingImage;
	}

#if VULKAN_USE_NEW_COMMAND_BUFFERS
	void FStagingManager::ReleaseImage(FVulkanCmdBuffer* CmdBuffer, FStagingImage*& StagingImage)
	{
		FScopeLock Lock(&GAllocationLock);
		UsedStagingImages.RemoveSingleSwap(StagingImage);
		FPendingItem* Entry = new (PendingFreeStagingImages) FPendingItem;
		Entry->CmdBuffer = CmdBuffer;
		Entry->FenceCounter = CmdBuffer->GetFenceSignaledCounter();
		Entry->Resource = StagingImage;
		StagingImage = nullptr;
	}
#else
	void FStagingManager::ReleaseImage(FStagingImage*& StagingImage)
	{
		FScopeLock Lock(&GAllocationLock);
		UsedStagingImages.RemoveSingleSwap(StagingImage);
		FreeStagingImages.Add(StagingImage);
		StagingImage = nullptr;
	}
#endif

	FStagingBuffer* FStagingManager::AcquireBuffer(uint32 Size)
	{
		//#todo-rco: Better locking!
		{
			FScopeLock Lock(&GAllocationLock);

			for (int32 Index = 0; Index < FreeStagingBuffers.Num(); ++Index)
			{
				auto* FreeBuffer = FreeStagingBuffers[Index];
				if (FreeBuffer->Allocation->GetSize() == Size)
				{
					FreeStagingBuffers.RemoveAtSwap(Index, 1, false);
					UsedStagingBuffers.Add(FreeBuffer);
					return FreeBuffer;
				}
			}
		}

		auto* StagingBuffer = new FStagingBuffer();

		VkBufferCreateInfo StagingBufferCreateInfo;
		FMemory::Memzero(StagingBufferCreateInfo);
		StagingBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		StagingBufferCreateInfo.size = Size;
		StagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VkDevice VulkanDevice = Device->GetInstanceHandle();

		VERIFYVULKANRESULT(vkCreateBuffer(VulkanDevice, &StagingBufferCreateInfo, nullptr, &StagingBuffer->Buffer));

		VkMemoryRequirements MemReqs;
		vkGetBufferMemoryRequirements(VulkanDevice, StagingBuffer->Buffer, &MemReqs);

		StagingBuffer->Allocation = Device->GetMemoryManager().Alloc(MemReqs.size, MemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, __FILE__, __LINE__);

		VERIFYVULKANRESULT(vkBindBufferMemory(VulkanDevice, StagingBuffer->Buffer, StagingBuffer->Allocation->GetHandle(), 0));

		{
			FScopeLock Lock(&GAllocationLock);
			UsedStagingBuffers.Add(StagingBuffer);
		}
		return StagingBuffer;
	}

#if VULKAN_USE_NEW_COMMAND_BUFFERS
	void FStagingManager::ReleaseBuffer(FVulkanCmdBuffer* CmdBuffer, FStagingBuffer*& StagingBuffer)
	{
		FScopeLock Lock(&GAllocationLock);
		UsedStagingBuffers.RemoveSingleSwap(StagingBuffer);
		FPendingItem* Entry = new (PendingFreeStagingBuffers) FPendingItem;
		Entry->CmdBuffer = CmdBuffer;
		Entry->FenceCounter = CmdBuffer->GetFenceSignaledCounter();
		Entry->Resource = StagingBuffer;
		StagingBuffer = nullptr;
	}
#else
	void FStagingManager::ReleaseBuffer(FStagingBuffer*& StagingBuffer)
	{
		FScopeLock Lock(&GAllocationLock);
		UsedStagingBuffers.RemoveSingleSwap(StagingBuffer);
		FreeStagingBuffers.Add(StagingBuffer);
		StagingBuffer = nullptr;
	}
#endif

	void FStagingManager::ProcessPendingFree()
	{
#if VULKAN_USE_NEW_COMMAND_BUFFERS
		FScopeLock Lock(&GAllocationLock);
		for (int32 Index = PendingFreeStagingBuffers.Num() - 1; Index >= 0; --Index)
		{
			FPendingItem& Entry = PendingFreeStagingBuffers[Index];
			if (Entry.FenceCounter < Entry.CmdBuffer->GetFenceSignaledCounter())
			{
				auto* StagingBuffer = (FStagingBuffer*)Entry.Resource;
				PendingFreeStagingBuffers.RemoveAtSwap(Index, 1, false);
				FreeStagingBuffers.Add(StagingBuffer);
			}
		}

		for (int32 Index = PendingFreeStagingImages.Num() - 1; Index >= 0; --Index)
		{
			FPendingItem& Entry = PendingFreeStagingImages[Index];
			if (Entry.FenceCounter < Entry.CmdBuffer->GetFenceSignaledCounter())
			{
				auto* StagingImage = (FStagingImage*)Entry.Resource;
				PendingFreeStagingImages.RemoveAtSwap(Index, 1, false);
				FreeStagingImages.Add(StagingImage);
			}
		}
#endif
	}

	FFence::FFence(FVulkanDevice* InDevice, FFenceManager* InOwner)
		: State(FFence::EState::NotReady)
		, Owner(InOwner)
	{
		VkFenceCreateInfo Info;
		FMemory::Memzero(Info);
		Info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		VERIFYVULKANRESULT(vkCreateFence(InDevice->GetInstanceHandle(), &Info, nullptr, &Handle));
	}

	FFence::~FFence()
	{
		checkf(Handle == VK_NULL_HANDLE, TEXT("Didn't get properly destroyed by FFenceManager!"));
	}

	FFenceManager::~FFenceManager()
	{
		check(UsedFences.Num() == 0);
	}

	void FFenceManager::Init(FVulkanDevice* InDevice)
	{
		Device = InDevice;
	}

	void FFenceManager::Deinit()
	{
		FScopeLock Lock(&GFenceLock);
		checkf(UsedFences.Num() == 0, TEXT("No all fences are done!"));
		VkDevice DeviceHandle = Device->GetInstanceHandle();
		for (FFence* Fence : FreeFences)
		{
			vkDestroyFence(DeviceHandle, Fence->GetHandle(), nullptr);
			Fence->Handle = VK_NULL_HANDLE;
			delete Fence;
		}
	}

	FFence* FFenceManager::AllocateFence()
	{
		FScopeLock Lock(&GFenceLock);
		if (FreeFences.Num() != 0)
		{
			auto* Fence = FreeFences[0];
			FreeFences.RemoveAtSwap(0, 1, false);
			UsedFences.Add(Fence);
			return Fence;
		}

		auto* NewFence = new FFence(Device, this);
		UsedFences.Add(NewFence);
		return NewFence;
	}

	// Sets it to nullptr
	void FFenceManager::ReleaseFence(FFence*& Fence)
	{
		FScopeLock Lock(&GFenceLock);
		ResetFence(Fence);
		UsedFences.RemoveSingleSwap(Fence);
		FreeFences.Add(Fence);
		Fence = nullptr;
	}

	void FFenceManager::WaitAndReleaseFence(FFence*& Fence, uint64 TimeInNanoseconds)
	{
		FScopeLock Lock(&GFenceLock);
		if (!Fence->IsSignaled())
		{
			WaitForFence(Fence, TimeInNanoseconds);
		}

		ResetFence(Fence);
		UsedFences.RemoveSingleSwap(Fence);
		FreeFences.Add(Fence);
		Fence = nullptr;
	}

	bool FFenceManager::CheckFenceState(FFence* Fence)
	{
		check(UsedFences.Contains(Fence));
		check(Fence->State == FFence::EState::NotReady);
		auto Result = vkGetFenceStatus(Device->GetInstanceHandle(), Fence->Handle);
		switch (Result)
		{
		case VK_SUCCESS:
			Fence->State = FFence::EState::Signaled;
			return true;

		case VK_NOT_READY:
			break;

		default:
			VERIFYVULKANRESULT(Result);
			break;
		}

		return false;
	}

	bool FFenceManager::WaitForFence(FFence* Fence, uint64 TimeInNanoseconds)
	{
		check(UsedFences.Contains(Fence));
		check(Fence->State == FFence::EState::NotReady);
		auto Result = vkWaitForFences(Device->GetInstanceHandle(), 1, &Fence->Handle, true, TimeInNanoseconds);
		switch (Result)
		{
		case VK_SUCCESS:
			Fence->State = FFence::EState::Signaled;
			return true;
		case VK_TIMEOUT:
			break;
		default:
			VERIFYVULKANRESULT(Result);
			break;
		}

		return false;
	}

	void FFenceManager::ResetFence(FFence* Fence)
	{
		if (Fence->State != FFence::EState::NotReady)
		{
			VERIFYVULKANRESULT(vkResetFences(Device->GetInstanceHandle(), 1, &Fence->Handle));
			Fence->State = FFence::EState::NotReady;
		}
	}

	FDeferredDeletionQueue::FDeferredDeletionQueue(FVulkanDevice* InDevice)
		: FDeviceChild(InDevice)
	{
	}

	FDeferredDeletionQueue::~FDeferredDeletionQueue()
	{
		Clear();
	}

	void FDeferredDeletionQueue::EnqueueResource(FVulkanCmdBuffer* CmdBuffer, FRefCount* Resource)
	{
#if VULKAN_USE_NEW_COMMAND_BUFFERS && VULKAN_USE_NEW_RESOURCE_MANAGEMENT
		Resource->AddRef();

		FEntry Entry;
		Entry.CmdBuffer = CmdBuffer;
		Entry.FenceCounter = CmdBuffer->GetFenceSignaledCounter();
		Entry.Resource = Resource;
		{
			FScopeLock ScopeLock(&CS);
			Entries.Add(Entry);
		}
#endif
	}

	void FDeferredDeletionQueue::ReleaseResources(/*bool bDeleteImmediately*/)
	{
#if VULKAN_USE_NEW_COMMAND_BUFFERS && VULKAN_USE_NEW_RESOURCE_MANAGEMENT
		FScopeLock ScopeLock(&CS);

		// Traverse list backwards to the swap switches to elements already tested
		for (int32 Index = Entries.Num() - 1; Index >= 0; --Index)
		{
			FEntry* Entry = &Entries[Index];
			if (Entry->FenceCounter < Entry->CmdBuffer->GetFenceSignaledCounter())
			{
				Entry->Resource->Release();
				Entries.RemoveAtSwap(Index, 1, false);
			}
		}
#endif
	}
}
