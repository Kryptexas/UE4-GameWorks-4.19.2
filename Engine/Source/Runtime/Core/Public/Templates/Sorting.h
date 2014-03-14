// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Sorting.h: Generic sorting definitions.
=============================================================================*/

#pragma once

/*----------------------------------------------------------------------------
	Sorting template.
----------------------------------------------------------------------------*/

/**
 * Default predicate class. Assumes < operator is defined for the template type.
 */
template<typename T>
struct TLess
{
	FORCEINLINE bool operator()(const T& A, const T& B) const { return A < B; }
};

/**
 * Utility predicate class. Assumes < operator is defined for the template type.
 */
template<typename T>
struct TGreater
{
	FORCEINLINE bool operator()(const T& A, const T& B) const { return B < A; }
};

/**
 * Helper class for dereferencing pointer types in Sort function
 */
template<typename T, class PREDICATE_CLASS> 
struct TDereferenceWrapper
{
	const PREDICATE_CLASS& Predicate;

	TDereferenceWrapper( const PREDICATE_CLASS& InPredicate )
		: Predicate( InPredicate ) {}
  
	/** Pass through for non-pointer types */
	FORCEINLINE bool operator()( T& A, T& B ) { return Predicate( A, B ); } 
	FORCEINLINE bool operator()( const T& A, const T& B ) const { return Predicate( A, B ); } 
};
/** Partially specialized version of the above class */
template<typename T, class PREDICATE_CLASS> 
struct TDereferenceWrapper<T*, PREDICATE_CLASS>
{
	const PREDICATE_CLASS& Predicate;

	TDereferenceWrapper( const PREDICATE_CLASS& InPredicate )
		: Predicate( InPredicate ) {}
  
	/** Dereferennce pointers */
	FORCEINLINE bool operator()( T* A, T* B ) const 
	{
		check( A != NULL );
		check( B != NULL );
		return Predicate( *A, *B ); 
	} 
};

/**
 * Sort elements using user defined predicate class. The sort is unstable, meaning that the ordering of equal items is not necessarily preserved.
 * This is the internal sorting function used by Sort overrides.
 *
 * @param	First	pointer to the first element to sort
 * @param	Num		the number of items to sort
 * @param Predicate predicate class
 * @param Deref dereference class used to dereference pointer types
 */
template<class T, class PREDICATE_CLASS> 
void SortInternal( T* First, const int32 Num, const PREDICATE_CLASS& Predicate )
{
	struct FStack
	{
		T* Min;
		T* Max;
	};

	if( Num < 2 )
	{
		return;
	}
	FStack RecursionStack[32]={{First,First+Num-1}}, Current, Inner;
	for( FStack* StackTop=RecursionStack; StackTop>=RecursionStack; --StackTop )
	{
		Current = *StackTop;
	Loop:
		PTRINT Count = Current.Max - Current.Min + 1;
		if( Count <= 8 )
		{
			// Use simple bubble-sort.
			while( Current.Max > Current.Min )
			{
				T *Max, *Item;
				for( Max=Current.Min, Item=Current.Min+1; Item<=Current.Max; Item++ )
				{
					if( Predicate( *Max, *Item ) )
					{
						Max = Item;
					}
				}
				Exchange( *Max, *Current.Max-- );
			}
		}
		else
		{
			// Grab middle element so sort doesn't exhibit worst-cast behavior with presorted lists.
			Exchange( Current.Min[Count/2], Current.Min[0] );

			// Divide list into two halves, one with items <=Current.Min, the other with items >Current.Max.
			Inner.Min = Current.Min;
			Inner.Max = Current.Max+1;
			for( ; ; )
			{
				while( ++Inner.Min<=Current.Max && !Predicate( *Current.Min, *Inner.Min ) );
				while( --Inner.Max> Current.Min && !Predicate( *Inner.Max, *Current.Min ) );
				if( Inner.Min>Inner.Max )
				{
					break;
				}
				Exchange( *Inner.Min, *Inner.Max );
			}
			Exchange( *Current.Min, *Inner.Max );

			// Save big half and recurse with small half.
			if( Inner.Max-1-Current.Min >= Current.Max-Inner.Min )
			{
				if( Current.Min+1 < Inner.Max )
				{
					StackTop->Min = Current.Min;
					StackTop->Max = Inner.Max - 1;
					StackTop++;
				}
				if( Current.Max>Inner.Min )
				{
					Current.Min = Inner.Min;
					goto Loop;
				}
			}
			else
			{
				if( Current.Max>Inner.Min )
				{
					StackTop->Min = Inner  .Min;
					StackTop->Max = Current.Max;
					StackTop++;
				}
				if( Current.Min+1<Inner.Max )
				{
					Current.Max = Inner.Max - 1;
					goto Loop;
				}
			}
		}
	}
}

/**
 * Sort elements using user defined predicate class. The sort is unstable, meaning that the ordering of equal items is not necessarily preserved.
 *
 * @param	First	pointer to the first element to sort
 * @param	Num		the number of items to sort
 * @param Predicate predicate class
 */
template<class T, class PREDICATE_CLASS> 
void Sort( T* First, const int32 Num, const PREDICATE_CLASS& Predicate )
{
	SortInternal( First, Num, TDereferenceWrapper<T, PREDICATE_CLASS>( Predicate ) );
}

/**
 * Specialized version of the above Sort function for pointers to elements.
 *
 * @param	First	pointer to the first element to sort
 * @param	Num		the number of items to sort
 * @param Predicate predicate class
 */
template<class T, class PREDICATE_CLASS> 
void Sort( T** First, const int32 Num, const PREDICATE_CLASS& Predicate )
{
	SortInternal( First, Num, TDereferenceWrapper<T*, PREDICATE_CLASS>( Predicate ) );
}

/**
 * Sort elements. The sort is unstable, meaning that the ordering of equal items is not necessarily preserved.
 * Assumes < operator is defined for the template type.
 *
 * @param	First	pointer to the first element to sort
 * @param	Num		the number of items to sort
 */
template<class T> 
void Sort( T* First, const int32 Num )
{
	SortInternal( First, Num, TDereferenceWrapper<T, TLess<T> >( TLess<T>() ) );
}

/**
 * Specialized version of the above Sort function for pointers to elements.
 *
 * @param	First	pointer to the first element to sort
 * @param	Num		the number of items to sort
 */
template<class T> 
void Sort( T** First, const int32 Num )
{
	SortInternal( First, Num, TDereferenceWrapper<T*, TLess<T> >( TLess<T>() ) );
}

