// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Template for numeric interval
 */
template<typename ElementType> struct TInterval
{
	static_assert(TIsArithmeticType<ElementType>::Value, "Interval can be used only with numeric types");
	
	/** Holds the lower bound of the interval. */
	ElementType Min;
	
	/** Holds the upper bound of the interval. */
	ElementType Max;
		
public:

	/**
	 * Default constructor.
	 *
	 * The interval is empty
	 */
	TInterval()
		: Min(TNumericLimits<ElementType>::Max())
		, Max(TNumericLimits<ElementType>::Lowest())
	{ }

    /**
	 * Creates and initializes a new interval with the specified lower and upper bounds.
	 *
	 * @param InMin The lower bound of the constructed interval.
	 * @param InMax The upper bound of the constructed interval.
	 */
	TInterval( ElementType InMin, ElementType InMax )
		: Min(InMin)
		, Max(InMax)
	{ }

public:

	/**
	 * Offset the interval by adding X.
	 *
	 * @param X The offset.
	 */
	void operator+= ( ElementType X )
	{
		if (!IsEmpty())
		{
			Min += X;
			Max += X;
		}
	}

	/**
	 * Offset the interval by subtracting X.
	 *
	 * @param X The offset.
	 */
	void operator-= ( ElementType X )
	{
		if (!IsEmpty())
		{
			Min -= X;
			Max -= X;
		}
	}

public:
	
	/**
	 * Computes the size of this interval.
	 *
	 * @return Interval size.
	 */
	ElementType Size() const
	{
		return (Max - Min);
	}

	/**
	 * Whether interval is empty.
	 *
	 * @return false when interval is empty, true otherwise
	 */
	ElementType IsEmpty() const
	{
		return (Min >= Max);
	}
	
	/**
	 * Checks whether this interval contains the specified element.
	 *
	 * @param Element The element to check.
	 * @return true if the range interval the element, false otherwise.
	 */
	bool Contains( const ElementType& Element ) const
	{
		return !IsEmpty() && (Element >= Min && Element <= Max);
	}

	/**
	 * Expands this interval to both sides by the specified amount.
	 *
	 * @param ExpandAmount The amount to expand by.
	 */
	void Expand( ElementType ExpandAmount )
	{
		if (!IsEmpty())
		{
			Min -= ExpandAmount;
			Max += ExpandAmount;
		}
	}

	/**
	 * Expands this interval if necessary to include the specified element.
	 *
	 * @param X The element to include.
	 */
	void Include( ElementType X )
	{
		if (IsEmpty())
		{
			Min = X;
			Max = X;
		}
		else
		{
			if (X < Min)
			{
				Min = X;
			}

			if (X > Max)
			{
				Max = X;
			}
		}
	}

	/**
	 * Interval interpolation
	 *
	 * @param Alpha interpolation amount
	 * @return interpolation result
	 */
	ElementType Interpolate( float Alpha ) const
	{
		if (!IsEmpty())
		{
			return Min + ElementType(Alpha*Size());
		}
		
		return ElementType();
	}

public:

	/**
	 * Calculates the intersection of two intervals.
	 *
	 * @param A The first interval.
	 * @param B The second interval.
	 * @return The intersection.
	 */
	friend TInterval Intersect( const TInterval& A, const TInterval& B )
	{
		if (A.IsEmpty() || B.IsEmpty())
		{
			return TInterval();
		}

		return TInterval(FMath::Max(A.Min, B.Min), FMath::Min(A.Max, B.Max));
	}

	/**
	 * Serializes the interval.
	 *
	 * @param Ar The archive to serialize into.
	 * @param Interval The interval to serialize.
	 * @return Reference to the Archive after serialization.
	 */
	friend class FArchive& operator<<( class FArchive& Ar, TInterval& Interval )
	{
		return Ar << Interval.Min << Interval.Max;
	}
	
	/**
	 * Gets the hash for the specified interval.
	 *
	 * @param Interval The Interval to get the hash for.
	 * @return Hash value.
	 */
	friend uint32 GetTypeHash(const TInterval& Interval)
	{
		return HashCombine(GetTypeHash(Interval.Min), GetTypeHash(Interval.Max));
	}
};

/* Default intervals for built-in types
 *****************************************************************************/

typedef TInterval<float> FInterval;
typedef TInterval<float> FFloatInterval;
typedef TInterval<int32> FInt32Interval;
