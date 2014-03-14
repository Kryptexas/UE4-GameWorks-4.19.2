// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Box.h: Declares the FBox class.
=============================================================================*/

#pragma once


/**
 * Implements a rectangular box.
 */
class FBox
{
public:

	/**
	 * Holds the box's minimum point.
	 */
	FVector Min;

	/**
	 * Holds the box's maximum point.
	 */
	FVector Max;

	/**
	 * Holds a flag indicating whether this box is valid.
	 */
	uint8 IsValid;


public:

	/**
	 * Default constructor.
	 */
	FBox( ) { }

	/**
	 * Creates and initializes a new box.
	 */
	FBox( int32 )
	{
		Init();
	}

	/**
	 * Creates and initializes a new box.
	 *
	 * @param EForceInit - Force Init Enum.
	 */
	explicit FBox( EForceInit )
	{
		Init();
	}

	/**
	 * Creates and initializes a new box from the specified parameters.
	 *
	 * @param InMin - The box's minimum point.
	 * @param InMax - The box's maximum point.
	 */
	FBox( const FVector& InMin, const FVector& InMax )
		: Min(InMin)
		, Max(InMax)
		, IsValid(1)
	{ }

	/**
	 * Creates and initializes a new box from the given set of points.
	 *
	 * @param Points - Array of Points to create for the bounding volume.
	 * @param Count - The number of points.
	 */
	CORE_API FBox( const FVector* Points, int32 Count );

	/**
	 * Creates and initializes a new box from an array of points.
	 *
	 * @param Points - Array of Points to create for the bounding volume.
	 */
	CORE_API FBox( const TArray<FVector>& Points );


public:

	/**
	 * Adds to this bounding box to include a given point.
	 *
	 * @param Other the point to increase the bounding volume to.
	 *
	 * @return Reference to this bounding box after resizing to include the other point.
	 */
	FORCEINLINE FBox& operator+=( const FVector &Other );

	/**
	 * Gets the result of addition to this bounding volume.
	 *
	 * @param Other The other point to add to this.
	 *
	 * @return A new bounding volume.
	 */
	FBox operator+( const FVector& Other ) const
	{
		return FBox(*this) += Other;
	}

	/**
	 * Adds to this bounding box to include a new bounding volume.
	 *
	 * @param Other the bounding volume to increase the bounding volume to.
	 *
	 * @return Reference to this bounding volume after resizing to include the other bounding volume.
	 */
	FORCEINLINE FBox& operator+=( const FBox& Other );

	/**
	 * Gets the result of addition to this bounding volume.
	 *
	 * @param Other The other volume to add to this.
	 *
	 * @return A new bounding volume.
	 */
	FBox operator+( const FBox& Other ) const
	{
		return FBox(*this) += Other;
	}

	/**
	 * Gets reference to the min or max of this bounding volume.
	 *
	 * @param Index the index into points of the bounding volume.
	 *
	 * @return a reference to a point of the bounding volume.
	 */
    FVector& operator[]( int32 Index )
	{
		check((Index >= 0) && (Index < 2));

		if (Index == 0)
		{
			return Min;
		}

		return Max;
	}
	bool operator==( const FBox& Other ) const
	{
		return ( Min == Other.Min ) && ( Max == Other.Max );
	}


public:



	/** 
	 * Calculates the distance of a point to this box.
	 *
	 * @param Point - The point.
	 *
	 * @return The distance.
	 */
	FORCEINLINE float ComputeSquaredDistanceToPoint( const FVector& Point ) const
	{
		return ComputeSquaredDistanceFromBoxToPoint(Min, Max, Point);
	}

	/** 
	 * Increase the bounding box volume
	 *
	 * @param	W size to increase volume by
	 * @return	new bounding box increased in size
	 */
	FBox ExpandBy( float W ) const
	{
		return FBox(Min - FVector(W, W, W), Max + FVector(W, W, W));
	}

	/** 
	 * Shift bounding box position
	 *
	 * @param	Offset vector to shift by
	 * @return	new shifted bounding box 
	 */
	FBox ShiftBy( const FVector& Offset ) const
	{
		return FBox(Min + Offset, Max + Offset);
	}


	/**
	 * Gets the box's center point.
	 *
	 * @return Center point.
	 */
	FVector GetCenter( ) const
	{
		return FVector((Min + Max) * 0.5f);
	}

	/**
	 * Get the center and extents
	 *
	 * @param center[out] reference to center point
	 * @param Extents[out] reference to the extent around the center
	 */
	void GetCenterAndExtents( FVector & center, FVector & Extents ) const
	{
		Extents = GetExtent();
		center = Min + Extents;
	}

