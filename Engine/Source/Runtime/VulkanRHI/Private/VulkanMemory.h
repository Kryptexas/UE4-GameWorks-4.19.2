// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanMemory.h: Vulkan Memory RHI definitions.
=============================================================================*/

#pragma once 

#define VULKAN_TRACK_MEMORY_USAGE	1

class FVulkanQueue;
class FVulkanCmdBuffer;

namespace VulkanRHI
{
	class FFenceManager;

	// Custom ref counting
	class FRefCount
	{
	public:
		FRefCount() {}
		virtual ~FRefCount()
		{
			check(NumRefs.GetValue() == 0);
		}

		inline uint32 AddRef()
		{
			int32 NewValue = NumRefs.Increment();
			check(NewValue > 0);
			return (uint32)NewValue;
		}

		inline uint32 Release()
		{
			int32 NewValue = NumRefs.Decrement();
			if (NewValue == 0)
			{
				delete this;
			}
			check(NewValue >= 0);
			return (uint32)NewValue;
		}

		inline uint32 GetRefCount() const
		{
			int32 Value = NumRefs.GetValue();
			check(Value >= 0);
			return (uint32)Value;
		}

	private:
		FThreadSafeCounter NumRefs;
	};

	class FDeviceChild
	{
	public:
		FDeviceChild(FVulkanDevice* InDevice = nullptr) :
			Device(InDevice)
		{
		}

		inline FVulkanDevice* GetParent() const
		{
			// Has to have one if we are asking for it...
			check(Device);
			return Device;
		}

		inline void SetParent(FVulkanDevice* InDevice)
		{
			check(!Device);
			Device = InDevice;
		}

	protected:
		FVulkanDevice* Device;
	};

	// An Allocation of a Device Heap. Lowest level of allocations and bounded by VkPhysicalDeviceLimits::maxMemoryAllocationCount.
	class FDeviceMemoryAllocation
	{
	public:
		FDeviceMemoryAllocation()
			: DeviceHandle(VK_NULL_HANDLE)
			, Handle(VK_NULL_HANDLE)
			, Size(0)
			, MemoryTypeIndex(0)
			, bCanBeMapped(0)
			, MappedPointer(nullptr)
			, bIsCoherent(0)
			, bIsCached(0)
			, bFreedBySystem(false)
#if VULKAN_TRACK_MEMORY_USAGE
			, File(nullptr)
			, Line(0)
#endif
		{
		}

		void* Map(VkDeviceSize Size, VkDeviceSize Offset);
		void Unmap();

		inline void* GetMappedPointer()
		{
			return MappedPointer;
		}

		inline bool CanBeMapped() const
		{
			return bCanBeMapped != 0;
		}

		inline bool IsMapped() const
		{
			return !!MappedPointer;
		}

		inline bool IsCoherent() const
		{
			return bIsCoherent != 0;
		}

		void FlushMappedMemory()
		{
			check(!IsCoherent());
			check(IsMapped());
			VkMappedMemoryRange Range;
			FMemory::Memzero(Range);
			Range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			Range.memory = Handle;
			//Range.offset = 0;
			Range.size = Size;
			VERIFYVULKANRESULT(vkFlushMappedMemoryRanges(DeviceHandle, 1, &Range));
		}

		void InvalidateMappedMemory()
		{
			check(!IsCoherent());
			check(IsMapped());
			VkMappedMemoryRange Range;
			FMemory::Memzero(Range);
			Range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			Range.memory = Handle;
			//Range.offset = 0;
			Range.size = Size;
			VERIFYVULKANRESULT(vkInvalidateMappedMemoryRanges(DeviceHandle, 1, &Range));
		}

		inline VkDeviceMemory GetHandle() const
		{
			return Handle;
		}

		inline VkDeviceSize GetSize() const
		{
			return Size;
		}

		inline uint32 GetMemoryTypeIndex() const
		{
			return MemoryTypeIndex;
		}

	protected:
		VkDeviceSize Size;
		VkDevice DeviceHandle;
		VkDeviceMemory Handle;
		void* MappedPointer;
		uint32 MemoryTypeIndex : 8;
		uint32 bCanBeMapped : 1;
		uint32 bIsCoherent : 1;
		uint32 bIsCached : 1;
		uint32 bFreedBySystem : 1;
		uint32 : 0;

#if VULKAN_TRACK_MEMORY_USAGE
		const char* File;
		uint32 Line;
#endif
		// Only owner can delete!
		~FDeviceMemoryAllocation();

		friend class FDeviceMemoryManager;
	};

