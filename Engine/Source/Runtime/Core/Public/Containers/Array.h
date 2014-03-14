// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Array.h: Dynamic array definitions.
=============================================================================*/

#pragma once

#define DEBUG_HEAP 0

#include "Core.h"

/**
 * Generic iterator which can operate on types that expose the following:
 * - A type called ElementType representing the contained type.
 * - A method IndexType Num() const that returns the number of items in the container.
 * - A method bool IsValidIndex(IndexType index) which returns whether a given index is valid in the container.
 * - A method T& operator[](IndexType index) which returns a reference to a contained object by index.
 */
template< typename ContainerType, typename ElementType, typename IndexType>
class TIndexedContainerIterator
{
public:
	TIndexedContainerIterator(ContainerType& InContainer, IndexType StartIndex = 0)
		: Container(InContainer)
		, Index    (StartIndex)
	{
	}

	/** Advances iterator to the next element in the container. */
	TIndexedContainerIterator& operator++()
	{
		++Index;
		return *this;
	}
	TIndexedContainerIterator operator++(int)
	{
		TIndexedContainerIterator Tmp(*this);
		++Index;
		return Tmp;
	}

	/** Moves iterator to the previous element in the container. */
	TIndexedContainerIterator& operator--()
	{
		--Index;
		return *this;
	}
	TIndexedContainerIterator operator--(int)
	{
		TIndexedContainerIterator Tmp(*this);
		--Index;
		return Tmp;
	}

	/** iterator arithmetic support */
	TIndexedContainerIterator& operator+=(int32 Offset)
	{
		Index += Offset;
		return *this;
	}

	TIndexedContainerIterator operator+(int32 Offset) const
	{
		TIndexedContainerIterator Tmp(*this);
		return Tmp += Offset;
	}

	TIndexedContainerIterator& operator-=(int32 Offset)
	{
		return *this += -Offset;
	}

	TIndexedContainerIterator operator-(int32 Offset) const
	{
		TIndexedContainerIterator Tmp(*this);
		return Tmp -= Offset;
	}

	/** @name Element access */
	//@{
	ElementType& operator* () const
	{
		return Container[ Index ];
	}

	ElementType* operator-> () const
	{
		return &Container[ Index ];
	}
	//@}

	/** conversion to "bool" returning true if the iterator has not reached the last element. */
	FORCEINLINE_EXPLICIT_OPERATOR_BOOL() const
	{
		return Container.IsValidIndex(Index);
	}
	/** inverse of the "bool" operator */
	FORCEINLINE bool operator !() const 
	{
		return !(bool)*this;
	}

	/** Returns an index to the current element. */
	IndexType GetIndex() const
	{
		return Index;
	}

	/** Resets the iterator to the first element. */
	void Reset()
	{
		Index = 0;
	}

	FORCEINLINE friend bool operator==(const TIndexedContainerIterator& Lhs, const TIndexedContainerIterator& Rhs) { return &Lhs.Container == &Rhs.Container && Lhs.Index == Rhs.Index; }
	FORCEINLINE friend bool operator!=(const TIndexedContainerIterator& Lhs, const TIndexedContainerIterator& Rhs) { return &Lhs.Container != &Rhs.Container || Lhs.Index != Rhs.Index; }

private:
	ContainerType& Container;
	IndexType      Index;
};

/** operator + */
template <typename ContainerType, typename ElementType, typename IndexType>
FORCEINLINE TIndexedContainerIterator<ContainerType, ElementType, IndexType> operator+(int32 Offset, TIndexedContainerIterator<ContainerType, ElementType, IndexType> RHS)
{
	return RHS + Offset;
}

/**
 * Base dynamic array.
 * An untyped data array; mirrors a TArray's members, but doesn't need an exact C++ type for its elements.
 **/
class FScriptArray : protected FHeapAllocator::ForAnyElementType
{
public:
	void* GetData()
	{
		return this->GetAllocation();
	}
	const void* GetData() const
	{
		return this->GetAllocation();
	}
	bool IsValidIndex( int32 i ) const
	{
		return i>=0 && i<ArrayNum;
	}
	FORCEINLINE int32 Num() const
	{
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		return ArrayNum;
	}
	void InsertZeroed( int32 Index, int32 Count, int32 NumBytesPerElement )
	{
		Insert( Index, Count, NumBytesPerElement );
		FMemory::Memzero( (uint8*)this->GetAllocation()+Index*NumBytesPerElement, Count*NumBytesPerElement );
	}
	void Insert( int32 Index, int32 Count, int32 NumBytesPerElement )
	{
		check(Count>=0);
		check(ArrayNum>=0);
		check(ArrayMax>=ArrayNum);
		check(Index>=0);
		check(Index<=ArrayNum);

		const int32 OldNum = ArrayNum;
		if( (ArrayNum+=Count)>ArrayMax )
		{
			ArrayMax = this->CalculateSlack( ArrayNum, ArrayMax, NumBytesPerElement );
			this->ResizeAllocation(OldNum,ArrayMax,NumBytesPerElement);
		}
		FMemory::Memmove
		(
			(uint8*)this->GetAllocation() + (Index+Count )*NumBytesPerElement,
			(uint8*)this->GetAllocation() + (Index       )*NumBytesPerElement,
			                                               (OldNum-Index)*NumBytesPerElement
		);
	}
	int32 Add( int32 Count, int32 NumBytesPerElement )
	{
		check(Count>=0);
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);

		const int32 OldNum = ArrayNum;
		if( (ArrayNum+=Count)>ArrayMax )
		{
			ArrayMax = this->CalculateSlack( ArrayNum, ArrayMax, NumBytesPerElement );
			this->ResizeAllocation(OldNum,ArrayMax,NumBytesPerElement);
		}

		return OldNum;
	}
	int32 AddZeroed( int32 Count, int32 NumBytesPerElement )
	{
		const int32 Index = Add( Count, NumBytesPerElement );
		FMemory::Memzero( (uint8*)this->GetAllocation()+Index*NumBytesPerElement, Count*NumBytesPerElement );
		return Index;
	}
	void Shrink( int32 NumBytesPerElement )
	{
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		if( ArrayMax != ArrayNum )
		{
			ArrayMax = ArrayNum;
			this->ResizeAllocation(ArrayNum,ArrayMax,NumBytesPerElement);
		}
	}
	void Empty( int32 Slack, int32 NumBytesPerElement )
	{
		checkSlow(Slack>=0);
		ArrayNum = 0;
		// only reallocate if we need to, I don't trust realloc to the same size to work
		if (ArrayMax != Slack)
		{
			ArrayMax = Slack;
			this->ResizeAllocation(0,ArrayMax,NumBytesPerElement);
		}
	}
	void SwapMemory(int32 A, int32 B, int32 NumBytesPerElement )
	{
		FMemory::Memswap(
			(uint8*)this->GetAllocation()+(NumBytesPerElement*A),
			(uint8*)this->GetAllocation()+(NumBytesPerElement*B),
			NumBytesPerElement
			);
	}
	FScriptArray()
	:   ArrayNum( 0 )
	,	ArrayMax( 0 )
	{
	}
	void CountBytes( FArchive& Ar, int32 NumBytesPerElement  )
	{
		Ar.CountBytes( ArrayNum*NumBytesPerElement, ArrayMax*NumBytesPerElement );
	}
	/**
	 * Returns the amount of slack in this array in elements.
	 */
	int32 GetSlack() const
	{
		return ArrayMax - ArrayNum;
	}
		
	void Remove( int32 Index, int32 Count, int32 NumBytesPerElement  )
	{
		checkSlow(Count >= 0);
		checkSlow(Index >= 0); 
		checkSlow(Index <= ArrayNum);
		checkSlow(Index + Count <= ArrayNum);

		// Skip memmove in the common case that there is nothing to move.
		int32 NumToMove = ArrayNum - Index - Count;
		if( NumToMove )
		{
			FMemory::Memmove
			(
				(uint8*)this->GetAllocation() + (Index      ) * NumBytesPerElement,
				(uint8*)this->GetAllocation() + (Index+Count) * NumBytesPerElement,
				NumToMove * NumBytesPerElement
			);
		}
		ArrayNum -= Count;
		
		const int32 NewArrayMax = this->CalculateSlack(ArrayNum,ArrayMax,NumBytesPerElement);
		if(NewArrayMax != ArrayMax)
		{
			ArrayMax = NewArrayMax;
			this->ResizeAllocation(ArrayNum,ArrayMax,NumBytesPerElement);
		}
		checkSlow(ArrayNum >= 0);
		checkSlow(ArrayMax >= ArrayNum);
	}

protected:

	FScriptArray( int32 InNum, int32 NumBytesPerElement  )
	:   ArrayNum( InNum )
	,	ArrayMax( InNum )

	{
		this->ResizeAllocation(0,ArrayMax,NumBytesPerElement);
	}
	int32	  ArrayNum;
	int32	  ArrayMax;

public:
	// These should really be private, because they shouldn't be called, but there's a bunch of code
	// that needs to be fixed first.
	FScriptArray(const FScriptArray&) { check(false); }
	void operator=(const FScriptArray&) { check(false); }
};

template<> struct TIsZeroConstructType<FScriptArray> { enum { Value = true }; };


/**
 * TReversePredicateWrapper class used by implicit heaps. 
 * This is similar to TDereferenceWrapper from Sorting.h except it reverses the comparison at the same time
 */
template <typename ElementType, typename PREDICATE_CLASS>
class TReversePredicateWrapper
{
	const PREDICATE_CLASS& Predicate;
public:
	TReversePredicateWrapper( const PREDICATE_CLASS& InPredicate )
		: Predicate( InPredicate )
	{}

	FORCEINLINE bool operator()( ElementType& A, ElementType& B ) const { return Predicate( B, A ); }
	FORCEINLINE bool operator()( const ElementType& A, const ElementType& B ) const { return Predicate( B, A ); }
};