	/**
	 * Calculates the closest point on or inside the box to a given point in space.
	 *
	 * @param Point - The point in space.
	 *
	 * @return The closest point on or inside the box.
	 */
	FORCEINLINE FVector GetClosestPointTo( const FVector& Point ) const;

	/**
	 * Gets the box extents around the center.
	 *
	 * @return Box extents.
	 */
	FVector GetExtent( ) const
	{
		return 0.5f * (Max - Min);
	}

	/**
	 * Gets a reference to the specified point of the bounding box.
	 *
	 * @param PointIndex - Index of point
	 *
	 * @return A reference to the point.
	 */
	FVector& GetExtrema( int PointIndex )
	{
		return (&Min)[PointIndex];
	}

	/**
	 * Gets a read-only reference to the specified point of the bounding box.
	 *
	 * @param PointIndex - Index of point
	 *
	 * @return A read-only reference to the point.
	 */
	const FVector& GetExtrema( int PointIndex ) const
	{
		return (&Min)[PointIndex];
	}

	/**
	 * Gets the box size.
	 *
	 * @return Box size.
	 */
	FVector GetSize( ) const
	{
		return (Max - Min);
	}

	/**
	 * Gets the box volume.
	 *
	 * @return Box volume.
	 */
	float GetVolume( ) const
	{
		return ((Max.X - Min.X) * (Max.Y - Min.Y) * (Max.Z - Min.Z));
	}

	/**
	 * Set the initial values of the bounding box to Zero.
	 */
	void Init( )
	{
		Min = Max = FVector::ZeroVector;
		IsValid = 0;
	}

	/**
	 * Test for intersection of bounding box and this bounding box
	 *
	 * @param other bounding box to test intersection
	 * @return true if the other box intersects this box, otherwise false
	 */
	FORCEINLINE bool Intersect( const FBox & other ) const;

	/**
	 * Test for intersection of bounding box and this bounding box in the XY plane
	 *
	 * @param other bounding box to test intersection
	 * @return true if the other box intersects this box in the XY Plane, otherwise false
	 */
	FORCEINLINE bool IntersectXY( const FBox& other ) const;

	/**
	  * Gets a bounding volume transformed by an inverted FTransform object.
	  *
	  * @param M the FTransform object which translation will be inverted.
	  * @return	transformed box
	  */
	CORE_API FBox InverseTransformBy( const FTransform & M ) const;

	/**
	 * Gets a bounding volume transformed by a matrix.
	 *
	 * @param M the matrix .
	 * @return	transformed box
	 */
	CORE_API FBox TransformBy( const FMatrix& M ) const;

	/**
	 * Gets a bounding volume transformed by a FTransform object.
	 *
	 * @param M the FTransform object.
	 * @return	transformed box
	 */
	CORE_API FBox TransformBy( const FTransform & M ) const;

	/** 
	 * Transforms and projects a world bounding box to screen space
	 *
	 * @param	ProjM - projection matrix
	 * @return	transformed box
	 */
	CORE_API FBox TransformProjectBy( const FMatrix& ProjM ) const;
	
	/** 
	 * Checks to see if the location is inside this box
	 * 
	 * @param In - The location to test for inside the bounding volume
	 *
	 * @return true if location is inside this volume
	 */
	bool IsInside( const FVector& In ) const
	{
		return ((In.X > Min.X) && (In.X < Max.X) && (In.Y > Min.Y) && (In.Y < Max.Y) && (In.Z > Min.Z) && (In.Z < Max.Z));
	}

	/** 
	 * Checks to see if the location is inside this box in the XY plane
	 * 
	 * @param In - The location to test for inside the bounding box
	 *
	 * @return true if location is inside this box in the XY plane
	 */
	bool IsInsideXY( const FVector& In ) const
	{
		return ((In.X > Min.X) && (In.X < Max.X) && (In.Y > Min.Y) && (In.Y < Max.Y));
	}
	

	/** 
	 * Checks to see if given box is fully encapsulated by this box
	 * 
	 * @param Other box to test for encapsulation within the bounding volume
	 * @return true if box is inside this volume
	 */
	bool IsInside( const FBox& Other ) const
	{
		return (IsInside(Other.Min) && IsInside(Other.Max));
	}

	/** 
	 * Checks to see if given box is fully encapsulated by this box in the XY plane
	 * 
	 * @param Other box to test for encapsulation within the bounding box
	 * @return true if box is inside this box in the XY plane
	 */
	bool IsInsideXY( const FBox& Other ) const
	{
		return (IsInsideXY(Other.Min) && IsInsideXY(Other.Max));
	}