	// Manager of Device Heap Allocations. Calling Alloc/Free is expensive!
	class FDeviceMemoryManager
	{
	public:
		FDeviceMemoryManager();
		~FDeviceMemoryManager();

		void Init(FVulkanDevice* InDevice);

		void Deinit();

		inline uint32 GetNumMemoryTypes() const
		{
			return MemoryProperties.memoryTypeCount;
		}

		//#todo-rco: Might need to revisit based on https://gitlab.khronos.org/vulkan/vulkan/merge_requests/1165
		inline VkResult GetMemoryTypeFromProperties(uint32 TypeBits, VkMemoryPropertyFlags Properties, uint32* OutTypeIndex)
		{
			// Search memtypes to find first index with those properties
			for (uint32 i = 0; i < MemoryProperties.memoryTypeCount && TypeBits; i++)
			{
				if ((TypeBits & 1) == 1)
				{
					// Type is available, does it match user properties?
					if ((MemoryProperties.memoryTypes[i].propertyFlags & Properties) == Properties)
					{
						*OutTypeIndex = i;
						return VK_SUCCESS;
					}
				}
				TypeBits >>= 1;
			}

			// No memory types matched, return failure
			return VK_ERROR_FEATURE_NOT_PRESENT;
		}

		inline const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const
		{
			return MemoryProperties;
		}

		FDeviceMemoryAllocation* Alloc(VkDeviceSize AllocationSize, uint32 MemoryTypeIndex, const char* File, uint32 Line);

		inline FDeviceMemoryAllocation* Alloc(VkDeviceSize AllocationSize, uint32 MemoryTypeBits, VkMemoryPropertyFlags MemoryPropertyFlags, const char* File, uint32 Line)
		{
			uint32 MemoryTypeIndex = ~0;
			VERIFYVULKANRESULT(this->GetMemoryTypeFromProperties(MemoryTypeBits, MemoryPropertyFlags, &MemoryTypeIndex));
			return Alloc(AllocationSize, MemoryTypeIndex, File, Line);
		}

		// Sets the Allocation to nullptr
		void Free(FDeviceMemoryAllocation*& Allocation);

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		void DumpMemory();
#endif

	protected:
		VkPhysicalDeviceMemoryProperties MemoryProperties;
		VkDevice DeviceHandle;
		FVulkanDevice* Device;
		uint32 NumAllocations;
		uint32 PeakNumAllocations;

		struct FHeapInfo
		{
			VkDeviceSize TotalSize;
			VkDeviceSize UsedSize;
			TArray<FDeviceMemoryAllocation*> Allocations;

			FHeapInfo() :
				TotalSize(0),
				UsedSize(0)
			{
			}
		};

		TArray<FHeapInfo> HeapInfos;
	};

	class FResourceHeap;
	class FResourceHeapPage;
	class FResourceHeapManager;

	// A sub allocation for a specific memory type
	class FResourceAllocation : public FRefCount
	{
	public:
		FResourceAllocation(FResourceHeapPage* InOwner, FDeviceMemoryAllocation* InDeviceMemoryAllocation, uint32 InSize, uint32 InOffset, char* File, uint32 Line);
		virtual ~FResourceAllocation();

		inline uint32 GetSize() const
		{
			return Size;
		}

		inline uint32 GetOffset() const
		{
			return Offset;
		}

		inline VkDeviceMemory GetHandle() const
		{
			return DeviceMemoryAllocation->GetHandle();
		}

		void* GetMappedPointer();

		inline uint32 GetMemoryTypeIndex() const
		{
			return DeviceMemoryAllocation->GetMemoryTypeIndex();
		}

	private:
		FResourceHeapPage* Owner;
		uint32 Size;
		uint32 Offset;
		FDeviceMemoryAllocation* DeviceMemoryAllocation;

#if VULKAN_TRACK_MEMORY_USAGE
		char* File;
		uint32 Line;
#endif

		friend class FResourceHeapPage;
	};

	// One device allocation that is shared amongst different resources
	class FResourceHeapPage
	{
	public:
		FResourceHeapPage(FResourceHeap* InOwner, FDeviceMemoryAllocation* InDeviceMemoryAllocation, uint32 InSize)
			: Owner(InOwner)
			, DeviceMemoryAllocation(InDeviceMemoryAllocation)
			, MaxSize(InSize)
			, UsedSize(0)
			, PeakNumAllocations(0)
		{
			FPair Pair;
			Pair.Offset = 0;
			Pair.Size = MaxSize;
			FreeList.Add(Pair);
		}

		~FResourceHeapPage();

		FResourceAllocation* TryAllocate(uint32 Size, uint32 Alignment, char* File, uint32 Line);

