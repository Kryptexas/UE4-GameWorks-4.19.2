// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

FORCEINLINE void FPackedNormal::operator=(const FVector& InVector)
{
	Vector.X = FMath::Clamp(FMath::Trunc(InVector.X * 127.5f + 127.5f),0,255);
	Vector.Y = FMath::Clamp(FMath::Trunc(InVector.Y * 127.5f + 127.5f),0,255);
	Vector.Z = FMath::Clamp(FMath::Trunc(InVector.Z * 127.5f + 127.5f),0,255);
	Vector.W = 128;
}

FORCEINLINE void FPackedNormal::operator=(const FVector4& InVector)
{
	Vector.X = FMath::Clamp(FMath::Trunc(InVector.X * 127.5f + 127.5f),0,255);
	Vector.Y = FMath::Clamp(FMath::Trunc(InVector.Y * 127.5f + 127.5f),0,255);
	Vector.Z = FMath::Clamp(FMath::Trunc(InVector.Z * 127.5f + 127.5f),0,255);
	Vector.W = FMath::Clamp(FMath::Trunc(InVector.W * 127.5f + 127.5f),0,255);
}

FORCEINLINE bool FPackedNormal::operator==(const FPackedNormal& B) const
{
	if(Vector.Packed != B.Vector.Packed)
		return 0;

	FVector	V1 = *this,
			V2 = B;

	if(FMath::Abs(V1.X - V2.X) > THRESH_NORMALS_ARE_SAME * 4.0f)
		return 0;

	if(FMath::Abs(V1.Y - V2.Y) > THRESH_NORMALS_ARE_SAME * 4.0f)
		return 0;

	if(FMath::Abs(V1.Z - V2.Z) > THRESH_NORMALS_ARE_SAME * 4.0f)
		return 0;

	return 1;
}

FORCEINLINE bool FPackedNormal::operator!=(const FPackedNormal& B) const
{
	if(Vector.Packed == B.Vector.Packed)
		return 0;

	FVector	V1 = *this,
			V2 = B;

	if(FMath::Abs(V1.X - V2.X) > THRESH_NORMALS_ARE_SAME * 4.0f)
		return 1;

	if(FMath::Abs(V1.Y - V2.Y) > THRESH_NORMALS_ARE_SAME * 4.0f)
		return 1;

	if(FMath::Abs(V1.Z - V2.Z) > THRESH_NORMALS_ARE_SAME * 4.0f)
		return 1;

	return 0;
}

FORCEINLINE FPackedNormal::operator FVector() const
{
	VectorRegister VectorToUnpack = GetVectorRegister();
	// Write to FVector and return it.
	FVector UnpackedVector;
	VectorStoreFloat3( VectorToUnpack, &UnpackedVector );
	return UnpackedVector;
}

FORCEINLINE VectorRegister FPackedNormal::GetVectorRegister() const
{
	// Rescale [0..255] range to [-1..1]
	VectorRegister VectorToUnpack		= VectorLoadByte4( this );
	VectorToUnpack						= VectorMultiplyAdd( VectorToUnpack, VectorReplicate(GVectorPackingConstants,2), VectorReplicate(GVectorPackingConstants,3) );
	VectorResetFloatRegisters();
	// Return unpacked vector register.
	return VectorToUnpack;
}
