// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MallocBinned.cpp: Binned memory allocator
=============================================================================*/

#include "CorePrivatePCH.h"

#include "MallocBinned2.h"
#include "MemoryMisc.h"
#include "HAL/PlatformAtomics.h"

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
struct FMallocBinned2::FFreeMem
{
	/** Next or MemLastPool[], always in order by pool. */
	FFreeMem*	NextFreePool;
	/** Number of consecutive free blocks here, at least 1. */
	uint32		NumFreeBlocks;
};

// Memory pool info. 32 bytes.
struct FMallocBinned2::FPoolInfo
{
	/** Number of allocated elements in this pool, when counts down to zero can free the entire pool. */
	uint16			Taken;		// 2
	/** Index of pool. Index into MemSizeToPoolTable[]. Valid when < MAX_SMALL_POOLED_ALLOCATION_SIZE, -1 is OsTable.
		When AllocSize is 0, this is the number of pages to step back to find the base address of an allocation. See FindPoolInfo() */
	uint16			TableIndex; // 4
	/** Number of bytes allocated, or 0, this is a trailing pool (i.e. a pool used to find its way back to the base pool) */
	uint32			AllocSize;	// 8
	/** Pointer to first free memory in this pool or the OS Allocation Size in bytes if this allocation is not binned*/
	FFreeMem*		FirstMem;   // 12/16
	FPoolInfo*		Next;		// 16/24
	FPoolInfo**		PrevLink;	// 20/32
#if PLATFORM_32BITS
	/** Explicit padding for 32 bit builds */
	uint8 Padding[12]; // 32
#endif
	uint32 GetTrailingPoolOffset() const
	{
		checkSlow(IsTrailingPool());
		return TableIndex;
	}

	void SetTrailingPoolOffset(uint32 InOffset)
	{
		TableIndex = InOffset;
		AllocSize  = 0;
	}

	void SetPoolAllocationSizes(uint32 InBytes, uint32 InTableIndex)
	{
		// Shouldn't be pooling zero byte allocations
		checkSlow(InBytes != 0);

		TableIndex = InTableIndex;
		AllocSize  = InBytes;
	}

	void SetOSAllocationSizes(uint32 InBytes, UPTRINT InOsBytes)
	{
		// Shouldn't be pooling zero byte allocations
		checkSlow(InBytes != 0);

		TableIndex = (uint16)-1;
		AllocSize  = InBytes;
		FirstMem   = (FFreeMem*)InOsBytes;
	}

	UPTRINT GetOsBytes() const
	{
		check(IsOSAllocation());
		return (UPTRINT)FirstMem;
	}

	FORCEINLINE bool IsTrailingPool() const
	{
		return AllocSize == 0;
	}

	FORCEINLINE bool IsOSAllocation() const
	{
		return TableIndex == (uint16)-1;
	}

	void Link( FPoolInfo*& Before )
	{
		if( Before )
		{
			Before->PrevLink = &Next;
		}
		Next     = Before;
		PrevLink = &Before;
		Before   = this;
	}

