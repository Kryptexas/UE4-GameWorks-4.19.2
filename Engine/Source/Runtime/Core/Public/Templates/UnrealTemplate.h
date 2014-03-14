// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UnrealTemplate.h: Unreal common template definitions.
=============================================================================*/

#pragma once

#include "EnableIf.h"

/*-----------------------------------------------------------------------------
	Standard templates.
-----------------------------------------------------------------------------*/

#if PLATFORM_SUPPORTS_CanConvertPointerFromTo

/** 
* CanConvertPointerFromTo<From, To>::Result is an enum value equal to 1 if From* is automatically convertable to a To* (without regard to const!)
**/
template<class From, class To>
class CanConvertPointerFromTo
{
	static uint8 Test(...);
	static uint16 Test(To const*);
public:
	enum Type
	{
		Result = sizeof(Test((From*)0)) - 1
	};
};

class CanConvertPointerFromTo_TestBase
{
};

class CanConvertPointerFromTo_TestDerived : public CanConvertPointerFromTo_TestBase
{
};

class CanConvertPointerFromTo_Unrelated
{
};

checkAtCompileTime((CanConvertPointerFromTo<bool, bool>::Result), Platform_failed__CanConvertPointerFromTo1);
checkAtCompileTime((CanConvertPointerFromTo<void, void>::Result), Platform_failed__CanConvertPointerFromTo2);
checkAtCompileTime((CanConvertPointerFromTo<bool, void>::Result), Platform_failed__CanConvertPointerFromTo2b);
checkAtCompileTime((CanConvertPointerFromTo<const bool, void>::Result), Platform_failed__CanConvertPointerFromTo3);
checkAtCompileTime((CanConvertPointerFromTo<CanConvertPointerFromTo_TestDerived, CanConvertPointerFromTo_TestBase>::Result), Platform_failed__CanConvertPointerFromTo4);
checkAtCompileTime((CanConvertPointerFromTo<CanConvertPointerFromTo_TestDerived, const CanConvertPointerFromTo_TestBase>::Result), Platform_failed__CanConvertPointerFromTo5);
checkAtCompileTime((CanConvertPointerFromTo<const CanConvertPointerFromTo_TestDerived, CanConvertPointerFromTo_TestBase>::Result), Platform_failed__CanConvertPointerFromTo6);
checkAtCompileTime((CanConvertPointerFromTo<const CanConvertPointerFromTo_TestDerived, const CanConvertPointerFromTo_TestBase>::Result), Platform_failed__CanConvertPointerFromTo7);
checkAtCompileTime((CanConvertPointerFromTo<CanConvertPointerFromTo_TestBase, CanConvertPointerFromTo_TestBase>::Result), Platform_failed__CanConvertPointerFromTo8);
checkAtCompileTime((CanConvertPointerFromTo<CanConvertPointerFromTo_TestBase, void>::Result), Platform_failed__CanConvertPointerFromTo9);

checkAtCompileTime(!(CanConvertPointerFromTo<CanConvertPointerFromTo_TestBase, CanConvertPointerFromTo_TestDerived>::Result), Platform_failed__CanConvertPointerFromTo10);
checkAtCompileTime(!(CanConvertPointerFromTo<CanConvertPointerFromTo_Unrelated, CanConvertPointerFromTo_TestBase>::Result), Platform_failed__CanConvertPointerFromTo10b);
checkAtCompileTime(!(CanConvertPointerFromTo<bool, CanConvertPointerFromTo_TestBase>::Result), Platform_failed__CanConvertPointerFromTo11);
checkAtCompileTime(!(CanConvertPointerFromTo<void, CanConvertPointerFromTo_TestBase>::Result), Platform_failed__CanConvertPointerFromTo12);
checkAtCompileTime(!(CanConvertPointerFromTo<CanConvertPointerFromTo_TestBase, bool>::Result), Platform_failed__CanConvertPointerFromTo13);
checkAtCompileTime(!(CanConvertPointerFromTo<void, bool>::Result), Platform_failed__CanConvertPointerFromTo14);

#endif



/**
 * Aligns a value to the nearest higher multiple of 'Alignment', which must be a power of two.
 *
 * @param Ptr			Value to align
 * @param Alignment		Alignment, must be a power of two
 * @return				Aligned value
 */
