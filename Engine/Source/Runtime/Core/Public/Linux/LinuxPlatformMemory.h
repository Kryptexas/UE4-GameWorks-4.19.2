// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	LinuxPlatformMemory.h: Linux platform memory functions
==============================================================================================*/

#pragma once

/**
 *	Linux implementation of the FGenericPlatformMemoryStats.
 */
struct FPlatformMemoryStats : public FGenericPlatformMemoryStats
{};

/**
* Linux implementation of the memory OS functions
**/
struct CORE_API FLinuxPlatformMemory : public FGenericPlatformMemory
{
	// Begin FGenericPlatformMemory interface
	static void Init();
	static class FMalloc* BaseAllocator();
	static FPlatformMemoryStats GetStats();
	static const FPlatformMemoryConstants& GetConstants();
	static void* BinnedAllocFromOS( SIZE_T Size );
	static void BinnedFreeToOS( void* Ptr );
	// End FGenericPlatformMemory interface
};

typedef FLinuxPlatformMemory FPlatformMemory;



