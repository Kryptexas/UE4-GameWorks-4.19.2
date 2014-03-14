// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	HTML5PlatformMemory.h: HTML5 platform memory functions
==============================================================================================*/

#pragma once

/**
 *	HTML5 implementation of the FGenericPlatformMemoryStats.
 */
struct FPlatformMemoryStats : public FGenericPlatformMemoryStats
{};

/**
* HTML5 implementation of the memory OS functions
**/
struct CORE_API FHTML5PlatformMemory : public FGenericPlatformMemory
{
	// Begin FGenericPlatformMemory interface
	static void Init();
	static const FPlatformMemoryConstants& GetConstants();
	static FPlatformMemoryStats GetStats();
	static FMalloc* BaseAllocator();
	static void* BinnedAllocFromOS( SIZE_T Size );
	static void BinnedFreeToOS( void* Ptr );
	// End FGenericPlatformMemory interface
};

typedef FHTML5PlatformMemory FPlatformMemory;