template< class T > inline T Align( const T Ptr, int32 Alignment )
{
	return (T)(((PTRINT)Ptr + Alignment - 1) & ~(Alignment-1));
}
/**
 * Aligns a value to the nearest higher multiple of 'Alignment'.
 *
 * @param Ptr			Value to align
 * @param Alignment		Alignment, can be any arbitrary uint32
 * @return				Aligned value
 */
template< class T > inline T AlignArbitrary( const T Ptr, uint32 Alignment )
{
	return (T) ( ( ((UPTRINT)Ptr + Alignment - 1) / Alignment ) * Alignment );
}
template< class T > inline void Swap( T& A, T& B )
{
	const T Temp = A;
	A = B;
	B = Temp;
}
template< class T > inline void Exchange( T& A, T& B )
{
	Swap(A, B);
}

/**
 * Chooses between the two parameters based on whether the first is NULL or not.
 * @return If the first parameter provided is non-NULL, it is returned; otherwise the second parameter is returned.
 */
template<typename ReferencedType>
FORCEINLINE ReferencedType* IfAThenAElseB(ReferencedType* A,ReferencedType* B)
{
	const PTRINT IntA = reinterpret_cast<PTRINT>(A);
	const PTRINT IntB = reinterpret_cast<PTRINT>(B);

	// Compute a mask which has all bits set if IntA is zero, and no bits set if it's non-zero.
	const PTRINT MaskB = -(!IntA);

	return reinterpret_cast<ReferencedType*>(IntA | (MaskB & IntB));
}

/** branchless pointer selection based on predicate
* return PTRINT(Predicate) ? A : B;
**/
template<typename PredicateType,typename ReferencedType>
FORCEINLINE ReferencedType* IfPThenAElseB(PredicateType Predicate,ReferencedType* A,ReferencedType* B)
{
	const PTRINT IntA = reinterpret_cast<PTRINT>(A);
	const PTRINT IntB = reinterpret_cast<PTRINT>(B);

	// Compute a mask which has all bits set if Predicate is zero, and no bits set if it's non-zero.
	const PTRINT MaskB = -(!PTRINT(Predicate));

	return reinterpret_cast<ReferencedType*>((IntA & ~MaskB) | (IntB & MaskB));
}

/** A logical exclusive or function. */
inline bool XOR(bool A, bool B)
{
	return A != B;
}

/** This is used to provide type specific behavior for a copy which cannot change the value of B. */
template<typename T>
FORCEINLINE void Move(T& A,typename TMoveSupportTraits<T>::Copy B)
{
	// Destruct the previous value of A.
	A.~T();

	// Use placement new and a copy constructor so types with const members will work.
	new(&A) T(B);
}

/** This is used to provide type specific behavior for a move which may change the value of B. */
template<typename T>
FORCEINLINE void Move(T& A,typename TMoveSupportTraits<T>::Move B)
{
	// Destruct the previous value of A.
	A.~T();

	// Use placement new and a copy constructor so types with const members will work.
	new(&A) T(MoveTemp(B));
}
/*----------------------------------------------------------------------------
	Standard macros.
----------------------------------------------------------------------------*/

template <typename T, uint32 N>
char (&ArrayCountHelper(const T (&)[N]))[N];

// Number of elements in an array.
#define ARRAY_COUNT( array ) sizeof(ArrayCountHelper(array))

// Offset of a struct member.
#define STRUCT_OFFSET( struc, member )	offsetof(struc, member)

#if PLATFORM_VTABLE_AT_END_OF_CLASS
	#error need implementation
#else
	#define VTABLE_OFFSET( Class, MultipleInheritenceParent )	( ((PTRINT) static_cast<MultipleInheritenceParent*>((Class*)1)) - 1)
#endif


/*-----------------------------------------------------------------------------
	Allocators.
-----------------------------------------------------------------------------*/

template <class T> class TAllocator
{};

/**
 * works just like std::min_element.
 */
template<class ForwardIt> inline
ForwardIt MinElement(ForwardIt First, ForwardIt Last)
{
	ForwardIt Result = First;
	for (; ++First != Last; )
	{
		if (*First < *Result) 
		{
			Result = First;
		}
	}
	return Result;
}

