// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MallocBinned.cpp: Binned memory allocator
=============================================================================*/

#include "CorePrivatePCH.h"

#include "MallocBinned2.h"
#include "MemoryMisc.h"
#include "HAL/PlatformAtomics.h"

#define BINNED2_LARGE_ALLOC         65536 // Alignment of OS-allocated pointer - pool-allocated pointers will have a non-aligned pointer
#define BINNED2_MINIMUM_ALIGNMENT   16    // Alignment of blocks
#define BINNED2_MAX_SMALL_POOL_SIZE 32768 // Maximum block size in GMallocBinned2SmallBlockSizes


static int32 GMallocBinned2PerThreadCaches = 1;
static FAutoConsoleVariableRef GMallocBinned2PerThreadCachesCVar(
	TEXT("MallocBinned2.PerThreadCaches"),
	GMallocBinned2PerThreadCaches,
	TEXT("Enables per-thread caches of small (<= 32768 byte) allocations from FMallocBinned2")
);

static int32 GMallocBinned2BundleSize = BINNED2_LARGE_ALLOC * 2;
static FAutoConsoleVariableRef GMallocBinned2BundleSizeCVar(
	TEXT("MallocBinned2.BundleSize"),
	GMallocBinned2BundleSize,
	TEXT("Size in bytes of per-block bundles used in the recycling process")
);

static int32 GMallocBinned2MaxBundlesBeforeRecycle = 4;
static FAutoConsoleVariableRef GMallocBinned2MaxBundlesBeforeRecycleCVar(
	TEXT("MallocBinned2.BundleRecycleCount"),
	GMallocBinned2MaxBundlesBeforeRecycle,
	TEXT("Number of freed bundles in the global recycler before it returns them to the system, per-block size")
);

// Block sizes are based around getting the maximum amount of allocations per pool, with as little alignment waste as possible.
// Block sizes should be close to even divisors of the system page size, and well distributed.
// They must be 16-byte aligned as well.
const uint32 GMallocBinned2SmallBlockSizes[] =
{
	16,		32,		48,		64,		80,		96,		112,	128,
	160,	192,	224,	256,	288,	320,	384,	448,
	512,	576,	640,	704,	768,	896,	1024,	1168,
	1360,	1632,	2048,	2336,	2720,	3264,	4096,	4672,
	5456,	6544,	8192,	9360,	10912,	13104,	16384,	21840,
	32768
};

// Mapping of sizes to small table indices
uint32 GMallocBinned2MemSizeToIndex[BINNED2_MAX_SMALL_POOL_SIZE];

/** Information about a piece of free memory. */
struct FMallocBinned2::FFreeBlock
{
	FFreeBlock(uint32 InPageSize, uint32 InBlockSize)
		: NextFreeBlock       (nullptr)
		, NumFreeBlocks       (InPageSize / InBlockSize - 1)
		, BlockSize           (InBlockSize)
		, bShortBlockAllocated(false)
	{
	}

	uint32 GetNumFreeRegularBlocks() const
	{
		return NumFreeBlocks;
	}

	uint32 GetShortBlockSize() const
	{
		return BlockSize - BINNED2_MINIMUM_ALIGNMENT;
	}

	bool HasFreeShortBlock() const
	{
		return !bShortBlockAllocated;
	}

	void* AllocateRegularBlock()
	{
		--NumFreeBlocks;
		void* Result = (uint8*)this + (NumFreeBlocks + IsAligned(this, BINNED2_LARGE_ALLOC)) * BlockSize;

		return Result;
	}

	void* AllocateShortBlock()
	{
		// Make sure this isn't a fake block
		checkSlow(IsAligned(this, BINNED2_LARGE_ALLOC));

		if (bShortBlockAllocated)
		{
			return nullptr;
		}

		bShortBlockAllocated = true;
		return (uint8*)this + BINNED2_MINIMUM_ALIGNMENT;
	}

 public:	void*  NextFreeBlock;             // Next free block in another pool
 public:	uint32 NumFreeBlocks;             // Number of consecutive free blocks here, at least 1.
 public:	uint32 BlockSize            : 31; // Size of the blocks that this list points to
 public:	uint32 bShortBlockAllocated : 1;  // Whether the short block is allocated or not
};

FMallocBinned2::FPoolList::FPoolList()
	: Front(nullptr)
{
}

FMallocBinned2::FPoolTable::FPoolTable()
	: BlockSize(0)
{
}

