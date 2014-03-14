// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MemoryOps.h: Functions to do bulk operations on contiguous blocks of objects.
=============================================================================*/

#pragma once

#include <new>

#include "UnrealMemory.h"
#include "Platform.h"
#include "UnrealTemplate.h"
#include "UnrealTypeTraits.h"

/**
 * Default constructs a range of items in memory.
 *
 * @param	Elements	The address of the first memory location to construct at.
 * @param	Count		The number of elements to destruct.
 */
template <typename ElementType>
FORCEINLINE typename TEnableIf<!TIsZeroConstructType<ElementType>::Value>::Type DefaultConstructItems(void* Address, int32 Count)
{
	ElementType* Element = (ElementType*)Address;
	while (Count)
	{
		new (Element) ElementType;
		++Element;
		--Count;
	}
}

template <typename ElementType>
FORCEINLINE typename TEnableIf<TIsZeroConstructType<ElementType>::Value>::Type DefaultConstructItems(void* Elements, int32 Count)
{
	FMemory::Memset(Elements, 0, sizeof(ElementType) * Count);
}

/**
 * Destructs a range of items in memory.
 *
 * @param	Elements	A pointer to the first item to destruct.
 * @param	Count		The number of elements to destruct.
 */
template <typename ElementType>
FORCEINLINE typename TEnableIf<TTypeTraits<ElementType>::NeedsDestructor>::Type DestructItems(ElementType* Element, int32 Count)
{
	while (Count)
	{
		// We need a typedef here because VC won't compile the destructor call below if ElementType itself has a member called ElementType
		typedef ElementType DestructItemsElementTypeTypedef;

		Element->DestructItemsElementTypeTypedef::~DestructItemsElementTypeTypedef();
		++Element;
		--Count;
	}
}

template <typename ElementType>
FORCEINLINE typename TEnableIf<!TTypeTraits<ElementType>::NeedsDestructor>::Type DestructItems(ElementType* Elements, int32 Count)
{
}

/**
 * Copy constructs a range of items into memory.
 *
 * @param	Dest		The memory location to start copying into.
 * @param	Source		A pointer to the first item to copy from.
 * @param	Count		The number of elements to copy.
 */
template <typename ElementType>
FORCEINLINE typename TEnableIf<TTypeTraits<ElementType>::NeedsCopyConstructor>::Type CopyConstructItems(void* Dest, const ElementType* Source, int32 Count)
{
	while (Count)
	{
		new (Dest) ElementType(*Source);
		++(ElementType*&)Dest;
		++Source;
		--Count;
	}
}

template <typename ElementType>
FORCEINLINE typename TEnableIf<!TTypeTraits<ElementType>::NeedsCopyConstructor>::Type CopyConstructItems(void* Dest, const ElementType* Source, int32 Count)
{
	FMemory::Memcpy(Dest, Source, sizeof(ElementType) * Count);
}

/**
 * Copy assigns a range of items.
 *
 * @param	Dest		The memory location to start assigning to.
 * @param	Source		A pointer to the first item to assign.
 * @param	Count		The number of elements to assign.
 */
template <typename ElementType>
FORCEINLINE typename TEnableIf<TTypeTraits<ElementType>::NeedsCopyAssignment>::Type CopyAssignItems(ElementType* Dest, const ElementType* Source, int32 Count)
{
	while (Count)
	{
		*Dest = *Source;
		++Dest;
		++Source;
		--Count;
	}
}

template <typename ElementType>
FORCEINLINE typename TEnableIf<!TTypeTraits<ElementType>::NeedsCopyAssignment>::Type CopyAssignItems(ElementType* Dest, const ElementType* Source, int32 Count)
{
	FMemory::Memcpy(Dest, Source, sizeof(ElementType) * Count);
}