/**
 * works just like std::min_element.
 */
template<class ForwardIt, class PredicateType> inline
ForwardIt MinElement(ForwardIt First, ForwardIt Last, PredicateType Predicate)
{
	ForwardIt Result = First;
	for (; ++First != Last; )
	{
		if (Predicate(*First,*Result))
		{
			Result = First;
		}
	}
	return Result;
}

/**
* works just like std::max_element.
*/
template<class ForwardIt> inline
ForwardIt MaxElement(ForwardIt First, ForwardIt Last)
{
	ForwardIt Result = First;
	for (; ++First != Last; )
	{
		if (*Result < *First) 
		{
			Result = First;
		}
	}
	return Result;
}

/**
* works just like std::max_element.
*/
template<class ForwardIt, class PredicateType> inline
ForwardIt MaxElement(ForwardIt First, ForwardIt Last, PredicateType Predicate)
{
	ForwardIt Result = First;
	for (; ++First != Last; )
	{
		if (Predicate(*Result,*First))
		{
			Result = First;
		}
	}
	return Result;
}

/**
 * utility template for a class that should not be copyable.
 * Derive from this class to make your class non-copyable
 */
class FNoncopyable
{
protected:
	// ensure the class cannot be constructed directly
	FNoncopyable() {}
	// the class should not be used polymorphically
	~FNoncopyable() {}
private:
	FNoncopyable(const FNoncopyable&);
	FNoncopyable& operator=(const FNoncopyable&);
};


/** 
 * exception-safe guard around saving/restoring a value.
 * Commonly used to make sure a value is restored 
 * even if the code early outs in the future.
 * Usage:
 *  	TGuardValue<bool> GuardSomeBool(bSomeBool, false); // Sets bSomeBool to false, and restores it in dtor.
 */
template <typename Type>
struct TGuardValue : private FNoncopyable
{
	TGuardValue(Type& ReferenceValue, const Type& NewValue)
	: RefValue(ReferenceValue), OldValue(ReferenceValue)
	{
		RefValue = NewValue;
	}
	~TGuardValue()
	{
		RefValue = OldValue;
	}

	/**
	 * Overloaded dereference operator.
	 * Provides read-only access to the original value of the data being tracked by this struct
	 *
	 * @return	a const reference to the original data value
	 */
	FORCEINLINE const Type& operator*() const
	{
		return OldValue;
	}

private:
	Type& RefValue;
	Type OldValue;
};

/** Chooses between two different classes based on a boolean. */
template<bool Predicate,typename TrueClass,typename FalseClass>
class TChooseClass;

template<typename TrueClass,typename FalseClass>
class TChooseClass<true,TrueClass,FalseClass>
{
public:
	typedef TrueClass Result;
};

template<typename TrueClass,typename FalseClass>
class TChooseClass<false,TrueClass,FalseClass>
{
public:
	typedef FalseClass Result;
};

/**
 * Helper class to make it easy to use key/value pairs with a container.
 */
template <typename KeyType, typename ValueType>
struct TKeyValuePair
{
	TKeyValuePair( const KeyType& InKey, const ValueType& InValue )
	:	Key(InKey), Value(InValue)
	{
	}
	TKeyValuePair( const KeyType& InKey )
	:	Key(InKey)
	{
	}
	TKeyValuePair()
	{
	}
	bool operator==( const TKeyValuePair& Other ) const
	{
		return Key == Other.Key;
	}
	bool operator!=( const TKeyValuePair& Other ) const
	{
		return Key != Other.Key;
	}
	bool operator<( const TKeyValuePair& Other ) const
	{
		return Key < Other.Key;
	}
	FORCEINLINE bool operator()( const TKeyValuePair& A, const TKeyValuePair& B ) const
	{
		return A.Key < B.Key;
	}
	KeyType		Key;
	ValueType	Value;
};

/** Tests whether two typenames refer to the same type. */
template<typename A,typename B>
struct TAreTypesEqual;

template<typename,typename>
struct TAreTypesEqual
{
	enum { Value = false };
};

template<typename A>
struct TAreTypesEqual<A,A>
{
	enum { Value = true };
};

