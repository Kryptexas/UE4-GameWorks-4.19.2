// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	FMallocThreadSafeProxy.h: FMalloc proxy used to render any FMalloc thread
							  safe.
=============================================================================*/

#pragma once

/**
 * FMalloc proxy that synchronizes access, making the used malloc thread safe.
 */
class FMallocThreadSafeProxy : public FMalloc
{
private:
	/** Malloc we're based on, aka using under the hood							*/
	FMalloc*			UsedMalloc;
	/** Object used for synchronization via a scoped lock						*/
	FCriticalSection	SynchronizationObject;

public:
	/**
	 * Constructor for thread safe proxy malloc that takes a malloc to be used and a
	 * synchronization object used via FScopeLock as a parameter.
	 * 
	 * @param	InMalloc					FMalloc that is going to be used for actual allocations
	 */
	FMallocThreadSafeProxy( FMalloc* InMalloc)
	:	UsedMalloc( InMalloc )
	{}

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
	virtual SIZE_T QuantizeSize( SIZE_T Size, uint32 Alignment ) OVERRIDE
	{
		return UsedMalloc->QuantizeSize(Size,Alignment); 
	}
	/** 
	 * Malloc
	 */
	void* Malloc( SIZE_T Size, uint32 Alignment ) OVERRIDE
	{
		FScopeLock ScopeLock( &SynchronizationObject );
		STAT(TotalMallocCalls++);
		return UsedMalloc->Malloc( Size, Alignment );
	}

	/** 
	 * Realloc
	 */
	void* Realloc( void* Ptr, SIZE_T NewSize, uint32 Alignment ) OVERRIDE
	{
		FScopeLock ScopeLock( &SynchronizationObject );
		STAT(TotalReallocCalls++);
		return UsedMalloc->Realloc( Ptr, NewSize, Alignment );
	}

	/** 
	 * Free
	 */
	void Free( void* Ptr ) OVERRIDE
	{
		if( Ptr )
		{
			FScopeLock ScopeLock( &SynchronizationObject );
			STAT(TotalFreeCalls++);
			UsedMalloc->Free( Ptr );
		}
	}

	/**
	 * Passes request for gathering memory allocations for both virtual and physical allocations
	 * on to used memory manager.
	 *
	 * @param FMemoryAllocationStats_DEPRECATED	[out] structure containing information about the size of allocations
	 */
	void GetAllocationInfo( FMemoryAllocationStats_DEPRECATED& MemStats ) OVERRIDE
	{
		FScopeLock ScopeLock( &SynchronizationObject );
		UsedMalloc->GetAllocationInfo( MemStats );
	}

	/**
	 * Dumps details about all allocations to an output device
	 *
	 * @param Ar	[in] Output device
	 */
	virtual void DumpAllocations( class FOutputDevice& Ar ) OVERRIDE
	{
		FScopeLock Lock( &SynchronizationObject );
		UsedMalloc->DumpAllocations( Ar );
	}

	/**
	 * Validates the allocator's heap
	 */
	virtual bool ValidateHeap() OVERRIDE
	{
		FScopeLock Lock( &SynchronizationObject );
		return( UsedMalloc->ValidateHeap() );
	}

	bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar ) OVERRIDE
	{
		FScopeLock ScopeLock( &SynchronizationObject );
		return UsedMalloc->Exec( InWorld, Cmd, Ar);
	}


	/** Called every game thread tick */
	void Tick( float DeltaTime ) OVERRIDE
	{
		FScopeLock ScopeLock( &SynchronizationObject );
		return UsedMalloc->Tick( DeltaTime );
	}

	/**
	* If possible determine the size of the memory allocated at the given address
	*
	* @param Original - Pointer to memory we are checking the size of
	* @param SizeOut - If possible, this value is set to the size of the passed in pointer
	* @return true if succeeded
	*/
	bool GetAllocationSize(void *Original, SIZE_T &SizeOut) OVERRIDE
	{
		FScopeLock ScopeLock( &SynchronizationObject );
		return UsedMalloc->GetAllocationSize(Original,SizeOut);
	}

	virtual const TCHAR * GetDescriptiveName() OVERRIDE
	{ 
		FScopeLock ScopeLock( &SynchronizationObject );
		check(UsedMalloc);
		return UsedMalloc->GetDescriptiveName(); 
	}
};

