// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * A 2x1 of FLOATs.
 */
struct FVector2D 
{
	float	X,
			Y;

	/** Constructor */
	FORCEINLINE FVector2D();

	/**
	 * Constructor
	 *
	 * @param InX X coordinate.
	 * @param InY Y coordinate.
	 */
	FORCEINLINE FVector2D(float InX,float InY);

	/**
	 * Constructor
	 *
	 * @param InPos Integer point used to set this vector.
	 */
	FORCEINLINE FVector2D(FIntPoint InPos);

	/**
	 * Constructor
	 *
	 * @param EForceInit Force init enum
	 */
	explicit FORCEINLINE FVector2D(EForceInit);

	/**
	 * Copy Constructor
	 *
	 * @param V Vector to copy from.
	 */
	explicit FORCEINLINE FVector2D( const FVector& V );

	/**
	 * Gets the result of adding two vectors together.
	 *
	 * @param V The other vector to add to this.
	 *
	 * @return The result of adding the vectors together.
	 */
	FORCEINLINE FVector2D operator+( const FVector2D& V ) const;

	/**
	 * Gets the result of subtracting a vector from this one.
	 *
	 * @param V The other vector to subtract.
	 *
	 * @return The result of the subtraction.
	 */
	FORCEINLINE FVector2D operator-( const FVector2D& V ) const;

	/**
	 * Gets the result of scaling this vector.
	 *
	 * @param Scale What to scale the vector by.
	 *
	 * @return The result of scaling this vector.
	 */
	FORCEINLINE FVector2D operator*( float Scale ) const;

	/**
	 * Gets the result of dividing this vector.
	 *
	 * @param Scale What to divide the vector by.
	 *
	 * @return The result of division on this vector.
	 */
	FVector2D operator/( float Scale ) const;

	/**
	 * Gets the result of this vector + float A.
	 *
	 * @param A		Float to add.
	 *
	 * @return		The result of this vector + float A.
	 */
	FORCEINLINE FVector2D operator+( float A ) const;

	/**
	 * Gets the result of this vector - float A
	 *
	 * @param A		Float to subtract
	 *
	 * @return		The result of this vector - float A.
	 */
	FORCEINLINE FVector2D operator-( float A ) const;

	/**
	 * Gets the result of multiplying this vector by another.
	 *
	 * @param V The other vector to multiply this by.
	 *
	 * @return The result of the multiplication.
	 */
	FORCEINLINE FVector2D operator*( const FVector2D& V ) const;

	/**
	 * Gets the result of dividing this vector by another.
	 *
	 * @param V The other vector to divide this by.
	 *
	 * @return The result of the division.
	 */
	FVector2D operator/( const FVector2D& V ) const;

	/**
	 * Calculates Dot product of this vector and another.
	 *
	 * @param V The other vector.
	 *
	 * @return The Dot product.
	 */
	FORCEINLINE float operator|( const FVector2D& V) const;

	/**
	 * Calculates Cross product of this vector and another.
	 *
	 * @param V The other vector.
	 *
	 * @return The Cross product.
	 */
	FORCEINLINE float operator^( const FVector2D& V) const;

	/**
	 * Calculates the Dot product of two vectors.
	 *
	 * @param A The first vector.
	 * @param B The second vector.
	 *
	 * @return The Dot product.
	 */
	FORCEINLINE static float DotProduct( const FVector2D& A, const FVector2D& B );

	/**
	 * Squared distance between two 2D points.
	 *
	 * @param V1 The first point.
	 * @param V2 The second point.
	 *
	 * @return The squared distance between two 2D points.
	 */
	FORCEINLINE static float DistSquared( const FVector2D& V1, const FVector2D& V2 );

	/**
	 * Compares this vector against another for equality.
	 *
	 * @param V The vector to compare against.
	 *
	 * @return true if the two vectors are equal, otherwise false.
	 */
	bool operator==( const FVector2D& V ) const;

