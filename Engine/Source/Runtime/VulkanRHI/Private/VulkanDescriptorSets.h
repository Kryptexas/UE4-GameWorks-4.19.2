// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanPipelineState.h: Vulkan pipeline state definitions.
=============================================================================*/

#pragma once

#include "VulkanConfiguration.h"
#include "VulkanMemory.h"
#include "VulkanGlobalUniformBuffer.h"

class FVulkanCommandBufferPool;

// Information for the layout of descriptor sets; does not hold runtime objects
class FVulkanDescriptorSetsLayoutInfo
{
public:
	FVulkanDescriptorSetsLayoutInfo()
	{
		FMemory::Memzero(LayoutTypes);
	}

	inline uint32 GetTypesUsed(VkDescriptorType Type) const
	{
		return LayoutTypes[Type];
	}

	struct FSetLayout
	{
		TArray<VkDescriptorSetLayoutBinding> LayoutBindings;
	};

	const TArray<FSetLayout>& GetLayouts() const
	{
		return SetLayouts;
	}

	void AddBindingsForStage(VkShaderStageFlagBits StageFlags, EDescriptorSetStage DescSet, const FVulkanCodeHeader& CodeHeader);

	friend uint32 GetTypeHash(const FVulkanDescriptorSetsLayoutInfo& In)
	{
		return In.Hash;
	}

	inline bool operator == (const FVulkanDescriptorSetsLayoutInfo& In) const
	{
		if (In.SetLayouts.Num() != SetLayouts.Num())
		{
			return false;
		}

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
		if (In.TypesUsageID != TypesUsageID)
		{
			return false;
		}
#endif
		for (int32 Index = 0; Index < In.SetLayouts.Num(); ++Index)
		{
			int32 NumBindings = SetLayouts[Index].LayoutBindings.Num();
			if (In.SetLayouts[Index].LayoutBindings.Num() != NumBindings)
			{
				return false;
			}

			if (NumBindings != 0 && FMemory::Memcmp(&In.SetLayouts[Index].LayoutBindings[0], &SetLayouts[Index].LayoutBindings[0], NumBindings * sizeof(VkDescriptorSetLayoutBinding)))
			{
				return false;
			}
		}

		return true;
	}

	void CopyFrom(const FVulkanDescriptorSetsLayoutInfo& Info)
	{
		FMemory::Memcpy(LayoutTypes, Info.LayoutTypes, sizeof(LayoutTypes));
		Hash = Info.Hash;
#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
		TypesUsageID = Info.TypesUsageID;
#endif
		SetLayouts = Info.SetLayouts;
	}

	inline const uint32* GetLayoutTypes() const
	{
		return LayoutTypes;
	}

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
	inline uint32 GetTypesUsageID() const
	{
		return TypesUsageID;
	}
#endif
protected:
	uint32 LayoutTypes[VK_DESCRIPTOR_TYPE_RANGE_SIZE];
	TArray<FSetLayout> SetLayouts;

	uint32 Hash = 0;

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
	uint32 TypesUsageID = ~0;

	void CompileTypesUsageID();
#endif
	void AddDescriptor(int32 DescriptorSetIndex, const VkDescriptorSetLayoutBinding& Descriptor, int32 BindingIndex);

	friend class FVulkanPipelineStateCache;
};

// The actual run-time descriptor set layouts
class FVulkanDescriptorSetsLayout : public FVulkanDescriptorSetsLayoutInfo
{
public:
	FVulkanDescriptorSetsLayout(FVulkanDevice* InDevice);
	~FVulkanDescriptorSetsLayout();

	// Can be called only once, the idea is that the Layout remains fixed.
	void Compile();

	inline const TArray<VkDescriptorSetLayout>& GetHandles() const
	{
		return LayoutHandles;
	}

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
	inline const VkDescriptorSetAllocateInfo& GetAllocateInfo() const
	{
		return DescriptorSetAllocateInfo;
	}
#endif

