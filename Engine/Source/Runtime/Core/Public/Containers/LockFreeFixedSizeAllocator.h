// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LockFreeFixedSizeAllocator.h: A lock free pooled allocator for fixed size blocks of a particular type
=============================================================================*/

#pragma once

#include "LockFreeList.h"

/** If this is set to one, then the current allocation state is monitored with thread safe pointers **/
#define MONITOR_FIXED_ALLOCATION (0)

#define USE_RECYCLING (1)

#if !USE_RECYCLING

/** Thread safe, lock free pooling allocator of fixed size blocks that never returns free space until program shutdown. **/
template<int32 SIZE>
class TLockFreeFixedSizeAllocator	// alignment isn't handled, assumes FMemory::Malloc will work
{
public:
	/** Returns a memory block of size SIZE **/
	void* Allocate()
	{
		return FMemory::Malloc(SIZE);
	}
	/** Given a memory block (obtained from Allocate) free it **/
	void Free(void *Item)
	{
		FMemory::Free(Item);
	}
};


#else
/** Thread safe, lock free pooling allocator of fixed size blocks that never returns free space until program shutdown. **/
template<int32 SIZE>
class TLockFreeFixedSizeAllocator	// alignment isn't handled, assumes FMemory::Malloc will work
{
public:
	/** Destructor, returns all memory via FMemory::Free **/
	~TLockFreeFixedSizeAllocator()
	{
		check(!NumUsed.GetValue());
		while (void* Mem = FreeList.Pop())
		{
			FMemory::Free(Mem);
		}
		check(!NumFree.GetValue());
	}
	/** Returns a memory block of size SIZE **/
	void* Allocate()
	{
		NumUsed.Increment();
		void *Memory = FreeList.Pop();
		if (Memory)
		{
			verify(NumFree.Decrement() >= 0);
		}
		else
		{
			Memory = FMemory::Malloc(SIZE);
		}
		return Memory;
	}
	/** Given a memory block (obtained from Allocate) put it on the free list for future use. **/
	void Free(void *Item)
	{
		NumUsed.Decrement();
		FreeList.Push(Item);
		NumFree.Increment();
	}
private:
	/** Lock free list of free memory blocks **/
	TLockFreePointerList<void>		FreeList;

#if MONITOR_FIXED_ALLOCATION
	/** Total number of blocks outstanding and not in the free list **/
	FThreadSafeCounter	NumUsed; 
	/** Total number of blocks in the free list **/
	FThreadSafeCounter	NumFree;
#else
	/** If we aren't monitoring allocation, these do nothing **/
	FNoopCounter		NumUsed; 
	FNoopCounter		NumFree;
#endif
};
#endif

/** Thread safe, lock free pooling allocator of memory for instances of T. Never returns free space until program shutdown. **/
template<class T>
class TLockFreeClassAllocator : private TLockFreeFixedSizeAllocator<sizeof(T)>
{
public:
	/** Returns a memory block of size sizeof(T) **/
	void* Allocate()
	{
		return TLockFreeFixedSizeAllocator<sizeof(T)>::Allocate();
	}
	/** Returns a new T using the default contructor **/
	T* New()
	{
		return new (Allocate()) T();
	}
	/** Calls a destructor on Item and returns the memory to the free list for recycling. **/
	void Free(T *Item)
	{
		Item->~T();
		TLockFreeFixedSizeAllocator<sizeof(T)>::Free(Item);
	}
};