	void Unlink()
	{
		if( Next )
		{
			Next->PrevLink = PrevLink;
		}
		*PrevLink = Next;
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
	static_assert(ARRAY_COUNT(GMallocBinned2SmallBlockSizes) == SMALL_POOL_COUNT, "Small block size array size must match SMALL_POOL_COUNT");
	static_assert(sizeof(FMallocBinned2::FPoolInfo)          == 32,               "sizeof(FPoolInfo) is expected to be 32 bytes");

	// Default alignment for binned allocator
	enum { DEFAULT_BINNED_ALLOCATOR_ALIGNMENT = 16 };

	// Implementation. 
	static CA_NO_RETURN void OutOfMemory(uint64 Size, uint32 Alignment=0)
	{
		// this is expected not to return
		FPlatformMemory::OnOutOfMemory(Size, Alignment);
	}

	/** 
	 * Creates an array of FPoolInfo structures for tracking allocations.
	 */
	static FPoolInfo* CreatePoolArray(uint64 NumPools)
	{
		uint64 PoolArraySize = NumPools * sizeof(FPoolInfo);

		void* Result = FPlatformMemory::BinnedAllocFromOS(PoolArraySize);
		if (!Result)
		{
			OutOfMemory(PoolArraySize);
		}

		DefaultConstructItems<FPoolInfo>(Result, NumPools);
		return (FPoolInfo*)Result;
	}

	/**
	 * Gets the FPoolInfo for a memory address. If no valid info exists one is created.
	 */
	static FPoolInfo* GetOrCreatePoolInfo(FMallocBinned2& Allocator, void* InPtr)
	{
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
			// Are we leaking memory here?
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

	static FPoolInfo* FindPoolInfo(FMallocBinned2& Allocator, void* InPtr, void*& AllocationBase)
	{
		const UPTRINT PageSize = (UPTRINT)Allocator.PageSize;

		uint8* Ptr = AlignDown((uint8*)InPtr, PageSize);
		for (uint32 Repeat = 2; Repeat; --Repeat)
		{
			UPTRINT BucketIndex;
			uint32  PoolIndex;
			Allocator.PtrToPoolMapping.GetHashBucketAndPoolIndices(Ptr, BucketIndex, PoolIndex);

			PoolHashBucket* FirstBucket = &Allocator.HashBuckets[BucketIndex];
			PoolHashBucket* Collision   = FirstBucket;
			for (;;)
			{
				if (Collision->BucketIndex == BucketIndex)
				{
					if (!Collision->FirstPool[PoolIndex].IsTrailingPool())
					{
						AllocationBase = Ptr;
						return &Collision->FirstPool[PoolIndex];
					}

					Ptr -= PageSize * (Collision->FirstPool[PoolIndex].GetTrailingPoolOffset());
					break;
				}

				Collision = Collision->Next;
				if (Collision == FirstBucket)
				{
					Ptr -= PageSize;
					break;
				}
			}
		}

		return nullptr;
	}

	static FORCEINLINE FPoolInfo* FindPoolInfo(FMallocBinned2& Allocator, void* Ptr)
	{
		void* BasePtr;
		return FindPoolInfo(Allocator, Ptr, BasePtr);
	}

	static void* AllocateBlockFromPool(FPoolTable& Table, FPoolInfo* Pool)
	{
		checkSlow(!Pool->IsOSAllocation());
		checkSlow(Pool->FirstMem);
		checkSlow(Pool->FirstMem->NumFreeBlocks > 0);

		// Pick first available block and unlink it.
		++Pool->Taken;
		--Pool->FirstMem->NumFreeBlocks;

		void* Free = (uint8*)Pool->FirstMem + Pool->FirstMem->NumFreeBlocks * Table.BlockSize;
		if (Pool->FirstMem->NumFreeBlocks == 0)
		{
			Pool->FirstMem = Pool->FirstMem->NextFreePool;
			if( !Pool->FirstMem )
			{
				// Move to exhausted list.
				Pool->Unlink();
				Pool->Link( Table.ExhaustedPool );
			}
		}

		return Free;
	}
};

FMallocBinned2::FMallocBinned2(uint32 InPageSize, uint64 AddressLimit)
	: PageSize          (InPageSize)
	, NumPoolsPerPage   (PageSize / sizeof(FPoolInfo))
	, PtrToPoolMapping  (PageSize, NumPoolsPerPage)
	, HashBucketFreeList(nullptr)
{
	check(FMath::IsPowerOfTwo(PageSize));
	check(FMath::IsPowerOfTwo(AddressLimit));
	check(AddressLimit > PageSize); // Check to catch 32 bit overflow in AddressLimit
	check(GMallocBinned2SmallBlockSizes[SMALL_POOL_COUNT - 1] == MAX_SMALL_POOLED_ALLOCATION_SIZE);

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
		while (Pool->BlockSize < Index)
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
	Size      = Align(Size, Alignment);

	checkf(FMath::IsPowerOfTwo(Alignment), TEXT("Alignment must be a power of two"));
	checkf(Alignment <= PageSize, TEXT("Alignment cannot exceed the system page size"));

	// Only allocate from the small pools if the size is small enough and the alignment isn't crazy large.
	// With large alignments, we'll waste a lot of memory allocating an entire page, but such alignments are highly unlikely in practice.
	if (Size < MAX_SMALL_POOLED_ALLOCATION_SIZE && Alignment <= Private::DEFAULT_BINNED_ALLOCATOR_ALIGNMENT)
	{
		// Allocate from small object pool.
		FPoolTable* Table = MemSizeToPoolTable[Size];
		FPoolInfo*  Pool  = Table->FirstPool;
		if (!Pool)
		{
			// Allocate memory.
			FFreeMem* Free = (FFreeMem*)CachedOSPageAllocator.Allocate(PageSize);
			if (!Free)
			{
				Private::OutOfMemory(PageSize);
			}

			// Create pool in the pool page.
			Pool = Private::GetOrCreatePoolInfo(*this, Free);

			// Init pool.
			Pool->Link(Table->FirstPool);
			Pool->SetPoolAllocationSizes(PageSize, Size);
			Pool->Taken    = 0;
			Pool->FirstMem = Free;

			// Create first free item.
			Free->NumFreeBlocks = PageSize / Table->BlockSize;
			Free->NextFreePool  = nullptr;
		}

		void* Result = Private::AllocateBlockFromPool(*Table, Pool);
		return Align(Result, Alignment);
	}

	// Use OS for non-pooled allocations.
	UPTRINT AlignedSize = Align(Size, PageSize);
	FFreeMem* Result = (FFreeMem*)CachedOSPageAllocator.Allocate(AlignedSize);
	if (!Result)
	{
		Private::OutOfMemory(AlignedSize);
	}

	// OS allocations should always be page-aligned.
	checkSlow(IsAligned(Result, PageSize));

	void* AlignedResult = Align(Result, Alignment);

	// Create pool.
	FPoolInfo* Pool = Private::GetOrCreatePoolInfo(*this, Result);
	if (Result != AlignedResult)
	{
		// Mark the FPoolInfo for AlignedResult to jump back to the FPoolInfo for ptr.
		for (UPTRINT i = (UPTRINT)PageSize, Offset = 1; i < AlignedSize; i += PageSize, ++Offset)
		{
			FPoolInfo* TrailingPool = Private::GetOrCreatePoolInfo(*this, (uint8*)Result + i);

			// Set trailing pools to point back to first pool
			TrailingPool->SetTrailingPoolOffset(Offset);
		}
	}

	Pool->SetOSAllocationSizes(Size, AlignedSize);

	return AlignedResult;
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

	FPoolInfo* Pool = Private::FindPoolInfo(*this, Ptr);
	if (!Pool->IsOSAllocation())
	{
		// Reallocate to a smaller/bigger pool if necessary
		FPoolTable* Table = MemSizeToPoolTable[Pool->TableIndex];
		if (NewSize > Table->BlockSize || (Pool->TableIndex != 0 && NewSize <= Table[-1].BlockSize) || !IsAligned(Ptr, Alignment))
		{
			// Reallocate and copy the data across
			void* Result = FMallocBinned2::Malloc(NewSize, Alignment);
			FMemory::Memcpy(Result, Ptr, FMath::Min<SIZE_T>(NewSize, Table->BlockSize));
			FMallocBinned2::Free(Ptr);
			return Result;
		}

		return Ptr;
	}

	// Allocated from OS.
	UPTRINT PoolOsBytes = Pool->GetOsBytes();
	if( NewSize > PoolOsBytes || NewSize * 3 < PoolOsBytes * 2 )
	{
		// Grow or shrink.
		void* Result = FMallocBinned2::Malloc(NewSize, Alignment);
		FMemory::Memcpy(Result, Ptr, FMath::Min<SIZE_T>(NewSize, Pool->AllocSize));
		FMallocBinned2::Free(Ptr);
		return Result;
	}

	Pool->SetOSAllocationSizes(NewSize, PoolOsBytes);

	return Ptr;
}

void FMallocBinned2::Free( void* Ptr )
{
	if (!Ptr)
	{
		return;
	}

	void* BasePtr;
	FPoolInfo* Pool = Private::FindPoolInfo(*this, Ptr, BasePtr);
#if PLATFORM_IOS || PLATFORM_MAC
	if (Pool == NULL)
	{
		UE_LOG(LogMemory, Warning, TEXT("Attempting to free a pointer we didn't allocate!"));
		return;
	}
#endif
	checkSlow(Pool);
	checkSlow(!Pool->IsTrailingPool());
	if (!Pool->IsOSAllocation())
	{
		FPoolTable* Table = MemSizeToPoolTable[Pool->TableIndex];

		// If this pool was exhausted, move to available list.
		if (!Pool->FirstMem)
		{
			Pool->Unlink();
			Pool->Link(Table->FirstPool);
		}

		uint32 AlignOffset = ((UPTRINT)Ptr - (UPTRINT)BasePtr) % Table->BlockSize;

		// Patch pointer to include previously applied alignment.
		Ptr = (uint8*)Ptr - AlignOffset;

		// Free a pooled allocation.
		FFreeMem* Free      = (FFreeMem*)Ptr;
		Free->NumFreeBlocks = 1;
		Free->NextFreePool  = Pool->FirstMem;
		Pool->FirstMem      = Free;

		// Free this pool.
		checkSlow(Pool->Taken >= 1);
		if( --Pool->Taken == 0 )
		{
			// Free the OS memory.
			Pool->Unlink();
			CachedOSPageAllocator.Free(BasePtr, PageSize);
		}
	}
	else
	{
		// Free an OS allocation.
		checkSlow(IsAligned(Ptr, PageSize));
		SIZE_T OsBytes = Pool->GetOsBytes();

		CachedOSPageAllocator.Free(BasePtr, OsBytes);
	}
}

bool FMallocBinned2::GetAllocationSize(void* Original, SIZE_T& SizeOut)
{
	if (!Original)
	{
		return false;
	}

	FPoolInfo* Pool = Private::FindPoolInfo(*this, Original);
	SizeOut = Pool->IsOSAllocation() ? Pool->AllocSize : MemSizeToPoolTable[Pool->TableIndex]->BlockSize;
	return true;
}

bool FMallocBinned2::ValidateHeap()
{
	for (FPoolTable& Table : SmallPoolTables)
	{
		for( FPoolInfo** PoolPtr = &Table.FirstPool; *PoolPtr; PoolPtr = &(*PoolPtr)->Next )
		{
			FPoolInfo* Pool = *PoolPtr;
			check(Pool->PrevLink == PoolPtr);
			check(Pool->FirstMem);
			for( FFreeMem* Free = Pool->FirstMem; Free; Free = Free->NextFreePool )
			{
				check(Free->NumFreeBlocks > 0);
			}
		}
		for( FPoolInfo** PoolPtr = &Table.ExhaustedPool; *PoolPtr; PoolPtr = &(*PoolPtr)->Next )
		{
			FPoolInfo* Pool = *PoolPtr;
			check(Pool->PrevLink == PoolPtr);
			check(!Pool->FirstMem);
		}
	}

	return true;
}

const TCHAR* FMallocBinned2::GetDescriptiveName()
{
	return TEXT("binned2");
}