	inline uint32 GetHash() const
	{
		return Hash;
	}

private:
	FVulkanDevice* Device;
	//uint32 Hash = 0;
	TArray<VkDescriptorSetLayout> LayoutHandles;
#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
	VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo;
#endif
};

#if VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
typedef TArray<VkDescriptorSet, TInlineAllocator<SF_Compute>> FVulkanDescriptorSetArray;
#else
class FOLDVulkanDescriptorPool
{
public:
#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
	FOLDVulkanDescriptorPool(FVulkanDevice* InDevice, const FVulkanDescriptorSetsLayout& Layout);
#else
	FOLDVulkanDescriptorPool(FVulkanDevice* InDevice);
#endif
	~FOLDVulkanDescriptorPool();

	inline VkDescriptorPool GetHandle() const
	{
		return DescriptorPool;
	}

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
	inline bool CanAllocate(const FVulkanDescriptorSetsLayout& InLayout) const
#else
	inline bool CanAllocate(const FVulkanDescriptorSetsLayout& Layout) const
#endif
	{
#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
		return MaxDescriptorSets > NumAllocatedDescriptorSets + InLayout.GetLayouts().Num();
#else
		for (uint32 TypeIndex = VK_DESCRIPTOR_TYPE_BEGIN_RANGE; TypeIndex < VK_DESCRIPTOR_TYPE_END_RANGE; ++TypeIndex)
		{
			if (NumAllocatedTypes[TypeIndex] +	(int32)Layout.GetTypesUsed((VkDescriptorType)TypeIndex) > MaxAllocatedTypes[TypeIndex])
			{
				return false;
			}
		}
		return true;
#endif
	}

	void TrackAddUsage(const FVulkanDescriptorSetsLayout& Layout);
	void TrackRemoveUsage(const FVulkanDescriptorSetsLayout& Layout);

	inline bool IsEmpty() const
	{
		return NumAllocatedDescriptorSets == 0;
	}

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
	void Reset();
	bool AllocateDescriptorSets(const VkDescriptorSetAllocateInfo& InDescriptorSetAllocateInfo, VkDescriptorSet* OutSets);
#endif

private:
	FVulkanDevice* Device;

	uint32 MaxDescriptorSets;
	uint32 NumAllocatedDescriptorSets;
	uint32 PeakAllocatedDescriptorSets;

	// Tracks number of allocated types, to ensure that we are not exceeding our allocated limit
#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
	const FVulkanDescriptorSetsLayout& Layout;
#else
	int32 MaxAllocatedTypes[VK_DESCRIPTOR_TYPE_RANGE_SIZE];
	int32 NumAllocatedTypes[VK_DESCRIPTOR_TYPE_RANGE_SIZE];
	int32 PeakAllocatedTypes[VK_DESCRIPTOR_TYPE_RANGE_SIZE];
#endif
	VkDescriptorPool DescriptorPool;

	friend class FVulkanCommandListContext;
};


#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
class FVulkanTypedDescriptorPoolSet
{
	typedef TList<FOLDVulkanDescriptorPool*> FPoolList;

	FOLDVulkanDescriptorPool* GetFreePool(bool bForceNewPool = false);
	FOLDVulkanDescriptorPool* PushNewPool();

protected:
	friend class FVulkanDescriptorPoolSet;

	FVulkanTypedDescriptorPoolSet(FVulkanDevice* InDevice, class FVulkanDescriptorPoolSet* InOwner, const FVulkanDescriptorSetsLayout& InLayout)
		: Device(InDevice)
		, Owner(InOwner)
		, Layout(InLayout)
	{
		PushNewPool();
	};

	~FVulkanTypedDescriptorPoolSet();

	void Reset();

public:
	bool AllocateDescriptorSets(const FVulkanDescriptorSetsLayout& Layout, VkDescriptorSet* OutSets);

	class FVulkanDescriptorPoolSet* GetOwner() const
	{
		return Owner;
	}

private:
	FVulkanDevice* Device;
	class FVulkanDescriptorPoolSet* Owner;
	const FVulkanDescriptorSetsLayout& Layout;

