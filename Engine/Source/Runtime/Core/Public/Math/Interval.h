// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Template for interval of numbers.
 */
template<typename ElementType> struct TInterval
{
	/** Holds the lower bound of the interval. */
	ElementType Min;
	
	/** Holds the upper bound of the interval. */
	ElementType Max;
	
	/** Holds a flag indicating whether the interval is empty. */
	bool bIsEmpty;

public:

	/**
	 * Default constructor.
	 *
	 * The interval is initialized to [0, 0].
	 */
	TInterval()
		: Min(0)
		, Max(0)
		, bIsEmpty(true)
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
		, bIsEmpty(InMin >= InMax)
	{ }

public:

	/**
	 * Offset the interval by adding X.
	 *
	 * @param X The offset.
	 */
	void operator+= ( ElementType X )
	{
		if (!bIsEmpty)
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
		if (!bIsEmpty)
		{
			Min -= X;
			Max -= X;
		}
	}

public:

	/**
	 * Expands this interval to both sides by the specified amount.
	 *
	 * @param ExpandAmount The amount to expand by.
	 */
	void Expand( ElementType ExpandAmount )
	{
		if (!bIsEmpty)
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
		if (bIsEmpty)
		{
			Min = X;
			Max = X;
			bIsEmpty = false;
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
		if (A.bIsEmpty || B.bIsEmpty)
		{
			return TInterval();
		}

		return TInterval(FMath::Max(A.Min, B.Min), FMath::Min(A.Max, B.Max));
	}
};

/* Default intervals for built-in types
 *****************************************************************************/

typedef TInterval<float> FInterval;
typedef TInterval<float> FFloatInterval;
typedef TInterval<int32> FInt32Interval;