/**
 * Partially specialized version of the above.
 */
template <typename ElementType, typename PREDICATE_CLASS>
class TReversePredicateWrapper<ElementType*, PREDICATE_CLASS>
{
	const PREDICATE_CLASS& Predicate;
public:
	TReversePredicateWrapper( const PREDICATE_CLASS& InPredicate )
		: Predicate( InPredicate )
	{}

	FORCEINLINE bool operator()( ElementType* A, ElementType* B ) const 
	{
		check( A != NULL );
		check( B != NULL );
		return Predicate( *B, *A ); 
	}
	FORCEINLINE bool operator()( const ElementType* A, const ElementType* B ) const 
	{
		check( A != NULL );
		check( B != NULL );
		return Predicate( *B, *A ); 
	}
};

/**
 * Templated dynamic array
 *
 * A dynamically sized array of typed elements.  Makes the assumption that your elements are relocate-able; 
 * i.e. that they can be transparently moved to new memory without a copy constructor.  The main implication 
 * is that pointers to elements in the TArray may be invalidated by adding or removing other elements to the array. 
 * Removal of elements is O(N) and invalidates the indices of subsequent elements.
 *
 * Caution: as noted below some methods are not safe for element types that require constructors.
 *
 **/
template<typename InElementType, typename Allocator>
class TArray
{
public:
	typedef InElementType ElementType;

	TArray()
	:   ArrayNum( 0 )
	,	ArrayMax( 0 )
	{}

	/**
	 * Copy constructor. Use the common routine to perform the copy
	 *
	 * @param Other the source array to copy
	 */
	template<typename OtherAllocator>
	explicit TArray(const TArray<ElementType,OtherAllocator>& Other)
	{
		CopyToEmpty(Other);
	}

	TArray(const TArray<ElementType,Allocator>& Other)
	{
		CopyToEmpty(Other);
	}

	template<typename OtherAllocator>
	TArray& operator=( const TArray<ElementType,OtherAllocator>& Other )
	{
		DestructItems(GetTypedData(), ArrayNum);
		CopyToEmpty(Other);
		return *this;
	}

	TArray& operator=( const TArray<ElementType,Allocator>& Other )
	{
		if (this != &Other)
		{
			DestructItems(GetTypedData(), ArrayNum);
			CopyToEmpty(Other);
		}
		return *this;
	}

#if PLATFORM_COMPILER_HAS_RVALUE_REFERENCES

private:
	template <typename ArrayType>
	static FORCEINLINE typename TEnableIf<TContainerTraits<ArrayType>::MoveWillEmptyContainer>::Type MoveOrCopy(ArrayType& ToArray, ArrayType& FromArray)
	{
		ToArray.AllocatorInstance.MoveToEmpty(FromArray.AllocatorInstance);

		ToArray  .ArrayNum = FromArray.ArrayNum;
		ToArray  .ArrayMax = FromArray.ArrayMax;
		FromArray.ArrayNum = 0;
		FromArray.ArrayMax = 0;
	}

	template <typename ArrayType>
	static FORCEINLINE typename TEnableIf<!TContainerTraits<ArrayType>::MoveWillEmptyContainer>::Type MoveOrCopy(ArrayType& ToArray, ArrayType& FromArray)
	{
		ToArray.CopyToEmpty(FromArray);
	}

public:
	// We don't implement move semantics for general OtherAllocators, as there's no way
	// to tell if they're compatible with the current one.  Probably going to be a pretty
	// rare requirement anyway.

	TArray(TArray<ElementType,Allocator>&& Other)
	{
		MoveOrCopy(*this, Other);
	}

	TArray& operator=(TArray<ElementType,Allocator>&& Other)
	{
		if (this != &Other)
		{
			DestructItems(GetTypedData(), ArrayNum);
			MoveOrCopy(*this, Other);
		}
		return *this;
	}

#endif

	~TArray()
	{
		DestructItems(GetTypedData(), ArrayNum);

		#if defined(_MSC_VER)
			// ensure that DebugGet gets instantiated.
			//@todo it would be nice if we had a cleaner solution for DebugGet
			volatile const ElementType* Dummy = &DebugGet(0);
		#endif
	}

	/**
	 * Helper function for returning a typed pointer to the first array entry.
	 *
	 * @return pointer to first array entry or NULL if ArrayMax==0
	 */
	FORCEINLINE ElementType* GetTypedData()
	{
		return (ElementType*)AllocatorInstance.GetAllocation();
	}
	FORCEINLINE ElementType* GetData()
	{
		return (ElementType*)AllocatorInstance.GetAllocation();
	}
	/**
	 * Helper function for returning a typed pointer to the first array entry.
	 *
	 * @return pointer to first array entry or NULL if ArrayMax==0
	 */
	FORCEINLINE const ElementType* GetTypedData() const
	{
		return (const ElementType*)AllocatorInstance.GetAllocation();
	}
	FORCEINLINE const ElementType* GetData() const
	{
		return (const ElementType*)AllocatorInstance.GetAllocation();
	}
	/** 
	 * Helper function returning the size of the inner type
	 *
	 * @return size in bytes of array type
	 */
	FORCEINLINE uint32 GetTypeSize() const
	{
		return sizeof(ElementType);
	}

	/** 
	 * Helper function to return the amount of memory allocated by this container 
	 *
	 * @return number of bytes allocated by this container
	 */
	FORCEINLINE uint32 GetAllocatedSize( void ) const
	{
		return AllocatorInstance.GetAllocatedSize(ArrayMax, sizeof(ElementType));
	}

	/**
	 * Returns the amount of slack in this array in elements.
	 */
	int32 GetSlack() const
	{
		return ArrayMax - ArrayNum;
	}

	FORCEINLINE void CheckInvariants() const
	{
		checkSlow((ArrayNum >= 0) & (ArrayMax >= ArrayNum)); // & for one branch
	}
	FORCEINLINE void RangeCheck(int32 Index) const
	{
		CheckInvariants();

		// Template property, branch will be optimized out
		if (Allocator::RequireRangeCheck)
		{
			checkf((Index >= 0) & (Index < ArrayNum),TEXT("Array index out of bounds: %i from an array of size %i"),Index,ArrayNum); // & for one branch
		}
	}

	FORCEINLINE bool IsValidIndex( int32 i ) const
	{
		return i>=0 && i<ArrayNum;
	}
	FORCEINLINE int32 Num() const
	{
		return ArrayNum;
	}
	FORCEINLINE int32 Max() const
	{
		return ArrayMax;
	}

	FORCEINLINE ElementType& operator[]( int32 i )
	{
		RangeCheck(i);
		return GetTypedData()[i];
	}
	FORCEINLINE const ElementType& operator[]( int32 i ) const
	{
		RangeCheck(i);
		return GetTypedData()[i];
	}

