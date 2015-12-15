// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MallocBinned.cpp: Binned memory allocator
=============================================================================*/

#include "CorePrivatePCH.h"

#include "MallocBinned2.h"
#include "MemoryMisc.h"
#include "HAL/PlatformAtomics.h"

//when modifying the global allocator stats, if we are using COARSE locks, then all callsites for stat modification are covered by the allocator-wide access guard. Thus the stats can be modified directly.
//If we are using FINE locks, then we must modify the stats through atomics as the locks are either not actually covering the stat callsites, or are locking specific table locks which is not sufficient for stats.
#if STATS
#	define BINNED2_INCREMENT_STATCOUNTER(counter) (FPlatformAtomics::InterlockedIncrement(&(counter)))
#	define BINNED2_DECREMENT_STATCOUNTER(counter) (FPlatformAtomics::InterlockedDecrement(&(counter)))
#	define BINNED2_ADD_STATCOUNTER(counter, value) (FPlatformAtomics::InterlockedAdd(&counter, (value)))
#	define BINNED2_PEAK_STATCOUNTER(PeakCounter, CompareVal)	{																												\
																volatile TDecay<decltype(PeakCounter)>::Type NewCompare;													\
																volatile TDecay<decltype(PeakCounter)>::Type NewPeak;														\
																do																											\
																{																											\
																	NewCompare = (PeakCounter);																				\
																	NewPeak = FMath::Max((PeakCounter), (CompareVal));														\
																}																											\
																while (FPlatformAtomics::InterlockedCompareExchange(&(PeakCounter), NewPeak, NewCompare) != NewCompare);	\
															}
#else
#	define BINNED2_INCREMENT_STATCOUNTER(counter)
#	define BINNED2_DECREMENT_STATCOUNTER(counter)
#	define BINNED2_ADD_STATCOUNTER(counter, value)
#	define BINNED2_PEAK_STATCOUNTER(PeakCounter, CompareVal)
#endif

/** Malloc binned allocator specific stats. */
DECLARE_MEMORY_STAT_EXTERN(TEXT("Binned Os Current"),		STAT_Binned2_OsCurrent,STATGROUP_MemoryAllocator, CORE_API);
DECLARE_MEMORY_STAT_EXTERN(TEXT("Binned Os Peak"),			STAT_Binned2_OsPeak,STATGROUP_MemoryAllocator, CORE_API);
DECLARE_MEMORY_STAT_EXTERN(TEXT("Binned Waste Current"),	STAT_Binned2_WasteCurrent,STATGROUP_MemoryAllocator, CORE_API);
DECLARE_MEMORY_STAT_EXTERN(TEXT("Binned Waste Peak"),		STAT_Binned2_WastePeak,STATGROUP_MemoryAllocator, CORE_API);
DECLARE_MEMORY_STAT_EXTERN(TEXT("Binned Used Current"),		STAT_Binned2_UsedCurrent,STATGROUP_MemoryAllocator, CORE_API);
DECLARE_MEMORY_STAT_EXTERN(TEXT("Binned Used Peak"),		STAT_Binned2_UsedPeak,STATGROUP_MemoryAllocator, CORE_API);
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Binned Current Allocs"),	STAT_Binned2_CurrentAllocs,STATGROUP_MemoryAllocator, CORE_API);
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Binned Total Allocs"),		STAT_Binned2_TotalAllocs,STATGROUP_MemoryAllocator, CORE_API);
DECLARE_MEMORY_STAT_EXTERN(TEXT("Binned Slack Current"),	STAT_Binned2_SlackCurrent,STATGROUP_MemoryAllocator, CORE_API);

/** Malloc binned allocator specific stats. */
DEFINE_STAT(STAT_Binned2_OsCurrent);
DEFINE_STAT(STAT_Binned2_OsPeak);
DEFINE_STAT(STAT_Binned2_WasteCurrent);
DEFINE_STAT(STAT_Binned2_WastePeak);
DEFINE_STAT(STAT_Binned2_UsedCurrent);
DEFINE_STAT(STAT_Binned2_UsedPeak);
DEFINE_STAT(STAT_Binned2_CurrentAllocs);
DEFINE_STAT(STAT_Binned2_TotalAllocs);
DEFINE_STAT(STAT_Binned2_SlackCurrent);

// Block sizes are based around getting the maximum amount of allocations per pool, with as little alignment waste as possible.
// Block sizes should be close to even divisors of the POOL_SIZE, and well distributed. They must be 16-byte aligned as well.
const uint32 GMallocBinned2BlockSizes[] =
{
	8,		16,		32,		48,		64,		80,		96,		112,
	128,	160,	192,	224,	256,	288,	320,	384,
	448,	512,	576,	640,	704,	768,	896,	1024,
	1168,	1360,	1632,	2048,	2336,	2720,	3264,	4096,
	4672,	5456,	6544,	8192,	9360,	10912,	13104,	16384,
	21840,	32768
};

/** Information about a piece of free memory. 8 bytes */
struct FMallocBinned2::FFreeMem
{
	/** Next or MemLastPool[], always in order by pool. */
	FFreeMem*	Next;
	/** Number of consecutive free blocks here, at least 1. */
	uint32		NumFreeBlocks;
};