	FPoolList* PoolListHead = nullptr;
	FPoolList* PoolListCurrent = nullptr;
};

class FVulkanDescriptorPoolSet
{
public:
	FVulkanDescriptorPoolSet(FVulkanDevice* InDevice)
		: Device(InDevice)
		, LastFrameUsed(GFrameNumberRenderThread)
		, bUsed(true)
	{
	}

	~FVulkanDescriptorPoolSet();

	FVulkanTypedDescriptorPoolSet* AcquirePoolSet(const FVulkanDescriptorSetsLayout& Layout);

	void Reset();

	void SetUsed(bool bInUsed)
	{
		bUsed = bInUsed;
		LastFrameUsed = bUsed ? GFrameNumberRenderThread : LastFrameUsed;
	}

	bool IsUnused() const
	{
		return !bUsed;
	}

	uint32 GetLastFrameUsed() const
	{
		return LastFrameUsed;
	}

private:
	FVulkanDevice* Device;

	TMap<uint32, FVulkanTypedDescriptorPoolSet*> DescriptorPools;

	uint32 LastFrameUsed;
	bool bUsed;
};

class FVulkanDescriptorPoolsManager
{
	class FVulkanAsyncPoolSetDeletionWorker : public FNonAbandonableTask
	{
		FVulkanDescriptorPoolSet* PoolSet;

	public:
		FVulkanAsyncPoolSetDeletionWorker(FVulkanDescriptorPoolSet* InPoolSet)
			: PoolSet(InPoolSet)
		{};

		void DoWork()
		{
			check(PoolSet != nullptr);

			delete PoolSet;

			PoolSet = nullptr;
		}

		void SetPoolSet(FVulkanDescriptorPoolSet* InPoolSet)
		{
			check(PoolSet == nullptr);
			PoolSet = InPoolSet;
		}

		FORCEINLINE TStatId GetStatId() const
		{
			RETURN_QUICK_DECLARE_CYCLE_STAT(FVulkanAsyncPoolSetDeletionWorker, STATGROUP_ThreadPoolAsyncTasks);
		}
	};

public:
	~FVulkanDescriptorPoolsManager();

	void Init(FVulkanDevice* InDevice)
	{
		Device = InDevice;
	}

	FVulkanDescriptorPoolSet& AcquirePoolSet();
	void GC();

private:
	FVulkanDevice* Device = nullptr;
	FAsyncTask<FVulkanAsyncPoolSetDeletionWorker>* AsyncDeletionTask = nullptr;

	FCriticalSection CS;
	TArray<FVulkanDescriptorPoolSet*> PoolSets;
};
#endif

// The actual descriptor sets for a given pipeline
class FOLDVulkanDescriptorSets
{
public:
	~FOLDVulkanDescriptorSets();

	typedef TArray<VkDescriptorSet, TInlineAllocator<SF_Compute>> FDescriptorSetArray;

	inline const FDescriptorSetArray& GetHandles() const
	{
		return Sets;
	}

	inline void Bind(VkCommandBuffer CmdBuffer, VkPipelineLayout PipelineLayout, VkPipelineBindPoint BindPoint)
	{
		VulkanRHI::vkCmdBindDescriptorSets(CmdBuffer,
			BindPoint,
			PipelineLayout,
			0, Sets.Num(), Sets.GetData(),
			0, nullptr);
	}

private:
	FOLDVulkanDescriptorSets(FVulkanDevice* InDevice, const FVulkanDescriptorSetsLayout& InLayout, FVulkanCommandListContext* InContext);

	FVulkanDevice* Device;
	FOLDVulkanDescriptorPool* Pool;
	const FVulkanDescriptorSetsLayout& Layout;
	FDescriptorSetArray Sets;

	friend class FOLDVulkanDescriptorPool;
	friend class FOLDVulkanDescriptorSetRingBuffer;
	friend class FVulkanCommandListContext;
};
#endif