struct FMallocBinned2::FPoolInfo
{
 public:	uint16      Taken;          // Number of allocated elements in this pool, when counts down to zero can free the entire pool
private:	uint32      AllocSize;      // Number of bytes allocated
 public:	FFreeBlock* FirstFreeBlock; // Pointer to first free memory in this pool or the OS Allocation Size in bytes if this allocation is not binned
 public:	FPoolInfo*  Next;           // Pointer to next pool
 public:	FPoolInfo** PtrToPrevNext;  // Pointer to whichever pointer points to this pool

public:
	bool HasFreeRegularBlock() const
	{
		return FirstFreeBlock && FirstFreeBlock->GetNumFreeRegularBlocks() != 0;
	}

	bool HasFreeShortBlock() const
	{
		return FirstFreeBlock && FirstFreeBlock->HasFreeShortBlock();
	}

	uint32 GetShortBlockSize() const
	{
		return FirstFreeBlock ? FirstFreeBlock->GetShortBlockSize() : 0;
	}

	void* AllocateRegularBlock()
	{
		checkSlow(HasFreeRegularBlock());
		++Taken;
		void* Result = FirstFreeBlock->AllocateRegularBlock();
		ExhaustPoolIfNecessary();
		return Result;
	}

	void* AllocateShortBlock()
	{
		checkSlow(HasFreeShortBlock());
		++Taken;
		void* Result = FirstFreeBlock->AllocateShortBlock();
		ExhaustPoolIfNecessary();
		return Result;
	}

	uint32 GetOSRequestedBytes() const
	{
		return AllocSize;
	}

	UPTRINT GetOsAllocatedBytes() const
	{
		return (UPTRINT)FirstFreeBlock;
	}

	void SetOSAllocationSizes(uint32 InRequestedBytes, UPTRINT InAllocatedBytes)
	{
		checkSlow(InRequestedBytes != 0);                // Shouldn't be pooling zero byte allocations
		checkSlow(InAllocatedBytes >= InRequestedBytes); // We must be allocating at least as much as we requested

		AllocSize      = InRequestedBytes;
		FirstFreeBlock = (FFreeBlock*)InAllocatedBytes;
	}

	void Link(FPoolInfo*& PrevNext)
	{
		if (PrevNext)
		{
			PrevNext->PtrToPrevNext = &Next;
		}
		Next          = PrevNext;
		PtrToPrevNext = &PrevNext;
		PrevNext      = this;
	}

	void Unlink()
	{
		if (Next)
		{
			Next->PtrToPrevNext = PtrToPrevNext;
		}
		*PtrToPrevNext = Next;
	}

private:
	void ExhaustPoolIfNecessary()
	{
		if (FirstFreeBlock->GetNumFreeRegularBlocks() == 0)// && (FirstFreeBlock->GetShortBlockSize() == 0 || !FirstFreeBlock->HasFreeShortBlock()))
		{
			FirstFreeBlock = (FFreeBlock*)FirstFreeBlock->NextFreeBlock;
		}
		checkSlow(!FirstFreeBlock || FirstFreeBlock->GetNumFreeRegularBlocks() != 0 || FirstFreeBlock->HasFreeShortBlock());
	}
};

template <>
struct TIsZeroConstructType<FMallocBinned2::FPoolInfo>
{
	enum { Value = true };
};

/** Hash table struct for retrieving allocation book keeping information */
struct FMallocBinned2::PoolHashBucket
{
	UPTRINT         BucketIndex;
	FPoolInfo*      FirstPool;
	PoolHashBucket* Prev;
	PoolHashBucket* Next;

	PoolHashBucket()
	{
		BucketIndex = 0;
		FirstPool   = nullptr;
		Prev        = this;
		Next        = this;
	}

	void Link(PoolHashBucket* After)
	{
		After->Prev = Prev;
		After->Next = this;
		Prev ->Next = After;
		this ->Prev = After;
	}

	void Unlink()
	{
		Next->Prev = Prev;
		Prev->Next = Next;
		Prev       = this;
		Next       = this;
	}
};

struct FMallocBinned2::Private
{
	FORCEINLINE static bool IsOSAllocation(const void* Ptr)
	{
		return IsAligned(Ptr, BINNED2_LARGE_ALLOC);
	}

	static_assert(ARRAY_COUNT(GMallocBinned2SmallBlockSizes) == SMALL_POOL_COUNT, "Small block size array size must match SMALL_POOL_COUNT");
	static_assert(sizeof(FFreeBlock) <= BINNED2_MINIMUM_ALIGNMENT,                "Free block struct must be small enough to fit into a block.");

