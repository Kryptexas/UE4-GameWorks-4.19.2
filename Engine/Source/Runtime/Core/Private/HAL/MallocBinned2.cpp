// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MallocBinned.cpp: Binned memory allocator
=============================================================================*/

#include "CorePrivatePCH.h"

#include "MallocBinned2.h"
#include "MemoryMisc.h"
#include "HAL/PlatformAtomics.h"

// Alignment of OS-allocated pointer - pool-allocated pointers will have a non-aligned pointer
#define BINNED2_LARGE_ALLOC 65536


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

/** Information about a piece of free memory. */
struct FMallocBinned2::FFreeBlock
{
	void*  NextFreeBlock; // Next free block in another pool
	uint32 NumFreeBlocks; // Number of consecutive free blocks here, at least 1.
	uint32 BlockSize;     // Size of the blocks that this list points to
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
 public:	uint16      Taken;          // Number of allocated elements in this pool, when counts down to zero can free the entire pool.
private:	uint32      AllocSize;      // Number of bytes allocated
 public:	FFreeBlock* FirstFreeBlock; // Pointer to first free memory in this pool or the OS Allocation Size in bytes if this allocation is not binned
 public:	FPoolInfo*  Next;           // Pointer to next pool
 public:	FPoolInfo** PtrToPrevNext;  // Pointer to whichever pointer points to this pool

public:
	bool IsExhausted() const
	{
		return !FirstFreeBlock || FirstFreeBlock->NumFreeBlocks == 0;
	}

	void* AllocateBlock(uint32 BlockSize)
	{
		check(!IsExhausted());
		++Taken;
		--FirstFreeBlock->NumFreeBlocks;
		void* Result = (uint8*)FirstFreeBlock + (FirstFreeBlock->NumFreeBlocks + IsAligned(FirstFreeBlock, BINNED2_LARGE_ALLOC)) * BlockSize;

		if (FirstFreeBlock->NumFreeBlocks == 0)
		{
			FirstFreeBlock = (FFreeBlock*)FirstFreeBlock->NextFreeBlock;
		}
		checkSlow(!FirstFreeBlock || FirstFreeBlock->NumFreeBlocks != 0);

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
	enum
	{
		DEFAULT_BINNED_ALLOCATOR_ALIGNMENT = 16, // Default alignment for binned allocator
	};

	FORCEINLINE static bool IsOSAllocation(const void* Ptr)
	{
		return IsAligned(Ptr, BINNED2_LARGE_ALLOC);
	}

	static_assert(ARRAY_COUNT(GMallocBinned2SmallBlockSizes) == SMALL_POOL_COUNT, "Small block size array size must match SMALL_POOL_COUNT");
	static_assert(sizeof(FFreeBlock) <= DEFAULT_BINNED_ALLOCATOR_ALIGNMENT,       "Free block struct must be small enough to fit into a block.");

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
};

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
	FFreeBlock* Free = (FFreeBlock*)Allocator.CachedOSPageAllocator.Allocate(LocalPageSize);
	if (!Free)
	{
		Private::OutOfMemory(LocalPageSize);
	}

	// Create pool
	FPoolInfo* Result = Private::GetOrCreatePoolInfo(Allocator, Free);
	Result->Link(Front);
	Result->Taken          = 0;
	Result->FirstFreeBlock = Free;

	// Create first free item.
	Free->NumFreeBlocks = LocalPageSize / InBlockSize - 1;
	Free->NextFreeBlock = nullptr;
	Free->BlockSize     = InBlockSize;

	return *Result;
}

FMallocBinned2::FMallocBinned2(uint32 InPageSize, uint64 AddressLimit)
	: PageSize          (InPageSize)
	, NumPoolsPerPage   (PageSize / sizeof(FPoolInfo))
	, PtrToPoolMapping  (PageSize, NumPoolsPerPage)
	, HashBucketFreeList(nullptr)
{
	checkf(FMath::IsPowerOfTwo(PageSize),                                                           TEXT("OS page size must be a power of two"));
	checkf(FMath::IsPowerOfTwo(AddressLimit),                                                       TEXT("OS address limit must be a power of two"));
	checkf(AddressLimit > PageSize,                                                                 TEXT("OS address limit must be greater than the page size")); // Check to catch 32 bit overflow in AddressLimit
	checkf(GMallocBinned2SmallBlockSizes[SMALL_POOL_COUNT - 1] == MAX_SMALL_POOLED_ALLOCATION_SIZE, TEXT("MAX_SMALL_POOLED_ALLOCATION_SIZE must equal the smallest block size"));
	checkf(PageSize % BINNED2_LARGE_ALLOC == 0,                                                     TEXT("OS page size must be a multiple of BINNED2_LARGE_ALLOC"));
	checkf(sizeof(FMallocBinned2::FFreeBlock) <= GMallocBinned2SmallBlockSizes[0],                  TEXT("Pool header must be able to fit into the smallest block"));

	// Init pool tables.
	for (uint32 Index = 0; Index != SMALL_POOL_COUNT; ++Index)
	{
		checkf(Index == 0 || GMallocBinned2SmallBlockSizes[Index - 1] < GMallocBinned2SmallBlockSizes[Index], TEXT("Small block sizes must be strictly increasing"));
		checkf(GMallocBinned2SmallBlockSizes[Index] <= PageSize,                                              TEXT("Small block size must be small enough to fit into a page"));
		checkf(GMallocBinned2SmallBlockSizes[Index] % Private::DEFAULT_BINNED_ALLOCATOR_ALIGNMENT == 0,       TEXT("Small block size must be a multiple of DEFAULT_BINNED_ALLOCATOR_ALIGNMENT"));

		SmallPoolTables[Index].BlockSize = GMallocBinned2SmallBlockSizes[Index];
	}

	// Set up pool mappings
	FPoolTable** TableEntry = MemSizeToPoolTable;
	FPoolTable*  Pool       = SmallPoolTables;
	for (uint32 Index = 0; Index != MAX_SMALL_POOLED_ALLOCATION_SIZE; ++Index)
	{
		while (Pool->BlockSize - 1 < Index)
		{
			++Pool;
			check(Pool != SmallPoolTables + SMALL_POOL_COUNT);
		}
		*TableEntry++ = Pool;
	}

	uint64 MaxHashBuckets = PtrToPoolMapping.GetMaxHashBuckets(AddressLimit);

	HashBuckets = (PoolHashBucket*)FPlatformMemory::BinnedAllocFromOS(Align(MaxHashBuckets * sizeof(PoolHashBucket), PageSize));
	DefaultConstructItems<PoolHashBucket>(HashBuckets, MaxHashBuckets);
}

