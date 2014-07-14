// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Collision.cpp: AActor collision implementation
=============================================================================*/

#include "EnginePrivate.h"
#include "Collision.h"

//////////////////////////////////////////////////////////////////////////
// FHitResult

FHitResult::FHitResult(class AActor* InActor, class UPrimitiveComponent* InComponent, FVector const& HitLoc, FVector const& HitNorm)
{
	FMemory::Memzero(this, sizeof(FHitResult));
	Location = HitLoc;
	ImpactPoint = HitLoc;
	Normal = HitNorm;
	ImpactNormal = HitNorm;
	Actor = InActor;
	Component = InComponent;
}

AActor* FHitResult::GetActor() const
{
	return Actor.Get();
}

UPrimitiveComponent* FHitResult::GetComponent() const
{
	return Component.Get();
}

//////////////////////////////////////////////////////////////////////////
// FOverlapResult

AActor* FOverlapResult::GetActor() const
{
	return Actor.Get();
}

UPrimitiveComponent* FOverlapResult::GetComponent() const
{
	return Component.Get();
}


//////////////////////////////////////////////////////////////////////////
// FCollisionQueryParams

FCollisionQueryParams::FCollisionQueryParams(FName InTraceTag, bool bInTraceComplex, const AActor* InIgnoreActor)
{
	bTraceComplex = bInTraceComplex;
	TraceTag = InTraceTag;
	bTraceAsyncScene = false;
	bFindInitialOverlaps = true;
	bReturnFaceIndex = false;
	bReturnPhysicalMaterial = false;

	AddIgnoredActor(InIgnoreActor);
	if (InIgnoreActor != NULL)
	{
		OwnerTag = InIgnoreActor->GetFName();
	}
}

void FCollisionQueryParams::AddIgnoredActor(const AActor* InIgnoreActor)
{
	if (InIgnoreActor != NULL)
	{
		IgnoreActors.AddUnique(InIgnoreActor->GetUniqueID());
	}
}

void FCollisionQueryParams::AddIgnoredActors(const TArray<AActor*>& InIgnoreActors)
{
	for (int32 Idx = 0; Idx < InIgnoreActors.Num(); ++Idx)
	{
		AActor const* const A = InIgnoreActors[Idx];
		if (A)
		{
			IgnoreActors.Add(A->GetUniqueID());
		}
	}
}