// Memory pool info. 32 bytes.
struct FMallocBinned2::FPoolInfo
{
	/** Number of allocated elements in this pool, when counts down to zero can free the entire pool. */
	uint16			Taken;		// 2
	/** Index of pool. Index into MemSizeToPoolTable[]. Valid when < MAX_POOLED_ALLOCATION_SIZE, MAX_POOLED_ALLOCATION_SIZE is OsTable.
		When AllocSize is 0, this is the number of pages to step back to find the base address of an allocation. See FindPoolInfo()
	*/
	uint16			TableIndex; // 4		
	/** Number of bytes allocated */
	uint32			AllocSize;	// 8
	/** Pointer to first free memory in this pool or the OS Allocation Size in bytes if this allocation is not binned*/
	FFreeMem*		FirstMem;   // 12/16
	FPoolInfo*		Next;		// 16/24
	FPoolInfo**		PrevLink;	// 20/32
#if PLATFORM_32BITS
	/** Explicit padding for 32 bit builds */
	uint8 Padding[12]; // 32
#endif

	void SetAllocationSizes( uint32 InBytes, UPTRINT InOsBytes, uint32 InTableIndex, uint32 SmallAllocLimt )
	{
		TableIndex=InTableIndex;
		AllocSize=InBytes;
		if (TableIndex == SmallAllocLimt)
		{
			FirstMem=(FFreeMem*)InOsBytes;
		}
	}