// This container holds the actual VkWriteDescriptorSet structures; a Compute pipeline uses the arrays 'as-is', whereas a 
// Gfx PSO will have one big array and chunk it depending on the stage (eg Vertex, Pixel).
struct FVulkanDescriptorSetWriteContainer
{
	TArray<VkDescriptorImageInfo> DescriptorImageInfo;
	TArray<VkDescriptorBufferInfo> DescriptorBufferInfo;
	TArray<VkWriteDescriptorSet> DescriptorWrites;
};

// Layout for a Pipeline, also includes DescriptorSets layout
class FVulkanLayout : public VulkanRHI::FDeviceChild
{
public:
	FVulkanLayout(FVulkanDevice* InDevice);
	virtual ~FVulkanLayout();

	inline const FVulkanDescriptorSetsLayout& GetDescriptorSetsLayout() const
	{
		return DescriptorSetLayout;
	}

	inline VkPipelineLayout GetPipelineLayout() const
	{
		return PipelineLayout;
	}

	inline bool HasDescriptors() const
	{
		return DescriptorSetLayout.GetLayouts().Num() > 0;
	}

	inline uint32 GetDescriptorSetLayoutHash() const
	{
		return DescriptorSetLayout.GetHash();
	}

protected:
	FVulkanDescriptorSetsLayout DescriptorSetLayout;
	VkPipelineLayout PipelineLayout;

	inline void AddBindingsForStage(VkShaderStageFlagBits StageFlags, EDescriptorSetStage DescSet, const FVulkanCodeHeader& CodeHeader)
	{
		// Setting descriptor is only allowed prior to compiling the layout
		check(DescriptorSetLayout.GetHandles().Num() == 0);

		DescriptorSetLayout.AddBindingsForStage(StageFlags, DescSet, CodeHeader);
	}

	void Compile();

#if VULKAN_KEEP_CREATE_INFO
	VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo;
#endif

	friend class FVulkanComputePipeline;
	friend class FVulkanGfxPipeline;
	friend class FVulkanPipelineStateCache;
};

#if !VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
// This class handles allocating/reusing descriptor sets per command list for a specific pipeline layout (each context holds one of this)
class FOLDVulkanDescriptorSetRingBuffer : public VulkanRHI::FDeviceChild
{
public:
	FOLDVulkanDescriptorSetRingBuffer(FVulkanDevice* InDevice);
	virtual ~FOLDVulkanDescriptorSetRingBuffer();

	void Reset()
	{
		CurrDescriptorSets = nullptr;
	}

	inline void Bind(VkCommandBuffer CmdBuffer, VkPipelineLayout Layout, VkPipelineBindPoint BindPoint)
	{
		check(CurrDescriptorSets);
		CurrDescriptorSets->Bind(CmdBuffer, Layout, VK_PIPELINE_BIND_POINT_COMPUTE);
	}

protected:
	FOLDVulkanDescriptorSets* CurrDescriptorSets;

	struct FDescriptorSetsPair
	{
		uint64 FenceCounter;
		FOLDVulkanDescriptorSets* DescriptorSets;

		FDescriptorSetsPair()
			: FenceCounter(0)
			, DescriptorSets(nullptr)
		{
		}

		~FDescriptorSetsPair();
	};

	struct FDescriptorSetsEntry
	{
		FVulkanCmdBuffer* CmdBuffer;
		TArray<FDescriptorSetsPair> Pairs;

		FDescriptorSetsEntry(FVulkanCmdBuffer* InCmdBuffer)
			: CmdBuffer(InCmdBuffer)
		{
		}
	};
	TArray<FDescriptorSetsEntry*> DescriptorSetsEntries;

	FOLDVulkanDescriptorSets* RequestDescriptorSets(FVulkanCommandListContext* Context, FVulkanCmdBuffer* CmdBuffer, const FVulkanLayout& Layout);

	friend class FVulkanComputePipelineState;
	friend class FVulkanGfxPipelineState;
};
#endif