void FCollisionQueryParams::AddIgnoredActors(const TArray<TWeakObjectPtr<AActor> >& InIgnoreActors)
{
	for (int32 Idx = 0; Idx < InIgnoreActors.Num(); ++Idx)
	{
		AActor const* const A = InIgnoreActors[Idx].Get();
		if (A)
		{
			IgnoreActors.Add(A->GetUniqueID());
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// FSeparatingAxisPointCheck

TArray<FVector> FSeparatingAxisPointCheck::TriangleVertices;

bool FSeparatingAxisPointCheck::TestSeparatingAxisCommon(const FVector& Axis, float ProjectedPolyMin, float ProjectedPolyMax)
{
	const float ProjectedCenter = FVector::DotProduct(Axis, BoxCenter);
	const float ProjectedExtent = FVector::DotProduct(Axis.GetAbs(), BoxExtent);
	const float ProjectedBoxMin = ProjectedCenter - ProjectedExtent;
	const float ProjectedBoxMax = ProjectedCenter + ProjectedExtent;

	if (ProjectedPolyMin > ProjectedBoxMax || ProjectedPolyMax < ProjectedBoxMin)
	{
		return false;
	}

	if (bCalcLeastPenetration)
	{
		const float AxisMagnitudeSqr = Axis.SizeSquared();
		if (AxisMagnitudeSqr > (SMALL_NUMBER * SMALL_NUMBER))
		{
			const float InvAxisMagnitude = FMath::InvSqrt(AxisMagnitudeSqr);
			const float MinPenetrationDist = (ProjectedBoxMax - ProjectedPolyMin) * InvAxisMagnitude;
			const float	MaxPenetrationDist = (ProjectedPolyMax - ProjectedBoxMin) * InvAxisMagnitude;

			if (MinPenetrationDist < BestDist)
			{
				BestDist = MinPenetrationDist;
				HitNormal = -Axis * InvAxisMagnitude;
			}

			if (MaxPenetrationDist < BestDist)
			{
				BestDist = MaxPenetrationDist;
				HitNormal = Axis * InvAxisMagnitude;
			}
		}
	}

	return true;
}

bool FSeparatingAxisPointCheck::TestSeparatingAxisTriangle(const FVector& Axis)
{
	const float ProjectedV0 = FVector::DotProduct(Axis, PolyVertices[0]);
	const float ProjectedV1 = FVector::DotProduct(Axis, PolyVertices[1]);
	const float ProjectedV2 = FVector::DotProduct(Axis, PolyVertices[2]);
	const float ProjectedTriMin = FMath::Min3(ProjectedV0, ProjectedV1, ProjectedV2);
	const float ProjectedTriMax = FMath::Max3(ProjectedV0, ProjectedV1, ProjectedV2);

	return TestSeparatingAxisCommon(Axis, ProjectedTriMin, ProjectedTriMax);
}

bool FSeparatingAxisPointCheck::TestSeparatingAxisGeneric(const FVector& Axis)
{
	float ProjectedPolyMin = TNumericLimits<float>::Max();
	float ProjectedPolyMax = TNumericLimits<float>::Lowest();
	for (const auto& Vertex : PolyVertices)
	{
		const float ProjectedVertex = FVector::DotProduct(Axis, Vertex);
		ProjectedPolyMin = FMath::Min(ProjectedPolyMin, ProjectedVertex);
		ProjectedPolyMax = FMath::Max(ProjectedPolyMax, ProjectedVertex);
	}

	return TestSeparatingAxisCommon(Axis, ProjectedPolyMin, ProjectedPolyMax);
}

bool FSeparatingAxisPointCheck::FindSeparatingAxisTriangle()
{
	check(PolyVertices.Num() == 3);
	const FVector EdgeDir0 = PolyVertices[1] - PolyVertices[0];
	const FVector EdgeDir1 = PolyVertices[2] - PolyVertices[1];
	const FVector EdgeDir2 = PolyVertices[0] - PolyVertices[2];

	// Box Z edge x triangle edges.

	if (!TestSeparatingAxisTriangle(FVector(EdgeDir0.Y, -EdgeDir0.X, 0.0f)) ||
		!TestSeparatingAxisTriangle(FVector(EdgeDir1.Y, -EdgeDir1.X, 0.0f)) ||
		!TestSeparatingAxisTriangle(FVector(EdgeDir2.Y, -EdgeDir2.X, 0.0f)))
	{
		return false;
	}

	// Box Y edge x triangle edges.

	if (!TestSeparatingAxisTriangle(FVector(-EdgeDir0.Z, 0.0f, EdgeDir0.X)) ||
		!TestSeparatingAxisTriangle(FVector(-EdgeDir1.Z, 0.0f, EdgeDir1.X)) ||
		!TestSeparatingAxisTriangle(FVector(-EdgeDir2.Z, 0.0f, EdgeDir2.X)))
	{
		return false;
	}

	// Box X edge x triangle edges.

	if (!TestSeparatingAxisTriangle(FVector(0.0f, EdgeDir0.Z, -EdgeDir0.Y)) ||
		!TestSeparatingAxisTriangle(FVector(0.0f, EdgeDir1.Z, -EdgeDir1.Y)) ||
		!TestSeparatingAxisTriangle(FVector(0.0f, EdgeDir2.Z, -EdgeDir2.Y)))
	{
		return false;
	}

	// Box faces.

	if (!TestSeparatingAxisTriangle(FVector(0.0f, 0.0f, 1.0f)) ||
		!TestSeparatingAxisTriangle(FVector(1.0f, 0.0f, 0.0f)) ||
		!TestSeparatingAxisTriangle(FVector(0.0f, 1.0f, 0.0f)))
	{
		return false;
	}

	// Triangle normal.

	if (!TestSeparatingAxisTriangle(FVector::CrossProduct(EdgeDir1, EdgeDir0)))
	{
		return false;
	}

	return true;
}

bool FSeparatingAxisPointCheck::FindSeparatingAxisGeneric()
{
	check(PolyVertices.Num() > 3);
	int32 LastIndex = PolyVertices.Num() - 1;
	for (int32 Index = 0; Index < PolyVertices.Num(); Index++)
	{
		const FVector& V0 = PolyVertices[LastIndex];
		const FVector& V1 = PolyVertices[Index];
		const FVector EdgeDir = V1 - V0;

		// Box edges x polygon edge

		if (!TestSeparatingAxisGeneric(FVector(EdgeDir.Y, -EdgeDir.X, 0.0f)) ||
			!TestSeparatingAxisGeneric(FVector(-EdgeDir.Z, 0.0f, EdgeDir.X)) ||
			!TestSeparatingAxisGeneric(FVector(0.0f, EdgeDir.Z, -EdgeDir.Y)))
		{
			return false;
		}

		LastIndex = Index;
	}

	// Box faces.

	if (!TestSeparatingAxisGeneric(FVector(0.0f, 0.0f, 1.0f)) ||
		!TestSeparatingAxisGeneric(FVector(1.0f, 0.0f, 0.0f)) ||
		!TestSeparatingAxisGeneric(FVector(0.0f, 1.0f, 0.0f)))
	{
		return false;
	}

	// Polygon normal.

	int32 Index0 = PolyVertices.Num() - 2;
	int32 Index1 = Index0 + 1;
	for (int32 Index2 = 0; Index2 < PolyVertices.Num(); Index2++)
	{
		const FVector& V0 = PolyVertices[Index0];
		const FVector& V1 = PolyVertices[Index1];
		const FVector& V2 = PolyVertices[Index2];

		const FVector EdgeDir0 = V1 - V0;
		const FVector EdgeDir1 = V2 - V1;

		FVector Normal = FVector::CrossProduct(EdgeDir1, EdgeDir0);
		if (Normal.SizeSquared() > SMALL_NUMBER)
		{
			if (!TestSeparatingAxisGeneric(Normal))
			{
				return false;
			}
			break;
		}

		Index0 = Index1;
		Index1 = Index2;
	}

	return true;
}

