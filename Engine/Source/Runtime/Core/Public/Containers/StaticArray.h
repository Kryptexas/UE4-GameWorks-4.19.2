// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "Templates/UnrealTypeTraits.h"
#include "IntegerSequence.h"

/** An array with a static number of elements. */
template<typename TElement, uint32 NumElements, uint32 Alignment = alignof(TElement)>
class alignas(Alignment) TStaticArray
{
public:
	TStaticArray() 
		: Storage()
	{}

	explicit TStaticArray(const TElement& DefaultElement) 
		: Storage(TMakeIntegerSequence<uint32, NumElements>(), DefaultElement)
	{}

	TStaticArray(TStaticArray&& Other) = default;
	TStaticArray(const TStaticArray& Other) = default;
	TStaticArray& operator=(TStaticArray&& Other) = default;
	TStaticArray& operator=(const TStaticArray& Other) = default;

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,TStaticArray& StaticArray)
	{
		for(uint32 Index = 0;Index < NumElements;++Index)
		{
			Ar << StaticArray[Index];
		}
		return Ar;
	}

	// Accessors.
	TElement& operator[](uint32 Index)
	{
		check(Index < NumElements);
		return Storage.Elements[Index].Element;
	}

	const TElement& operator[](uint32 Index) const
	{
		check(Index < NumElements);
		return Storage.Elements[Index].Element;
	}

	// Comparisons.
	friend bool operator==(const TStaticArray& A,const TStaticArray& B)
	{
		for(uint32 ElementIndex = 0;ElementIndex < NumElements;++ElementIndex)
		{
			if(!(A[ElementIndex] == B[ElementIndex]))
			{
				return false;
			}
		}
		return true;
	}

	friend bool operator!=(const TStaticArray& A,const TStaticArray& B)
	{
		for(uint32 ElementIndex = 0;ElementIndex < NumElements;++ElementIndex)
		{
			if(!(A[ElementIndex] == B[ElementIndex]))
			{
				return true;
			}
		}
		return false;
	}

	/** The number of elements in the array. */
	int32 Num() const { return NumElements; }
	
	/** Hash function. */
	friend uint32 GetTypeHash(const TStaticArray& Array)
	{
		uint32 Result = 0;
		for(uint32 ElementIndex = 0;ElementIndex < NumElements;++ElementIndex)
		{
			Result ^= GetTypeHash(Array[ElementIndex]);
		}
		return Result;
	}

private:

	struct alignas(Alignment) TArrayStorageElementAligned
	{
		TArrayStorageElementAligned() {}
		TArrayStorageElementAligned(const TElement& InElement)
			: Element(InElement)
		{
		}

		TElement Element;
	};

	struct TArrayStorage
	{
		TArrayStorage()
			: Elements()
		{
		}

		template<uint32... Indices>
		TArrayStorage(TIntegerSequence<uint32, Indices...>, const TElement& DefaultElement)
			: Elements { ((void)Indices, DefaultElement)... } //Integer Sequence pack expansion duplicates DefaultElement NumElements times and the comma operator throws away the index
		{
		}

		TArrayStorageElementAligned Elements[NumElements];
	};

	TArrayStorage Storage;
};

/** Creates a static array filled with the specified value. */
template<typename TElement,uint32 NumElements>
TStaticArray<TElement,NumElements> MakeUniformStaticArray(typename TCallTraits<TElement>::ParamType InValue)
{
	TStaticArray<TElement,NumElements> Result;
	for(uint32 ElementIndex = 0;ElementIndex < NumElements;++ElementIndex)
	{
		Result[ElementIndex] = InValue;
	}
	return Result;
}