	ElementType Pop(bool bAllowShrinking=true)
	{
		RangeCheck(0);
		ElementType Result = MoveTemp(GetTypedData()[ArrayNum-1]);
		RemoveAt( ArrayNum-1, 1, bAllowShrinking );
		return Result;
	}
	void Push( ElementType&& Item )
	{
		Add(MoveTemp(Item));
	}
	void Push( const ElementType& Item )
	{
		Add(Item);
	}
	ElementType& Top()
	{
		return Last();
	}
	const ElementType& Top() const
	{
		return Last();
	}
	ElementType& Last( int32 c=0 )
	{
		RangeCheck(ArrayNum-c-1);
		return GetTypedData()[ArrayNum-c-1];
	}
	const ElementType& Last( int32 c=0 ) const
	{
		RangeCheck(ArrayNum-c-1);
		return GetTypedData()[ArrayNum-c-1];
	}
	void Shrink()
	{
		CheckInvariants();
		if( ArrayMax != ArrayNum )
		{
			ArrayMax = ArrayNum;
			AllocatorInstance.ResizeAllocation(ArrayNum,ArrayMax,sizeof(ElementType));
		}
	}
	FORCEINLINE bool Find( const ElementType& Item, int32& Index ) const
	{
		Index = this->Find(Item);
		return Index != INDEX_NONE;
	}
	int32 Find( const ElementType& Item ) const
	{
		const ElementType* RESTRICT Start = GetTypedData();
		for (const ElementType* RESTRICT Data = Start, * RESTRICT DataEnd = Data + ArrayNum; Data != DataEnd; ++Data)
		{
			if (*Data == Item)
				return (int32)(Data - Start);
		}
		return INDEX_NONE;
	}
	FORCEINLINE bool FindLast( const ElementType& Item, int32& Index ) const
	{
		Index = this->FindLast(Item);
		return Index != INDEX_NONE;
	}
	int32 FindLast( const ElementType& Item ) const
	{
		const ElementType* RESTRICT End = GetTypedData() + ArrayNum;
		for (const ElementType* RESTRICT Data = End, * RESTRICT DataStart = Data - ArrayNum; Data != DataStart; )
		{
			--Data;
			if (*Data == Item)
				return (int32)(Data - DataStart);
		}
		return INDEX_NONE;
	}
	template<typename MatchFunctorType>
	int32 FindMatch( const MatchFunctorType& Matcher ) const
	{
		const ElementType* const RESTRICT DataEnd = GetTypedData() + ArrayNum;
		for(const ElementType* RESTRICT Data = GetTypedData();
			Data < DataEnd;
			Data++
			)
		{
			if( Matcher.Matches(*Data) )
			{
				return (int32)(Data - GetTypedData());
			}
		}
		return INDEX_NONE;
	}
	/**
	 * Finds an item by key (assuming the ElementType overloads operator== for the comparison).
	 * @param Key	The key to search by
	 * @return		Index to the first matching element, or INDEX_NONE if none is found
	 */
	template <typename KeyType>
	int32 IndexOfByKey( const KeyType& Key ) const
	{
		const ElementType* RESTRICT Start = GetTypedData();
		for (const ElementType* RESTRICT Data = Start, * RESTRICT DataEnd = Start + ArrayNum; Data != DataEnd; ++Data)
		{
			if (*Data == Key)
				return (int32)(Data - Start);
		}
		return INDEX_NONE;
	}
	/**
	 * Finds an item by predicate.
	 * @param Pred	The predicate to match
	 * @return		Index to the first matching element, or INDEX_NONE if none is found
	 */
	template <typename Predicate>
	int32 IndexOfByPredicate(Predicate Pred) const
	{
		const ElementType* RESTRICT Start = GetTypedData();
		for (const ElementType* RESTRICT Data = Start, * RESTRICT DataEnd = Start + ArrayNum; Data != DataEnd; ++Data)
		{
			if (Pred(*Data))
				return (int32)(Data - Start);
		}
		return INDEX_NONE;
	}
	/**
	 * Finds an item by key (assuming the ElementType overloads operator== for the comparison).
	 * @param Key	The key to search by
	 * @return		Pointer to the first matching element, or NULL if none is found
	 */
	template <typename KeyType>
	FORCEINLINE const ElementType* FindByKey( const KeyType& Key ) const
	{
		return const_cast<TArray*>(this)->FindByKey(Key);
	}
	/**
	 * Finds an item by key (assuming the ElementType overloads operator== for the comparison).
	 * @param Key	The key to search by
	 * @return		Pointer to the first matching element, or NULL if none is found
	 */
	template <typename KeyType>
	ElementType* FindByKey( const KeyType& Key )
	{
		for (ElementType* RESTRICT Data = GetTypedData(), * RESTRICT DataEnd = Data + ArrayNum; Data != DataEnd; ++Data)
		{
			if (*Data == Key)
				return Data;
		}

		return NULL;
	}
	/**
	 * Finds an element which matches a predicate functor.
	 * @param  Pred The functor to apply to each element.
	 * @return      Pointer to the first element for which the predicate returns true, or NULL if none is found.
	 */
	template <typename Predicate>
	FORCEINLINE const ElementType* FindByPredicate(Predicate Pred) const
	{
		return const_cast<TArray*>(this)->FindByPredicate(Pred);
	}
	/**
	 * Finds an element which matches a predicate functor.
	 * @param  Pred The functor to apply to each element.
	 * @return      Pointer to the first element for which the predicate returns true, or NULL if none is found.
	 */
	template <typename Predicate>
	ElementType* FindByPredicate(Predicate Pred)
	{
		for (ElementType* RESTRICT Data = GetTypedData(), * RESTRICT DataEnd = Data + ArrayNum; Data != DataEnd; ++Data)
		{
			if (Pred(*Data))
				return Data;
		}

		return NULL;
	}
	/**
	 * @return		true if found
	 */
	bool Contains( const ElementType& Item ) const
	{
		for (const ElementType* RESTRICT Data = GetTypedData(), * RESTRICT DataEnd = Data + ArrayNum; Data != DataEnd; ++Data)
		{
			if (*Data == Item)
				return true;
		}
		return false;
	}
	/**
	 * @return		true if found
	 */
	template <typename Predicate>
	FORCEINLINE bool ContainsByPredicate( Predicate Pred ) const
	{
		return FindByPredicate(Pred) != NULL;
	}
	bool operator==(const TArray& OtherArray) const
	{
		int32 Count = Num();

		return Count == OtherArray.Num() && CompareItems(GetTypedData(), OtherArray.GetTypedData(), Count);
	}
	bool operator!=(const TArray& OtherArray) const
	{
		return !(*this == OtherArray);
	}
	friend FArchive& operator<<( FArchive& Ar, TArray& A )
	{
		A.CountBytes( Ar );
		if( sizeof(ElementType)==1 )
		{
			// Serialize simple bytes which require no construction or destruction.
			Ar << A.ArrayNum;
			check( A.ArrayNum >= 0 );
			if( Ar.IsLoading() )
			{
				A.ArrayMax = A.ArrayNum;
				A.AllocatorInstance.ResizeAllocation(0,A.ArrayMax,sizeof(ElementType));
			}
			Ar.Serialize( A.GetData(), A.Num() );
		}
		else if( Ar.IsLoading() )
		{
			// Load array.
			int32 NewNum;
			Ar << NewNum;
			A.Empty( NewNum );
			for( int32 i=0; i<NewNum; i++ )
			{
				Ar << *::new(A)ElementType;
			}
		}
		else
		{
			// Save array.
			Ar << A.ArrayNum;
			for( int32 i=0; i<A.ArrayNum; i++ )
			{
				Ar << A[ i ];
			}
		}
		return Ar;
	}
	/**
	 * Bulk serialize array as a single memory blob when loading. Uses regular serialization code for saving
	 * and doesn't serialize at all otherwise (e.g. transient, garbage collection, ...).
	 * 
	 * Requirements:
	 *   - T's << operator needs to serialize ALL member variables in the SAME order they are layed out in memory.
	 *   - T's << operator can NOT perform any fixup operations. This limitation can be lifted by manually copying
	 *     the code after the BulkSerialize call.
	 *   - T can NOT contain any member variables requiring constructor calls or pointers
	 *   - sizeof(ElementType) must be equal to the sum of sizes of it's member variables.
	 *        - e.g. use pragma pack (push,1)/ (pop) to ensure alignment
	 *        - match up uint8/ WORDs so everything always end up being properly aligned
	 *   - Code can not rely on serialization of T if neither ArIsLoading nor ArIsSaving is true.
	 *   - Can only be called platforms that either have the same endianness as the one the content was saved with
	 *     or had the endian conversion occur in a cooking process like e.g. for consoles.
	 *
	 * Notes:
	 *   - it is safe to call BulkSerialize on TTransArrays
	 *
	 * IMPORTANT:
	 *   - This is Overridden in XeD3dResourceArray.h  Please make certain changes are propogated accordingly
	 *
	 * @param Ar	FArchive to bulk serialize this TArray to/from
	 */
	void BulkSerialize(FArchive& Ar, bool bForcePerElementSerialization = false)
	{
		int32 ElementSize = sizeof(ElementType);
		// Serialize element size to detect mismatch across platforms.
		int32 SerializedElementSize = ElementSize;
		Ar << SerializedElementSize;

		if (bForcePerElementSerialization
			|| (Ar.IsSaving()			// if we are saving, we always do the ordinary serialize as a way to make sure it matches up with bulk serialization
				&& !Ar.IsCooking())		// but cooking is performance critical, so we skip that
			|| Ar.IsByteSwapping()		// if we are byteswapping, we need to do that per-element
			)
		{
			Ar << *this;
		}
		else 
		{
			CountBytes(Ar);
			if (Ar.IsLoading())
			{
				// Basic sanity checking to ensure that sizes match.
				checkf(SerializedElementSize==0 || SerializedElementSize==ElementSize,TEXT("Expected %i, Got: %i"),ElementSize,SerializedElementSize);
				// Serialize the number of elements, block allocate the right amount of memory and deserialize
				// the data as a giant memory blob in a single call to Serialize. Please see the function header
				// for detailed documentation on limitations and implications.
				int32 NewArrayNum;
				Ar << NewArrayNum;
				Empty( NewArrayNum );
				AddUninitialized( NewArrayNum );
				Ar.Serialize( GetData(), NewArrayNum * SerializedElementSize );
			}
			else if (Ar.IsSaving())
			{
				int32 ArrayCount = Num();
				Ar << ArrayCount;
				Ar.Serialize( GetData(), ArrayCount * SerializedElementSize );
			}
		}
	}
	void CountBytes( FArchive& Ar )
	{
		Ar.CountBytes( ArrayNum*sizeof(ElementType), ArrayMax*sizeof(ElementType) );
	}