#define ARE_TYPES_EQUAL(A,B) TAreTypesEqual<A,B>::Value

//
// Macros that can be used to specify multiple template parameters in a macro parameter.
// This is necessary to prevent the macro parsing from interpreting the template parameter
// delimiting comma as a macro parameter delimiter.
// 

#define TEMPLATE_PARAMETERS2(X,Y) X,Y


/*
 * TRemoveReference<type> will remove any references from a type.
 */
template <typename T> struct TRemoveReference      { typedef T Type; };
template <typename T> struct TRemoveReference<T& > { typedef T Type; };
template <typename T> struct TRemoveReference<T&&> { typedef T Type; };

/*
 * MoveTemp will cast a reference to an rvalue reference.
 * This is UE's equivalent of std::move.
 */
template <typename T>
FORCEINLINE typename TRemoveReference<T>::Type&& MoveTemp(T&& Obj)
{
	return (typename TRemoveReference<T>::Type&&)Obj;
}

/**
 * Forward will cast a reference to an rvalue reference.
 * This is UE's equivalent of std::forward.
 */
template <typename T>
FORCEINLINE T&& Forward(typename TRemoveReference<T>::Type& Obj)
{
	return (T&&)Obj;
}

template <typename T>
FORCEINLINE T&& Forward(typename TRemoveReference<T>::Type&& Obj)
{
	return (T&&)Obj;
}

/**
 * This exists to avoid a Visual Studio bug where using a cast to forward an rvalue reference array argument
 * to a pointer parameter will cause bad code generation.  Wrapping the cast in a function causes the correct
 * code to be generated.
 */
template <typename T, typename ArgType>
FORCEINLINE T StaticCast(ArgType&& Arg)
{
	return static_cast<T>(Arg);
}

/**
 * TRValueToLValueReference converts any rvalue reference type into the equivalent lvalue reference, otherwise returns the same type.
 */
template <typename T> struct TRValueToLValueReference      { typedef T  Type; };
template <typename T> struct TRValueToLValueReference<T&&> { typedef T& Type; };

/**
 * A traits class which tests if a type is a C++ array.
 */
template <typename T>           struct TIsCPPArray       { enum { Value = false }; };
template <typename T, uint32 N> struct TIsCPPArray<T[N]> { enum { Value = true  }; };

/**
 * Reverses the order of the bits of a value.
 * This is an TEnableIf'd template to ensure that no undesirable conversions occur.  Overloads for other types can be added in the same way.
 *
 * @param Bits - The value to bit-swap.
 * @return The bit-swapped value.
 */
template <typename T>
FORCEINLINE typename TEnableIf<TAreTypesEqual<T, uint32>::Value, T>::Type ReverseBits( T Bits )
{
	Bits = ( Bits << 16) | ( Bits >> 16);
	Bits = ( (Bits & 0x00ff00ff) << 8 ) | ( (Bits & 0xff00ff00) >> 8 );
	Bits = ( (Bits & 0x0f0f0f0f) << 4 ) | ( (Bits & 0xf0f0f0f0) >> 4 );
	Bits = ( (Bits & 0x33333333) << 2 ) | ( (Bits & 0xcccccccc) >> 2 );
	Bits = ( (Bits & 0x55555555) << 1 ) | ( (Bits & 0xaaaaaaaa) >> 1 );
	return Bits;
}

/** Template for initializing a singleton at the boot. */
template< class T >
struct TForceInitAtBoot
{
	TForceInitAtBoot()
	{
		T::Get();
	}
};

/**
 * Copies the cv-qualifiers from one type to another, e.g.:
 *
 * TCopyQualifiersFromTo<const    T1,       T2>::Type == const T2
 * TCopyQualifiersFromTo<volatile T1, const T2>::Type == const volatile T2
 */
template <typename From, typename To> struct TCopyQualifiersFromTo                          { typedef                To Type; };
template <typename From, typename To> struct TCopyQualifiersFromTo<const          From, To> { typedef const          To Type; };
template <typename From, typename To> struct TCopyQualifiersFromTo<      volatile From, To> { typedef       volatile To Type; };
template <typename From, typename To> struct TCopyQualifiersFromTo<const volatile From, To> { typedef const volatile To Type; };