	// Implementation. 
	static CA_NO_RETURN void OutOfMemory(uint64 Size, uint32 Alignment=0)
	{
		// this is expected not to return
		FPlatformMemory::OnOutOfMemory(Size, Alignment);
	}

	/**
	 * Gets the FPoolInfo for a memory address. If no valid info exists one is created.
	 */
	static FPoolInfo* GetOrCreatePoolInfo(FMallocBinned2& Allocator, void* InPtr)
	{
		/** 
		 * Creates an array of FPoolInfo structures for tracking allocations.
		 */
		auto CreatePoolArray = [](uint64 NumPools)
		{
			uint64 PoolArraySize = NumPools * sizeof(FPoolInfo);

			void* Result = FPlatformMemory::BinnedAllocFromOS(PoolArraySize);
			if (!Result)
			{
				OutOfMemory(PoolArraySize);
			}

			DefaultConstructItems<FPoolInfo>(Result, NumPools);
			return (FPoolInfo*)Result;
		};

		UPTRINT BucketIndex;
		uint32  PoolIndex;
		Allocator.PtrToPoolMapping.GetHashBucketAndPoolIndices(InPtr, BucketIndex, PoolIndex);

		PoolHashBucket* FirstBucket = &Allocator.HashBuckets[BucketIndex];
		PoolHashBucket* Collision   = FirstBucket;
		do
		{
			if (!Collision->FirstPool)
			{
				Collision->BucketIndex = BucketIndex;
				Collision->FirstPool   = CreatePoolArray(Allocator.NumPoolsPerPage);

				return &Collision->FirstPool[PoolIndex];
			}

			if (Collision->BucketIndex == BucketIndex)
			{
				return &Collision->FirstPool[PoolIndex];
			}

			Collision = Collision->Next;
		}
		while (Collision != FirstBucket);

		// Create a new hash bucket entry
		if (!Allocator.HashBucketFreeList)
		{
			const uint32 PageSize = Allocator.PageSize;

			Allocator.HashBucketFreeList = (PoolHashBucket*)FPlatformMemory::BinnedAllocFromOS(PageSize);

			for (UPTRINT i = 0, n = PageSize / sizeof(PoolHashBucket); i < n; ++i)
			{
				Allocator.HashBucketFreeList->Link(new (Allocator.HashBucketFreeList + i) PoolHashBucket());
			}
		}

		PoolHashBucket* NextFree  = Allocator.HashBucketFreeList->Next;
		PoolHashBucket* NewBucket = Allocator.HashBucketFreeList;

		NewBucket->Unlink();

		if (NextFree == NewBucket)
		{
			NextFree = nullptr;
		}
		Allocator.HashBucketFreeList = NextFree;

		if (!NewBucket->FirstPool)
		{
			NewBucket->FirstPool = CreatePoolArray(Allocator.NumPoolsPerPage);
		}

		NewBucket->BucketIndex = BucketIndex;

		FirstBucket->Link(NewBucket);

		return &NewBucket->FirstPool[PoolIndex];
	}

	static FPoolInfo* FindPoolInfo(FMallocBinned2& Allocator, void* InPtr)
	{
		const UPTRINT PageSize = (UPTRINT)Allocator.PageSize;

		UPTRINT BucketIndex;
		uint32  PoolIndex;
		Allocator.PtrToPoolMapping.GetHashBucketAndPoolIndices(InPtr, BucketIndex, PoolIndex);

		PoolHashBucket* FirstBucket = &Allocator.HashBuckets[BucketIndex];
		PoolHashBucket* Collision   = FirstBucket;
		do
		{
			if (Collision->BucketIndex == BucketIndex)
			{
				return &Collision->FirstPool[PoolIndex];
			}

			Collision = Collision->Next;
		}
		while (Collision != FirstBucket);

		return nullptr;
	}

	static FFreeBlock* GetPoolHeaderFromPointer(void* Ptr)
	{
		return (FFreeBlock*)AlignDown(Ptr, BINNED2_LARGE_ALLOC);
	}

	struct FBundleNode
	{
		FBundleNode* NextNodeInCurrentBundle;
		FBundleNode* NextBundle;
	};

	struct FBundle
	{
		FBundle()
		{
			Reset();
		}

		void Reset()
		{
			Head        = nullptr;
			ApproxBytes = 0;
		}

		void PushHead(FBundleNode* Node, uint32 NodeSize)
		{
			Node->NextNodeInCurrentBundle = Head;
			Head         = Node;
			ApproxBytes += NodeSize;
		}