// This class encapsulates updating VkWriteDescriptorSet structures (but doesn't own them), and their flags for dirty ranges; it is intended
// to be used to access a sub-region of a long array of VkWriteDescriptorSet (ie FVulkanDescriptorSetWriteContainer)
class FVulkanDescriptorSetWriter
{
public:
	FVulkanDescriptorSetWriter()
		: WriteDescriptors(nullptr)
		, DirtyMask(0)
		, FullMask(0)
		, NumWrites(0)
	{
	}

	void ResetDirty()
	{
		DirtyMask = 0;
	}

	void MarkAllDirty()
	{
		DirtyMask = FullMask;
	}

	void WriteUniformBuffer(uint32 DescriptorIndex, VkBuffer Buffer, VkDeviceSize Offset, VkDeviceSize Range)
	{
		check(DescriptorIndex < NumWrites);
		check(WriteDescriptors[DescriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || WriteDescriptors[DescriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
		VkDescriptorBufferInfo* BufferInfo = const_cast<VkDescriptorBufferInfo*>(WriteDescriptors[DescriptorIndex].pBufferInfo);
		check(BufferInfo);
		BufferInfo->buffer = Buffer;
		BufferInfo->offset = Offset;
		BufferInfo->range = Range;
		DirtyMask = DirtyMask | ((uint64)1 << DescriptorIndex);
	}

	void WriteSampler(uint32 DescriptorIndex, VkSampler Sampler)
	{
		check(DescriptorIndex < NumWrites);
		check(WriteDescriptors[DescriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER || WriteDescriptors[DescriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		VkDescriptorImageInfo* ImageInfo = const_cast<VkDescriptorImageInfo*>(WriteDescriptors[DescriptorIndex].pImageInfo);
		check(ImageInfo);
		ImageInfo->sampler = Sampler;
		DirtyMask = DirtyMask | ((uint64)1 << DescriptorIndex);
	}

	void WriteImage(uint32 DescriptorIndex, VkImageView ImageView, VkImageLayout Layout)
	{
		check(DescriptorIndex < NumWrites);
		check(WriteDescriptors[DescriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || WriteDescriptors[DescriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		VkDescriptorImageInfo* ImageInfo = const_cast<VkDescriptorImageInfo*>(WriteDescriptors[DescriptorIndex].pImageInfo);
		check(ImageInfo);
		ImageInfo->imageView = ImageView;
		ImageInfo->imageLayout = Layout;
		DirtyMask = DirtyMask | ((uint64)1 << DescriptorIndex);
	}

	void WriteStorageImage(uint32 DescriptorIndex, VkImageView ImageView, VkImageLayout Layout)
	{
		check(DescriptorIndex < NumWrites);
		check(WriteDescriptors[DescriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		VkDescriptorImageInfo* ImageInfo = const_cast<VkDescriptorImageInfo*>(WriteDescriptors[DescriptorIndex].pImageInfo);
		check(ImageInfo);
		ImageInfo->imageView = ImageView;
		ImageInfo->imageLayout = Layout;
		DirtyMask = DirtyMask | ((uint64)1 << DescriptorIndex);
	}

	void WriteStorageTexelBuffer(uint32 DescriptorIndex, FVulkanBufferView* View)
	{
		check(DescriptorIndex < NumWrites);
		check(WriteDescriptors[DescriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER);
		WriteDescriptors[DescriptorIndex].pTexelBufferView = &View->View;
		DirtyMask = DirtyMask | ((uint64)1 << DescriptorIndex);
		BufferViewReferences[DescriptorIndex] = View;
	}

	void WriteStorageBuffer(uint32 DescriptorIndex, VkBuffer Buffer, uint32 Offset, uint32 Range)
	{
		check(DescriptorIndex < NumWrites);
		check(WriteDescriptors[DescriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || WriteDescriptors[DescriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC);
		VkDescriptorBufferInfo* BufferInfo = const_cast<VkDescriptorBufferInfo*>(WriteDescriptors[DescriptorIndex].pBufferInfo);
		check(BufferInfo);
		BufferInfo->buffer = Buffer;
		BufferInfo->offset = Offset;
		BufferInfo->range = Range;
		DirtyMask = DirtyMask | ((uint64)1 << DescriptorIndex);
	}

	void WriteUniformTexelBuffer(uint32 DescriptorIndex, FVulkanBufferView* View)
	{
		check(DescriptorIndex < NumWrites);
		check(WriteDescriptors[DescriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER);
		WriteDescriptors[DescriptorIndex].pTexelBufferView = &View->View;
		DirtyMask = DirtyMask | ((uint64)1 << DescriptorIndex);
		BufferViewReferences[DescriptorIndex] = View;
	}

	void ClearBufferView(uint32 DescriptorIndex)
	{
		BufferViewReferences[DescriptorIndex] = nullptr;
	}

	void SetDescriptorSet(VkDescriptorSet DescriptorSet)
	{
		for (uint32 Index = 0; Index < NumWrites; ++Index)
		{
			WriteDescriptors[Index].dstSet = DescriptorSet;
		}
	}

protected:
	// A view into someone else's descriptors
	VkWriteDescriptorSet* WriteDescriptors;

	uint64 DirtyMask;
	uint64 FullMask;
	uint32 NumWrites;
	TArray<TRefCountPtr<FVulkanBufferView>> BufferViewReferences;

	void SetupDescriptorWrites(const FNEWVulkanShaderDescriptorInfo& Info, VkWriteDescriptorSet* InWriteDescriptors, VkDescriptorImageInfo* InImageInfo, VkDescriptorBufferInfo* InBufferInfo);

	friend class FVulkanComputePipelineState;
	friend class FVulkanGfxPipelineState;
};

#if VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
namespace VulkanRHI
{
	class FDescriptorSetsAllocator : public VulkanRHI::FDeviceChild
	{
	public:
		FDescriptorSetsAllocator(FVulkanDevice* InDevice, const FVulkanLayout& InLayout, uint32 InNumAllocationsPerPool);
		~FDescriptorSetsAllocator();

		TArrayView<VkDescriptorSet> Allocate(const FVulkanLayout& Layout, uint32 InNumAllocations/*, FVulkanCmdBuffer* CmdBuffer*/);
		void Reset(FVulkanCmdBuffer* CmdBuffer);
		inline void SetFence(FVulkanCmdBuffer* InCmdBuffer, uint64 InCurrentFence)
		{
			CurrentCmdBuffer = InCmdBuffer;
			CurrentFence = InCurrentFence;
		}

	protected:
		FVulkanCmdBuffer* CurrentCmdBuffer = nullptr;
		uint64 CurrentFence = 0;

		struct FPoolEntry
		{
			VkDescriptorPool Pool = VK_NULL_HANDLE;

			// We don't use a TArray as these get copied by value!
			VkDescriptorSet* Sets;
			int32 MaxSets = 0;

			FPoolEntry() = default;

			FPoolEntry(const FPoolEntry& In)
				: Pool(In.Pool)
				, Sets(In.Sets)
				, MaxSets(In.MaxSets)
			{
			}
		};

		struct FFreePoolEntry : public FPoolEntry
		{
			uint32 LastUsedFrame = 0;

			FFreePoolEntry() = default;

			FFreePoolEntry(const FPoolEntry& In)
				: FPoolEntry(In)
			{
			}
		};

		struct FFencedPoolEntry : public FPoolEntry
		{
			int32 NumUsedSets = 0;
			FVulkanCmdBuffer* CmdBuffer = nullptr;
			uint64 Fence = 0;

			FFencedPoolEntry() = default;

			FFencedPoolEntry(const FPoolEntry& In)
				: FPoolEntry(In)
			{
			}
		};
		TArray<FFencedPoolEntry> UsedEntries;
		TArray<FFreePoolEntry> FreeEntries;

		FFencedPoolEntry* CreatePool(const FVulkanLayout& Layout);

		VkDescriptorPoolCreateInfo CreateInfo;
		TArray<VkDescriptorPoolSize> CreateInfoTypes;
		uint32 NumAllocationsPerPool = 0;
		friend class FVulkanCommandBufferPool;
	};
}
#endif
