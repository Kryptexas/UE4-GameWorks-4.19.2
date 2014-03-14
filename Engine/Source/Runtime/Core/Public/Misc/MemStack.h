// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MemStack.h: FMemStack class, ultra-fast temporary memory allocation
=============================================================================*/

#pragma once

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

// Enums for specifying memory allocation type.
enum EMemZeroed {MEM_Zeroed=1};
enum EMemOned   {MEM_Oned  =1};

/**
 * Simple linear-allocation memory stack.
 * Items are allocated via PushBytes() or the specialized operator new()s.
 * Items are freed en masse by using FMemMark to Pop() them.
 **/
class CORE_API FMemStack : public FThreadSingleton<FMemStack>
{
public:

	/** Initialization constructor. */
	FMemStack(int32 DefaultChunkSize = DEFAULT_CHUNK_SIZE);

	/** Destructor. */
	~FMemStack();

	// Get bytes.
	FORCEINLINE uint8* PushBytes( int32 AllocSize, int32 Alignment )
	{
		Alignment = FMath::Max(AllocSize >= 16 ? (int32)16 : (int32)8, Alignment);

		// Debug checks.
		checkSlow(AllocSize>=0);
		checkSlow((Alignment&(Alignment-1))==0);
		checkSlow(Top<=End);
		checkSlow(NumMarks > 0);

		// Try to get memory from the current chunk.
		uint8* Result = Align( Top, Alignment );
		uint8* NewTop = Result + AllocSize;

		// Make sure we didn't overflow.
		if ( NewTop <= End )
		{
			Top	= NewTop;
		}
		else
		{
			// We'd pass the end of the current chunk, so allocate a new one.
			AllocateNewChunk( AllocSize + Alignment );
			Result = Align( Top, Alignment );
			NewTop = Result + AllocSize;
			Top = NewTop;
		}
		return Result;
	}

	/** Timer tick. Makes sure the memory stack is empty. */
	void Tick() const;

	/** @return the number of bytes allocated for this FMemStack that are currently in use. */
	int32 GetByteCount() const;

	/** @return the number of bytes allocated for this FMemStack that are currently unused and available. */
	int32 GetUnusedByteCount() const;

	// Returns true if the pointer was allocated using this allocator
	bool ContainsPointer(const void* Pointer) const;

	// Friends.
	friend class FMemMark;
	friend void* operator new( size_t Size, FMemStack& Mem, int32 Count, int32 Align );
	friend void* operator new( size_t Size, FMemStack& Mem, EMemZeroed Tag, int32 Count, int32 Align );
	friend void* operator new( size_t Size, FMemStack& Mem, EMemOned Tag, int32 Count, int32 Align );
	friend void* operator new[]( size_t Size, FMemStack& Mem, int32 Count, int32 Align );
	friend void* operator new[]( size_t Size, FMemStack& Mem, EMemZeroed Tag, int32 Count, int32 Align );
	friend void* operator new[]( size_t Size, FMemStack& Mem, EMemOned Tag, int32 Count, int32 Align );

	// Types.
	struct FTaggedMemory
	{
		FTaggedMemory* Next;
		int32 DataSize;
		uint8 Data[1];
	};

private:
	// Constants.
	enum 
	{
		MAX_CHUNKS=1024,
		DEFAULT_CHUNK_SIZE=65536,
	};

	// Variables.
	uint8*			Top;				// Top of current chunk (Top<=End).
	uint8*			End;				// End of current chunk.
	int32			DefaultChunkSize;	// Maximum chunk size to allocate.
	FTaggedMemory*	TopChunk;			// Only chunks 0..ActiveChunks-1 are valid.

	/** The top mark on the stack. */
	class FMemMark*	TopMark;

	/** The memory chunks that have been allocated but are currently unused. */
	FTaggedMemory*	UnusedChunks;

	/** The number of marks on this stack. */
	int32 NumMarks;

	/**
	 * Allocate a new chunk of memory of at least MinSize size,
	 * updates the memory stack's Chunks table and ActiveChunks counter.
	 */
	void AllocateNewChunk( int32 MinSize );

	/** Frees the chunks above the specified chunk on the stack. */
	void FreeChunks( FTaggedMemory* NewTopChunk );
};

/*-----------------------------------------------------------------------------
	FMemStack templates.
-----------------------------------------------------------------------------*/

// Operator new for typesafe memory stack allocation.
template <class T> inline T* New( FMemStack& Mem, int32 Count=1, int32 Align=DEFAULT_ALIGNMENT )
{
	return (T*)Mem.PushBytes( Count*sizeof(T), Align );
}
template <class T> inline T* NewZeroed( FMemStack& Mem, int32 Count=1, int32 Align=DEFAULT_ALIGNMENT )
{
	uint8* Result = Mem.PushBytes( Count*sizeof(T), Align );
	FMemory::Memzero( Result, Count*sizeof(T) );
	return (T*)Result;
}
template <class T> inline T* NewOned( FMemStack& Mem, int32 Count=1, int32 Align=DEFAULT_ALIGNMENT )
{
	uint8* Result = Mem.PushBytes( Count*sizeof(T), Align );
	FMemory::Memset( Result, 0xff, Count*sizeof(T) );
	return (T*)Result;
}

/*-----------------------------------------------------------------------------
	FMemStack operator new's.
-----------------------------------------------------------------------------*/

