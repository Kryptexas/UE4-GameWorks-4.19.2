// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "UnrealMemory.h"

/*=============================================================================
	MemoryBase.h: Base memory management definitions.
=============================================================================*/

#ifndef __MEMORYBASE_H__
#define __MEMORYBASE_H__


/** The global memory allocator. */
CORE_API extern class FMalloc* GMalloc;

/** Global FMallocProfiler variable to allow multiple malloc profilers to communicate. */
MALLOC_PROFILER( extern class FMallocProfiler* GMallocProfiler; )

// Memory allocator.

/** 
 * Struct used to hold memory allocation statistics. 
 * NOTE: Be sure to check hardcoded/mirrored usage found in MemoryProfiler2 StreamToken.cs
 * Only for backward compatibility with MemoryProfiler2.
 * @todo remove it later.
 * Contains only allocator specific stats.
 */
struct FMemoryAllocationStats_DEPRECATED
{
	SIZE_T	TotalUsed;			/** The total amount of memory used by the game. */	
	SIZE_T	TotalAllocated;		/** The total amount of memory allocated from the OS. */

	// Virtual memory for Xbox and PC / Main memory for PS3 (tracked in the allocators)
	SIZE_T	CPUUsed;			/** The allocated in use by the application virtual memory. */
	SIZE_T	CPUSlack;			/** The allocated from the OS/allocator, but not used by the application. */
	SIZE_T	CPUWaste;			/** Alignment waste from a pooled allocator plus book keeping overhead. */
	SIZE_T	CPUAvailable;		/** MemoryStats.AvailableVirtual */

	SIZE_T	NotUsed0;			
	SIZE_T	NotUsed1;
	SIZE_T	NotUsed2;
	SIZE_T	NotUsed3;

	SIZE_T	NotUsed4;
	SIZE_T	OSReportedFree;		/** Free memory as reported by the operating system. (Xbox360, PS3) */
	SIZE_T	NotUsed5;
	SIZE_T	NotUsed6;

	SIZE_T	NotUsed7;
	SIZE_T	NotUsed8;
	SIZE_T	NotUsed9;
	SIZE_T	NotUsedA;

	SIZE_T	AllocatedTextureMemorySize; /** Size of allocated memory in the texture pool. */
	SIZE_T	AvailableTextureMemorySize; /** Size of available memory in the texture pool. */

	/** Returns statistics count. */
	static uint8 GetStatsNum()
	{
		const uint8 ItemsCount = (uint8)(sizeof(FMemoryAllocationStats_DEPRECATED) / sizeof(SIZE_T));
		return ItemsCount;
	}

	/** 
	 * Constructor. 
	 */
	FMemoryAllocationStats_DEPRECATED()
	{
		TotalUsed = 0;
		TotalAllocated = 0;

		CPUUsed = 0;
		CPUSlack = 0;
		CPUWaste = 0;
		CPUAvailable = 0;

		NotUsed0 = 0;
		NotUsed1 = 0;	
		NotUsed2 = 0;
		NotUsed3 = 0;

		NotUsed4 = 0;
		OSReportedFree = 0;
		NotUsed5 = 0;
		NotUsed6 = 0;

		NotUsed6 = 0;	
		NotUsed8 = 0;	
		NotUsed9 = 0;
		NotUsedA = 0;

		AllocatedTextureMemorySize = 0;
		AvailableTextureMemorySize = 0;
	}
};

/**
 * Inherit from FUseSystemMallocForNew if you want your objects to be placed in memory
 * alloced by the system malloc routines, bypassing GMalloc. This is e.g. used by FMalloc
 * itself.
 */
class CORE_API FUseSystemMallocForNew
{
public:
	/**
	 * Overloaded new operator using the system allocator.
	 *
	 * @param	Size	Amount of memory to allocate (in bytes)
	 * @return			A pointer to a block of memory with size Size or NULL
	 */
	void* operator new( size_t Size )
	{
		return FMemory::SystemMalloc( Size );
	}

	/**
	 * Overloaded delete operator using the system allocator
	 *
	 * @param	Ptr		Pointer to delete
	 */
	void operator delete( void* Ptr )
	{
		FMemory::SystemFree( Ptr );
	}

