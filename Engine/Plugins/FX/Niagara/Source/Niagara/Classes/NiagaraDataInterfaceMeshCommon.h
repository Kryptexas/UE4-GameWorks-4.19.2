// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "NiagaraDataInterface.h"
#include "NiagaraCommon.h"
#include "VectorVM.h"
#include "NiagaraDataInterfaceMeshCommon.generated.h"

//A coordinate on a mesh usable in Niagara.
//Do not alter this struct without updating the data interfaces that use it!
USTRUCT()
struct FMeshTriCoordinate
{
	GENERATED_USTRUCT_BODY();
	
	UPROPERTY(EditAnywhere, Category="Coordinate")
	int32 Tri;

	UPROPERTY(EditAnywhere, Category="Coordinate")
	FVector BaryCoord;
};

FORCEINLINE FVector RandomBarycentricCoord(FRandomStream& RandStream)
{
	//TODO: This is gonna be slooooow. Move to an LUT possibly or find faster method.
	//Can probably handle lower quality randoms / uniformity for a decent speed win.
	float r0 = RandStream.GetFraction();
	float r1 = RandStream.GetFraction();
	float sqrt0 = FMath::Sqrt(r0);
	float sqrt1 = FMath::Sqrt(r1);
	return FVector(1.0f - sqrt0, sqrt0 * (1.0 - r1), r1 * sqrt0);
}

template<typename T>
FORCEINLINE T BarycentricInterpolate(float BaryX, float BaryY, float BaryZ, T V0, T V1, T V2)
{
	return V0 * BaryX + V1 * BaryY + V2 * BaryZ;
}

// Overload for FVector4 to work around C2719: (formal parameter with requested alignment of 16 won't be aligned)
FORCEINLINE FVector4 BarycentricInterpolate(float BaryX, float BaryY, float BaryZ, const FVector4& V0, const FVector4& V1, const FVector4& V2)
{
	return V0 * BaryX + V1 * BaryY + V2 * BaryZ;
}