	// Add, Insert, Remove, Empty interface.
	/** Caution, Add() will create elements without calling the constructor and this is not appropriate for element types that require a constructor to function properly. */
	int32 AddUninitialized( int32 Count=1 )
	{
		CheckInvariants();
		checkSlow(Count>=0);

		const int32 OldNum = ArrayNum;
		if( (ArrayNum+=Count)>ArrayMax )
		{
			ArrayMax = AllocatorInstance.CalculateSlack( ArrayNum, ArrayMax, sizeof(ElementType) );
			AllocatorInstance.ResizeAllocation(OldNum,ArrayMax, sizeof(ElementType));
		}

		return OldNum;
	}
	/** Caution, Insert() will create elements without calling the constructor and this is not appropriate for element types that require a constructor to function properly. */
	void InsertUninitialized( int32 Index, int32 Count=1 )
	{
		CheckInvariants();
		checkSlow((Count>=0) & (Index>=0) & (Index<=ArrayNum));

		const int32 OldNum = ArrayNum;
		if( (ArrayNum+=Count)>ArrayMax )
		{
			ArrayMax = AllocatorInstance.CalculateSlack( ArrayNum, ArrayMax, sizeof(ElementType) );
			AllocatorInstance.ResizeAllocation(OldNum,ArrayMax,sizeof(ElementType));
		}
		FMemory::Memmove
		(
			(uint8*)AllocatorInstance.GetAllocation() + (Index+Count )*sizeof(ElementType),
			(uint8*)AllocatorInstance.GetAllocation() + (Index       )*sizeof(ElementType),
			                                               (OldNum-Index)*sizeof(ElementType)
		);
	}
	/** Caution, InsertZeroed() will create elements without calling the constructor and this is not appropriate for element types that require a constructor to function properly. */
	void InsertZeroed( int32 Index, int32 Count=1 )
	{
		InsertUninitialized( Index, Count );
		FMemory::Memzero( (uint8*)AllocatorInstance.GetAllocation()+Index*sizeof(ElementType), Count*sizeof(ElementType) );
	}
	int32 Insert( const TArray<ElementType>& Items, const int32 InIndex )
	{
		check(this!=&Items);
		InsertUninitialized( InIndex, Items.Num() );
		int32 Index=InIndex;
		for( auto It=Items.CreateConstIterator(); It; ++It )
		{
			RangeCheck(Index);
			new(GetTypedData() + Index++) ElementType(MoveTemp(*It));
		}
		return InIndex;
	}
	int32 Insert( ElementType&& Item, int32 Index )
	{
		// It isn't valid to specify an Item that is in the array, since adding an item might resize the array, which would make the item invalid
		check( ((&Item) < GetTypedData()) || ((&Item) >= GetTypedData()+ArrayMax) );
		// construct a copy in place at Index (this new operator will insert at 
		// Index, then construct that memory with Item)
		InsertUninitialized(Index,1);
		new(GetTypedData() + Index) ElementType(MoveTemp(Item));
		return Index;
	}
	int32 Insert( const ElementType& Item, int32 Index )
	{
		// It isn't valid to specify an Item that is in the array, since adding an item might resize the array, which would make the item invalid
		check( ((&Item) < GetTypedData()) || ((&Item) >= GetTypedData()+ArrayMax) );
		// construct a copy in place at Index (this new operator will insert at 
		// Index, then construct that memory with Item)
		InsertUninitialized(Index,1);
		new(GetTypedData() + Index) ElementType(Item);
		return Index;
	}
	void RemoveAt( int32 Index, int32 Count=1, bool bAllowShrinking=true )
	{
		CheckInvariants();
		checkSlow((Count >= 0) & (Index >= 0) & (Index+Count <= ArrayNum));

		DestructItems(GetTypedData() + Index, Count);

		// Skip memmove in the common case that there is nothing to move.
		int32 NumToMove = ArrayNum - Index - Count;
		if( NumToMove )
		{
			FMemory::Memmove
			(
				(uint8*)AllocatorInstance.GetAllocation() + (Index      ) * sizeof(ElementType),
				(uint8*)AllocatorInstance.GetAllocation() + (Index+Count) * sizeof(ElementType),
				NumToMove * sizeof(ElementType)
			);
		}
		ArrayNum -= Count;
		
		const int32 NewArrayMax = AllocatorInstance.CalculateSlack(ArrayNum,ArrayMax,sizeof(ElementType));
		if(NewArrayMax != ArrayMax && bAllowShrinking)
		{
			ArrayMax = NewArrayMax;
			AllocatorInstance.ResizeAllocation(ArrayNum,ArrayMax,sizeof(ElementType));
		}
	}
	// RemoveSwap, this version is much more efficient O(Count) instead of O(ArrayNum), but does not preserve the order
	void RemoveAtSwap( int32 Index, int32 Count=1, bool bAllowShrinking=true)
	{
		CheckInvariants();
		checkSlow((Count >= 0) & (Index >= 0) & (Index+Count <= ArrayNum));

		DestructItems(GetTypedData() + Index, Count);
		
		// Replace the elements in the hole created by the removal with elements from the end of the array, so the range of indices used by the array is contiguous.
		const int32 NumElementsInHole = Count;
		const int32 NumElementsAfterHole = ArrayNum - (Index + Count);
		const int32 NumElementsToMoveIntoHole = FMath::Min(NumElementsInHole,NumElementsAfterHole);
		if(NumElementsToMoveIntoHole)
		{
			FMemory::Memcpy(
				(uint8*)AllocatorInstance.GetAllocation() + (Index                             ) * sizeof(ElementType),
				(uint8*)AllocatorInstance.GetAllocation() + (ArrayNum-NumElementsToMoveIntoHole) * sizeof(ElementType),
				NumElementsToMoveIntoHole * sizeof(ElementType)
				);
		}
		ArrayNum -= Count;

		const int32 NewArrayMax = AllocatorInstance.CalculateSlack(ArrayNum,ArrayMax,sizeof(ElementType));
		if(NewArrayMax != ArrayMax && bAllowShrinking)
		{
			ArrayMax = NewArrayMax;
			AllocatorInstance.ResizeAllocation(ArrayNum,ArrayMax,sizeof(ElementType));
		}
	}

	void Empty( int32 Slack=0 )
	{
		DestructItems(GetTypedData(), ArrayNum);

		checkSlow(Slack>=0);
		ArrayNum = 0;
		// only reallocate if we need to, I don't trust realloc to the same size to work
		if (ArrayMax != Slack)
		{
			ArrayMax = Slack;
			AllocatorInstance.ResizeAllocation(0,ArrayMax,sizeof(ElementType));
		}
	}

	void SetNum( int32 NewNum, bool bAllowShrinking=true )
	{
		if (NewNum > Num())
		{
			const int32 Diff  = NewNum - ArrayNum;
			const int32 Index = AddUninitialized(Diff);
			DefaultConstructItems<ElementType>((uint8*)AllocatorInstance.GetAllocation() + Index * sizeof(ElementType), Diff);
		}
		else if (NewNum < Num())
		{
			RemoveAt(NewNum, Num() - NewNum, bAllowShrinking);
		}
	}

	void SetNumZeroed( int32 NewNum )
	{
		if (NewNum > Num())
		{
			AddZeroed(NewNum-Num());
		}
		else if (NewNum < Num())
		{
			RemoveAt(NewNum, Num() - NewNum);
		}
	}

	void SetNumUninitialized( int32 NewNum )
	{
		if (NewNum > Num())
		{
			AddUninitialized(NewNum-Num());
		}
		else if (NewNum < Num())
		{
			RemoveAt(NewNum, Num() - NewNum);
		}
	}

	/**
	 * Appends the specified array to this array.
	 *
	 * @param Source: The array to append.
	 */
	template <typename OtherAllocator>
	FORCEINLINE void Append(const TArray<InElementType, OtherAllocator>& Source)
	{
		check((void*)this != (void*)&Source);

		int32 SourceCount = Source.Num();

		// Do nothing if the source is empty.
		if (!SourceCount)
		{
			return;
		}

		// Allocate memory for the new elements.
		Reserve(ArrayNum + SourceCount);

		CopyConstructItems(GetTypedData() + ArrayNum, Source.GetTypedData(), SourceCount);

		ArrayNum += SourceCount;
	}

	FORCEINLINE void Append(TArray&& Source)
	{
		check((void*)this != (void*)&Source);

		int32 SourceCount = Source.Num();

		// Do nothing if the source is empty.
		if (!SourceCount)
		{
			return;
		}

		// Allocate memory for the new elements.
		Reserve(ArrayNum + SourceCount);

		RelocateItems(GetTypedData() + ArrayNum, Source.GetTypedData(), SourceCount);
		Source.ArrayNum = 0;

		ArrayNum += SourceCount;
	}

	/**
	 * Appends the specified array to this array.
	 * Cannot append to self.
	 */
	FORCEINLINE TArray& operator+=(      TArray&& Other) { Append(MoveTemp(Other)); return *this; }
	FORCEINLINE TArray& operator+=(const TArray&  Other) { Append(         Other ); return *this; }

	/**
	 * Adds a new item to the end of the array, possibly reallocating the whole array to fit.
	 *
	 * @param Item	The item to add
	 * @return		Index to the new item
	 */
	template <typename ArgsType>
	int32 Emplace( ArgsType&& Args )
	{
		const int32 Index = AddUninitialized(1);
		new(GetTypedData() + Index) ElementType(Forward<ArgsType>(Args));
		return Index;
	}

	/**
	 * Adds a new item to the end of the array, possibly reallocating the whole array to fit.
	 *
	 * @param Item	The item to add
	 * @return		Index to the new item
	 */
	FORCEINLINE int32 Add(       ElementType&& Item ) { check( ((&Item) < GetTypedData()) || ((&Item) >= GetTypedData()+ArrayMax) ); return Emplace(MoveTemp(Item)); }
	FORCEINLINE int32 Add( const ElementType&  Item ) { check( ((&Item) < GetTypedData()) || ((&Item) >= GetTypedData()+ArrayMax) ); return Emplace(         Item ); }

	/** Caution, AddZeroed() will create elements without calling the constructor and this is not appropriate for element types that require a constructor to function properly. */
	int32 AddZeroed( int32 Count=1 )
	{
		const int32 Index = AddUninitialized( Count );
		FMemory::Memzero( (uint8*)AllocatorInstance.GetAllocation()+Index*sizeof(ElementType), Count*sizeof(ElementType) );
		return Index;
	}

private:
	template <typename ArgsType>
	int32 AddUniqueImpl( ArgsType&& Args )
	{
		int32 Index;
		if (Find(Args, Index))
			return Index;

		return Add(Forward<ArgsType>(Args));
	}

public:
	FORCEINLINE int32 AddUnique(       ElementType&& Item ) { return AddUniqueImpl(MoveTemp(Item)); }
	FORCEINLINE int32 AddUnique( const ElementType&  Item ) { return AddUniqueImpl(         Item ); }

	/**
	 * Reserves memory such that the array can contain at least Number elements.
	 */
	void Reserve(int32 Number)
	{
		if (Number > ArrayMax)
		{
			ArrayMax = Number;
			AllocatorInstance.ResizeAllocation(ArrayNum,ArrayMax,sizeof(ElementType));
		}
	}
	
	/** Sets the size of the array. */
	void Init(int32 Number)
	{
		Empty(Number);
		AddUninitialized(Number);
	}

	/** Sets the size of the array, filling it with the given element. */
	void Init(const ElementType& Element,int32 Number)
	{
		Empty(Number);
		for(int32 Index = 0;Index < Number;++Index)
		{
			new(*this) ElementType(Element);
		}
	}