	/**
	 * Overloaded array new operator using the system allocator.
	 *
	 * @param	Size	Amount of memory to allocate (in bytes)
	 * @return			A pointer to a block of memory with size Size or NULL
	 */
	void* operator new[]( size_t Size )
	{
		return FMemory::SystemMalloc( Size );
	}

	/**
	 * Overloaded array delete operator using the system allocator
	 *
	 * @param	Ptr		Pointer to delete
	 */
	void operator delete[]( void* Ptr )
	{
		FMemory::SystemFree( Ptr );
	}
};

/** The global memory allocator's interface. */
class CORE_API FMalloc  : 
	public FUseSystemMallocForNew,
	public FExec
{
public:
	/** 
	 * QuantizeSize returns the actual size of allocation request likely to be returned
	 * so for the template containers that use slack, they can more wisely pick
	 * appropriate sizes to grow and shrink to.
	 *
	 * CAUTION: QuantizeSize is a special case and is NOT guarded by a thread lock, so must be intrinsically thread safe!
	 *
	 * @param Size			The size of a hypothetical allocation request
	 * @param Alignment		The alignment of a hypothetical allocation request
	 * @return				Returns the usable size that the allocation request would return. In other words you can ask for this greater amount without using any more actual memory.
	 */
	virtual SIZE_T QuantizeSize( SIZE_T Size, uint32 Alignment )
	{
		return Size; // implementations where this is not possible need not implement it.
	}

	/** 
	 * Malloc
	 */
	virtual void* Malloc( SIZE_T Count, uint32 Alignment=DEFAULT_ALIGNMENT ) = 0;

	/** 
	 * Realloc
	 */
	virtual void* Realloc( void* Original, SIZE_T Count, uint32 Alignment=DEFAULT_ALIGNMENT ) = 0;

	/** 
	 * Free
	 */
	virtual void Free( void* Original ) = 0;
		
	/** 
	 * Handles any commands passed in on the command line
	 */
	virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar ) OVERRIDE
	{ 
		return false; 
	}
	
	/** 
	 * Called every game thread tick 
	 */
	virtual void Tick( float DeltaTime ) 
	{ 
	}

	/**
	 * Returns if the allocator is guaranteed to be thread-safe and therefore
	 * doesn't need a unnecessary thread-safety wrapper around it.
	 */
	virtual bool IsInternallyThreadSafe() const 
	{ 
		return false; 
	}

	/**
	 * Gathers all current memory stats
	 *
	 * @param FMemoryAllocationStats_DEPRECATED	[out] structure containing information about the size of allocations
	 */
	virtual void GetAllocationInfo( FMemoryAllocationStats_DEPRECATED& MemStats ) 
	{
	}

	/**
	 * Dumps details about all allocations to an output device
	 *
	 * @param Ar	[in] Output device
	 */
	virtual void DumpAllocations( class FOutputDevice& Ar ) 
	{
		Ar.Logf( TEXT( "DumpAllocations not implemented" ) );
	}

	/**
	 * Validates the allocator's heap
	 */
	virtual bool ValidateHeap()
	{
		return( true );
	}

	/**
	* If possible determine the size of the memory allocated at the given address
	*
	* @param Original - Pointer to memory we are checking the size of
	* @param SizeOut - If possible, this value is set to the size of the passed in pointer
	* @return true if succeeded
	*/
	virtual bool GetAllocationSize(void *Original, SIZE_T &SizeOut)
	{
		return false; // Default implementation has no way of determining this
	}

	/**
	 * Gets descriptive name for logging purposes.
	 *
	 * @return pointer to human-readable malloc name
	 */
	virtual const TCHAR * GetDescriptiveName()
	{
		return TEXT("Unspecified allocator");
	}

	/** Total number of calls Malloc, if implemented by derived class. */
	static uint64 TotalMallocCalls;
	/** Total number of calls Malloc, if implemented by derived class. */
	static uint64 TotalFreeCalls;
	/** Total number of calls Malloc, if implemented by derived class. */
	static uint64 TotalReallocCalls;
};

#endif