FMallocBinned2::~FMallocBinned2()
{
}

bool FMallocBinned2::IsInternallyThreadSafe() const
{ 
	return false;
}

void* FMallocBinned2::Malloc(SIZE_T Size, uint32 Alignment)
{
	Alignment = FMath::Max<uint32>(Alignment, Private::DEFAULT_BINNED_ALLOCATOR_ALIGNMENT);
	Size      = Align(FMath::Max((SIZE_T)1, Size), Alignment);

	checkf(FMath::IsPowerOfTwo(Alignment), TEXT("Alignment must be a power of two"));
	checkf(Alignment <= PageSize, TEXT("Alignment cannot exceed the system page size"));

	// Only allocate from the small pools if the size is small enough and the alignment isn't crazy large.
	// With large alignments, we'll waste a lot of memory allocating an entire page, but such alignments are highly unlikely in practice.
	if (Size <= MAX_SMALL_POOLED_ALLOCATION_SIZE && Alignment <= Private::DEFAULT_BINNED_ALLOCATOR_ALIGNMENT)
	{
		// Allocate from small object pool.
		FPoolTable& Table = *MemSizeToPoolTable[Size - 1];

		FPoolInfo* Pool;
		if (!Table.ActivePools.IsEmpty())
		{
			Pool = &Table.ActivePools.GetFrontPool();
		}
		else
		{
			Pool = &Table.ActivePools.PushNewPoolToFront(*this, Table.BlockSize);
		}

		void* Result = Pool->AllocateBlock(Table.BlockSize);
		if (Pool->IsExhausted())
		{
			Table.ExhaustedPools.LinkToFront(Pool);
		}

		return Result;
	}

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

	Alignment = FMath::Max<uint32>(Alignment, Private::DEFAULT_BINNED_ALLOCATOR_ALIGNMENT);
	NewSize   = Align(NewSize, Alignment);

	checkf(FMath::IsPowerOfTwo(Alignment), TEXT("Alignment must be a power of two"));
	checkf(Alignment <= PageSize, TEXT("Alignment cannot exceed the system page size"));

	if (!Private::IsOSAllocation(Ptr))
	{
		// Reallocate to a smaller/bigger pool if necessary
		FFreeBlock* Free  = Private::GetPoolHeaderFromPointer(Ptr);
		FPoolTable& Table = *MemSizeToPoolTable[Free->BlockSize - 1];
		if (NewSize <= Table.BlockSize && (&Table == &SmallPoolTables[0] || NewSize > (&Table)[-1].BlockSize) && IsAligned(Ptr, Alignment))
		{
			return Ptr;
		}

		// Reallocate and copy the data across
		void* Result = FMallocBinned2::Malloc(NewSize, Alignment);
		FMemory::Memcpy(Result, Ptr, FMath::Min<SIZE_T>(NewSize, Table.BlockSize));
		FMallocBinned2::Free(Ptr);
		return Result;
	}

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
	if (Pool == NULL)
	{
		UE_LOG(LogMemory, Warning, TEXT("Attempting to free a pointer we didn't allocate!"));
		return;
	}
#endif

	checkSlow(Pool);
	if (!Private::IsOSAllocation(Ptr))
	{
		FFreeBlock* BasePtr = Private::GetPoolHeaderFromPointer(Ptr);
		FPoolTable& Table   = *MemSizeToPoolTable[BasePtr->BlockSize - 1];

		// If this pool was exhausted, move to available list.
		if (!Pool->FirstFreeBlock)
		{
			Table.ActivePools.LinkToFront(Pool);
		}

		// Free a pooled allocation.
		FFreeBlock* Free     = (FFreeBlock*)Ptr;
		Free->NumFreeBlocks  = 1;
		Free->NextFreeBlock  = Pool->FirstFreeBlock;
		Free->BlockSize      = Table.BlockSize;
		Pool->FirstFreeBlock = Free;

		// Free this pool.
		checkSlow(Pool->Taken >= 1);
		if (--Pool->Taken == 0)
		{
			// Free the OS memory.
			Pool->Unlink();
			CachedOSPageAllocator.Free(BasePtr, PageSize);
		}
	}
	else
	{
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
		FFreeBlock* Free = Private::GetPoolHeaderFromPointer(Ptr);
		SizeOut = MemSizeToPoolTable[Free->BlockSize - 1]->BlockSize;
		return true;
	}

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
	for (FPoolTable& Table : SmallPoolTables)
	{
		Table.ActivePools.Validate(
			[](FMallocBinned2::FPoolInfo& Pool)
			{
				check(Pool.FirstFreeBlock);
				for (FFreeBlock* Free = Pool.FirstFreeBlock; Free; Free = (FFreeBlock*)Free->NextFreeBlock)
				{
					check(Free->NumFreeBlocks > 0);
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