	/**
	 * Removes the first occurrence of the specified item in the array, maintaining order but not indices.
	 *
	 * @param	Item	The item to remove
	 *
	 * @return	The number of items removed.  For RemoveSingleItem, this is always either 0 or 1.
	 */
	int32 RemoveSingle( const ElementType& Item )
	{
		// It isn't valid to specify an Item that is in the array, since removing that item will change Item's value.
		check( ((&Item) < GetTypedData()) || ((&Item) >= GetTypedData()+ArrayMax) );

		for( int32 Index=0; Index<ArrayNum; Index++ )
		{
			if( GetTypedData()[Index] == Item )
			{
				// Destruct items that match the specified Item.
				DestructItems(GetTypedData() + Index, 1);
				const int32 NextIndex = Index + 1;
				if( NextIndex < ArrayNum )
				{
					const int32 NumElementsToMove = ArrayNum - NextIndex;
					FMemory::Memmove(&GetTypedData()[Index],&GetTypedData()[NextIndex],sizeof(ElementType) * NumElementsToMove);
				}

				// Update the array count
				--ArrayNum;

				// Removed one item
				return 1;
			}
		}

		// Specified item was not found.  Removed zero items.
		return 0;
	}

	/** Removes as many instances of Item as there are in the array, maintaining order but not indices. */
	int32 Remove( const ElementType& Item )
	{
		// It isn't valid to specify an Item that is in the array, since removing that item will change Item's value.
		check( ((&Item) < GetTypedData()) || ((&Item) >= GetTypedData()+ArrayMax) );

		const int32 OriginalNum = ArrayNum;
		if (!OriginalNum)
		{
			return 0; // nothing to do, loop assumes one item so need to deal with this edge case here
		}

		int32 WriteIndex = 0;
		int32 ReadIndex = 0;
		bool NotMatch = !(GetTypedData()[ReadIndex] == Item); // use a ! to guarantee it can't be anything other than zero or one
		do
		{
			int32 RunStartIndex = ReadIndex++;
			while (ReadIndex < OriginalNum && NotMatch == !(GetTypedData()[ReadIndex] == Item))
			{
				ReadIndex++;
			}
			int32 RunLength = ReadIndex - RunStartIndex;
			checkSlow(RunLength > 0);
			if (NotMatch)
			{
				// this was a non-matching run, we need to move it
				if (WriteIndex != RunStartIndex)
				{
					FMemory::Memmove( &GetTypedData()[ WriteIndex ], &GetTypedData()[ RunStartIndex ], sizeof(ElementType) * RunLength );
				}
				WriteIndex += RunLength;
			}
			else
			{
				// this was a matching run, delete it
				DestructItems(GetTypedData() + RunStartIndex, RunLength);
			}
			NotMatch = !NotMatch;
		} while (ReadIndex < OriginalNum);

		ArrayNum = WriteIndex;
		return OriginalNum - ArrayNum;
	}


	/**
	 * Remove all instances that match the predicate
	 *
	 * @param Predicate Predicate class instance
	 */
	template <class PREDICATE_CLASS>
	void RemoveAll( const PREDICATE_CLASS& Predicate )
	{
		for ( int32 ItemIndex=0; ItemIndex < Num(); )
		{
			if ( Predicate( (*this)[ItemIndex] ) )
			{
				RemoveAt(ItemIndex);
			}
			else
			{
				++ItemIndex;
			}
		}
	}

	/**
	 * Remove all instances that match the predicate
	 *
	 * @param Predicate Predicate class instance
	 */
	template <class PREDICATE_CLASS>
	void RemoveAllSwap( const PREDICATE_CLASS& Predicate )
	{
		for ( int32 ItemIndex=0; ItemIndex < Num(); )
		{
			if ( Predicate( (*this)[ItemIndex] ) )
			{
				RemoveAtSwap(ItemIndex);
			}
			else
			{
				++ItemIndex;
			}
		}
	}

	/**
	 * Removes the first occurrence of the specified item in the array.  This version is much more efficient
	 * O(Count) instead of O(ArrayNum), but does not preserve the order
	 *
	 * @param	Item	The item to remove
	 *
	 * @return	The number of items removed.  For RemoveSingleItem, this is always either 0 or 1.
	 */
	int32 RemoveSingleSwap( const ElementType& Item )
	{
		check( ((&Item) < (ElementType*)AllocatorInstance.GetAllocation()) || ((&Item) >= (ElementType*)AllocatorInstance.GetAllocation()+ArrayMax) );
		for( int32 Index=0; Index<ArrayNum; Index++ )
		{
			if( (*this)[Index]==Item )
			{
				RemoveAtSwap(Index);

				// Removed one item
				return 1;
			}
		}

		// Specified item was not found.  Removed zero items.
		return 0;
	}

	/** RemoveItemSwap, this version is much more efficient O(Count) instead of O(ArrayNum), but does not preserve the order */
	int32 RemoveSwap( const ElementType& Item )
	{
		check( ((&Item) < (ElementType*)AllocatorInstance.GetAllocation()) || ((&Item) >= (ElementType*)AllocatorInstance.GetAllocation()+ArrayMax) );
		const int32 OriginalNum=ArrayNum;
		for( int32 Index=0; Index<ArrayNum; Index++ )
		{
			if( (*this)[Index]==Item )
			{
				RemoveAtSwap( Index-- );
			}
		}
		return OriginalNum - ArrayNum;
	}

	void SwapMemory(int32 A, int32 B)
	{
		FMemory::Memswap(
			(uint8*)AllocatorInstance.GetAllocation()+(sizeof(ElementType)*A),
			(uint8*)AllocatorInstance.GetAllocation()+(sizeof(ElementType)*B),
			sizeof(ElementType)
			);
	}

	void Swap(int32 A, int32 B)
	{
		check((A >= 0) && (B >= 0));
		check((ArrayNum > A) && (ArrayNum > B));
		if (A != B)
		{
			SwapMemory(A,B);
		}
	}

	/**
	 * Same as empty, but doesn't change memory allocations, unless the new size is larger than
	 * the current array. It calls the destructors on held items if needed and then zeros the ArrayNum.
	 *
	 * @param NewSize the expected usage size
	 */
	void Reset(int32 NewSize = 0)
	{
		// If we have space to hold the excepted size, then don't reallocate
		if (NewSize <= ArrayMax)
		{
			DestructItems(GetTypedData(), ArrayNum);
			ArrayNum = 0;
		}
		else
		{
			Empty(NewSize);
		}
	}

	/**
	 * Searches for the first entry of the specified type, will only work
	 * with TArray<UObject*>.  Optionally return the item's index, and can
	 * specify the start index.
	 */
	template<typename SearchType> bool FindItemByClass(SearchType **Item = NULL, int32 *ItemIndex = NULL, int32 StartIndex = 0) const
	{
		UClass* SearchClass = SearchType::StaticClass();
		for (int32 Idx = StartIndex; Idx < ArrayNum; Idx++)
		{
			if ((*this)[Idx] != NULL && (*this)[Idx]->IsA(SearchClass))
			{
				if (Item != NULL)
				{
					*Item = (SearchType*)((*this)[Idx]);
				}
				if (ItemIndex != NULL)
				{
					*ItemIndex = Idx;
				}
				return true;
			}
		}
		return false;
	}

	// Iterators
	typedef TIndexedContainerIterator<      TArray,       ElementType, int32> TIterator;
	typedef TIndexedContainerIterator<const TArray, const ElementType, int32> TConstIterator;

	/** Creates an iterator for the contents of this array */
	TIterator CreateIterator()
	{
		return TIterator(*this);
	}

	/** Creates a const iterator for the contents of this array */
	TConstIterator CreateConstIterator() const
	{
		return TConstIterator(*this);
	}

private:
	/**
	 * DO NOT USE DIRECTLY
	 * STL-like iterators to enable range-based for loop support.
	 */
	FORCEINLINE friend TIterator      begin(      TArray& Array) { return TIterator     (Array); }
	FORCEINLINE friend TConstIterator begin(const TArray& Array) { return TConstIterator(Array); }
	FORCEINLINE friend TIterator      end  (      TArray& Array) { return TIterator     (Array, Array.Num()); }
	FORCEINLINE friend TConstIterator end  (const TArray& Array) { return TConstIterator(Array, Array.Num()); }

public:
	/** Sorts the array assuming < operator is defined for the item type */
	void Sort()
	{
		::Sort( GetTypedData(), Num() );
	}

	/**
	 * Sorts the array using user define predicate class.
	 *
	 * @param Predicate Predicate class instance
	 */
	template <class PREDICATE_CLASS>
	void Sort( const PREDICATE_CLASS& Predicate )
	{
		::Sort( GetTypedData(), Num(), Predicate );
	}

#if defined(_MSC_VER)
private:
	/**
	* Helper function that can be used inside the debuggers watch window to debug TArrays. E.g. "*Class->Defaults.DebugGet(5)". 
	*
	* @param	i	Index
	* @return		pointer to type T at Index i
	*/
	FORCENOINLINE const ElementType& DebugGet( int32 i ) const
	{
		return GetTypedData()[i];
	}
#endif

private:

	/**
	 * Copies data from one array into this array. Uses the fast path if the
	 * data in question does not need a constructor.
	 *
	 * @param Source the source array to copy
	 */
	template<typename OtherAllocator>
	void CopyToEmpty(const TArray<ElementType,OtherAllocator>& Source)
	{
		int32 SourceCount = Source.Num();
		AllocatorInstance.ResizeAllocation(0, SourceCount, sizeof(ElementType));

		CopyConstructItems(GetTypedData(), Source.GetTypedData(), SourceCount);

		ArrayNum = SourceCount;
		ArrayMax = SourceCount;
	}

protected:

	typedef typename TChooseClass<
		Allocator::NeedsElementType,
		typename Allocator::template ForElementType<ElementType>,
		typename Allocator::ForAnyElementType
		>::Result ElementAllocatorType;

	ElementAllocatorType AllocatorInstance;
	int32	  ArrayNum;
	int32	  ArrayMax;

	/**
	 * Implicit heaps
	 */

public:

	/** 
	 * Builds an implicit heap from the array
	 *
	 * @pram Predicate Predicate class instance
	 */
	template <class PREDICATE_CLASS>
	void Heapify(const PREDICATE_CLASS& Predicate)
	{
		TDereferenceWrapper< ElementType, PREDICATE_CLASS> PredicateWrapper( Predicate );
		for(int32 Index=HeapGetParentIndex(Num()-1); Index>=0; Index--)
		{
			SiftDown(Index, Num(), PredicateWrapper);
		}

#if DEBUG_HEAP
		VerifyHeap(PredicateWrapper);
#endif
	}

