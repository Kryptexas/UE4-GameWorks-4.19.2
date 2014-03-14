// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MemStack.cpp: Unreal memory grabbing functions
=============================================================================*/

#include "CorePrivate.h"

static struct FForceInitAtBootFMemStack : public TForceInitAtBoot<FMemStack>
{} FForceInitAtBootFMemStack;

template<> uint32 FThreadSingleton<FMemStack>::TlsSlot = 0;

DECLARE_MEMORY_STAT(TEXT("MemStack Allocated (all threads)"), STAT_MemStackAllocated,STATGROUP_Memory);
DECLARE_MEMORY_STAT(TEXT("MemStack Used (all threads)"), STAT_MemStackUsed,STATGROUP_Memory);

/*-----------------------------------------------------------------------------
	FMemStack implementation.
-----------------------------------------------------------------------------*/

FMemStack::FMemStack( int32 InDefaultChunkSize /*= DEFAULT_CHUNK_SIZE*/ )
:	Top(NULL)
,	End(NULL)
,	DefaultChunkSize(InDefaultChunkSize)
,	TopChunk(NULL)
,	TopMark(NULL)
,	UnusedChunks(NULL)
,	NumMarks(0)
{
}

FMemStack::~FMemStack()
{
	check(GIsCriticalError || !NumMarks);

	Tick();
	while( UnusedChunks )
	{
		FTaggedMemory* Old = UnusedChunks;
		UnusedChunks = UnusedChunks->Next;
		DEC_MEMORY_STAT_BY( STAT_MemStackAllocated, Old->DataSize + sizeof(FTaggedMemory) );
		FMemory::Free( Old );
	}
}

void FMemStack::Tick() const
{
	check(TopChunk==NULL);
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
int32 FMemStack::GetUnusedByteCount() const
{
	int32 Count = 0;
	for( FTaggedMemory* Chunk=UnusedChunks; Chunk; Chunk=Chunk->Next )
	{
		Count += Chunk->DataSize;
	}
	return Count;
}

void FMemStack::AllocateNewChunk( int32 MinSize )
{
	FTaggedMemory* Chunk=NULL;
	for( FTaggedMemory** Link=&UnusedChunks; *Link; Link=&(*Link)->Next )
	{
		// Find existing chunk.
		if( (*Link)->DataSize >= MinSize )
		{
			Chunk = *Link;
			*Link = (*Link)->Next;
			break;
		}
	}

	if( !Chunk )
	{
		// Create new chunk.
		const int32 DataSize   = AlignArbitrary<int32>( MinSize + (int32)sizeof(FTaggedMemory), DefaultChunkSize ) - sizeof(FTaggedMemory);
		const uint32 AllocSize = DataSize + sizeof(FTaggedMemory);
		Chunk                  = (FTaggedMemory*)FMemory::Malloc( AllocSize );
		Chunk->DataSize        = DataSize;
		INC_MEMORY_STAT_BY( STAT_MemStackAllocated, AllocSize );	
	}

	if( Chunk != TopChunk )
	{
		INC_MEMORY_STAT_BY( STAT_MemStackUsed, Chunk->DataSize );
	}

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
		RemoveChunk->Next          = UnusedChunks;
		UnusedChunks               = RemoveChunk;

		DEC_MEMORY_STAT_BY( STAT_MemStackUsed, RemoveChunk->DataSize );
	}
	Top = NULL;
	End = NULL;
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