		FBundleNode* PopHead(uint32 NodeSize)
		{
			check(Head);
			FBundleNode* Result = Head;

			ApproxBytes = FMath::Max<int32>(ApproxBytes - NodeSize, 0);
			Head        = Head->NextNodeInCurrentBundle;

			return Result;
		}

		FBundleNode* Head;
		uint32       ApproxBytes;
	};

	static_assert(sizeof(FBundleNode) <= BINNED2_MINIMUM_ALIGNMENT, "Bundle nodes must fit into the smallest block size");

	struct FGlobalRecycler
	{
		FGlobalRecycler()
		{
			DefaultConstructItems<FBundleNode*>(CurrentBundle, SMALL_POOL_COUNT);
		}

		void PushBundle(uint32 InPoolIndex, FBundleNode* InBundle, FBundleNode*& OutBundlesToRecycle)
		{
			FBundleNode** CurrentHeadPtr = &CurrentBundle[InPoolIndex];
			FBundleNode*  CurrentHead;
			do
			{
				CurrentHead          = *CurrentHeadPtr;
				InBundle->NextBundle = CurrentHead;
			}
			while (FPlatformAtomics::InterlockedCompareExchangePointer((void**)CurrentHeadPtr, InBundle, CurrentHead) != CurrentHead);

			if (BundleCount[InPoolIndex].Increment() >= GMallocBinned2MaxBundlesBeforeRecycle)
			{
				OutBundlesToRecycle = PopBundle(InPoolIndex);
				OutBundlesToRecycle->NextBundle = nullptr;
			}
			else
			{
				OutBundlesToRecycle = nullptr;
			}
		}

		FBundleNode* PopBundle(uint32 InPoolIndex)
		{
			FBundleNode** CurrentHeadPtr = &CurrentBundle[InPoolIndex];
			FBundleNode*  CurrentHead;
			FBundleNode*  NextHead;
			do 
			{
				CurrentHead = *CurrentHeadPtr;
				if (!CurrentHead)
				{
					return nullptr;
				}

				NextHead = CurrentHead->NextBundle;
			}
			while (FPlatformAtomics::InterlockedCompareExchangePointer((void**)CurrentHeadPtr, NextHead, CurrentHead) != CurrentHead);

			BundleCount[InPoolIndex].Decrement();

			return CurrentHead;
		}

	private:
		FBundleNode*       CurrentBundle[SMALL_POOL_COUNT];
		FThreadSafeCounter BundleCount  [SMALL_POOL_COUNT];
	};

	static FGlobalRecycler GGlobalRecycler;

	struct FFreeBlockList
	{
		void PushToFront(void* InPtr, uint32 InPoolIndex, FBundleNode*& OutBundlesToRecycle)
		{
			checkSlow(InPtr);

			OutBundlesToRecycle = nullptr;
			if (PartialBundle.ApproxBytes >= (uint32)GMallocBinned2BundleSize)
			{
				if (FullBundle.Head)
				{
					GGlobalRecycler.PushBundle(InPoolIndex, FullBundle.Head, OutBundlesToRecycle);
				}

				FullBundle = PartialBundle;
				PartialBundle.Reset();
			}

			PartialBundle.PushHead((FBundleNode*)InPtr, GMallocBinned2SmallBlockSizes[InPoolIndex]);
		}

		void* PopFromFront(uint32 InPoolIndex)
		{
			if (!PartialBundle.Head)
			{
				FBundle NewPartialBundle = FullBundle;
				if (!NewPartialBundle.Head)
				{
					NewPartialBundle.Head = GGlobalRecycler.PopBundle(InPoolIndex);
					if (!NewPartialBundle.Head)
					{
						return nullptr;
					}

					NewPartialBundle.ApproxBytes = (uint32)GMallocBinned2BundleSize;
				}

				PartialBundle = NewPartialBundle;
				FullBundle.Reset();
			}

			void* Result = PartialBundle.PopHead(GMallocBinned2SmallBlockSizes[InPoolIndex]);
			return Result;
		}

		FBundleNode* PopBundles(uint32 InPoolIndex)
		{
			FBundleNode* Partial = PartialBundle.Head;
			if (Partial)
			{
				PartialBundle.Reset();
				Partial->NextBundle = nullptr;
			}

			FBundleNode* Full = FullBundle.Head;
			if (Full)
			{
				FullBundle.Reset();
				Full->NextBundle = nullptr;
			}

			FBundleNode* Result = Partial;
			if (Result)
			{
				Result->NextBundle = Full;
			}
			else
			{
				Result = Full;
			}

			return Result;
		}

		bool IsEmpty() const
		{
			return !PartialBundle.Head && !FullBundle.Head;
		}

