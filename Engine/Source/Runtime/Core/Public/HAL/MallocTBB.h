// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MallocTBB.h: Intel-TBB 64-bit scalable memory allocator.
=============================================================================*/

#pragma once

//
// TBB 64-bit scalable memory allocator.
//
class FMallocTBB : public FMalloc
{
private:
	double		MemTime;

	// Implementation. 
	void OutOfMemory(uint64 Size, uint32 Alignment)
	{
		// this is expected not to return
		FPlatformMemory::OnOutOfMemory(Size, Alignment);
	}

public:
	// FMalloc interface.
	FMallocTBB() :
		MemTime		( 0.0 )
	{
	}
	
	/** 
	 * Malloc
	 */
	virtual void* Malloc( SIZE_T Size, uint32 Alignment ) OVERRIDE;
	
	/** 
	 * Realloc
	 */
	virtual void* Realloc( void* Ptr, SIZE_T NewSize, uint32 Alignment ) OVERRIDE;
	
	/** 
	 * Free
	 */
	virtual void Free( void* Ptr ) OVERRIDE;
	
	/**
	 * Dumps details about all allocations to an output device
	 *
	 * @param Ar	[in] Output device
	 */
	virtual void DumpAllocations( FOutputDevice& Ar ) OVERRIDE;

	/**
	 * Returns stats about current memory usage.
	 *
	 * @param FMemoryAllocationStats_DEPRECATED	[out] structure containing information about the size of allocations
	 */
	CORE_API virtual void GetAllocationInfo( FMemoryAllocationStats_DEPRECATED& MemStats ) OVERRIDE;

	/**
	 * If possible determine the size of the memory allocated at the given address
	 *
 	 * @param Original - Pointer to memory we are checking the size of
	 * @param SizeOut - If possible, this value is set to the size of the passed in pointer
	 * @return true if succeeded
	 */
	virtual bool GetAllocationSize(void *Original, SIZE_T &SizeOut) OVERRIDE;

	/**
	 * Returns if the allocator is guaranteed to be thread-safe and therefore
	 * doesn't need a unnecessary thread-safety wrapper around it.
	 */
	virtual bool IsInternallyThreadSafe() const OVERRIDE
	{ 
		return true;
	}

	virtual const TCHAR * GetDescriptiveName() OVERRIDE { return TEXT("TBB"); }
};