	void Heapify()
	{
		Heapify( TLess<ElementType>() );
	}

	/** 
	 * Adds a new element to the heap
	 *
	 * @pram InIntem Item to be added
	 * @pram Predicate Predicate class instance
	 */
	template <class PREDICATE_CLASS>
	void HeapPush( const ElementType& InItem, const PREDICATE_CLASS& Predicate )
	{
		// Add at the end, then sift up
		Add(InItem);
		TDereferenceWrapper< ElementType, PREDICATE_CLASS> PredicateWrapper(Predicate);
		SiftUp(0, Num()-1, PredicateWrapper);

#if DEBUG_HEAP
		VerifyHeap(PredicateWrapper);
#endif
	}

	void HeapPush( const ElementType& InItem )
	{
		HeapPush( InItem, TLess<ElementType>() );
	}

	/** 
	 * Removes the top element from the heap.
	 *
	 * @pram OutItem The removed item
	 * @pram Predicate Predicate class instance
	 */
	template <class PREDICATE_CLASS>
	void HeapPop( ElementType& OutItem, const PREDICATE_CLASS& Predicate )
	{
		OutItem = (*this)[ 0 ];
		RemoveAtSwap( 0 );

		TDereferenceWrapper< ElementType, PREDICATE_CLASS> PredicateWrapper(Predicate);
		SiftDown(0, Num(), PredicateWrapper);

#if DEBUG_HEAP
		VerifyHeap(PredicateWrapper);
#endif
	}

	void HeapPop( ElementType& OutItem )
	{
		HeapPop( OutItem, TLess<ElementType>() );
	}

	template <class PREDICATE_CLASS>
	void VerifyHeap(const PREDICATE_CLASS& Predicate )
	{
		// Verify Predicate
		ElementType* Heap = GetTypedData();
		for( int32 Index=1; Index<Num(); Index++ )
		{
			int32 ParentIndex = HeapGetParentIndex(Index);
			if( Predicate(Heap[Index], Heap[ParentIndex]) )
			{
				check( false );
			}
		}
	}

	/** 
	 * Removes the top element from the heap.
	 *
	 * @pram Predicate Predicate class instance
	 */
	template <class PREDICATE_CLASS>
	void HeapPopDiscard(const PREDICATE_CLASS& Predicate )
	{
		RemoveAtSwap( 0 );
		TDereferenceWrapper< ElementType, PREDICATE_CLASS> PredicateWrapper(Predicate);
		SiftDown(0, Num(), PredicateWrapper);

#if DEBUG_HEAP
		VerifyHeap(PredicateWrapper);
#endif
	}

	void HeapPopDiscard()
	{
		HeapPopDiscard( TLess<ElementType>() );
	}

	/** 
	 * Returns the top element from the heap (does not remove the element).
	 */
	const ElementType& HeapTop() const
	{
		return (*this)[ 0 ];
	}

	/** 
	 * Returns the top element from the heap (does not remove the element).
	 */
	ElementType& HeapTop()
	{
		return (*this)[ 0 ];
	}

	/**
	 * Removes an element from the heap.
	 *
	 * @param Index at which to remove item.
	 * @param Predicate Predicate class.
	 */
	template <class PREDICATE_CLASS>
	void HeapRemoveAt( int32 Index, const PREDICATE_CLASS& Predicate )
	{
		RemoveAtSwap(Index);

		TDereferenceWrapper< ElementType, PREDICATE_CLASS> PredicateWrapper(Predicate);
		SiftDown(Index, Num(), PredicateWrapper);
		SiftUp(0, Index, PredicateWrapper);

#if DEBUG_HEAP
		VerifyHeap(PredicateWrapper);
#endif
	}

	void HeapRemoveAt( int32 Index )
	{
		HeapRemoveAt( Index, TLess< ElementType >() );
	}

	/**
	 * Performs heap sort on the array.
	 *
	 * @param Predicate Predicate class instance.
	 */
	template <class PREDICATE_CLASS>
	void HeapSort( const PREDICATE_CLASS& Predicate )
	{
 		TReversePredicateWrapper< ElementType, PREDICATE_CLASS > ReversePredicateWrapper(Predicate);
		Heapify(ReversePredicateWrapper);

		ElementType* Heap = GetTypedData();
		for(int32 Index=Num()-1; Index>0; Index--)
		{
			Exchange(Heap[0], Heap[Index]);
			SiftDown(0, Index, ReversePredicateWrapper);
		}

#if DEBUG_HEAP
		TDereferenceWrapper< ElementType, PREDICATE_CLASS> PredicateWrapper(Predicate);

		// Verify Heap Property
		VerifyHeap(PredicateWrapper);

		// Also verify Array is properly sorted
		for(int32 Index=1; Index<Num(); Index++)
		{
			if( PredicateWrapper(Heap[Index], Heap[Index-1]) )
			{
				check( false );
			}
		}
#endif
	}

	void HeapSort()
	{
		HeapSort( TLess<ElementType>() );
	}

private:

	/**
	 * Gets the index of the left child of node at Index.
	 *
	 * @param Index Node for which the left child index is to be returned.
	 * @return Index of the left child.
	 */
	FORCEINLINE int32 HeapGetLeftChildIndex( int32 Index ) const
	{
		return Index * 2 + 1;
	}

	/** 
	 * Checks if node located at Index is a leaf or not .
	 *
	 * @param Index - Node index.
	 * @return true if node is a leaf.
	 */
	FORCEINLINE bool HeapIsLeaf( int32 Index, int32 Count ) const
	{
		return HeapGetLeftChildIndex( Index ) >= Count;
	}

	/**
	 * Gets the parent index for node at Index.
	 *
	 * @param Index node index.
	 * @return Parent index.
	 */
	FORCEINLINE int32 HeapGetParentIndex( int32 Index ) const
	{
		return ( Index - 1 ) / 2;
	}

	/**
	 * Fixes a possible violation of order property between node at Index and a child.
	 *
	 * @param Index node index.
	 * @param Count Size of the heap (to avoid using Num()).
	 * @param Predicate Predicate class instance.
	 */
	template <class PREDICATE_CLASS>
	FORCEINLINE void SiftDown(int32 Index, const int32 Count, const PREDICATE_CLASS& Predicate)
	{
		ElementType* Heap = GetTypedData();
		while( !HeapIsLeaf(Index, Count) )
		{
			const int32 LeftChildIndex = HeapGetLeftChildIndex(Index);
			const int32 RightChildIndex = LeftChildIndex + 1;

			int32 MinChildIndex = LeftChildIndex;
			if( RightChildIndex < Count )
			{
				MinChildIndex = Predicate(Heap[LeftChildIndex], Heap[RightChildIndex]) ? LeftChildIndex : RightChildIndex;
			}

			if( Predicate(Heap[MinChildIndex], Heap[Index]) )
			{
				Exchange(Heap[Index], Heap[MinChildIndex]);
				Index = MinChildIndex;
			}
			else
			{
				break;
			}
		}
	}

	/**
	 * Fixes a possible violation of order property between node at NodeIndex and a parent.
	 *
	 * @param RootIndex, how far to go up
	 * @param NodeIndex node index.
	 * @param Predicate Predicate class instance.
	 */
	template <class PREDICATE_CLASS>
	FORCEINLINE void SiftUp(int32 RootIndex, const int32 NodeIndex, const PREDICATE_CLASS& Predicate)
	{
		ElementType* Heap = GetTypedData();
		int32 Index = FMath::Min(NodeIndex, Num() - 1);
		while(Index > RootIndex)
		{
			int32 ParentIndex = HeapGetParentIndex(Index);
			if( Predicate(Heap[Index], Heap[ParentIndex]) )
			{
				Exchange(Heap[Index], Heap[ParentIndex]);
				Index = ParentIndex; 
			}
			else
			{
				break;
			}
		}
	}
};

template <typename InElementType, typename Allocator>
struct TIsZeroConstructType<TArray<InElementType, Allocator>>
{
	enum { Value = TAllocatorTraits<Allocator>::IsZeroConstruct };
};

template <typename InElementType, typename Allocator>
struct TContainerTraits<TArray<InElementType, Allocator> > : public TContainerTraitsBase<TArray<InElementType, Allocator> >
{
	enum { MoveWillEmptyContainer =
		PLATFORM_COMPILER_HAS_RVALUE_REFERENCES &&
		TAllocatorTraits<Allocator>::SupportsMove };
};


/**
 * Traits class which determines whether or not a type is a TArray.
 */
template <typename T> struct TIsTArray { enum { Value = false }; };

template <typename InElementType, typename Allocator> struct TIsTArray<               TArray<InElementType, Allocator>> { enum { Value = true }; };
template <typename InElementType, typename Allocator> struct TIsTArray<const          TArray<InElementType, Allocator>> { enum { Value = true }; };
template <typename InElementType, typename Allocator> struct TIsTArray<      volatile TArray<InElementType, Allocator>> { enum { Value = true }; };
template <typename InElementType, typename Allocator> struct TIsTArray<const volatile TArray<InElementType, Allocator>> { enum { Value = true }; };


//
// Array operator news.
//
template <typename T,typename Allocator> void* operator new( size_t Size, TArray<T,Allocator>& Array )
{
	check(Size == sizeof(T));
	const int32 Index = Array.AddUninitialized(1);
	return &Array[Index];
}
template <typename T,typename Allocator> void* operator new( size_t Size, TArray<T,Allocator>& Array, int32 Index )
{
	check(Size == sizeof(T));
	Array.InsertUninitialized(Index,1);
	return &Array[Index];
}