	/**
	 * Compares this vector against another for inequality.
	 *
	 * @param V The vector to compare against.
	 *
	 * @return true if the two vectors are not equal, otherwise false.
	 */
	bool operator!=( const FVector2D& V ) const;

	/**
	 * Checks whether the components of this vector are less than another.
	 *
	 * @param Other The vector to compare against.
	 *
	 * @return true if this is the smaller vector, otherwise false.
	 */
	bool operator<( const FVector2D& Other ) const;

	/**
	 * Checks whether the components of this vector are greater than another.
	 *
	 * @param Other The vector to compare against.
	 *
	 * @return true if this is the larger vector, otherwise false.
	 */
	bool operator>( const FVector2D& Other ) const;

	/**
	 * Checks whether the components of this vector are less than or equal to another.
	 *
	 * @param Other The vector to compare against.
	 *
	 * @return true if this vector is less than or equal to the other vector, otherwise false.
	 */
	bool operator<=( const FVector2D& Other ) const;

	/**
	 * Checks whether the components of this vector are greater than or equal to another.
	 *
	 * @param Other The vector to compare against.
	 *
	 * @return true if this vector is greater than or equal to the other vector, otherwise false.
	 */
	bool operator>=( const FVector2D& Other ) const;

	/**
	 * Checks for equality with error-tolerant comparison.
	 *
	 * @param V The vector to compare.
	 * @param Tolerance Error tolerance.
	 *
	 * @return true if the vectors are equal within specified tolerance, otherwise false.
	 */
	bool Equals(const FVector2D& V, float Tolerance) const;

	/**
	 * Gets a negated copy of the vector.
	 *
	 * @return A negated copy of the vector.
	 */
	FORCEINLINE FVector2D operator-() const;

	/**
	 * Adds another vector to this.
	 *
	 * @param V The other vector to add.
	 *
	 * @return Copy of the vector after addition.
	 */
	FORCEINLINE FVector2D operator+=( const FVector2D& V );

	/**
	 * Subtracts another vector from this.
	 *
	 * @param V The other vector to subtract.
	 *
	 * @return Copy of the vector after subtraction.
	 */
	FORCEINLINE FVector2D operator-=( const FVector2D& V );

	/**
	 * Scales this vector.
	 *
	 * @param Scale The scale to multiply vector by.
	 *
	 * @return Copy of the vector after scaling.
	 */
	FORCEINLINE FVector2D operator*=( float Scale );

	/**
	 * Divides this vector.
	 *
	 * @param V What to divide vector by.
	 *
	 * @return Copy of the vector after division.
	 */
	FVector2D operator/=( float V );

	/**
	 * Multiplies this vector with another vector.
	 *
	 * @param V The vector to multiply with.
	 *
	 * @return Copy of the vector after multiplication.
	 */
	FVector2D operator*=( const FVector2D& V );

	/**
	 * Divides this vector by another vector.
	 *
	 * @param V The vector to divide by.
	 *
	 * @return Copy of the vector after division.
	 */
	FVector2D operator/=( const FVector2D& V );

	/**
	 * Gets specific component of the vector.
	 *
	 * @param Index the index of vector component
	 *
	 * @return reference to component.
	 */
    float& operator[]( int32 Index );

	/**
	 * Gets specific component of the vector.
	 *
	 * @param Index the index of vector component
	 *
	 * @return copy of component value.
	 */
	float operator[]( int32 Index ) const;

	/**
	 * Set the values of the vector directly.
	 *
	 * @param InX New X coordinate.
	 * @param InY New Y coordinate.
	 */
	void Set( float InX, float InY );

	/**
	 * Get the maximum of the vectors coordinates.
	 *
	 * @return The maximum of the vectors coordinates.
	 */
	float GetMax() const;

	/**
	 * Get the absolute maximum of the vectors coordinates.
	 *
	 * @return The absolute maximum of the vectors coordinates.
	 */
	float GetAbsMax() const;