// Operator new for typesafe memory stack allocation.
inline void* operator new( size_t Size, FMemStack& Mem, int32 Count=1, int32 Align=DEFAULT_ALIGNMENT )
{
	// Get uninitialized memory.
	return Mem.PushBytes( Size*Count, Align );
}
inline void* operator new( size_t Size, FMemStack& Mem, EMemZeroed Tag, int32 Count=1, int32 Align=DEFAULT_ALIGNMENT )
{
	// Get zero-filled memory.
	uint8* Result = Mem.PushBytes( Size*Count, Align );
	FMemory::Memzero( Result, Size*Count );
	return Result;
}
inline void* operator new( size_t Size, FMemStack& Mem, EMemOned Tag, int32 Count=1, int32 Align=DEFAULT_ALIGNMENT )
{
	// Get one-filled memory.
	uint8* Result = Mem.PushBytes( Size*Count, Align );
	FMemory::Memset( Result, 0xff, Size*Count );
	return Result;
}
inline void* operator new[]( size_t Size, FMemStack& Mem, int32 Count=1, int32 Align=DEFAULT_ALIGNMENT )
{
	// Get uninitialized memory.
	return Mem.PushBytes( Size*Count, Align );
}
inline void* operator new[]( size_t Size, FMemStack& Mem, EMemZeroed Tag, int32 Count=1, int32 Align=DEFAULT_ALIGNMENT )
{
	// Get zero-filled memory.
	uint8* Result = Mem.PushBytes( Size*Count, Align );
	FMemory::Memzero( Result, Size*Count );
	return Result;
}
inline void* operator new[]( size_t Size, FMemStack& Mem, EMemOned Tag, int32 Count=1, int32 Align=DEFAULT_ALIGNMENT )
{
	// Get one-filled memory.
	uint8* Result = Mem.PushBytes( Size*Count, Align );
	FMemory::Memset( Result, 0xff, Size*Count );
	return Result;
}

/** A container allocator that allocates from a mem-stack. */
template<uint32 Alignment = DEFAULT_ALIGNMENT>
class TMemStackAllocator
{
public:

	enum { NeedsElementType = true };
	enum { RequireRangeCheck = true };

	template<typename ElementType>
	class ForElementType
	{
	public:

		/** Default constructor. */
		ForElementType():
			Data(NULL)
		{}

		// FContainerAllocatorInterface
		FORCEINLINE ElementType* GetAllocation() const
		{
			return Data;
		}
		void ResizeAllocation(int32 PreviousNumElements,int32 NumElements,int32 NumBytesPerElement)
		{
			void* OldData = Data;
			if( NumElements )
			{
				// Allocate memory from the stack.
				Data = (ElementType*)FMemStack::Get().PushBytes(
					NumElements * NumBytesPerElement,
					FMath::Max(Alignment,(uint32)ALIGNOF(ElementType))
					);

				// If the container previously held elements, copy them into the new allocation.
				if(OldData && PreviousNumElements)
				{
					const int32 NumCopiedElements = FMath::Min(NumElements,PreviousNumElements);
					FMemory::Memcpy(Data,OldData,NumCopiedElements * NumBytesPerElement);
				}
			}
		}
		int32 CalculateSlack(int32 NumElements,int32 NumAllocatedElements,int32 NumBytesPerElement) const
		{
			return DefaultCalculateSlack(NumElements,NumAllocatedElements,NumBytesPerElement);
		}

		int32 GetAllocatedSize(int32 NumAllocatedElements, int32 NumBytesPerElement) const
		{
			return NumAllocatedElements * NumBytesPerElement;
		}
			
	private:

		/** A pointer to the container's elements. */
		ElementType* Data;
	};
	
	typedef ForElementType<FScriptContainerElement> ForAnyElementType;
};

/**
 * FMemMark marks a top-of-stack position in the memory stack.
 * When the marker is constructed or initialized with a particular memory 
 * stack, it saves the stack's current position. When marker is popped, it
 * pops all items that were added to the stack subsequent to initialization.
 */
class FMemMark
{
public:
	// Constructors.
	FMemMark( FMemStack& InMem )
	:	Mem(InMem)
	,	Top(InMem.Top)
	,	SavedChunk(InMem.TopChunk)
	,	bPopped(false)
	,	NextTopmostMark(InMem.TopMark)
	{
		Mem.TopMark = this;

		// Track the number of outstanding marks on the stack.
		Mem.NumMarks++;
	}

	/** Destructor. */
	~FMemMark()
	{
		Pop();
	}

	/** Free the memory allocated after the mark was created. */
	void Pop()
	{
		if(!bPopped)
		{
			check(Mem.TopMark == this);
			bPopped = true;

			// Track the number of outstanding marks on the stack.
			--Mem.NumMarks;

			// Unlock any new chunks that were allocated.
			if( SavedChunk != Mem.TopChunk )
			{
				Mem.FreeChunks( SavedChunk );
			}

			// Restore the memory stack's state.
			Mem.Top = Top;
			Mem.TopMark = NextTopmostMark;

			// Ensure that the mark is only popped once by clearing the top pointer.
			Top = NULL;
		}
	}

private:
	// Implementation variables.
	FMemStack& Mem;
	uint8* Top;
	FMemStack::FTaggedMemory* SavedChunk;
	bool bPopped;
	FMemMark* NextTopmostMark;
};
