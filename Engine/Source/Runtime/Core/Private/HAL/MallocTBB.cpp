// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MallocTTB.cpp: IntelTTB Malloc
=============================================================================*/

#include "CorePrivate.h"
#include "MallocTBB.h"

// Only use for supported platforms
#if PLATFORM_SUPPORTS_TBB && TBB_ALLOCATOR_ALLOWED

/** Value we fill a memory block with after it is free, in UE_BUILD_DEBUG **/
#define DEBUG_FILL_FREED (0xdd)

/** Value we fill a new memory block with, in UE_BUILD_DEBUG **/
#define DEBUG_FILL_NEW (0xcd)

// Statically linked tbbmalloc requires tbbmalloc_debug.lib in debug
#if UE_BUILD_DEBUG && !defined(NDEBUG)	// Use !defined(NDEBUG) to check to see if we actually are linking with Debug third party libraries (bDebugBuildsActuallyUseDebugCRT)
	#ifndef TBB_USE_DEBUG
		#define TBB_USE_DEBUG 1
	#endif
#endif
#include <tbb/scalable_allocator.h>

#define MEM_TIME(st)

void FMallocTBB::GetAllocationInfo( FMemoryAllocationStats_DEPRECATED& MemStats )
{
	FPlatformMemoryStats MemoryStats = FPlatformMemory::GetStats();

	MemStats.TotalUsed = MemoryStats.WorkingSetSize;
	MemStats.TotalAllocated = MemoryStats.WorkingSetSize;
	MemStats.CPUUsed = MemoryStats.PagefileUsage;
}

void FMallocTBB::DumpAllocations( FOutputDevice& Ar )
{
	STAT(Ar.Logf( TEXT("Memory Allocation Status") ));
	MEM_TIME(Ar.Logf( TEXT("Seconds     % 5.3f"), MemTime ));
}

void* FMallocTBB::Malloc( SIZE_T Size, uint32 Alignment )
{
	MEM_TIME(MemTime -= FPlatformTime::Seconds());

	void* Free = NULL;
	if( Alignment != DEFAULT_ALIGNMENT )
	{
		Alignment = FMath::Max(Size >= 16 ? (uint32)16 : (uint32)8, Alignment);
		Free = scalable_aligned_malloc( Size, Alignment );
	}
	else
	{
		Free = scalable_malloc( Size );
	}

	if( !Free )
	{
		OutOfMemory(Size, Alignment);
	}
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	else if (Size)
	{
		FMemory::Memset(Free, DEBUG_FILL_NEW, Size); 
	}
#endif
	MEM_TIME(MemTime += FPlatformTime::Seconds());
	return Free;
}

void* FMallocTBB::Realloc( void* Ptr, SIZE_T NewSize, uint32 Alignment )
{
	MEM_TIME(MemTime -= FPlatformTime::Seconds())
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	SIZE_T OldSize = 0;
	if (Ptr)
	{
		OldSize = scalable_msize(Ptr);
		if (NewSize < OldSize)
		{
			FMemory::Memset((uint8*)Ptr + NewSize, DEBUG_FILL_FREED, OldSize - NewSize); 
		}
	}
#endif
	void* NewPtr = NULL;
	if (Alignment != DEFAULT_ALIGNMENT)
	{
		Alignment = FMath::Max(NewSize >= 16 ? (uint32)16 : (uint32)8, Alignment);
		NewPtr = scalable_aligned_realloc(Ptr, NewSize, Alignment);
	}
	else
	{
		NewPtr = scalable_realloc(Ptr, NewSize);
	}
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	if (NewPtr && NewSize > OldSize )
	{
		FMemory::Memset((uint8*)NewPtr + OldSize, DEBUG_FILL_NEW, NewSize - OldSize); 
	}
#endif
	if( !NewPtr && NewSize )
	{
		OutOfMemory(NewSize, Alignment);
	}
	MEM_TIME(MemTime += FPlatformTime::Seconds())
	return NewPtr;
}

void FMallocTBB::Free( void* Ptr )
{
	if( !Ptr )
	{
		return;
	}
	MEM_TIME(MemTime -= FPlatformTime::Seconds())
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	FMemory::Memset(Ptr, DEBUG_FILL_FREED, scalable_msize(Ptr)); 
#endif
	scalable_free(Ptr);

	MEM_TIME(MemTime += FPlatformTime::Seconds())
}

bool FMallocTBB::GetAllocationSize(void *Original, SIZE_T &SizeOut)
{
	SizeOut = scalable_msize(Original);
	return true;
}

#undef MEM_TIME

#endif // PLATFORM_SUPPORTS_TBB && TBB_ALLOCATOR_ALLOWED