	/**
	 * Get the minimum of the vectors coordinates.
	 *
	 * @return The minimum of the vectors coordinates.
	 */
	float GetMin() const;

	/**
	 * Get the length of this vector.
	 *
	 * @return The length of this vector.
	 */
	float Size() const;

	/**
	 * Get the squared length of this vector.
	 *
	 * @return The squared length of this vector.
	 */
	float SizeSquared() const;

	/**
	 * Get a normalized copy of this vector if it is large enough.
	 *
	 * @param Tolerance Minimum squared length of vector for normalization.
	 *
	 * @return A normalized copy of the vector if safe, (0,0) otherwise.
	 */
	FVector2D SafeNormal(float Tolerance=SMALL_NUMBER) const;

	/**
	 * Normalize this vector if it is large enough, set it to (0,0) otherwise.
	 *
	 * @param Tolerance Minimum squared length of vector for normalization.
	 */
	void Normalize(float Tolerance=SMALL_NUMBER);

	/**
	 * Checks whether vector is near to zero within a specified tolerance.
	 *
	 * @param Tolerance Error tolerance.
	 *
	 * @return true if vector is in tolerance to zero, otherwise false.
	 */
	bool IsNearlyZero(float Tolerance=KINDA_SMALL_NUMBER) const;

	/**
	 * Checks whether vector is exactly zero.
	 *
	 * @return true if vector is exactly zero, otherwise false.
	 */
	bool IsZero() const;

	/**
	 * Gets a specific component of the vector.
	 *
	 * @param Index The index of the component required.
	 *
	 * @return Reference to the specified component.
	 */
	float& Component( int32 Index );

	/**
	 * Gets a specific component of the vector.
	 *
	 * @param Index The index of the component required.
	 *
	 * @return Copy of the specified component.
	 */
	float Component( int32 Index ) const;

	/**
	 * Get this vector as an Int Point.
	 *
	 * @return New Int Point from this vector.
	 */
	FIntPoint IntPoint() const;

	/**
	 * Creates a copy of this vector with both axes clamped to the given range.
	 * @return New vector with clamped axes.
	 */
	FVector2D ClampAxes( float MinAxisVal, float MaxAxisVal ) const;

	/**
	 * Get a textual representation of the vector.
	 *
	 * @return Text describing the vector.
	 */
	FString ToString() const;

	/**
	 * Initialize this Vector based on an FString. The String is expected to contain X=, Y=.
	 * The FVector2D will be bogus when InitFromString returns false.
	 *
	 * @param	InSourceString	FString containing the vector values.
	 *
	 * @return true if the X,Y values were read successfully; false otherwise.
	 */
	bool InitFromString( const FString & InSourceString );

	/**
	 * Serialize a vector.
	 *
	 * @param Ar Serialization archive.
	 * @param V Vector being serialized.
	 *
	 * @return Reference to Archive after serialization.
	 */
	friend FArchive& operator<<( FArchive& Ar, FVector2D& V )
	{
		// @warning BulkSerialize: FVector2D is serialized as memory dump
		// See TArray::BulkSerialize for detailed description of implied limitations.
		return Ar << V.X << V.Y;
	}

	/**
	 * Utility to check if there are any NaNs in this vector.
	 *
	 * @return true if there are any NaNs in this vector, otherwise false.
	 */
	FORCEINLINE bool ContainsNaN() const
	{
		return (FMath::IsNaN(X) || !FMath::IsFinite(X) || 
			FMath::IsNaN(Y) || !FMath::IsFinite(Y));
	}

	CORE_API bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	/** Converts spherical coordinates on the unit sphere into a cartesian unit length vector. */
	inline FVector SphericalToUnitCartesian() const;

	/** Global 2D zero vector constant */
	static CORE_API const FVector2D ZeroVector;

	/** Global 2D unit vector constant */
	static CORE_API const FVector2D UnitVector;
};

