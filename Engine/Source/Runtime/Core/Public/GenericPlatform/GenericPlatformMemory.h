// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	GenericPlatformMemory.h: Generic platform memory classes
==============================================================================================*/

#pragma once

// @todo: Add stats

/** 
 * Struct used to hold common memory constants for all platforms.
 * These values don't change over the entire life of the executable.
 */
struct FGenericPlatformMemoryConstants
{
	/** The amount of actual physical memory, in bytes. */
	SIZE_T TotalPhysical;

	/** The amount of virtual memory, in bytes. */
	SIZE_T TotalVirtual;

	/** The size of a page, in bytes. */
	SIZE_T PageSize;

	/** Approximate physical RAM in GB; 1 on everything except PC. Used for "course tuning", like FPlatformMisc::NumberOfCores(). */
	uint32 TotalPhysicalGB;

	/** Default constructor, clears all variables. */
	FGenericPlatformMemoryConstants()
		: TotalPhysical( 0 )
		, TotalVirtual( 0 )
		, PageSize( 0 )
		, TotalPhysicalGB( 1 )
	{}
};

/** 
 * Struct used to hold common memory stats for all platforms.
 * These values may change over the entire life of the executable.
 */
struct FGenericPlatformMemoryStats
{
	/** The amount of physical memory currently available, in bytes. */
	SIZE_T AvailablePhysical;

	/** The amount of virtual memory currently available, in bytes. */
	SIZE_T AvailableVirtual;

	/** The current working set size, in bytes. This is the amount of physical memory used by the process. */
	SIZE_T WorkingSetSize;

	/** The peak working set size, in bytes. */
	SIZE_T PeakWorkingSetSize;

	/** Total amount of virtual memory used by the process. */
	SIZE_T PagefileUsage;
	
	/** Default constructor, clears all variables. */
	FGenericPlatformMemoryStats()
		: AvailablePhysical( 0 )
		, AvailableVirtual( 0 )
		, WorkingSetSize( 0 )
		, PeakWorkingSetSize( 0 )
		, PagefileUsage( 0 )		
	{}
};

struct FPlatformMemoryStats;

typedef FGenericPlatformMemoryConstants FPlatformMemoryConstants;

/**
 * FMemory_Alloca/alloca implementation. This can't be a function, even FORCEINLINE'd because there's no guarantee that 
 * the memory returned in a function will stick around for the caller to use.
 */
#if PLATFORM_USES_MICROSOFT_LIBC_FUNCTIONS
#define __FMemory_Alloca_Func _alloca
#else
#define __FMemory_Alloca_Func alloca
#endif

#define FMemory_Alloca(Size )((Size==0) ? 0 : (void*)(((PTRINT)__FMemory_Alloca_Func(Size + 15) + 15) & ~15))

/** Generic implementation for most platforms, these tend to be unused and unimplemented. */
struct CORE_API FGenericPlatformMemory
{
	/**
	 * Various memory regions that can be used with memory stats. The exact meaning of
	 * the enums are relatively platform-dependent, although the general ones (Physical, GPU)
	 * are straightforward. A platform can add more of these, and it won't affect other 
	 * platforms, other than a miniscule amount of memory for the StatManager to track the
	 * max available memory for each region (uses an array FPlatformMemory::MCR_MAX big)
	 */
	enum EMemoryCounterRegion
	{
		MCR_Invalid, // not memory
		MCR_Physical, // main system memory
		MCR_GPU, // memory directly a GPU (graphics card, etc)
		MCR_GPUSystem, // system memory directly accessible by a GPU
		MCR_TexturePool, // presized texture pools
		MCR_MAX
	};


	/** Initializes platform memory specific constants. */
	static void Init();
	
	static void OnOutOfMemory(uint64 Size, uint32 Alignment);

	/** Initializes the memory pools, should be called by the init function. */
	static void SetupMemoryPools();

	/**
	 * @return the default allocator.
	 */
	static class FMalloc* BaseAllocator();

	/**
	 * @return platform specific current memory statistics.
	 */
	static FPlatformMemoryStats GetStats();

	/**
	 * @return platform specific memory constants.
	 */
	static const FPlatformMemoryConstants& GetConstants();
	
	/**
	 * @return approximate physical RAM in GB.
	 */
	static uint32 GetPhysicalGBRam();

	/**
	 * Allocates pages from the OS.
	 *
	 * @param Size Size to allocate, not necessarily aligned
	 *
	 * @return OS allocated pointer for use by binned allocator
	 */
	static void* BinnedAllocFromOS( SIZE_T Size );
	
	/**
	 * Returns pages allocated by BinnedAllocFromOS to the OS.
	 *
	 * @param A pointer previously returned from BinnedAllocFromOS
	 */
	static void BinnedFreeToOS( void* Ptr );

	/** Dumps basic platform memory statistics into the specified output device. */
	static void DumpStats( class FOutputDevice& Ar );

	/** Dumps basic platform memory statistics and allocator specific statistics into the specified output device. */
	static void DumpPlatformAndAllocatorStats( class FOutputDevice& Ar );

	/** @name Memory functions */

	/** Copies count bytes of characters from Src to Dest. If some regions of the source
	 * area and the destination overlap, memmove ensures that the original source bytes
	 * in the overlapping region are copied before being overwritten.  NOTE: make sure
	 * that the destination buffer is the same size or larger than the source buffer!
	 */
	static FORCEINLINE void* Memmove( void* Dest, const void* Src, SIZE_T Count )
	{
		return memmove( Dest, Src, Count );
	}

	static FORCEINLINE int32 Memcmp( const void* Buf1, const void* Buf2, SIZE_T Count )
	{
		return memcmp( Buf1, Buf2, Count );
	}

	static FORCEINLINE void* Memset(void* Dest, uint8 Char, SIZE_T Count)
	{
		return memset( Dest, Char, Count );
	}

	static FORCEINLINE void* Memzero(void* Dest, SIZE_T Count)
	{
		return memset( Dest, 0, Count );
	}

	static FORCEINLINE void* Memcpy(void* Dest, const void* Src, SIZE_T Count)
	{
		return memcpy( Dest, Src, Count );
	}

	/** Memcpy optimized for big blocks. */
	static FORCEINLINE void* BigBlockMemcpy(void* Dest, const void* Src, SIZE_T Count)
	{
		return memcpy(Dest, Src, Count);
	}

	/** On some platforms memcpy optimized for big blocks that avoid L2 cache pollution are available */
	static FORCEINLINE void* StreamingMemcpy(void* Dest, const void* Src, SIZE_T Count)
	{
		return memcpy( Dest, Src, Count );
	}

	static void Memswap( void* Ptr1, void* Ptr2, SIZE_T Size );
};