	UPTRINT GetOsBytes( uint32 InPageSize, uint32 SmallAllocLimt ) const
	{
		if (TableIndex == SmallAllocLimt)
		{
			return (UPTRINT)FirstMem;
		}
		else
		{
			return Align(AllocSize, InPageSize);
		}
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

/** Hash table struct for retrieving allocation book keeping information */
struct FMallocBinned2::PoolHashBucket
{
	UPTRINT         Key;
	FPoolInfo*      FirstPool;
	PoolHashBucket* Prev;
	PoolHashBucket* Next;

	PoolHashBucket()
	{
		Key      = 0;
		FirstPool= nullptr;
		Prev     = this;
		Next     = this;
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
	static_assert(ARRAY_COUNT(GMallocBinned2BlockSizes) == POOL_COUNT, "Block size array size must match POOL_COUNT");
	static_assert(sizeof(FMallocBinned2::FPoolInfo)     == 32,         "sizeof(FPoolInfo) is expected to be 32 bytes");

	// Default alignment for binned allocator
	enum { DEFAULT_BINNED_ALLOCATOR_ALIGNMENT = sizeof(FFreeMem) };

	// There is internal limit on page size of 64k
	enum { PAGE_SIZE_LIMIT = 65536 };

	// BINNED_ALLOC_POOL_SIZE can be increased beyond 64k to cause binned malloc to allocate
	// the small size bins in bigger chunks. If OS Allocation is slow, increasing
	// this number *may* help performance but YMMV.
	enum { BINNED_ALLOC_POOL_SIZE = 65536 }; 

	// Implementation. 
	static CA_NO_RETURN void OutOfMemory(uint64 Size, uint32 Alignment=0)
	{
		// this is expected not to return
		FPlatformMemory::OnOutOfMemory(Size, Alignment);
	}

	static FORCEINLINE void TrackStats(FPoolTable* Table, SIZE_T Size)
	{
#if STATS
		// keep track of memory lost to padding
		Table->TotalWaste += Table->BlockSize - Size;
		Table->TotalRequests++;
		Table->ActiveRequests++;
		Table->MaxActiveRequests = FMath::Max(Table->MaxActiveRequests, Table->ActiveRequests);
		Table->MaxRequest = Size > Table->MaxRequest ? Size : Table->MaxRequest;
		Table->MinRequest = Size < Table->MinRequest ? Size : Table->MinRequest;
#endif
	}

	/** 
	 * Create a 64k page of FPoolInfo structures for tracking allocations
	 */
	static FPoolInfo* CreateIndirect(FMallocBinned2& Allocator)
	{
		uint64 IndirectPoolBlockSizeBytes = Allocator.IndirectPoolBlockSize * sizeof(FPoolInfo);

		checkSlow(IndirectPoolBlockSizeBytes <= Allocator.PageSize);
		FPoolInfo* Indirect = (FPoolInfo*)FPlatformMemory::BinnedAllocFromOS(IndirectPoolBlockSizeBytes);
		if( !Indirect )
		{
			OutOfMemory(IndirectPoolBlockSizeBytes);
		}
		FMemory::Memset(Indirect, 0, IndirectPoolBlockSizeBytes);

		BINNED2_PEAK_STATCOUNTER(Allocator.Stats.OsPeak,    BINNED2_ADD_STATCOUNTER(Allocator.Stats.OsCurrent,    (int64)(Align(IndirectPoolBlockSizeBytes, Allocator.PageSize))));
		BINNED2_PEAK_STATCOUNTER(Allocator.Stats.WastePeak, BINNED2_ADD_STATCOUNTER(Allocator.Stats.WasteCurrent, (int64)(Align(IndirectPoolBlockSizeBytes, Allocator.PageSize))));

		return Indirect;
	}

	/** 
	 * Gets the FPoolInfo for a memory address. If no valid info exists one is created. 
	 * NOTE: This function requires a mutex across threads, but it's the caller's responsibility to 
	 * acquire the mutex before calling
	 */
	static /*FORCEINLINE*/ FPoolInfo* GetPoolInfo(FMallocBinned2& Allocator, void* InPtr)
	{
		UPTRINT Ptr = (UPTRINT)InPtr;

		UPTRINT Key       = Ptr >> Allocator.HashKeyShift;
		UPTRINT Hash      = Key & (Allocator.MaxHashBuckets - 1);
		UPTRINT PoolIndex = (Ptr >> Allocator.PoolBitShift) & Allocator.PoolMask;

		PoolHashBucket* Collision = &Allocator.HashBuckets[Hash];
		do
		{
			if (!Collision->FirstPool)
			{
				Collision->Key       = Key;
				Collision->FirstPool = CreateIndirect(Allocator);

				return &Collision->FirstPool[PoolIndex];
			}

			if (Collision->Key == Key)
			{
				return &Collision->FirstPool[PoolIndex];
			}

			Collision = Collision->Next;
		} while (Collision != &Allocator.HashBuckets[Hash]);

		// Create a new hash bucket entry
		if (!Allocator.HashBucketFreeList) 
		{
			const uint32 PageSize = Allocator.PageSize;

			Allocator.HashBucketFreeList = (PoolHashBucket*)FPlatformMemory::BinnedAllocFromOS(PageSize);

			BINNED2_PEAK_STATCOUNTER(Allocator.Stats.OsPeak,    BINNED2_ADD_STATCOUNTER(Allocator.Stats.OsCurrent,    PageSize));
			BINNED2_PEAK_STATCOUNTER(Allocator.Stats.WastePeak, BINNED2_ADD_STATCOUNTER(Allocator.Stats.WasteCurrent, PageSize));
			
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
			NewBucket->FirstPool = CreateIndirect(Allocator);
		}

		NewBucket->Key = Key;

		Allocator.HashBuckets[Hash].Link(NewBucket);

		return &NewBucket->FirstPool[PoolIndex];
	}

	static FPoolInfo* FindPoolInfo(FMallocBinned2& Allocator, void* InPtr, void*& AllocationBase)
	{
		const UPTRINT PageSize = (UPTRINT)Allocator.PageSize;

		UPTRINT Ptr = (UPTRINT)InPtr &~ (PageSize - 1);
		for (uint32 Repeat = (BINNED_ALLOC_POOL_SIZE / PageSize) + 1; Repeat; --Repeat)
		{
			uint32 Key       = Ptr >> Allocator.HashKeyShift;
			uint32 Hash      = Key & (Allocator.MaxHashBuckets - 1);
			uint32 PoolIndex = (Ptr >> Allocator.PoolBitShift) & Allocator.PoolMask;

			PoolHashBucket* Collision = &Allocator.HashBuckets[Hash];
			for (;;)
			{
				if (Collision->Key == Key)
				{
					if (Collision->FirstPool[PoolIndex].AllocSize)
					{
						AllocationBase = (void*)Ptr;
						return &Collision->FirstPool[PoolIndex];
					}

					Ptr = ((Ptr - (PageSize * Collision->FirstPool[PoolIndex].TableIndex)) - 1) &~ (PageSize - 1);
					break;
				}

				Collision = Collision->Next;
				if (Collision == &Allocator.HashBuckets[Hash])
				{
					Ptr = (Ptr - 1) &~ (PageSize - 1);
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

	static FPoolInfo* AllocatePoolMemory(FMallocBinned2& Allocator, FPoolTable& Table, uint32 PoolSize, uint16 TableIndex)
	{
		const uint32 PageSize  = Allocator.PageSize;
		const uint32 BlockSize = Table.BlockSize;

		// Must create a new pool.
		uint32 Blocks   = PoolSize / BlockSize;
		uint32 Bytes    = Blocks * BlockSize;
		UPTRINT OsBytes = Align(Bytes, PageSize);

		checkSlow(Blocks >= 1);
		checkSlow(Blocks * BlockSize <= Bytes && PoolSize >= Bytes);

		// Allocate memory.
		FFreeMem* Free = (FFreeMem*)Allocator.CachedOSPageAllocator.Allocate(OsBytes);
		if( !Free )
		{
			OutOfMemory(OsBytes);
		}

		checkSlow(IsAligned(Free, PageSize));

		// Create pool in the indirect table.
		FPoolInfo* Pool = GetPoolInfo(Allocator, Free);
		for (UPTRINT i = (UPTRINT)PageSize, Offset = 0; i < OsBytes; i += PageSize, ++Offset)
		{
			FPoolInfo* TrailingPool = GetPoolInfo(Allocator, (void*)(((UPTRINT)Free) + i));
			check(TrailingPool);

			//Set trailing pools to point back to first pool
			TrailingPool->SetAllocationSizes(0, 0, Offset, Allocator.BinnedOSTableIndex);
		}

		BINNED2_PEAK_STATCOUNTER(Allocator.Stats.OsPeak,    BINNED2_ADD_STATCOUNTER(Allocator.Stats.OsCurrent,    OsBytes));
		BINNED2_PEAK_STATCOUNTER(Allocator.Stats.WastePeak, BINNED2_ADD_STATCOUNTER(Allocator.Stats.WasteCurrent, (OsBytes - Bytes)));

		// Init pool.
		Pool->Link( Table.FirstPool );
		Pool->SetAllocationSizes(Bytes, OsBytes, TableIndex, Allocator.BinnedOSTableIndex);
		Pool->Taken		 = 0;
		Pool->FirstMem   = Free;

#if STATS
		Table.NumActivePools++;
		Table.MaxActivePools = FMath::Max(Table.MaxActivePools, Table.NumActivePools);
#endif
		// Create first free item.
		Free->NumFreeBlocks = Blocks;
		Free->Next          = nullptr;

		return Pool;
	}

	static /*FORCEINLINE*/ FFreeMem* AllocateBlockFromPool(FMallocBinned2& Allocator, FPoolTable& Table, FPoolInfo* Pool, uint32 Alignment)
	{
		// Pick first available block and unlink it.
		Pool->Taken++;
		checkSlow(Pool->TableIndex < Allocator.BinnedOSTableIndex); // if this is false, FirstMem is actually a size not a pointer
		checkSlow(Pool->FirstMem);
		checkSlow(Pool->FirstMem->NumFreeBlocks > 0);
		checkSlow(Pool->FirstMem->NumFreeBlocks < PAGE_SIZE_LIMIT);
		FFreeMem* Free = (FFreeMem*)((uint8*)Pool->FirstMem + --Pool->FirstMem->NumFreeBlocks * Table.BlockSize);
		if( !Pool->FirstMem->NumFreeBlocks )
		{
			Pool->FirstMem = Pool->FirstMem->Next;
			if( !Pool->FirstMem )
			{
				// Move to exhausted list.
				Pool->Unlink();
				Pool->Link( Table.ExhaustedPool );
			}
		}
		BINNED2_PEAK_STATCOUNTER(Allocator.Stats.UsedPeak, BINNED2_ADD_STATCOUNTER(Allocator.Stats.UsedCurrent, Table.BlockSize));
		return Align(Free, Alignment);
	}

#if	STATS
	static void UpdateSlackStat(FMallocBinned2& Allocator)
	{
		SIZE_T LocalWaste = Allocator.Stats.WasteCurrent;
		double Waste      = 0.0;
		for (FPoolTable& Table : Allocator.PoolTable)
		{
			Waste += ( ( double )Table.TotalWaste / ( double )Table.TotalRequests ) * ( double )Table.ActiveRequests;
			Waste += Table.NumActivePools * ( BINNED_ALLOC_POOL_SIZE - ( ( BINNED_ALLOC_POOL_SIZE / Table.BlockSize ) * Table.BlockSize ) );
		}
		LocalWaste += ( uint32 )Waste;
		Allocator.Stats.SlackCurrent = Allocator.Stats.OsCurrent - LocalWaste - Allocator.Stats.UsedCurrent;
	}
#endif // STATS
};

void FMallocBinned2::GetAllocatorStats( FGenericMemoryStats& out_Stats )
{
	FMalloc::GetAllocatorStats( out_Stats );

#if	STATS
	Private::UpdateSlackStat(*this);

	// Malloc binned stats.
	out_Stats.Add( GET_STATDESCRIPTION( STAT_Binned2_OsCurrent ),     Stats.OsCurrent );
	out_Stats.Add( GET_STATDESCRIPTION( STAT_Binned2_OsPeak ),        Stats.OsPeak );
	out_Stats.Add( GET_STATDESCRIPTION( STAT_Binned2_WasteCurrent ),  Stats.WasteCurrent );
	out_Stats.Add( GET_STATDESCRIPTION( STAT_Binned2_WastePeak ),     Stats.WastePeak );
	out_Stats.Add( GET_STATDESCRIPTION( STAT_Binned2_UsedCurrent ),   Stats.UsedCurrent );
	out_Stats.Add( GET_STATDESCRIPTION( STAT_Binned2_UsedPeak ),      Stats.UsedPeak );
	out_Stats.Add( GET_STATDESCRIPTION( STAT_Binned2_CurrentAllocs ), Stats.CurrentAllocs );
	out_Stats.Add( GET_STATDESCRIPTION( STAT_Binned2_TotalAllocs ),   Stats.TotalAllocs );
	out_Stats.Add( GET_STATDESCRIPTION( STAT_Binned2_SlackCurrent ),  Stats.SlackCurrent );
#endif // STATS
}

void FMallocBinned2::InitializeStatsMetadata()
{
	FMalloc::InitializeStatsMetadata();

	// Initialize stats metadata here instead of UpdateStats.
	// Mostly to avoid dead-lock when stats malloc profiler is enabled.
	GET_STATFNAME(STAT_Binned2_OsCurrent);
	GET_STATFNAME(STAT_Binned2_OsPeak);
	GET_STATFNAME(STAT_Binned2_WasteCurrent);
	GET_STATFNAME(STAT_Binned2_WastePeak);
	GET_STATFNAME(STAT_Binned2_UsedCurrent);
	GET_STATFNAME(STAT_Binned2_UsedPeak);
	GET_STATFNAME(STAT_Binned2_CurrentAllocs);
	GET_STATFNAME(STAT_Binned2_TotalAllocs);
	GET_STATFNAME(STAT_Binned2_SlackCurrent);
}

FMallocBinned2::FMallocBinned2(uint32 InPageSize, uint64 AddressLimit)
	: PageSize             (InPageSize)
	, PoolBitShift         (FPlatformMath::CeilLogTwo(InPageSize))
	, IndirectPoolBlockSize(InPageSize / sizeof(FPoolInfo))
	, IndirectPoolBitShift (FPlatformMath::CeilLogTwo(IndirectPoolBlockSize))
	, MaxHashBuckets       (AddressLimit >> (IndirectPoolBitShift + PoolBitShift))
	, HashKeyShift         (PoolBitShift + IndirectPoolBitShift)
	, PoolMask             ((1ull << (HashKeyShift - PoolBitShift)) - 1)
	, BinnedSizeLimit      (Private::PAGE_SIZE_LIMIT / 2)
	, BinnedOSTableIndex   (BinnedSizeLimit + EXTENDED_PAGE_POOL_ALLOCATION_COUNT)
	, HashBucketFreeList   (nullptr)
{
	check(FMath::IsPowerOfTwo(PageSize));
	check(FMath::IsPowerOfTwo(AddressLimit));
	check(FMath::IsPowerOfTwo(BinnedSizeLimit));
	check(PageSize <= Private::PAGE_SIZE_LIMIT);
	check(AddressLimit > PageSize); // Check to catch 32 bit overflow in AddressLimit

	// Init tables.

	/** The following options are not valid for page sizes less than 64k. They are here to reduce waste */
	PagePoolTable[0].BlockSize = (PageSize == Private::PAGE_SIZE_LIMIT) ? BinnedSizeLimit + (BinnedSizeLimit / 2) : 0;
	PagePoolTable[1].BlockSize = (PageSize == Private::PAGE_SIZE_LIMIT) ? PageSize        +  BinnedSizeLimit      : 0;

	for (uint32 Index = 0; Index != POOL_COUNT; ++Index)
	{
		PoolTable[Index].BlockSize  = GMallocBinned2BlockSizes[Index];
#if STATS
		PoolTable[Index].MinRequest = GMallocBinned2BlockSizes[Index];
#endif
	}

	for (uint32 Index = 0; Index != MAX_POOLED_ALLOCATION_SIZE; ++Index)
	{
		FPoolTable* Pool = PoolTable;
		while (Pool->BlockSize < Index)
		{
			++Pool;
			checkSlow(Pool != PoolTable + POOL_COUNT);
		}
		MemSizeToPoolTable[Index] = Pool;
	}

	MemSizeToPoolTable[BinnedSizeLimit    ] = &PagePoolTable[0];
	MemSizeToPoolTable[BinnedSizeLimit + 1] = &PagePoolTable[1];

	HashBuckets = (PoolHashBucket*)FPlatformMemory::BinnedAllocFromOS(Align(MaxHashBuckets * sizeof(PoolHashBucket), PageSize));
	for (uint32 i = 0; i < MaxHashBuckets; ++i) 
	{
		new (HashBuckets + i) PoolHashBucket();
	}

	check(MAX_POOLED_ALLOCATION_SIZE - 1 == PoolTable[POOL_COUNT - 1].BlockSize);
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
	// Handle DEFAULT_ALIGNMENT for binned allocator.
	if (Alignment == DEFAULT_ALIGNMENT)
	{
		Alignment = Private::DEFAULT_BINNED_ALLOCATOR_ALIGNMENT;
	}

	Alignment = FMath::Max<uint32>(Alignment, Private::DEFAULT_BINNED_ALLOCATOR_ALIGNMENT);
	SIZE_T SpareBytesCount = FMath::Min<SIZE_T>(Private::DEFAULT_BINNED_ALLOCATOR_ALIGNMENT, Size);
	Size = FMath::Max<SIZE_T>(PoolTable[0].BlockSize, Size + (Alignment - SpareBytesCount));

	BINNED2_INCREMENT_STATCOUNTER(Stats.CurrentAllocs);
	BINNED2_INCREMENT_STATCOUNTER(Stats.TotalAllocs);

	if (Size < BinnedSizeLimit)
	{
		// Allocate from pool.
		FPoolTable* Table = MemSizeToPoolTable[Size];

		checkSlow(Size <= Table->BlockSize);

		Private::TrackStats(Table, Size);

		FPoolInfo* Pool = Table->FirstPool;
		if (!Pool)
		{
			Pool = Private::AllocatePoolMemory(*this, *Table, Private::BINNED_ALLOC_POOL_SIZE/*PageSize*/, Size);
		}

		FFreeMem* Result = Private::AllocateBlockFromPool(*this, *Table, Pool, Alignment);
		return Result;
	}

	if (Size <= PagePoolTable[0].BlockSize || (Size > PageSize && Size <= PagePoolTable[1].BlockSize))
	{
		// Bucket in a pool of 3*PageSize or 6*PageSize
		uint32      BinType   = Size < PageSize ? 0 : 1;
		uint32      PageCount = 3 * BinType + 3;
		FPoolTable* Table     = &PagePoolTable[BinType];

		checkSlow(Size <= Table->BlockSize);

		Private::TrackStats(Table, Size);

		FPoolInfo* Pool = Table->FirstPool;
		if (!Pool)
		{
			Pool = Private::AllocatePoolMemory(*this, *Table, PageCount*PageSize, BinnedSizeLimit+BinType);
		}

		FFreeMem* Result = Private::AllocateBlockFromPool(*this, *Table, Pool, Alignment);
		return Result;
	}

	// Use OS for large allocations.
	UPTRINT AlignedSize = Align(Size, PageSize);
	FFreeMem* Result = (FFreeMem*)CachedOSPageAllocator.Allocate(AlignedSize);
	if (!Result)
	{
		Private::OutOfMemory(AlignedSize);
	}

	// OS allocations should always be page-aligned.
	checkSlow(IsAligned(Result, PageSize));

	void* AlignedResult = Align(Result, Alignment);

	// Create indirect.
	FPoolInfo* Pool = Private::GetPoolInfo(*this, Result);
	if (Result != AlignedResult)
	{
		// Mark the FPoolInfo for AlignedResult to jump back to the FPoolInfo for ptr.
		for (UPTRINT i = (UPTRINT)PageSize, Offset = 0; i < AlignedSize; i += PageSize, ++Offset)
		{
			FPoolInfo* TrailingPool = Private::GetPoolInfo(*this, (void*)((UPTRINT)Result + i));
			check(TrailingPool);

			//Set trailing pools to point back to first pool
			TrailingPool->SetAllocationSizes(0, 0, Offset, BinnedOSTableIndex);
		}
	}

	Pool->SetAllocationSizes(Size, AlignedSize, BinnedOSTableIndex, BinnedOSTableIndex);

	BINNED2_PEAK_STATCOUNTER(Stats.OsPeak,    BINNED2_ADD_STATCOUNTER(Stats.OsCurrent,    AlignedSize));
	BINNED2_PEAK_STATCOUNTER(Stats.UsedPeak,  BINNED2_ADD_STATCOUNTER(Stats.UsedCurrent,  Size));
	BINNED2_PEAK_STATCOUNTER(Stats.WastePeak, BINNED2_ADD_STATCOUNTER(Stats.WasteCurrent, (int64)(AlignedSize - Size)));

	return AlignedResult;
}

void* FMallocBinned2::Realloc(void* Ptr, SIZE_T NewSize, uint32 Alignment)
{
	// Handle DEFAULT_ALIGNMENT for binned allocator.
	Alignment = FMath::Max<uint32>(Alignment, Private::DEFAULT_BINNED_ALLOCATOR_ALIGNMENT);

	if (!Ptr)
	{
		void* Result = FMallocBinned2::Malloc(NewSize, Alignment);
		return Result;
	}

	if (NewSize == 0)
	{
		FMallocBinned2::Free(Ptr);
		return nullptr;
	}

	const uint32 NewSizeUnmodified = NewSize;
	SIZE_T SpareBytesCount = FMath::Min<SIZE_T>(Private::DEFAULT_BINNED_ALLOCATOR_ALIGNMENT, NewSize);
	if (NewSize)
	{
		NewSize = FMath::Max<SIZE_T>(PoolTable[0].BlockSize, NewSize + (Alignment - SpareBytesCount));
	}

	FPoolInfo* Pool = Private::FindPoolInfo(*this, Ptr);
	if (Pool->TableIndex < BinnedOSTableIndex)
	{
		// Allocated from pool, so grow or shrink if necessary.
		check(Pool->TableIndex > 0); // it isn't possible to allocate a size of 0, Malloc will increase the size to DEFAULT_BINNED_ALLOCATOR_ALIGNMENT
		if (NewSizeUnmodified > MemSizeToPoolTable[Pool->TableIndex]->BlockSize || NewSizeUnmodified <= MemSizeToPoolTable[Pool->TableIndex - 1]->BlockSize)
		{
			void* Result = FMallocBinned2::Malloc(NewSizeUnmodified, Alignment);
			FMemory::Memcpy(Result, Ptr, FMath::Min<SIZE_T>(NewSizeUnmodified, MemSizeToPoolTable[Pool->TableIndex]->BlockSize - (Alignment - SpareBytesCount)));
			FMallocBinned2::Free(Ptr);

			return Result;
		}

		if (!IsAligned(Ptr, Alignment))
		{
			void* Result = Align(Ptr, Alignment);
			FMemory::Memmove(Result, Ptr, NewSize);
			return Result;
		}

		return Ptr;
	}

	// Allocated from OS.
	UPTRINT PoolOsBytes = Pool->GetOsBytes(PageSize, BinnedOSTableIndex);
	if( NewSize > PoolOsBytes || NewSize * 3 < PoolOsBytes * 2 )
	{
		// Grow or shrink.
		void* Result = FMallocBinned2::Malloc(NewSizeUnmodified, Alignment);
		FMemory::Memcpy(Result, Ptr, FMath::Min<SIZE_T>(NewSizeUnmodified, Pool->AllocSize));
		FMallocBinned2::Free(Ptr);

		return Result;
	}

	//need a lock to cover the SetAllocationSizes()
	int32 UsedChange = (NewSize - Pool->AllocSize);

	// Keep as-is, reallocation isn't worth the overhead.
	BINNED2_ADD_STATCOUNTER (Stats.UsedCurrent,  UsedChange);
	BINNED2_PEAK_STATCOUNTER(Stats.UsedPeak,     Stats.UsedCurrent);
	BINNED2_ADD_STATCOUNTER (Stats.WasteCurrent, (Pool->AllocSize - NewSize));

	Pool->SetAllocationSizes(NewSizeUnmodified, PoolOsBytes, BinnedOSTableIndex, BinnedOSTableIndex);

	return Ptr;
}

void FMallocBinned2::Free( void* Ptr )
{
	if (!Ptr)
	{
		return;
	}

	BINNED2_DECREMENT_STATCOUNTER(Stats.CurrentAllocs);

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
	checkSlow(Pool->AllocSize != 0);
	if (Pool->TableIndex < BinnedOSTableIndex)
	{
		FPoolTable* Table = MemSizeToPoolTable[Pool->TableIndex];
#if STATS
		Table->ActiveRequests--;
#endif
		// If this pool was exhausted, move to available list.
		if (!Pool->FirstMem)
		{
			Pool->Unlink();
			Pool->Link(Table->FirstPool);
		}

		check((UPTRINT)BasePtr <= (UPTRINT)Ptr);

		uint32 AlignOffset = ((UPTRINT)Ptr - (UPTRINT)BasePtr) % Table->BlockSize;

		// Patch pointer to include previously applied alignment.
		Ptr = (void*)((PTRINT)Ptr - (PTRINT)AlignOffset);

		// Free a pooled allocation.
		FFreeMem* Free		= (FFreeMem*)Ptr;
		Free->NumFreeBlocks	= 1;
		Free->Next			= Pool->FirstMem;
		Pool->FirstMem		= Free;
		BINNED2_ADD_STATCOUNTER(Stats.UsedCurrent, -(int64)Table->BlockSize);

		// Free this pool.
		checkSlow(Pool->Taken >= 1);
		if( --Pool->Taken == 0 )
		{
#if STATS
			Table->NumActivePools--;
#endif
			// Free the OS memory.
			SIZE_T OsBytes = Pool->GetOsBytes(PageSize, BinnedOSTableIndex);
			BINNED2_ADD_STATCOUNTER(Stats.OsCurrent,    -(int64)OsBytes);
			BINNED2_ADD_STATCOUNTER(Stats.WasteCurrent, -(int64)(OsBytes - Pool->AllocSize));
			Pool->Unlink();
			Pool->SetAllocationSizes(0, 0, 0, BinnedOSTableIndex);
			CachedOSPageAllocator.Free(BasePtr, OsBytes);
		}
	}
	else
	{
		// Free an OS allocation.
		checkSlow(IsAligned(Ptr, PageSize));
		SIZE_T OsBytes = Pool->GetOsBytes(PageSize, BinnedOSTableIndex);

		BINNED2_ADD_STATCOUNTER(Stats.UsedCurrent,  -(int64)Pool->AllocSize);
		BINNED2_ADD_STATCOUNTER(Stats.OsCurrent,    -(int64)OsBytes);
		BINNED2_ADD_STATCOUNTER(Stats.WasteCurrent, -(int64)(OsBytes - Pool->AllocSize));
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
	SizeOut = Pool->TableIndex < BinnedOSTableIndex ? MemSizeToPoolTable[Pool->TableIndex]->BlockSize : Pool->AllocSize;
	return true;
}

bool FMallocBinned2::ValidateHeap()
{
	for (FPoolTable& Table : PoolTable)
	{
		for( FPoolInfo** PoolPtr = &Table.FirstPool; *PoolPtr; PoolPtr = &(*PoolPtr)->Next )
		{
			FPoolInfo* Pool = *PoolPtr;
			check(Pool->PrevLink == PoolPtr);
			check(Pool->FirstMem);
			for( FFreeMem* Free = Pool->FirstMem; Free; Free = Free->Next )
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

	return( true );
}

void FMallocBinned2::UpdateStats()
{
	FMalloc::UpdateStats();
#if STATS

	Private::UpdateSlackStat(*this);

	SET_MEMORY_STAT( STAT_Binned2_OsCurrent,     Stats.OsCurrent );
	SET_MEMORY_STAT( STAT_Binned2_OsPeak,        Stats.OsPeak );
	SET_MEMORY_STAT( STAT_Binned2_WasteCurrent,  Stats.WasteCurrent );
	SET_MEMORY_STAT( STAT_Binned2_WastePeak,     Stats.WastePeak );
	SET_MEMORY_STAT( STAT_Binned2_UsedCurrent,   Stats.UsedCurrent );
	SET_MEMORY_STAT( STAT_Binned2_UsedPeak,      Stats.UsedPeak );
	SET_DWORD_STAT ( STAT_Binned2_CurrentAllocs, Stats.CurrentAllocs );
	SET_DWORD_STAT ( STAT_Binned2_TotalAllocs,   Stats.TotalAllocs );
	SET_MEMORY_STAT( STAT_Binned2_SlackCurrent,  Stats.SlackCurrent );
#endif
}

void FMallocBinned2::DumpAllocatorStats( class FOutputDevice& Ar )
{
	FBufferedOutputDevice BufferedOutput;
	{
		ValidateHeap();
#if STATS
		Private::UpdateSlackStat(*this);
#if !NO_LOGGING
		// This is all of the memory including stuff too big for the pools
		BufferedOutput.CategorizedLogf( LogMemory.GetCategoryName(), ELogVerbosity::Log, TEXT( "Allocator Stats for %s:" ), GetDescriptiveName() );
		// Waste is the total overhead of the memory system
		BufferedOutput.CategorizedLogf( LogMemory.GetCategoryName(), ELogVerbosity::Log, TEXT( "Current Memory %.2f MB used, plus %.2f MB waste" ), Stats.UsedCurrent / (1024.0f * 1024.0f), (Stats.OsCurrent - Stats.UsedCurrent) / (1024.0f * 1024.0f) );
		BufferedOutput.CategorizedLogf( LogMemory.GetCategoryName(), ELogVerbosity::Log, TEXT( "Peak Memory %.2f MB used, plus %.2f MB waste" ), Stats.UsedPeak / (1024.0f * 1024.0f), (Stats.OsPeak - Stats.UsedPeak) / (1024.0f * 1024.0f) );

		BufferedOutput.CategorizedLogf( LogMemory.GetCategoryName(), ELogVerbosity::Log, TEXT( "Current OS Memory %.2f MB, peak %.2f MB" ), Stats.OsCurrent / (1024.0f * 1024.0f), Stats.OsPeak / (1024.0f * 1024.0f) );
		BufferedOutput.CategorizedLogf( LogMemory.GetCategoryName(), ELogVerbosity::Log, TEXT( "Current Waste %.2f MB, peak %.2f MB" ), Stats.WasteCurrent / (1024.0f * 1024.0f), Stats.WastePeak / (1024.0f * 1024.0f) );
		BufferedOutput.CategorizedLogf( LogMemory.GetCategoryName(), ELogVerbosity::Log, TEXT( "Current Used %.2f MB, peak %.2f MB" ), Stats.UsedCurrent / (1024.0f * 1024.0f), Stats.UsedPeak / (1024.0f * 1024.0f) );
		BufferedOutput.CategorizedLogf( LogMemory.GetCategoryName(), ELogVerbosity::Log, TEXT( "Current Slack %.2f MB" ), Stats.SlackCurrent / (1024.0f * 1024.0f) );

		BufferedOutput.CategorizedLogf( LogMemory.GetCategoryName(), ELogVerbosity::Log, TEXT( "Allocs      % 6i Current / % 6i Total" ), Stats.CurrentAllocs, Stats.TotalAllocs );

		// This is the memory tracked inside individual allocation pools
		BufferedOutput.CategorizedLogf( LogMemory.GetCategoryName(), ELogVerbosity::Log, TEXT( "" ) );
		BufferedOutput.CategorizedLogf( LogMemory.GetCategoryName(), ELogVerbosity::Log, TEXT( "Block Size Num Pools Max Pools Cur Allocs Total Allocs Min Req Max Req Mem Used Mem Slack Mem Waste Efficiency" ) );
		BufferedOutput.CategorizedLogf( LogMemory.GetCategoryName(), ELogVerbosity::Log, TEXT( "---------- --------- --------- ---------- ------------ ------- ------- -------- --------- --------- ----------" ) );

		uint32 TotalMemory = 0;
		uint32 TotalWaste = 0;
		uint32 TotalActiveRequests = 0;
		uint32 TotalTotalRequests = 0;
		uint32 TotalPools = 0;
		uint32 TotalSlack = 0;

		FPoolTable* Table = nullptr;
		for( int32 i = 0; i < BinnedSizeLimit + EXTENDED_PAGE_POOL_ALLOCATION_COUNT; i++ )
		{
			if( Table == MemSizeToPoolTable[i] || MemSizeToPoolTable[i]->BlockSize == 0 )
				continue;

			Table = MemSizeToPoolTable[i];

			uint32 TableAllocSize = (Table->BlockSize > BinnedSizeLimit ? (((3 * (i - BinnedSizeLimit)) + 3)*Private::BINNED_ALLOC_POOL_SIZE) : Private::BINNED_ALLOC_POOL_SIZE);
			// The amount of memory allocated from the OS
			uint32 MemAllocated = (Table->NumActivePools * TableAllocSize) / 1024;
			// Amount of memory actually in use by allocations
			uint32 MemUsed = (Table->BlockSize * Table->ActiveRequests) / 1024;
			// Wasted memory due to pool size alignment
			uint32 PoolMemWaste = Table->NumActivePools * (TableAllocSize - ((TableAllocSize / Table->BlockSize) * Table->BlockSize)) / 1024;
			// Wasted memory due to individual allocation alignment. This is an estimate.
			uint32 MemWaste = (uint32)(((double)Table->TotalWaste / (double)Table->TotalRequests) * (double)Table->ActiveRequests) / 1024 + PoolMemWaste;
			// Memory that is reserved in active pools and ready for future use
			uint32 MemSlack = MemAllocated - MemUsed - PoolMemWaste;

			BufferedOutput.CategorizedLogf( LogMemory.GetCategoryName(), ELogVerbosity::Log, TEXT( "% 10i % 9i % 9i % 10i % 12i % 7i % 7i % 7iK % 8iK % 8iK % 9.2f%%" ),
										  Table->BlockSize,
										  Table->NumActivePools,
										  Table->MaxActivePools,
										  Table->ActiveRequests,
										  (uint32)Table->TotalRequests,
										  Table->MinRequest,
										  Table->MaxRequest,
										  MemUsed,
										  MemSlack,
										  MemWaste,
										  MemAllocated ? 100.0f * (MemAllocated - MemWaste) / MemAllocated : 100.0f );

			TotalMemory += MemAllocated;
			TotalWaste += MemWaste;
			TotalSlack += MemSlack;
			TotalActiveRequests += Table->ActiveRequests;
			TotalTotalRequests += Table->TotalRequests;
			TotalPools += Table->NumActivePools;
		}

		BufferedOutput.CategorizedLogf( LogMemory.GetCategoryName(), ELogVerbosity::Log, TEXT( "" ) );
		BufferedOutput.CategorizedLogf( LogMemory.GetCategoryName(), ELogVerbosity::Log, TEXT( "%iK allocated in pools (with %iK slack and %iK waste). Efficiency %.2f%%" ), TotalMemory, TotalSlack, TotalWaste, TotalMemory ? 100.0f * (TotalMemory - TotalWaste) / TotalMemory : 100.0f );
		BufferedOutput.CategorizedLogf( LogMemory.GetCategoryName(), ELogVerbosity::Log, TEXT( "Allocations %i Current / %i Total (in %i pools)" ), TotalActiveRequests, TotalTotalRequests, TotalPools );
		BufferedOutput.CategorizedLogf( LogMemory.GetCategoryName(), ELogVerbosity::Log, TEXT( "" ) );
#endif
#endif
	}

	BufferedOutput.RedirectTo( Ar );
}

const TCHAR* FMallocBinned2::GetDescriptiveName()
{
	return TEXT("binned");
}
