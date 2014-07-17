// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MemStack.cpp: Unreal memory grabbing functions
=============================================================================*/

#include "Core.h"

DECLARE_THREAD_SINGLETON( FMemStack );

DECLARE_MEMORY_STAT(TEXT("MemStack Large Block"), STAT_MemStackLargeBLock,STATGROUP_Memory);
DECLARE_MEMORY_STAT(TEXT("PageAllocator Free"), STAT_PageAllocatorFree, STATGROUP_Memory);
DECLARE_MEMORY_STAT(TEXT("PageAllocator Used"), STAT_PageAllocatorUsed, STATGROUP_Memory);


TLockFreeFixedSizeAllocator<FPageAllocator::PageSize, FThreadSafeCounter> FPageAllocator::TheAllocator;

#if STATS
void FPageAllocator::UpdateStats()
{
	SET_MEMORY_STAT(STAT_PageAllocatorFree, BytesFree());
	SET_MEMORY_STAT(STAT_PageAllocatorUsed, BytesUsed());
}
#endif
/*-----------------------------------------------------------------------------
	FMemStack implementation.
-----------------------------------------------------------------------------*/

FMemStack::FMemStack()
:	Top(nullptr)
,	End(nullptr)
,	TopChunk(nullptr)
,	TopMark(nullptr)
,	NumMarks(0)
{
}

FMemStack::~FMemStack()
{
	check((GIsCriticalError || !NumMarks) && IsEmpty());
}

int32 FMemStack::GetByteCount() const
{
	int32 Count = 0;
	for( FTaggedMemory* Chunk=TopChunk; Chunk; Chunk=Chunk->Next )
	{
		if( Chunk!=TopChunk )
		{
			Count += Chunk->DataSize;
		}
		else
		{
			Count += Top - Chunk->Data;
		}
	}
	return Count;
}

void FMemStack::AllocateNewChunk( int32 MinSize )
{
	FTaggedMemory* Chunk=nullptr;
	// Create new chunk.
	const int32 DataSize = AlignArbitrary<int32>(MinSize + (int32)sizeof(FTaggedMemory), FPageAllocator::PageSize) - sizeof(FTaggedMemory);
	const uint32 AllocSize = DataSize + sizeof(FTaggedMemory);
	if (AllocSize == FPageAllocator::PageSize)
	{
		Chunk = (FTaggedMemory*)FPageAllocator::Alloc();
	}
	else
	{
		Chunk = (FTaggedMemory*)FMemory::Malloc(AllocSize);
		INC_MEMORY_STAT_BY(STAT_MemStackLargeBLock, AllocSize);
	}
	Chunk->DataSize        = DataSize;

	Chunk->Next = TopChunk;
	TopChunk    = Chunk;
	Top         = Chunk->Data;
	End         = Top + Chunk->DataSize;
}

void FMemStack::FreeChunks( FTaggedMemory* NewTopChunk )
{
	while( TopChunk!=NewTopChunk )
	{
		FTaggedMemory* RemoveChunk = TopChunk;
		TopChunk                   = TopChunk->Next;

		if (RemoveChunk->DataSize + sizeof(FTaggedMemory) == FPageAllocator::PageSize)
		{
			FPageAllocator::Free(RemoveChunk);
		}
		else
		{
			DEC_MEMORY_STAT_BY(STAT_MemStackLargeBLock, RemoveChunk->DataSize + sizeof(FTaggedMemory));
			FMemory::Free(RemoveChunk);
		}

	}
	Top = nullptr;
	End = nullptr;
	if( TopChunk )
	{
		Top = TopChunk->Data;
		End = Top + TopChunk->DataSize;
	}
}

bool FMemStack::ContainsPointer( const void* Pointer ) const
{
	const uint8* Ptr = (const uint8*)Pointer;
	for (const FTaggedMemory* Chunk = TopChunk; Chunk; Chunk = Chunk->Next)
	{
		if (Ptr >= Chunk->Data && Ptr < Chunk->Data + Chunk->DataSize)
		{
			return true;
		}
	}

	return false;
}