	private:
		FBundle PartialBundle;
		FBundle FullBundle;
	};

	// We've reinvented TThreadSingleton here because we don't want it to be re-entering our allocation
	// functions!
	template <typename T>
	struct TThreadSingleton2
	{
		FORCEINLINE static T& Get(uint32 PageSize)
		{
			static uint32 TlsSlot = 0;

			if (TlsSlot == 0)
			{
				const uint32 ThisTlsSlot = FPlatformTLS::AllocTlsSlot();
				const uint32 PrevTlsSlot = FPlatformAtomics::InterlockedCompareExchange((int32*)&TlsSlot, (int32)ThisTlsSlot, 0);
				if (PrevTlsSlot != 0)
				{
					FPlatformTLS::FreeTlsSlot(ThisTlsSlot);
				}
			}

			T* ThreadSingleton = (T*)FPlatformTLS::GetTlsValue(TlsSlot);
			if (!ThreadSingleton)
			{
				ThreadSingleton = new (FPlatformMemory::BinnedAllocFromOS(Align(sizeof(T), PageSize))) T();
				FPlatformTLS::SetTlsValue(TlsSlot, ThreadSingleton);
			}

			return *ThreadSingleton;
		}
	};

	struct FPerThreadFreeBlockLists : TThreadSingleton2<FPerThreadFreeBlockLists>
	{
		FPerThreadFreeBlockLists()
		{
		}

		void* Malloc(uint32 InPoolIndex)
		{
			checkSlow(InPoolIndex >= 0 && InPoolIndex < SMALL_POOL_COUNT);

			FFreeBlockList& List = FreeLists[InPoolIndex];
			if (List.IsEmpty())
			{
				return nullptr;
			}

			void* Result = List.PopFromFront(InPoolIndex);
			return Result;
		}

		void Free(void* InPtr, uint32 InPoolIndex, FBundleNode*& OutBundlesToRecycle)
		{
			checkSlow(InPoolIndex >= 0 && InPoolIndex < SMALL_POOL_COUNT);

			FreeLists[InPoolIndex].PushToFront(InPtr, InPoolIndex, OutBundlesToRecycle);
		}

		FBundleNode* PopBundles(uint32 InPoolIndex)
		{
			checkSlow(InPoolIndex >= 0 && InPoolIndex < SMALL_POOL_COUNT);

			FBundleNode* Result = FreeLists[InPoolIndex].PopBundles(InPoolIndex);
			return Result;
		}

	private:
		FFreeBlockList FreeLists[SMALL_POOL_COUNT];
	};

	static void FreeBundles(FMallocBinned2& Allocator, FBundleNode* BundlesToRecycle, uint32 InBlockSize, uint32 InPoolIndex)
	{
		FPoolTable& Table = Allocator.SmallPoolTables[InPoolIndex];

		FBundleNode* Bundle = BundlesToRecycle;
		while (Bundle)
		{
			FBundleNode* NextBundle = Bundle->NextBundle;

			FBundleNode* Node = Bundle;
			do
			{
				FBundleNode* NextNode = Node->NextNodeInCurrentBundle;
				FPoolInfo*   NodePool = FindPoolInfo(Allocator, Node);

				// If this pool was exhausted, move to available list.
				if (!NodePool->FirstFreeBlock)
				{
					Table.ActivePools.LinkToFront(NodePool);
				}

				// Free a pooled allocation.
				FFreeBlock* Free = (FFreeBlock*)Node;
				Free->NumFreeBlocks = 1;
				Free->NextFreeBlock = NodePool->FirstFreeBlock;
				Free->BlockSize     = InBlockSize;
				Free->bShortBlockAllocated = false;
				NodePool->FirstFreeBlock   = Free;

				// Free this pool.
				checkSlow(NodePool->Taken >= 1);
				if (--NodePool->Taken == 0)
				{
					FFreeBlock* BasePtrOfNode = GetPoolHeaderFromPointer(Node);

					// Free the OS memory.
					NodePool->Unlink();
					Allocator.CachedOSPageAllocator.Free(BasePtrOfNode, Allocator.PageSize);
				}

				Node = NextNode;
			} while (Node);

			Bundle = NextBundle;
		}
	}
};

FMallocBinned2::Private::FGlobalRecycler FMallocBinned2::Private::GGlobalRecycler;

FORCEINLINE bool FMallocBinned2::FPoolList::IsEmpty() const
{
	return Front == nullptr;
}

