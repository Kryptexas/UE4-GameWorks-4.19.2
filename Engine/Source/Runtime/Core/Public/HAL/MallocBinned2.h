// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Allocators/CachedOSPageAllocator.h"
#include "Function.h"

#define BINNED2_MAX_CACHED_OS_FREES (64)
#if PLATFORM_64BITS
	#define BINNED2_MAX_CACHED_OS_FREES_BYTE_LIMIT (64*1024*1024)
#else
	#define BINNED2_MAX_CACHED_OS_FREES_BYTE_LIMIT (16*1024*1024)
#endif

//
// Optimized virtual memory allocator.
//
class FMallocBinned2 : public FMalloc
{
	struct Private;

private:
	// Counts.
	enum { SMALL_POOL_COUNT = 41 };

	/** Maximum allocation for the pooled allocator */
	enum { MAX_SMALL_POOLED_ALLOCATION_SIZE = 32768 };
	enum { MAX_LARGE_POOLED_PAGE_FRACTIONS  = 2 };

	// Forward declares.
	struct FFreeBlock;
	struct FPoolInfo;
	struct FPoolTable;
	struct PoolHashBucket;

	struct FPoolList
	{
		FPoolList();

		bool IsEmpty() const;

		      FPoolInfo& GetFrontPool();
		const FPoolInfo& GetFrontPool() const;

		void LinkToFront(FPoolInfo* Pool);

		FPoolInfo& PushNewPoolToFront(FMallocBinned2& Allocator, uint32 InBytes);

		void Validate(TFunctionRef<void(FPoolInfo&)>);

	private:
		FPoolInfo* Front;
	};

	/** Pool table. */
	struct FPoolTable
	{
		FPoolList ActivePools;
		FPoolList ExhaustedPools;
		uint32    BlockSize;

		FPoolTable();
	};

	const uint32 PageSize;
	const uint64 NumPoolsPerPage;

	struct FPtrToPoolMapping
	{
		explicit FPtrToPoolMapping(uint32 InPageSize, uint64 InNumPoolsPerPage)
		{
			uint64 PoolPageToPoolBitShift = FPlatformMath::CeilLogTwo(InNumPoolsPerPage);

			PtrToPoolPageBitShift = FPlatformMath::CeilLogTwo(InPageSize);
			HashKeyShift          = PtrToPoolPageBitShift + PoolPageToPoolBitShift;
			PoolMask              = (1ull << PoolPageToPoolBitShift) - 1;
		}

		FORCEINLINE void GetHashBucketAndPoolIndices(const void* InPtr, UPTRINT& OutBucketIndex, uint32& OutPoolIndex) const
		{
			OutBucketIndex = (UPTRINT)InPtr >> HashKeyShift;
			OutPoolIndex   = ((UPTRINT)InPtr >> PtrToPoolPageBitShift) & PoolMask;
		}

		FORCEINLINE uint64 GetMaxHashBuckets(uint64 AddressLimit) const
		{
			return AddressLimit >> HashKeyShift;
		}

	private:
		/** Shift to apply to a pointer to get the reference from the indirect tables */
		uint64 PtrToPoolPageBitShift;

		/** Shift required to get required hash table key. */
		uint64 HashKeyShift;

		/** Used to mask off the bits that have been used to lookup the indirect table */
		uint64 PoolMask;
	};

	const FPtrToPoolMapping PtrToPoolMapping;

	// Pool tables for different pool sizes
	FPoolTable SmallPoolTables[SMALL_POOL_COUNT];

	// Mapping of indices to both large and small tables
	FPoolTable* MemSizeToPoolTable[MAX_SMALL_POOLED_ALLOCATION_SIZE];

	PoolHashBucket* HashBuckets;
	PoolHashBucket* HashBucketFreeList;

	TCachedOSPageAllocator<BINNED2_MAX_CACHED_OS_FREES, BINNED2_MAX_CACHED_OS_FREES_BYTE_LIMIT> CachedOSPageAllocator;

public:
	// FMalloc interface.
	// InPageSize - First parameter is page size, all allocs from BinnedAllocFromOS() MUST be aligned to this size
	// AddressLimit - Second parameter is estimate of the range of addresses expected to be returns by BinnedAllocFromOS(). Binned
	// Malloc will adjust its internal structures to make lookups for memory allocations O(1) for this range. 
	// It is ok to go outside this range, lookups will just be a little slower
	FMallocBinned2(uint32 InPageSize, uint64 AddressLimit);

	virtual ~FMallocBinned2();

	/**
	 * Returns if the allocator is guaranteed to be thread-safe and therefore
	 * doesn't need a unnecessary thread-safety wrapper around it.
	 */
	virtual bool IsInternallyThreadSafe() const override;

	/** 
	 * Malloc
	 */
	virtual void* Malloc(SIZE_T Size, uint32 Alignment) override;

	/** 
	 * Realloc
	 */
	virtual void* Realloc( void* Ptr, SIZE_T NewSize, uint32 Alignment ) override;

	/** 
	 * Free
	 */
	virtual void Free( void* Ptr ) override;

	/**
	 * If possible determine the size of the memory allocated at the given address
	 *
	 * @param Original - Pointer to memory we are checking the size of
	 * @param SizeOut - If possible, this value is set to the size of the passed in pointer
	 * @return true if succeeded
	 */
	virtual bool GetAllocationSize(void *Original, SIZE_T &SizeOut) override;

	/**
	 * Validates the allocator's heap
	 */
	virtual bool ValidateHeap() override;

	virtual const TCHAR* GetDescriptiveName() override;
};
