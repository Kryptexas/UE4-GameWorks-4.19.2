// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** Scale matrix */
class FScaleMatrix : public FMatrix
{
public:

	/**
	 * @param Scale uniform scale to apply to matrix.
	 */
	FScaleMatrix(float Scale);

	/**
	 * @param Scale Non-uniform scale to apply to matrix.
	 */
	FScaleMatrix(const FVector& Scale);
};


FORCEINLINE FScaleMatrix::FScaleMatrix(float Scale) :
	FMatrix(
		FPlane(Scale,	0.0f,	0.0f,	0.0f),
		FPlane(0.0f,	Scale,	0.0f,	0.0f),
		FPlane(0.0f,	0.0f,	Scale,	0.0f),
		FPlane(0.0f,	0.0f,	0.0f,	1.0f))
{
}

FORCEINLINE FScaleMatrix::FScaleMatrix(const FVector& Scale) :
	FMatrix(
		FPlane(Scale.X,	0.0f,		0.0f,		0.0f),
		FPlane(0.0f,	Scale.Y,	0.0f,		0.0f),
		FPlane(0.0f,	0.0f,		Scale.Z,	0.0f),
		FPlane(0.0f,	0.0f,		0.0f,		1.0f))
{
}