FORCEINLINE FMallocBinned2::FPoolInfo& FMallocBinned2::FPoolList::GetFrontPool()
{
	checkSlow(!IsEmpty());
	return *Front;
}

FORCEINLINE const FMallocBinned2::FPoolInfo& FMallocBinned2::FPoolList::GetFrontPool() const
{
	checkSlow(!IsEmpty());
	return *Front;
}

void FMallocBinned2::FPoolList::LinkToFront(FPoolInfo* Pool)
{
	Pool->Unlink();
	Pool->Link(Front);
}

FMallocBinned2::FPoolInfo& FMallocBinned2::FPoolList::PushNewPoolToFront(FMallocBinned2& Allocator, uint32 InBlockSize)
{
	const uint32 LocalPageSize = Allocator.PageSize;

	// Allocate memory.
	FFreeBlock* Free = new (Allocator.CachedOSPageAllocator.Allocate(LocalPageSize)) FFreeBlock(LocalPageSize, InBlockSize);
	if (!Free)
	{
		Private::OutOfMemory(LocalPageSize);
	}

	// Create pool
	FPoolInfo* Result = Private::GetOrCreatePoolInfo(Allocator, Free);
	Result->Link(Front);
	Result->Taken          = 0;
	Result->FirstFreeBlock = Free;

	return *Result;
}

FMallocBinned2::FMallocBinned2(uint32 InPageSize, uint64 AddressLimit)
	: PageSize          (InPageSize)
	, NumPoolsPerPage   (PageSize / sizeof(FPoolInfo))
	, PtrToPoolMapping  (PageSize, NumPoolsPerPage, AddressLimit)
	, HashBucketFreeList(nullptr)
{
	checkf(FMath::IsPowerOfTwo(PageSize),                                                      TEXT("OS page size must be a power of two"));
	checkf(FMath::IsPowerOfTwo(AddressLimit),                                                  TEXT("OS address limit must be a power of two"));
	checkf(AddressLimit > PageSize,                                                            TEXT("OS address limit must be greater than the page size")); // Check to catch 32 bit overflow in AddressLimit
	checkf(GMallocBinned2SmallBlockSizes[SMALL_POOL_COUNT - 1] == BINNED2_MAX_SMALL_POOL_SIZE, TEXT("BINNED2_MAX_SMALL_POOL_SIZE must equal the smallest block size"));
	checkf(PageSize % BINNED2_LARGE_ALLOC == 0,                                                TEXT("OS page size must be a multiple of BINNED2_LARGE_ALLOC"));
	checkf(sizeof(FMallocBinned2::FFreeBlock) <= GMallocBinned2SmallBlockSizes[0],             TEXT("Pool header must be able to fit into the smallest block"));

	// Init pool tables.
	for (uint32 Index = 0; Index != SMALL_POOL_COUNT; ++Index)
	{
		checkf(Index == 0 || GMallocBinned2SmallBlockSizes[Index - 1] < GMallocBinned2SmallBlockSizes[Index], TEXT("Small block sizes must be strictly increasing"));
		checkf(GMallocBinned2SmallBlockSizes[Index] <= PageSize,                                              TEXT("Small block size must be small enough to fit into a page"));
		checkf(GMallocBinned2SmallBlockSizes[Index] % BINNED2_MINIMUM_ALIGNMENT == 0,                         TEXT("Small block size must be a multiple of BINNED2_MINIMUM_ALIGNMENT"));

		SmallPoolTables[Index].BlockSize = GMallocBinned2SmallBlockSizes[Index];
	}

	// Set up pool mappings
	uint32* IndexEntry = GMallocBinned2MemSizeToIndex;
	uint32  PoolIndex  = 0;
	for (uint32 Index = 0; Index != BINNED2_MAX_SMALL_POOL_SIZE; ++Index)
	{
		while (GMallocBinned2SmallBlockSizes[PoolIndex] - 1 < Index)
		{
			++PoolIndex;
			check(PoolIndex != SMALL_POOL_COUNT);
		}
		*IndexEntry++ = PoolIndex;
	}

	uint64 MaxHashBuckets = PtrToPoolMapping.GetMaxHashBuckets();

	HashBuckets = (PoolHashBucket*)FPlatformMemory::BinnedAllocFromOS(Align(MaxHashBuckets * sizeof(PoolHashBucket), PageSize));
	DefaultConstructItems<PoolHashBucket>(HashBuckets, MaxHashBuckets);
}

FMallocBinned2::~FMallocBinned2()
{
}

bool FMallocBinned2::IsInternallyThreadSafe() const
{ 
	return true;
}

