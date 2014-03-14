// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FTranslationMatrix : public FMatrix
{
public:

	/** Constructor translation matrix based on given vector */
	FTranslationMatrix(const FVector& Delta);
};

FORCEINLINE FTranslationMatrix::FTranslationMatrix(const FVector& Delta) :
FMatrix(
	FPlane(1.0f,	0.0f,	0.0f,	0.0f),
	FPlane(0.0f,	1.0f,	0.0f,	0.0f),
	FPlane(0.0f,	0.0f,	1.0f,	0.0f),
	FPlane(Delta.X,	Delta.Y,Delta.Z,1.0f))
{
}
