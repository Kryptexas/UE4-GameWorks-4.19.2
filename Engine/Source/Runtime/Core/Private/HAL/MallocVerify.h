// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MallocVerify.h: Helper class to track memory allocations
=============================================================================*/

#pragma once

#include "Allocators/AnsiAllocator.h"

#define MALLOC_VERIFY 0

#if MALLOC_VERIFY

/**
/* Maintains a list of all pointers to currently allocated memory.
 */
class FMallocVerify
{
	/** List of all currently allocated pointers */
	TSet<void*, DefaultKeyFuncs<void*>, FAnsiSetAllocator> AllocatedPointers;
	FCriticalSection AllocatedPointersCritical;

public:

	/** Handles new allocated pointer */
	void Malloc(void* Ptr);

	/** Handles reallocation */
	void Realloc(void* OldPtr, void* NewPtr);

	/** Removes allocated pointer from list */
	void Free(void* Ptr);

	/** Returns singleton object */
	static FMallocVerify& Get()
	{
		static FMallocVerify Instance;
		return Instance;
	}
};

#endif // MALLOC_VERIFY
