// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UnrealTemplate.h"

// Single-ownership smart pointer in the vein of std::unique_ptr.
// Use this when you need an object's lifetime to be strictly bound to the lifetime of a single smart pointer.
//
// This class is non-copyable - ownership can only be transferred via a move operation, e.g.:
//
// TUniquePtr<MyClass> Ptr1(new MyClass);    // The MyClass object is owned by Ptr1.
// TUniquePtr<MyClass> Ptr2(Ptr1);           // Error - TUniquePtr is not copyable
// TUniquePtr<MyClass> Ptr3(MoveTemp(Ptr1)); // Ptr3 now owns the MyClass object - Ptr1 is now NULL.

template <typename T>
class TUniquePtr
{
public:
	/**
	 * Default constructor - initializes the TUniquePtr to null.
	 */
	FORCEINLINE TUniquePtr()
		: Ptr(NULL)
	{
	}

	/**
	 * Pointer constructor - takes ownership of the pointed-to object
	 *
	 * @param InPtr The pointed-to object to take ownership of.
	 */
	explicit FORCEINLINE TUniquePtr(T* InPtr)
		: Ptr(InPtr)
	{
	}

	/**
	 * Move constructor
	 */
	FORCEINLINE TUniquePtr(TUniquePtr&& Other)
		: Ptr(Other.Ptr)
	{
		Other.Ptr = NULL;
	}

	/**
	 * Move assignment operator
	 */
	FORCEINLINE TUniquePtr& operator=(TUniquePtr&& Other)
	{
		if (this != &Other)
		{
			// We delete last, because we don't want odd side effects if the destructor of T relies on the state of this or Other
			T* OldPtr = Ptr;
			Ptr = Other.Ptr;
			Other.Ptr = NULL;
			delete OldPtr;
		}

		return *this;
	}

	/**
	 * Destructor
	 */
	FORCEINLINE ~TUniquePtr()
	{
		delete Ptr;
	}

	/**
	 * operator bool
	 *
	 * @return true if the TUniquePtr currently owns an object, convertible-to-false otherwise.
	 */
	FORCEINLINE_EXPLICIT_OPERATOR_BOOL() const
	{
		return !!Ptr;
	}

	/**
	 * Logical not operator
	 *
	 * @return false if the TUniquePtr currently owns an object, true otherwise.
	 */
	FORCEINLINE bool operator!() const
	{
		return !Ptr;
	}

	/**
	 * Indirection operator
	 *
	 * @return A pointer to the object owned by the TUniquePtr.
	 */
	FORCEINLINE T* operator->() const
	{
		return Ptr;
	}

	/**
	 * Dereference operator
	 *
	 * @return A reference to the object owned by the TUniquePtr.
	 */
	FORCEINLINE T& operator*() const
	{
		return *Ptr;
	}

	/**
	 * Equality comparison operator
	 *
	 * @param Lhs The first TUniquePtr to compare.
	 * @param Rhs The second TUniquePtr to compare.
	 *
	 * @return true if the two TUniquePtrs are logically substitutable for each other, false otherwise.
	 */
	friend FORCEINLINE bool operator==(const TUniquePtr& Lhs, const TUniquePtr& Rhs)
	{
		return Lhs.Ptr == Rhs.Ptr;
	}

	/**
	 * Inequality comparison operator
	 *
	 * @param Lhs The first TUniquePtr to compare.
	 * @param Rhs The second TUniquePtr to compare.
	 *
	 * @return false if the two TUniquePtrs are logically substitutable for each other, true otherwise.
	 */
	friend FORCEINLINE bool operator!=(const TUniquePtr& Lhs, const TUniquePtr& Rhs)
	{
		return Lhs.Ptr != Rhs.Ptr;
	}

	/**
	 * Returns a pointer to the owned object without relinquishing ownership.
	 *
	 * @return A copy of the pointer to the object owned by the TUniquePtr, or NULL if no object is being owned.
	 */
	FORCEINLINE T* Get() const
	{
		return Ptr;
	}

	/**
	 * Relinquishes control of the owned object to the caller and nulls the TUniquePtr.
	 *
	 * @return The pointer to the object that was owned by the TUniquePtr, or NULL if no object was being owned.
	 */
	FORCEINLINE T* Release()
	{
		T* Result = Ptr;
		Ptr = NULL;
		return Result;
	}

	/**
	 * Replaces any owned object with aGives the TUniquePtr a new object to own, destroying any previously-owned object.
	 *
	 * @param InPtr A pointer to the object to take ownership of.
	 */
	FORCEINLINE void Reset(T* InPtr = NULL)
	{
		// We delete last, because we don't want odd side effects if the destructor of T relies on the state of this or Other
		T* OldPtr = Ptr;
		Ptr = InPtr;
		delete OldPtr;
	}

private:
	// Non-copyable
	TUniquePtr(const TUniquePtr&);
	TUniquePtr& operator=(const TUniquePtr&);

	T* Ptr;
};

template <typename T> struct TIsZeroConstructType<TUniquePtr<T>> { enum { Value = true }; };

#if PLATFORM_COMPILER_HAS_VARIADIC_TEMPLATES

	/**
	 * Constructs a new object with the given arguments and returns it as a TUniquePtr.
	 *
	 * @param Args The arguments to pass to the constructor of T.
	 *
	 * @return A TUniquePtr which points to a newly-constructed T with the specified Args.
	 */
	template <typename T, typename... TArgs>
	FORCEINLINE TUniquePtr<T> MakeUnique(TArgs&&... Args)
	{
		return TUniquePtr<T>(new T(Forward<TArgs>(Args)...));
	}

#else

	/**
	 * Constructs a new object with the given arguments and returns it as a TUniquePtr.
	 *
	 * @param Args The arguments to pass to the constructor of T.
	 *
	 * @return A TUniquePtr which points to a newly-constructed T with the specified Args.
	 *
	 * Note: Need to expand this for general argument list arity.
	 */
	template <typename T>
	FORCEINLINE TUniquePtr<T> MakeUnique()
	{
		return TUniquePtr<T>(new T());
	}

	template <typename T, typename TArg0>
	FORCEINLINE TUniquePtr<T> MakeUnique(TArg0&& Arg0)
	{
		return TUniquePtr<T>(new T(Forward<TArg0>(Arg0)));
	}

#endif