/** A specialization of the exchange macro that avoids reallocating when exchanging two arrays. */
template <typename T> inline void Exchange( TArray<T>& A, TArray<T>& B )
{
	FMemory::Memswap( &A, &B, sizeof(TArray<T>) );
}

/** A specialization of the exchange macro that avoids reallocating when exchanging two arrays. */
template<typename ElementType,typename Allocator>
inline void Exchange( TArray<ElementType,Allocator>& A, TArray<ElementType,Allocator>& B )
{
	FMemory::Memswap( &A, &B, sizeof(TArray<ElementType,Allocator>) );
}

/*-----------------------------------------------------------------------------
	MRU array.
-----------------------------------------------------------------------------*/

/**
 * Same as TArray except:
 * - Has an upper limit of the number of items it will store.
 * - Any item that is added to the array is moved to the top.
 */
template<typename T,typename Allocator = FDefaultAllocator>
class TMRUArray : public TArray<T,Allocator>
{
public:
	typedef TArray<T,Allocator> Super;

	/** The maximum number of items we can store in this array. */
	int32 MaxItems;

	TMRUArray()
	:	Super()
	{
		MaxItems = 0;
	}
	TMRUArray( int32 InNum )
	:	Super( InNum )
	{
		MaxItems = 0;
	}

#if PLATFORM_COMPILER_HAS_DEFAULTED_FUNCTIONS

	TMRUArray( const TMRUArray& ) = default;
	TMRUArray& operator=( const TMRUArray& ) = default;

	#if PLATFORM_COMPILER_HAS_RVALUE_REFERENCES

		TMRUArray( TMRUArray&& ) = default;
		TMRUArray& operator=( TMRUArray&& ) = default;

	#endif

#else

	FORCEINLINE TMRUArray( const TMRUArray& Other )
		: Super((const Super&)Other)
	{
	}

	FORCEINLINE TMRUArray& operator=( const TMRUArray& Other )
	{
		(Super&)*this = (const Super&)Other;
		return *this;
	}

	#if PLATFORM_COMPILER_HAS_RVALUE_REFERENCES

		FORCEINLINE TMRUArray( TMRUArray&& Other )
			: Super((TMRUArray&&)Other)
		{
		}

		FORCEINLINE TMRUArray& operator=( TMRUArray&& Other )
		{
			(Super&)*this = (Super&&)Other;
			return *this;
		}

	#endif

#endif

	int32 Add( const T& Item )
	{
		const int32 idx = Super::Add( Item );
		this->Swap( idx, 0 );
		CullArray();
		return 0;
	}
	int32 AddZeroed( int32 Count=1 )
	{
		const int32 idx = Super::AddZeroed( Count );
		this->Swap( idx, 0 );
		CullArray();
		return 0;
	}
	int32 AddUnique( const T& Item )
	{
		// Remove any existing copies of the item.
		this->Remove(Item);

		this->InsertUninitialized( 0 );
		(*this)[0] = Item;

		CullArray();

		return 0;
	}

	/**
	 * Makes sure that the array never gets beyond MaxItems in size.
	 */

	void CullArray()
	{
		// 0 = no limit
		if( !MaxItems )
		{
			return;
		}

		while( this->Num() > MaxItems )
		{
			this->RemoveAt( this->Num()-1, 1 );
		}
	}
};

template<typename T, typename Allocator>
struct TContainerTraits<TMRUArray<T, Allocator> > : public TContainerTraitsBase<TMRUArray<T, Allocator> >
{
	enum { MoveWillEmptyContainer =
		PLATFORM_COMPILER_HAS_RVALUE_REFERENCES &&
		TContainerTraitsBase<typename TMRUArray<T, Allocator>::Super>::MoveWillEmptyContainer };
};

/*-----------------------------------------------------------------------------
	Indirect array.
	Same as a TArray above, but stores pointers to the elements, to allow
	resizing the array index without relocating the actual elements.
-----------------------------------------------------------------------------*/

template<typename T,typename Allocator = FDefaultAllocator>
class TIndirectArray
{
public:
	typedef T                        ElementType;
	typedef TArray<void*, Allocator> InternalArrayType;

	TIndirectArray( int32 InNum )
	:	Array( InNum )
	{}

#if PLATFORM_COMPILER_HAS_DEFAULTED_FUNCTIONS

	TIndirectArray() = default;
	TIndirectArray( TIndirectArray&& ) = default;
	TIndirectArray& operator=( TIndirectArray&& ) = default;

#else

	            TIndirectArray() {}
	FORCEINLINE TIndirectArray(TIndirectArray&& Other) : Array(MoveTemp(Other.Array)) {}
	FORCEINLINE TIndirectArray& operator=(TIndirectArray&& Other) { Array = MoveTemp(Other.Temp); return *this; }

#endif

	TIndirectArray( const TIndirectArray& Other )
	{
		for (auto& Item : Other)
		{
			AddRawItem(new T(Item));
		}
	}

	TIndirectArray& operator=( const TIndirectArray& Other )
	{
		if (&Other != this)
		{
			Empty(Other.Num());
			for (auto& Item : Other)
			{
				AddRawItem(new T(Item));
			}
		}

		return *this;
	}

	~TIndirectArray()
	{
		Empty();
	}

	FORCEINLINE int32 Num() const
	{
		return Array.Num();
	}

	/**
	 * Helper function for returning a typed pointer to the first array entry.
	 *
	 * @return pointer to first array entry or NULL if this->ArrayMax==0
	 */
	FORCEINLINE T** GetTypedData()
	{
		return (T**)Array.GetTypedData();
	}
	/**
	 * Helper function for returning a typed pointer to the first array entry.
	 *
	 * @return pointer to first array entry or NULL if this->ArrayMax==0
	 */
	FORCEINLINE const T** GetTypedData() const
	{
		return (const T**)Array.GetTypedData();
	}
	/** 
	 * Helper function returning the size of the inner type
	 *
	 * @return size in bytes of array type
	 */
	uint32 GetTypeSize() const
	{
		return sizeof(T*);
	}
	T& operator[]( int32 i )
	{
		return *(T*)Array[i];
	}
	FORCEINLINE const T& operator[]( int32 i ) const
	{
		return *(T*)Array[i];
	}
	FORCEINLINE ElementType& Last( int32 c=0 )
	{
		return *(T*)Array.Last(c);
	}
	FORCEINLINE const ElementType& Last( int32 c=0 ) const
	{
		return *(T*)Array.Last(c);
	}
	void Shrink()
	{
		Array.Shrink( );
	}
	void Reset(int32 NewSize = 0)
	{
		Array.Reset(NewSize);
	}

	/**
	 * Special serialize function passing the owning UObject along as required by FUnytpedBulkData
	 * serialization.
	 *
	 * @param	Ar		Archive to serialize with
	 * @param	Owner	UObject this structure is serialized within
	 */
	void Serialize( FArchive& Ar, UObject* Owner )
	{
		CountBytes( Ar );
		if( Ar.IsLoading() )
		{
			// Load array.
			int32 NewNum;
			Ar << NewNum;
			Empty( NewNum );
			for( int32 i=0; i<NewNum; i++ )
			{
				new(*this) T;
			}
			for( int32 i=0; i<NewNum; i++ )
			{
				(*this)[i].Serialize( Ar, Owner, i );
			}
		}
		else
		{
			// Save array.
			int32 Num = Array.Num();
			Ar << Num;
			for( int32 i=0; i<Num; i++ )
			{
				(*this)[i].Serialize( Ar, Owner, i );
			}
		}
	}
	friend FArchive& operator<<( FArchive& Ar, TIndirectArray& A )
	{
		A.CountBytes( Ar );
		if( Ar.IsLoading() )
		{
			// Load array.
			int32 NewNum;
			Ar << NewNum;
			A.Empty( NewNum );
			for( int32 i=0; i<NewNum; i++ )
			{
				Ar << *new(A)T;
			}
		}
		else
		{
			// Save array.
			int32 Num = A.Num();
			Ar << Num;
			for( int32 i=0; i<Num; i++ )
			{
				Ar << A[ i ];
			}
		}
		return Ar;
	}
	void CountBytes( FArchive& Ar )
	{
		Array.CountBytes( Ar );
	}
	void RemoveAt( int32 Index, int32 Count=1, bool AllowShrinking=true )
	{
		check(Index >= 0);
		check(Index <= Array.Num());
		check(Index + Count <= Array.Num());
		T** Element = GetTypedData() + Index;
		for (int32 i = Count; i; --i)
		{
			// We need a typedef here because VC won't compile the destructor call below if T itself has a member called T
			typedef T IndirectArrayDestructElementType;

			(*Element)->IndirectArrayDestructElementType::~IndirectArrayDestructElementType();
			FMemory::Free(*Element);
			++Element;
		}
		Array.RemoveAt( Index, Count, AllowShrinking );
	}
	void RemoveAtSwap( int32 Index, int32 Count=1, bool AllowShrinking=true )
	{
		check(Index >= 0);
		check(Index <= Array.Num());
		check(Index + Count <= Array.Num());
		T** Element = GetTypedData() + Index;
		for (int32 i = Count; i; --i)
		{
			// We need a typedef here because VC won't compile the destructor call below if T itself has a member called T
			typedef T IndirectArrayDestructElementType;

			(*Element)->IndirectArrayDestructElementType::~IndirectArrayDestructElementType();
			FMemory::Free(*Element);
			++Element;
		}
		Array.RemoveAtSwap( Index, Count, AllowShrinking );
	}
	void Empty( int32 Slack=0 )
	{
		T** Element = GetTypedData();
		for (int32 i = Array.Num(); i; --i)
		{
			// We need a typedef here because VC won't compile the destructor call below if T itself has a member called T
			typedef T IndirectArrayDestructElementType;

			(*Element)->IndirectArrayDestructElementType::~IndirectArrayDestructElementType();
			FMemory::Free(*Element);
			++Element;
		}
		Array.Empty( Slack );
	}
	FORCEINLINE int32 Add(T* Item)
	{
		return Array.Add(Item);
	}
	FORCEINLINE int32 AddRawItem(T* Item)
	{
		return Array.Add(Item);
	}
	FORCEINLINE void Insert(T* Item, int32 Index)
	{
		Array.Insert(Item, Index);
	}
	FORCEINLINE void InsertRawItem(T* Item, int32 Index)
	{
		Array.Insert(Item, Index);
	}
	FORCEINLINE void Reserve(int32 Number)
	{
		Array.Reserve(Number);
	}
	FORCEINLINE bool IsValidIndex(int32 i) const
	{
		return Array.IsValidIndex(i);
	}
	/** 
	* Helper function to return the amount of memory allocated by this container 
	*
	* @return number of bytes allocated by this container
	*/
	uint32 GetAllocatedSize( void ) const
	{
		return Array.Max() * (sizeof(T) + sizeof(T*));
	}

