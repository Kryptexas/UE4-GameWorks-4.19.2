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
	virtual SIZE_T QuantizeSize( SIZE_T Size, uint32 Alignment ) override
	{
		return UsedMalloc->QuantizeSize(Size,Alignment); 
	}
	/** 
	 * Malloc
	 */
	void* Malloc( SIZE_T Size, uint32 Alignment ) override
	{
		FScopeLock ScopeLock( &SynchronizationObject );
		STAT(TotalMallocCalls++);
		return UsedMalloc->Malloc( Size, Alignment );
	}

	/** 
	 * Realloc
	 */
	void* Realloc( void* Ptr, SIZE_T NewSize, uint32 Alignment ) override
	{
		FScopeLock ScopeLock( &SynchronizationObject );
		STAT(TotalReallocCalls++);
		return UsedMalloc->Realloc( Ptr, NewSize, Alignment );
	}

	/** 
	 * Free
	 */
	void Free( void* Ptr ) override
	{
		if( Ptr )
		{
			FScopeLock ScopeLock( &SynchronizationObject );
			STAT(TotalFreeCalls++);
			UsedMalloc->Free( Ptr );
		}
	}

	/** Called once per frame, gathers and sets all memory allocator statistics into the corresponding stats. */
	virtual void UpdateStats()
	{
		FScopeLock ScopeLock( &SynchronizationObject );
		UsedMalloc->UpdateStats();
	}

	/** Writes allocator stats from the last update into the specified destination. */
	virtual void GetAllocatorStats( FGenericMemoryStats& out_Stats )
	{
		FScopeLock ScopeLock( &SynchronizationObject );
		UsedMalloc->GetAllocatorStats( out_Stats );
	}

	/** Dumps allocator stats to an output device. */
	virtual void DumpAllocatorStats( class FOutputDevice& Ar )
	{
		FScopeLock ScopeLock( &SynchronizationObject );
		UsedMalloc->DumpAllocatorStats( Ar );
	}

	/**
	 * Validates the allocator's heap
	 */
	virtual bool ValidateHeap() override
	{
		FScopeLock Lock( &SynchronizationObject );
		return( UsedMalloc->ValidateHeap() );
	}

	bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar ) override
	{
		FScopeLock ScopeLock( &SynchronizationObject );
		return UsedMalloc->Exec( InWorld, Cmd, Ar);
	}

	/**
	* If possible determine the size of the memory allocated at the given address
	*
	* @param Original - Pointer to memory we are checking the size of
	* @param SizeOut - If possible, this value is set to the size of the passed in pointer
	* @return true if succeeded
	*/
	bool GetAllocationSize(void *Original, SIZE_T &SizeOut) override
	{
		FScopeLock ScopeLock( &SynchronizationObject );
		return UsedMalloc->GetAllocationSize(Original,SizeOut);
	}

	virtual const TCHAR * GetDescriptiveName() override
	{ 
		FScopeLock ScopeLock( &SynchronizationObject );
		check(UsedMalloc);
		return UsedMalloc->GetDescriptiveName(); 
	}
};