void* FMallocBinned2::Malloc(SIZE_T Size, uint32 Alignment)
{
	Alignment = FMath::Max<uint32>(Alignment, BINNED2_MINIMUM_ALIGNMENT);
	Size      = Align(FMath::Max((SIZE_T)1, Size), Alignment);

	checkf(FMath::IsPowerOfTwo(Alignment), TEXT("Alignment must be a power of two"));
	checkf(Alignment <= PageSize, TEXT("Alignment cannot exceed the system page size"));

	// Only allocate from the small pools if the size is small enough and the alignment isn't crazy large.
	// With large alignments, we'll waste a lot of memory allocating an entire page, but such alignments are highly unlikely in practice.
	if (Size <= BINNED2_MAX_SMALL_POOL_SIZE && Alignment <= BINNED2_MINIMUM_ALIGNMENT)
	{
		uint32 PoolIndex = GMallocBinned2MemSizeToIndex[Size - 1];

		if (GMallocBinned2PerThreadCaches)
		{
			Private::FPerThreadFreeBlockLists& Lists = Private::FPerThreadFreeBlockLists::Get(PageSize);
			if (void* Result = Lists.Malloc(PoolIndex))
			{
				return Result;
			}
		}

		FScopeLock Lock(&Mutex);

		// Allocate from small object pool.
		FPoolTable& Table = SmallPoolTables[PoolIndex];

		FPoolInfo* Pool;
		if (!Table.ActivePools.IsEmpty())
		{
			Pool = &Table.ActivePools.GetFrontPool();
		}
		else
		{
			Pool = &Table.ActivePools.PushNewPoolToFront(*this, Table.BlockSize);
		}

		void* Result = Pool->AllocateRegularBlock();
		if (!Pool->HasFreeRegularBlock())
		{
			Table.ExhaustedPools.LinkToFront(Pool);
		}

		return Result;
	}

	FScopeLock Lock(&Mutex);

	// Use OS for non-pooled allocations.
	UPTRINT AlignedSize = Align(Size, PageSize);
	void* Result = CachedOSPageAllocator.Allocate(AlignedSize);
	if (!Result)
	{
		Private::OutOfMemory(AlignedSize);
	}

	// Create pool.
	FPoolInfo* Pool = Private::GetOrCreatePoolInfo(*this, Result);
	Pool->SetOSAllocationSizes(Size, AlignedSize);

	return Result;
}

void* FMallocBinned2::Realloc(void* Ptr, SIZE_T NewSize, uint32 Alignment)
{
	if (NewSize == 0)
	{
		FMallocBinned2::Free(Ptr);
		return nullptr;
	}

	if (!Ptr)
	{
		void* Result = FMallocBinned2::Malloc(NewSize, Alignment);
		return Result;
	}

	Alignment = FMath::Max<uint32>(Alignment, BINNED2_MINIMUM_ALIGNMENT);
	NewSize   = Align(NewSize, Alignment);

	checkf(FMath::IsPowerOfTwo(Alignment), TEXT("Alignment must be a power of two"));
	checkf(Alignment <= PageSize, TEXT("Alignment cannot exceed the system page size"));

	if (!Private::IsOSAllocation(Ptr))
	{
		// Reallocate to a smaller/bigger pool if necessary
		FFreeBlock* Free       = Private::GetPoolHeaderFromPointer(Ptr);
		uint32      TableIndex = GMallocBinned2MemSizeToIndex[Free->BlockSize - 1];
		FPoolTable& Table      = SmallPoolTables[TableIndex];
		if (NewSize <= Table.BlockSize && (TableIndex == 0 || NewSize > SmallPoolTables[TableIndex - 1].BlockSize) && IsAligned(Ptr, Alignment))
		{
			return Ptr;
		}

		// Reallocate and copy the data across
		void* Result = FMallocBinned2::Malloc(NewSize, Alignment);
		FMemory::Memcpy(Result, Ptr, FMath::Min<SIZE_T>(NewSize, Table.BlockSize));
		FMallocBinned2::Free(Ptr);
		return Result;
	}

	FScopeLock Lock(&Mutex);

	// Allocated from OS.
	FPoolInfo* Pool        = Private::FindPoolInfo(*this, Ptr);
	UPTRINT    PoolOsBytes = Pool->GetOsAllocatedBytes();
	if (NewSize > PoolOsBytes || NewSize * 3 < PoolOsBytes * 2)
	{
		// Grow or shrink.
		void* Result = FMallocBinned2::Malloc(NewSize, Alignment);
		FMemory::Memcpy(Result, Ptr, FMath::Min<SIZE_T>(NewSize, Pool->GetOSRequestedBytes()));
		FMallocBinned2::Free(Ptr);
		return Result;
	}

	Pool->SetOSAllocationSizes(NewSize, PoolOsBytes);

	return Ptr;
}