FORCEINLINE FVector2D operator*( float Scale, const FVector2D& V )
{
	return V.operator*( Scale );
}

template <> struct TIsPODType<FVector2D> { enum { Value = true }; };

FORCEINLINE FVector2D::FVector2D()
{}

FORCEINLINE FVector2D::FVector2D(float InX,float InY)
	:	X(InX), Y(InY)
{}

FORCEINLINE FVector2D::FVector2D(FIntPoint InPos)
{
	X = (float)InPos.X;
	Y = (float)InPos.Y;
}

FORCEINLINE FVector2D::FVector2D(EForceInit)
	: X(0), Y(0)
{
}

FORCEINLINE FVector2D FVector2D::operator+( const FVector2D& V ) const
{
	return FVector2D( X + V.X, Y + V.Y );
}

FORCEINLINE FVector2D FVector2D::operator-( const FVector2D& V ) const
{
	return FVector2D( X - V.X, Y - V.Y );
}

FORCEINLINE FVector2D FVector2D::operator*( float Scale ) const
{
	return FVector2D( X * Scale, Y * Scale );
}

FORCEINLINE FVector2D FVector2D::operator/( float Scale ) const
{
	const float RScale = 1.f/Scale;
	return FVector2D( X * RScale, Y * RScale );
}

FORCEINLINE FVector2D FVector2D::operator+( float A ) const
{
	return FVector2D( X + A, Y + A );
}

FORCEINLINE FVector2D FVector2D::operator-( float A ) const
{
	return FVector2D( X - A, Y - A );
}

FORCEINLINE FVector2D FVector2D::operator*( const FVector2D& V ) const
{
	return FVector2D( X * V.X, Y * V.Y );
}

FORCEINLINE FVector2D FVector2D::operator/( const FVector2D& V ) const
{
	return FVector2D( X / V.X, Y / V.Y );
}

FORCEINLINE float FVector2D::operator|( const FVector2D& V) const
{
	return X*V.X + Y*V.Y;
}

FORCEINLINE float FVector2D::operator^( const FVector2D& V) const
{
	return X*V.Y - Y*V.X;
}

FORCEINLINE float FVector2D::DotProduct( const FVector2D& A, const FVector2D& B )
{
	return A | B;
}

FORCEINLINE float FVector2D::DistSquared( const FVector2D &V1, const FVector2D &V2 )
{
	return FMath::Square(V2.X-V1.X) + FMath::Square(V2.Y-V1.Y);
}

FORCEINLINE bool FVector2D::operator==( const FVector2D& V ) const
{
	return X==V.X && Y==V.Y;
}

FORCEINLINE bool FVector2D::operator!=( const FVector2D& V ) const
{
	return X!=V.X || Y!=V.Y;
}

FORCEINLINE bool FVector2D::operator<( const FVector2D& Other ) const
{
	return X < Other.X && Y < Other.Y;
}

FORCEINLINE bool FVector2D::operator>( const FVector2D& Other ) const
{
	return X > Other.X && Y > Other.Y;
}

FORCEINLINE bool FVector2D::operator<=( const FVector2D& Other ) const
{
	return X <= Other.X && Y <= Other.Y;
}

FORCEINLINE bool FVector2D::operator>=( const FVector2D& Other ) const
{
	return X >= Other.X && Y >= Other.Y;
}

FORCEINLINE bool FVector2D::Equals(const FVector2D& V, float Tolerance) const
{
	return FMath::Abs(X-V.X) < Tolerance && FMath::Abs(Y-V.Y) < Tolerance;
}

FORCEINLINE FVector2D FVector2D::operator-() const
{
	return FVector2D( -X, -Y );
}

FORCEINLINE FVector2D FVector2D::operator+=( const FVector2D& V )
{
	X += V.X; Y += V.Y;
	return *this;
}

FORCEINLINE FVector2D FVector2D::operator-=( const FVector2D& V )
{
	X -= V.X; Y -= V.Y;
	return *this;
}