		FResourceAllocation* Allocate(uint32 Size, uint32 Alignment, char* File, uint32 Line)
		{
			FResourceAllocation* ResourceAllocation = TryAllocate(Size, Alignment, File, Line);
			check(ResourceAllocation);
			return ResourceAllocation;
		}

		void ReleaseAllocation(FResourceAllocation* Allocation);

	protected:
		FResourceHeap* Owner;
		FDeviceMemoryAllocation* DeviceMemoryAllocation;
		uint32 MaxSize;
		uint32 UsedSize;
		int32 PeakNumAllocations;
		TArray<FResourceAllocation*>  ResourceAllocations;

		bool JoinFreeBlocks();

		struct FPair
		{
			uint32 Offset;
			uint32 Size;

			bool operator<(const FPair& In) const
			{
				return Offset < In.Offset;
			}
		};
		TArray<FPair> FreeList;
		friend class FResourceHeap;
	};

	// A set of Device Allocations (Heap Pages) for a specific memory type. This handles pooling allocations inside memory pages to avoid
	// doing allocations directly off the device's heaps
	class FResourceHeap
	{
	public:
		FResourceHeap(FResourceHeapManager* InOwner, uint32 InMemoryTypeIndex, uint32 InPageSize);
		~FResourceHeap();

		void FreePage(FResourceHeapPage* InPage);

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		void DumpMemory();
#endif

	protected:
		FResourceHeapManager* Owner;
		uint32 MemoryTypeIndex;

		uint32 DefaultPageSize;
		uint32 PeakPageSize;
		uint64 UsedMemory;

		TArray<FResourceHeapPage*> UsedPages;
		TArray<FResourceHeapPage*> FreePages;

		FResourceAllocation* AllocateResource(uint32 Size, uint32 Alignment, bool bMapAllocation, char* File, uint32 Line);

		FCriticalSection CriticalSection;

		friend class FResourceHeapManager;
	};

	// Manages heaps and their interactions
	class FResourceHeapManager : public FDeviceChild
	{
	public:
		FResourceHeapManager(FVulkanDevice* InDevice);
		~FResourceHeapManager();

		void Init();
		void Deinit();

		inline FResourceAllocation* AllocateGPUOnlyResource(uint32 Size, uint32 Alignment, char* File, uint32 Line)
		{
			return GPUOnlyHeap->AllocateResource(Size, Alignment, false, File, Line);
		}

		inline FResourceAllocation* AllocateCPUStagingResource(uint32 Size, uint32 Alignment, char* File, uint32 Line)
		{
			return CPUStagingHeap->AllocateResource(Size, Alignment, true, File, Line);
		}

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		void DumpMemory();
#endif

	protected:
		FResourceHeap* GPUOnlyHeap;
		FResourceHeap* CPUStagingHeap;
	};

	class FStagingResource
	{
	public:
		FStagingResource()
			: Allocation(nullptr)
		{
		}

		inline void* Map()
		{
			return Allocation->Map(Allocation->GetSize(), 0);
		}

		inline void Unmap()
		{
			Allocation->Unmap();
		}

	protected:
		FDeviceMemoryAllocation* Allocation;

		// Owner maintains lifetime
		virtual ~FStagingResource();

		virtual void Destroy(FDeviceMemoryManager& Memory, VkDevice DeviceHandle) = 0;
	};

	class FStagingImage : public FStagingResource
	{
	public:
		FStagingImage(VkFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InDepth) :
			Image(VK_NULL_HANDLE),
			Format(InFormat),
			Width(InWidth),
			Height(InHeight),
			Depth(InDepth)
		{
		}

		VkImage GetHandle() const
		{
			return Image;
		}

	protected:
		VkImage Image;

		VkFormat Format;
		uint32 Width;
		uint32 Height;
		uint32 Depth;

		// Owner maintains lifetime
		virtual ~FStagingImage() {}

		virtual void Destroy(FDeviceMemoryManager& Memory, VkDevice DeviceHandle) override;

		friend class FStagingManager;
	};

	class FStagingBuffer : public FStagingResource
	{
	public:
		FStagingBuffer() :
			Buffer(VK_NULL_HANDLE)
		{
		}

		VkBuffer GetHandle() const
		{
			return Buffer;
		}

	protected:
		VkBuffer Buffer;

		// Owner maintains lifetime
		virtual ~FStagingBuffer() {}

		virtual void Destroy(FDeviceMemoryManager& Memory, VkDevice DeviceHandle) override;

		friend class FStagingManager;
	};

	class FStagingManager
	{
	public:
		FStagingManager() :
			Device(nullptr),
			Queue(nullptr)
		{
		}

		void Init(FVulkanDevice* InDevice, FVulkanQueue* InQueue);
		void Deinit();

