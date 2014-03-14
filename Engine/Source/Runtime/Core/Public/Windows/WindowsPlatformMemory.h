// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	WindowsPlatformMemory.h: Windows platform memory functions
==============================================================================================*/

#pragma once

// TODO: @JarekS 2013-12-10, 20:36 As stats?
/**
 *	Windows implementation of the FGenericPlatformMemoryStats.
 *	At this moment it's just the same as the FGenericPlatformMemoryStats.
 *	Can be extended as showed in the following example.
 */
struct FPlatformMemoryStats : public FGenericPlatformMemoryStats
{
	/** Default constructor, clears all variables. */
	FPlatformMemoryStats()
		: FGenericPlatformMemoryStats()
		, WindowsSpecificMemoryStat( 0 )
	{}

	/** Memory stat specific only for Windows. */
	SIZE_T WindowsSpecificMemoryStat;
};

/**
* Windows implementation of the memory OS functions
**/
struct CORE_API FWindowsPlatformMemory : public FGenericPlatformMemory
{
	enum EMemoryCounterRegion
	{
		MCR_Invalid, // not memory
		MCR_Physical, // main system memory
		MCR_GPU, // memory directly a GPU (graphics card, etc)
		MCR_GPUSystem, // system memory directly accessible by a GPU
		MCR_TexturePool, // presized texture pools
		MCR_SamplePlatformSpecifcMemoryRegion, 
		MCR_MAX
	};

	// Begin FGenericPlatformMemory interface
	static void Init();
	static class FMalloc* BaseAllocator();
	static FPlatformMemoryStats GetStats();
	static const FPlatformMemoryConstants& GetConstants();
	static void* BinnedAllocFromOS( SIZE_T Size );
	static void BinnedFreeToOS( void* Ptr );
	// End FGenericPlatformMemory interface
};

typedef FWindowsPlatformMemory FPlatformMemory;