void FMallocBinned2::Free(void* Ptr)
{
	if (!Ptr)
	{
		return;
	}

	FPoolInfo* Pool = Private::FindPoolInfo(*this, Ptr);
#if PLATFORM_IOS || PLATFORM_MAC
	if (!Pool)
	{
		UE_LOG(LogMemory, Warning, TEXT("Attempting to free a pointer we didn't allocate!"));
		return;
	}
#endif

	checkSlow(Pool);
	if (!Private::IsOSAllocation(Ptr))
	{
		FFreeBlock* BasePtr   = Private::GetPoolHeaderFromPointer(Ptr);
		uint32      BlockSize = BasePtr->BlockSize;
		uint32      PoolIndex = GMallocBinned2MemSizeToIndex[BlockSize - 1];

		Private::FBundleNode* BundlesToRecycle;
		if (GMallocBinned2PerThreadCaches)
		{
			Private::FPerThreadFreeBlockLists& Lists = Private::FPerThreadFreeBlockLists::Get(PageSize);
			Lists.Free(Ptr, PoolIndex, BundlesToRecycle);
		}
		else
		{
			BundlesToRecycle = (Private::FBundleNode*)Ptr;
			BundlesToRecycle->NextNodeInCurrentBundle = nullptr;
		}

		if (BundlesToRecycle)
		{
			FScopeLock Lock(&Mutex);

			Private::FreeBundles(*this, BundlesToRecycle, BlockSize, PoolIndex);
		}
	}
	else
	{
		FScopeLock Lock(&Mutex);

		// Free an OS allocation.
		CachedOSPageAllocator.Free(Ptr, Pool->GetOsAllocatedBytes());
	}
}

bool FMallocBinned2::GetAllocationSize(void* Ptr, SIZE_T& SizeOut)
{
	if (!Ptr)
	{
		return false;
	}

	if (!Private::IsOSAllocation(Ptr))
	{
		const FFreeBlock* Free = Private::GetPoolHeaderFromPointer(Ptr);
		SizeOut = SmallPoolTables[GMallocBinned2MemSizeToIndex[Free->BlockSize - 1]].BlockSize;
		return true;
	}

	FScopeLock Lock(&Mutex);

	FPoolInfo* Pool = Private::FindPoolInfo(*this, Ptr);
	SizeOut = Pool->GetOSRequestedBytes();
	return true;
}

void FMallocBinned2::FPoolList::Validate(TFunctionRef<void(FMallocBinned2::FPoolInfo&)> PoolValidate)
{
	for (FPoolInfo** PoolPtr = &Front; *PoolPtr; PoolPtr = &(*PoolPtr)->Next)
	{
		FPoolInfo* Pool = *PoolPtr;
		check(Pool->PtrToPrevNext == PoolPtr);
		PoolValidate(*Pool);
	}
}

bool FMallocBinned2::ValidateHeap()
{
	FScopeLock Lock(&Mutex);

	for (FPoolTable& Table : SmallPoolTables)
	{
		Table.ActivePools.Validate(
			[](FMallocBinned2::FPoolInfo& Pool)
			{
				check(Pool.FirstFreeBlock);
				for (FFreeBlock* Free = Pool.FirstFreeBlock; Free; Free = (FFreeBlock*)Free->NextFreeBlock)
				{
					check(Free->GetNumFreeRegularBlocks() > 0);
				}
			}
		);

		Table.ExhaustedPools.Validate(
			[](FMallocBinned2::FPoolInfo& Pool)
			{
				check(!Pool.FirstFreeBlock);
			}
		);
	}

	return true;
}

const TCHAR* FMallocBinned2::GetDescriptiveName()
{
	return TEXT("binned2");
}

void FMallocBinned2::FlushCurrentThreadCache()
{
	FScopeLock Lock(&Mutex);

	Private::FPerThreadFreeBlockLists& Lists = Private::FPerThreadFreeBlockLists::Get(PageSize);
	for (int32 PoolIndex = 0; PoolIndex != SMALL_POOL_COUNT; ++PoolIndex)
	{
		Private::FBundleNode* Bundles = Lists.PopBundles(PoolIndex);
		if (Bundles)
		{
			Private::FreeBundles(*this, Bundles, GMallocBinned2SmallBlockSizes[PoolIndex], PoolIndex);
		}
	}
}
