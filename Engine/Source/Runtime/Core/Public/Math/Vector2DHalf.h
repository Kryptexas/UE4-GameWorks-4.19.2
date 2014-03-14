// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
* A 2x1 of 16 bit floats.
*/
struct FVector2DHalf 
{
	FFloat16	X,
				Y;

	/** Default Constructor */
	FORCEINLINE FVector2DHalf();

	/** Constructor 
	 *
	 * InX half float X value
	 * Iny half float Y value
	 */
 	FORCEINLINE FVector2DHalf(const FFloat16& InX,const FFloat16& InY);

	/** Constructor 
	 *
	 * InX float X value
	 * Iny float Y value
	 */
	FORCEINLINE FVector2DHalf(float InX,float InY);

	/** Constructor 
	 *
	 * Vector2D float vector
	 */
	FORCEINLINE FVector2DHalf(const FVector2D& Vector2D);

	/** assignment operator */
 	FVector2DHalf& operator=(const FVector2D& Vector2D);

	/**
	 * Get a textual representation of the vector.
	 *
	 * @return Text describing the vector.
	 */
	FString ToString() const;


	/** Convert from FVector2D to FVector2DHalf. */
	operator FVector2D() const;

	/**
	 * Serializes the FVector2DHalf.
	 *
	 * @param Ar Reference to the serialization archive.
	 * @param V Reference to the FVector2DHalf being serialized.
	 *
	 * @return Reference to the Archive after serialization.
	 */
	friend FArchive& operator<<( FArchive& Ar, FVector2DHalf& V )
	{
		return Ar << V.X << V.Y;
	}
};

FORCEINLINE FVector2DHalf::FVector2DHalf()
{}

FORCEINLINE FVector2DHalf::FVector2DHalf(const FFloat16& InX,const FFloat16& InY)
 	:	X(InX), Y(InY)
{}

FORCEINLINE FVector2DHalf::FVector2DHalf(float InX,float InY)
	:	X(InX), Y(InY)
{}

FORCEINLINE FVector2DHalf::FVector2DHalf(const FVector2D& Vector2D)
	:	X(Vector2D.X), Y(Vector2D.Y)
{}

FORCEINLINE FVector2DHalf& FVector2DHalf::operator=(const FVector2D& Vector2D)
{
 	X = FFloat16(Vector2D.X);
 	Y = FFloat16(Vector2D.Y);
	return *this;
}

FORCEINLINE FString FVector2DHalf::ToString() const
{
	return FString::Printf(TEXT("X=%3.3f Y=%3.3f"), (float)X, (float)Y );
}

FORCEINLINE FVector2DHalf::operator FVector2D() const
{
	return FVector2D((float)X,(float)Y);
}