		FStagingImage* AcquireImage(VkFormat Format, uint32 Width, uint32 Height, uint32 Depth = 1);

		FStagingBuffer* AcquireBuffer(uint32 Size);

#if VULKAN_USE_NEW_COMMAND_BUFFERS
		// Sets pointer to nullptr
		void ReleaseImage(FVulkanCmdBuffer* CmdBuffer, FStagingImage*& StagingImage);

		// Sets pointer to nullptr
		void ReleaseBuffer(FVulkanCmdBuffer* CmdBuffer, FStagingBuffer*& StagingBuffer);
#else
		// Sets pointer to nullptr
		void ReleaseImage(FStagingImage*& StagingImage);

		// Sets pointer to nullptr
		void ReleaseBuffer(FStagingBuffer*& StagingBuffer);
#endif

		void ProcessPendingFree();

	protected:
		struct FPendingItem
		{
			FVulkanCmdBuffer* CmdBuffer;
			uint64 FenceCounter;
			FStagingResource* Resource;
		};

		TArray<FStagingImage*> UsedStagingImages;
#if VULKAN_USE_NEW_COMMAND_BUFFERS
		TArray<FPendingItem> PendingFreeStagingImages;
#endif
		TArray<FStagingImage*> FreeStagingImages;

		TArray<FStagingBuffer*> UsedStagingBuffers;
#if VULKAN_USE_NEW_COMMAND_BUFFERS
		TArray<FPendingItem> PendingFreeStagingBuffers;
#endif
		TArray<FStagingBuffer*> FreeStagingBuffers;

		FVulkanDevice* Device;
		FVulkanQueue* Queue;
	};

	class FFence
	{
	public:
		FFence(FVulkanDevice* InDevice, FFenceManager* InOwner);

		inline VkFence GetHandle() const
		{
			return Handle;
		}

		inline bool IsSignaled() const
		{
			return State == EState::Signaled;
		}

		FFenceManager* GetOwner()
		{
			return Owner;
		}

	protected:
		VkFence Handle;

		enum class EState
		{
			// Initial state
			NotReady,

			// After GPU processed it
			Signaled,
		};

		EState State;

		FFenceManager* Owner;

		// Only owner can delete!
		~FFence();
		friend class FFenceManager;
	};

	class FFenceManager
	{
	public:
		FFenceManager()
			: Device(nullptr)
		{
		}
		~FFenceManager();

		void Init(FVulkanDevice* InDevice);
		void Deinit();

		FFence* AllocateFence();

		inline bool IsFenceSignaled(FFence* Fence)
		{
			if (Fence->IsSignaled())
			{
				return true;
			}

			return CheckFenceState(Fence);
		}

		// Returns false if it timed out
		bool WaitForFence(FFence* Fence, uint64 TimeInNanoseconds);

		void ResetFence(FFence* Fence);

		// Sets it to nullptr
		void ReleaseFence(FFence*& Fence);

		// Sets it to nullptr
		void WaitAndReleaseFence(FFence*& Fence, uint64 TimeInNanoseconds);

	protected:
		FVulkanDevice* Device;
		TArray<FFence*> FreeFences;
		TArray<FFence*> UsedFences;

		// Returns true if signaled
		bool CheckFenceState(FFence* Fence);
	};

	class FDeferredDeletionQueue : public FDeviceChild
	{
		//typedef TPair<FRefCountedObject*, uint64> FFencedObject;
		//FThreadsafeQueue<FFencedObject> DeferredReleaseQueue;

	public:
		FDeferredDeletionQueue(FVulkanDevice* InDevice);
		~FDeferredDeletionQueue();

		void EnqueueResource(FVulkanCmdBuffer* CmdBuffer, FRefCount* Resource);
		void ReleaseResources(/*bool bDeleteImmediately = false*/);

		inline void Clear()
		{
			ReleaseResources(/*true*/);
		}
/*
		class FVulkanAsyncDeletionWorker : public FVulkanDeviceChild, FNonAbandonableTask
		{
		public:
			FVulkanAsyncDeletionWorker(FVulkanDevice* InDevice, FThreadsafeQueue<FFencedObject>* DeletionQueue);

			void DoWork();

		private:
			TQueue<FFencedObject> Queue;
		};
*/
	private:
		//TQueue<FAsyncTask<FVulkanAsyncDeletionWorker>*> DeleteTasks;
		struct FEntry
		{
			uint64 FenceCounter;
			FVulkanCmdBuffer* CmdBuffer;
			FRefCount* Resource;
		};
		FCriticalSection CS;
		TArray<FEntry> Entries;
	};
}