	// Iterators
	typedef TIndexedContainerIterator<      TIndirectArray,       ElementType, int32> TIterator;
	typedef TIndexedContainerIterator<const TIndirectArray, const ElementType, int32> TConstIterator;

	/** Creates an iterator for the contents of this array */
	TIterator CreateIterator()
	{
		return TIterator(*this);
	}

	/** Creates a const iterator for the contents of this array */
	TConstIterator CreateConstIterator() const
	{
		return TConstIterator(*this);
	}

private:
	/**
	 * DO NOT USE DIRECTLY
	 * STL-like iterators to enable range-based for loop support.
	 */
	FORCEINLINE friend TIterator      begin(      TIndirectArray& IndirectArray) { return TIterator     (IndirectArray); }
	FORCEINLINE friend TConstIterator begin(const TIndirectArray& IndirectArray) { return TConstIterator(IndirectArray); }
	FORCEINLINE friend TIterator      end  (      TIndirectArray& IndirectArray) { return TIterator     (IndirectArray, IndirectArray.Array.Num()); }
	FORCEINLINE friend TConstIterator end  (const TIndirectArray& IndirectArray) { return TConstIterator(IndirectArray, IndirectArray.Array.Num()); }

	InternalArrayType Array;
};

template<typename T, typename Allocator>
struct TContainerTraits<TIndirectArray<T, Allocator> > : public TContainerTraitsBase<TIndirectArray<T, Allocator> >
{
	enum { MoveWillEmptyContainer =
		PLATFORM_COMPILER_HAS_RVALUE_REFERENCES &&
		TContainerTraitsBase<typename TIndirectArray<T, Allocator>::InternalArrayType>::MoveWillEmptyContainer };
};

template <typename T,typename Allocator> void* operator new( size_t Size, TIndirectArray<T,Allocator>& Array )
{
	check(Size == sizeof(T));
	const int32 Index = Array.AddRawItem((T*)FMemory::Malloc(Size));
	return &Array[Index];
}

template <typename T,typename Allocator> void* operator new( size_t Size, TIndirectArray<T,Allocator>& Array, int32 Index )
{
	check(Size == sizeof(T));
	Array.InsertRawItem((T*)FMemory::Malloc(Size), Index);
	return &Array[Index];
}

/** A specialization of the exchange macro that avoids reallocating when exchanging two arrays. */
template <typename T> inline void Exchange( TIndirectArray<T>& A, TIndirectArray<T>& B )
{
	FMemory::Memswap( &A, &B, sizeof(TIndirectArray<T>) );
}

/** A specialization of the exchange macro that avoids reallocating when exchanging two arrays. */
template<typename ElementType,typename Allocator>
inline void Exchange( TIndirectArray<ElementType,Allocator>& A, TIndirectArray<ElementType,Allocator>& B )
{
	FMemory::Memswap( &A, &B, sizeof(TIndirectArray<ElementType,Allocator>) );
}

/*-----------------------------------------------------------------------------
	Transactional array.
-----------------------------------------------------------------------------*/

// NOTE: Right now, you can't use a custom allocation policy with transactional arrays. If
// you need to do it, you will have to fix up FTransaction::FObjectRecord to use the correct TArray<Allocator>.
template< typename T >
class TTransArray : public TArray<T>
{
public:
	typedef TArray<T> Super;

	// Constructors.
	explicit TTransArray( UObject* InOwner )
	:	Owner( InOwner )
	{
		checkSlow(Owner);
	}
	TTransArray( UObject* InOwner, const Super& Other )
	:	Super( Other )
	,	Owner( InOwner )
	{
		checkSlow(Owner);
	}

#if PLATFORM_COMPILER_HAS_DEFAULTED_FUNCTIONS

	TTransArray(const TTransArray&) = default;
	TTransArray& operator=(const TTransArray&) = default;

	#if PLATFORM_COMPILER_HAS_RVALUE_REFERENCES

		TTransArray(TTransArray&&) = default;
		TTransArray& operator=(TTransArray&&) = default;

	#endif

#else

	FORCEINLINE TTransArray(const TTransArray& Other)
		: Super((const Super&)Other)
		, Owner(Other.Owner)
	{
	}

	FORCEINLINE TTransArray& operator=(const TTransArray& Other)
	{
		(Super&)*this = (const Super&)Other;
		Owner         = Other.Owner;
		return *this;
	}

	#if PLATFORM_COMPILER_HAS_RVALUE_REFERENCES

		FORCEINLINE TTransArray(TTransArray&& Other)
			: Super((TTransArray&&)Other)
			, Owner(Other.Owner)
		{
		}

		FORCEINLINE TTransArray& operator=(TTransArray&& Other)
		{
			(Super&)*this = (Super&&)Other;
			Owner         = Other.Owner;
			return *this;
		}

	#endif

#endif

	// Add, Insert, Remove, Empty interface.
	int32 AddUninitialized( int32 Count=1 )
	{
		const int32 Index = Super::AddUninitialized( Count );
		if( GUndo )
		{
			GUndo->SaveArray( Owner, (FScriptArray*)this, Index, Count, 1, sizeof(T), SerializeItem, DestructItem );
		}
		return Index;
	}
	void InsertUninitialized( int32 Index, int32 Count=1 )
	{
		Super::InsertUninitialized( Index, Count );
		if( GUndo )
		{
			GUndo->SaveArray( Owner, (FScriptArray*)this, Index, Count, 1, sizeof(T), SerializeItem, DestructItem );
		}
	}
	void RemoveAt( int32 Index, int32 Count=1 )
	{
		if( GUndo )
		{
			GUndo->SaveArray( Owner, (FScriptArray*)this, Index, Count, -1, sizeof(T), SerializeItem, DestructItem );
		}
		Super::RemoveAt( Index, Count );
	}
	void Empty( int32 Slack=0 )
	{
		if( GUndo )
		{
			GUndo->SaveArray( Owner, (FScriptArray*)this, 0, this->ArrayNum, -1, sizeof(T), SerializeItem, DestructItem );
		}
		Super::Empty( Slack );
	}

	// Functions dependent on Add, Remove.
	void AssignButKeepOwner( const Super& Other )
	{
		(Super&)*this = Other;
	}

	int32 Add( const T& Item )
	{
		new(*this) T(Item);
		return this->Num() - 1;
	}
	int32 AddZeroed( int32 n=1 )
	{
		const int32 Index = AddUninitialized(n);
		FMemory::Memzero( this->GetTypedData() + Index, n*sizeof(T) );
		return Index;
	}
	int32 AddUnique( const T& Item )
	{
		for( int32 Index=0; Index<this->ArrayNum; Index++ )
		{
			if( (*this)[Index]==Item )
			{
				return Index;
			}
		}
		return Add( Item );
	}
	int32 Remove( const T& Item )
	{
		check( ((&Item) < (T*)this->AllocatorInstance.GetAllocation()) || ((&Item) >= (T*)this->AllocatorInstance.GetAllocation()+this->ArrayMax) );
		const int32 OriginalNum=this->ArrayNum;
		for( int32 Index=0; Index<this->ArrayNum; Index++ )
		{
			if( (*this)[Index]==Item )
			{
				RemoveAt( Index-- );
			}
		}
		return OriginalNum - this->ArrayNum;
	}

	// TTransArray interface.
	UObject* GetOwner() const
	{
		return Owner;
	}
	void SetOwner( UObject* NewOwner )
	{
		Owner = NewOwner;
	}
	void ModifyItem( int32 Index )
	{
		if( GUndo )
			GUndo->SaveArray( Owner, (FScriptArray*)this, Index, 1, 0, sizeof(T), SerializeItem, DestructItem );
	}
	void ModifyAllItems()
	{
		if( GUndo )
		{
			GUndo->SaveArray( Owner, (FScriptArray*)this, 0, this->Num(), 0, sizeof(T), SerializeItem, DestructItem );
		}
	}
	friend FArchive& operator<<( FArchive& Ar, TTransArray& A )
	{
		Ar << A.Owner;
		Ar << (Super&)A;
		return Ar;
	}
protected:
	static void SerializeItem( FArchive& Ar, void* TPtr )
	{
		Ar << *(T*)TPtr;
	}
	static void DestructItem( void* TPtr )
	{
		((T*)TPtr)->~T();
	}
	UObject* Owner;
};

template<typename T>
struct TContainerTraits<TTransArray<T> > : public TContainerTraitsBase<TTransArray<T> >
{
	enum { MoveWillEmptyContainer =
		PLATFORM_COMPILER_HAS_RVALUE_REFERENCES &&
		TContainerTraitsBase<typename TTransArray<T>::Super>::MoveWillEmptyContainer };
};

//
// Transactional array operator news.
//
template <typename T> void* operator new( size_t Size, TTransArray<T>& Array )
{
	check(Size == sizeof(T));
	const int32 Index = Array.AddUninitialized();
	return &Array[Index];
}
template <typename T> void* operator new( size_t Size, TTransArray<T>& Array, int32 Index )
{
	check(Size == sizeof(T));
	Array.Insert(Index);
	return &Array[Index];
}