	/**
	 * Get a textual representation of this box.
	 *
	 * @return A string describing the box.
	 */
	FString ToString( ) const;


public:

	/** 
	 * Utility function to build an AABB from Origin and Extent 
	 *
	 * @param Origin the location of the bounding box
	 * @param Extent half size of the bounding box
	 *
	 * @return a new AABB
	 */
	static FBox BuildAABB( const FVector& Origin, const FVector& Extent )
	{
		FBox NewBox(Origin - Extent, Origin + Extent);

		return NewBox;
	}


public:

	/**
	 * Serializes the bounding box.
	 *
	 * @param Ar - The archive to serialize into.
	 * @param Box - The box to serialize.
	 *
	 * @return Reference to the Archive after serialization.
	 */
	friend FArchive& operator<<( FArchive& Ar, FBox& Box )
	{
		return Ar << Box.Min << Box.Max << Box.IsValid;
	}
};


/**
 * FBox specialization for TIsPODType trait.
 */
template<> struct TIsPODType<FBox> { enum { Value = true }; };


/* FBox inline functions
 *****************************************************************************/

FORCEINLINE FBox& FBox::operator+=( const FVector &Other )
{
	if (IsValid)
	{
		Min.X = FMath::Min(Min.X, Other.X);
		Min.Y = FMath::Min(Min.Y, Other.Y);
		Min.Z = FMath::Min(Min.Z, Other.Z);

		Max.X = FMath::Max(Max.X, Other.X);
		Max.Y = FMath::Max(Max.Y, Other.Y);
		Max.Z = FMath::Max(Max.Z, Other.Z);
	}
	else
	{
		Min = Max = Other;
		IsValid = 1;
	}

	return *this;
}


FORCEINLINE FBox& FBox::operator+=( const FBox& Other )
{
	if (IsValid && Other.IsValid)
	{
		Min.X = FMath::Min(Min.X, Other.Min.X);
		Min.Y = FMath::Min(Min.Y, Other.Min.Y);
		Min.Z = FMath::Min(Min.Z, Other.Min.Z);

		Max.X = FMath::Max(Max.X, Other.Max.X);
		Max.Y = FMath::Max(Max.Y, Other.Max.Y);
		Max.Z = FMath::Max(Max.Z, Other.Max.Z);
	}
	else if (Other.IsValid)
	{
		*this = Other;
	}

	return *this;
}


FORCEINLINE FVector FBox::GetClosestPointTo( const FVector& Point ) const
{
	// start by considering the point inside the box
	FVector ClosestPoint = Point;

	// now clamp to inside box if it's outside
	if (Point.X < Min.X)
	{
		ClosestPoint.X = Min.X;
	}
	else if (Point.X > Max.X)
	{
		ClosestPoint.X = Max.X;
	}

	// now clamp to inside box if it's outside
	if (Point.Y < Min.Y)
	{
		ClosestPoint.Y = Min.Y;
	}
	else if (Point.Y > Max.Y)
	{
		ClosestPoint.Y = Max.Y;
	}

	// Now clamp to inside box if it's outside.
	if (Point.Z < Min.Z)
	{
		ClosestPoint.Z = Min.Z;
	}
	else if (Point.Z > Max.Z)
	{
		ClosestPoint.Z = Max.Z;
	}

	return ClosestPoint;
}


FORCEINLINE bool FBox::Intersect( const FBox & Other ) const
{
	if ((Min.X > Other.Max.X) || (Other.Min.X > Max.X))
	{
		return false;
	}

	if ((Min.Y > Other.Max.Y) || (Other.Min.Y > Max.Y))
	{
		return false;
	}

	if ((Min.Z > Other.Max.Z) || (Other.Min.Z > Max.Z))
	{
		return false;
	}

	return true;
}


FORCEINLINE bool FBox::IntersectXY( const FBox& Other ) const
{
	if ((Min.X > Other.Max.X) || (Other.Min.X > Max.X))
	{
		return false;
	}

	if ((Min.Y > Other.Max.Y) || (Other.Min.Y > Max.Y))
	{
		return false;
	}

	return true;
}


FORCEINLINE FString FBox::ToString( ) const
{
	return FString::Printf(TEXT("IsValid=%s, Min=(%s), Max=(%s)"), IsValid ? TEXT("true") : TEXT("false"), *Min.ToString(), *Max.ToString());
}