FORCEINLINE FVector2D FVector2D::operator*=( float Scale )
{
	X *= Scale; Y *= Scale;
	return *this;
}

FORCEINLINE FVector2D FVector2D::operator/=( float V )
{
	const float RV = 1.f/V;
	X *= RV; Y *= RV;
	return *this;
}

FORCEINLINE FVector2D FVector2D::operator*=( const FVector2D& V )
{
	X *= V.X; Y *= V.Y;
	return *this;
}

FORCEINLINE FVector2D FVector2D::operator/=( const FVector2D& V )
{
	X /= V.X; Y /= V.Y;
	return *this;
}

FORCEINLINE float& FVector2D::operator[]( int32 Index )
{
	check(Index>=0 && Index<2);
	return ((Index == 0) ? X : Y);
}

FORCEINLINE float FVector2D::operator[]( int32 Index ) const
{
	check(Index>=0 && Index<2);
	return ((Index == 0) ? X : Y);
}

FORCEINLINE void FVector2D::Set( float InX, float InY )
{
	X = InX;
	Y = InY;
}

FORCEINLINE float FVector2D::GetMax() const
{
	return FMath::Max(X,Y);
}

FORCEINLINE float FVector2D::GetAbsMax() const
{
	return FMath::Max(FMath::Abs(X),FMath::Abs(Y));
}

FORCEINLINE float FVector2D::GetMin() const
{
	return FMath::Min(X,Y);
}

FORCEINLINE float FVector2D::Size() const
{
	return FMath::Sqrt( X*X + Y*Y );
}

FORCEINLINE float FVector2D::SizeSquared() const
{
	return X*X + Y*Y;
}

FORCEINLINE FVector2D FVector2D::SafeNormal(float Tolerance) const
{	
	const float SquareSum = X*X + Y*Y;
	if( SquareSum > Tolerance )
	{
		const float Scale = FMath::InvSqrt(SquareSum);
		return FVector2D(X*Scale, Y*Scale);
	}
	return FVector2D(0.f, 0.f);
}

FORCEINLINE void FVector2D::Normalize(float Tolerance)
{
	const float SquareSum = X*X + Y*Y;
	if( SquareSum > Tolerance )
	{
		const float Scale = FMath::InvSqrt(SquareSum);
		X *= Scale;
		Y *= Scale;
		return;
	}
	X = 0.0f;
	Y = 0.0f;
}

FORCEINLINE bool FVector2D::IsNearlyZero(float Tolerance) const
{
	return	FMath::Abs(X)<Tolerance
		&&	FMath::Abs(Y)<Tolerance;
}

FORCEINLINE bool FVector2D::IsZero() const
{
	return X==0.f && Y==0.f;
}

FORCEINLINE float& FVector2D::Component( int32 Index )
{
	return (&X)[Index];
}

FORCEINLINE float FVector2D::Component( int32 Index ) const
{
	return (&X)[Index];
}

FORCEINLINE FIntPoint FVector2D::IntPoint() const
{
	return FIntPoint( FMath::Round(X), FMath::Round(Y) );
}

FORCEINLINE FVector2D FVector2D::ClampAxes( float MinAxisVal, float MaxAxisVal ) const
{
	return FVector2D( FMath::Clamp(X, MinAxisVal, MaxAxisVal), FMath::Clamp(Y, MinAxisVal, MaxAxisVal) );
}

FORCEINLINE FString FVector2D::ToString() const
{
	return FString::Printf(TEXT("X=%3.3f Y=%3.3f"), X, Y);
}

FORCEINLINE bool FVector2D::InitFromString( const FString & InSourceString )
{
	X = Y = 0;

	// The initialization is only successful if the X and Y values can all be parsed from the string
 	const bool bSuccessful = FParse::Value( *InSourceString, TEXT("X=") , X ) && FParse::Value( *InSourceString, TEXT("Y="), Y ) ;

	return bSuccessful;
}


