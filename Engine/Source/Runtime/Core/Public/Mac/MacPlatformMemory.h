// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	MacPlatformMemory.h: Mac platform memory functions
==============================================================================================*/

#pragma once
#include "GenericPlatform/GenericPlatformMemory.h"

/**
 *	Max implementation of the FGenericPlatformMemoryStats.
 */
struct FPlatformMemoryStats : public FGenericPlatformMemoryStats
{};

/**
* Mac implementation of the memory OS functions
**/
struct CORE_API FMacPlatformMemory : public FGenericPlatformMemory
{
	//~ Begin FGenericPlatformMemory Interface
	static void Init();
	static FPlatformMemoryStats GetStats();
	static const FPlatformMemoryConstants& GetConstants();
	static FMalloc* BaseAllocator();
	static bool PageProtect( void* const Ptr, const SIZE_T Size, const uint32 ProtectMode );
	static void* BinnedAllocFromOS( SIZE_T Size );
	static void BinnedFreeToOS( void* Ptr );
	//~ End FGenericPlatformMemory Interface
};

typedef FMacPlatformMemory FPlatformMemory;