/**
 * Relocates a range of items to a new memory location. This is a so-called 'destructive move' for which
 * there is no single operation in C++ but which can be implemented very efficiently in general.
 *
 * @param	Dest		The memory location to relocate to.
 * @param	Source		A pointer to the first item to relocate.
 * @param	Count		The number of elements to relocate.
 */
template <typename ElementType>
FORCEINLINE void RelocateItems(void* Dest, const ElementType* Source, int32 Count)
{
	/* All existing UE containers seem to assume trivial relocatability (i.e. memcpy'able) of their members,
	 * so we're going to assume that this is safe here.  However, it's not generally possible to assume this
	 * in general as objects which contain pointers/references to themselves are not safe to be trivially
	 * relocated.
	 *
	 * However, it is not yet possible to automatically infer this at compile time, so we can't enable
	 * different (i.e. safer) implementations anyway. */

	FMemory::Memcpy(Dest, Source, sizeof(ElementType) * Count);
}

#if PLATFORM_COMPILER_HAS_RVALUE_REFERENCES

	/**
	 * Move constructs a range of items into memory.
	 *
	 * @param	Dest		The memory location to start moving into.
	 * @param	Source		A pointer to the first item to move from.
	 * @param	Count		The number of elements to move.
	 */
	template <typename ElementType>
	FORCEINLINE typename TEnableIf<TTypeTraits<ElementType>::NeedsMoveConstructor>::Type MoveConstructItems(void* Dest, ElementType* Source, int32 Count)
	{
		while (Count)
		{
			new (Dest) ElementType((ElementType&&)*Source);
			++(ElementType*&)Dest;
			++Source;
			--Count;
		}
	}

	template <typename ElementType>
	FORCEINLINE typename TEnableIf<!TTypeTraits<ElementType>::NeedsMoveConstructor>::Type MoveConstructItems(void* Dest, ElementType* Source, int32 Count)
	{
		FMemory::Memcpy(Dest, Source, sizeof(ElementType) * Count);
	}

	/**
	 * Move assigns a range of items.
	 *
	 * @param	Dest		The memory location to start move assigning to.
	 * @param	Source		A pointer to the first item to move assign.
	 * @param	Count		The number of elements to move assign.
	 */
	template <typename ElementType>
	FORCEINLINE typename TEnableIf<TTypeTraits<ElementType>::NeedsMoveAssignment>::Type MoveAssignItems(ElementType* Dest, ElementType* Source, int32 Count)
	{
		while (Count)
		{
			*Dest = (ElementType&&)*Source;
			++Dest;
			++Source;
			--Count;
		}
	}

	template <typename ElementType>
	FORCEINLINE typename TEnableIf<!TTypeTraits<ElementType>::NeedsMoveAssignment>::Type MoveAssignItems(ElementType* Dest, ElementType* Source, int32 Count)
	{
		FMemory::Memcpy(Dest, Source, sizeof(ElementType) * Count);
	}

#else

	template <typename ElementType>
	FORCEINLINE void MoveConstructItems(void* Dest, ElementType* Source, int32 Count)
	{
		CopyConstructItems(Dest, Source, Count);
	}

	template <typename ElementType>
	FORCEINLINE void MoveAssignItems(ElementType* Dest, ElementType* Source, int32 Count)
	{
		CopyAssignItems(Dest, Source, Count);
	}

#endif

template <typename ElementType>
FORCEINLINE typename TEnableIf<TTypeTraits<ElementType>::IsBytewiseComparable, bool>::Type CompareItems(const ElementType* A, const ElementType* B, int32 Count)
{
	return !FMemory::Memcmp(A, B, sizeof(ElementType) * Count);
}

template <typename ElementType>
FORCEINLINE typename TEnableIf<!TTypeTraits<ElementType>::IsBytewiseComparable, bool>::Type CompareItems(const ElementType* A, const ElementType* B, int32 Count)
{
	while (Count)
	{
		if (!(*A == *B))
		{
			return false;
		}

		++A;
		++B;
		--Count;
	}

	return true;
}
