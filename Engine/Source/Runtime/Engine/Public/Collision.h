// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Collision.h: Common collision code.
=============================================================================*/

#pragma once

/**
 * Collision stats
 */
DECLARE_CYCLE_STAT_EXTERN(TEXT("RaycastAny"),STAT_Collision_RaycastAny,STATGROUP_Collision, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("RaycastSingle"),STAT_Collision_RaycastSingle,STATGROUP_Collision, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("RaycastMultiple"),STAT_Collision_RaycastMultiple,STATGROUP_Collision, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("GeomSweepAny"),STAT_Collision_GeomSweepAny,STATGROUP_Collision, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("GeomSweepSingle"),STAT_Collision_GeomSweepSingle,STATGROUP_Collision, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("GeomSweepMultiple"),STAT_Collision_GeomSweepMultiple,STATGROUP_Collision, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("GeomOverlapAny"),STAT_Collision_GeomOverlapAny,STATGROUP_Collision, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("GeomOverlapSingle"),STAT_Collision_GeomOverlapSingle,STATGROUP_Collision, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("GeomOverlapMultiple"),STAT_Collision_GeomOverlapMultiple,STATGROUP_Collision, );

/** Enable collision analyzer support */
#if (1 && !(UE_BUILD_SHIPPING || UE_BUILD_TEST) && WITH_EDITOR && WITH_UNREAL_DEVELOPER_TOOLS && WITH_PHYSX)
	#define ENABLE_COLLISION_ANALYZER 1
#else
	#define ENABLE_COLLISION_ANALYZER 0
#endif

//
//	FSeparatingAxisPointCheck - Checks for intersection between an oriented bounding box and a triangle.
//	HitNormal: The normal of the separating axis the bounding box is penetrating the least.
//	BestDist: The amount the bounding box is penetrating the axis defined by HitNormal.
//

struct FSeparatingAxisPointCheck
{
	FVector	HitNormal;
	float	BestDist;
	bool	Hit;

	const FVector&	V0,
					V1,
					V2;

	bool TestSeparatingAxis(
		const FVector& Axis,
		float ProjectedPoint,
		float ProjectedExtent
		)
	{
		float	ProjectedV0 = Axis | V0,
				ProjectedV1 = Axis | V1,
				ProjectedV2 = Axis | V2,
				TriangleMin = FMath::Min3(ProjectedV0,ProjectedV1,ProjectedV2) - ProjectedExtent,
				TriangleMax = FMath::Max3(ProjectedV0,ProjectedV1,ProjectedV2) + ProjectedExtent;

		if(ProjectedPoint >= TriangleMin && ProjectedPoint <= TriangleMax)
		{
			// Use inverse sqrt because that is fast and we do more math with the inverse value anyway
			float	InvAxisMagnitude = FMath::InvSqrt(Axis.X * Axis.X +	Axis.Y * Axis.Y + Axis.Z * Axis.Z),
					ScaledBestDist = BestDist / InvAxisMagnitude,
					MinPenetrationDist = ProjectedPoint - TriangleMin,
					MaxPenetrationDist = TriangleMax - ProjectedPoint;
			if(MinPenetrationDist < ScaledBestDist)
			{
				BestDist = MinPenetrationDist * InvAxisMagnitude;
				HitNormal = -Axis * InvAxisMagnitude;
			}
			if(MaxPenetrationDist < ScaledBestDist)
			{
				BestDist = MaxPenetrationDist * InvAxisMagnitude;
				HitNormal = Axis * InvAxisMagnitude;
			}
			return 1;
		}
		else
			return 0;
	}

		
	bool TestSeparatingAxis(
		const FVector& Axis,
		const FVector& Point,
		const FVector& BoxExtent
		)
	{
		float	ProjectedExtent = BoxExtent.X * FMath::Abs(Axis.X) + BoxExtent.Y * FMath::Abs(Axis.Y) + BoxExtent.Z * FMath::Abs(Axis.Z);
		return TestSeparatingAxis(Axis,Axis | Point,ProjectedExtent);
	}

	bool FindSeparatingAxis(
		const FVector& Point,
		const FVector& BoxExtent
		)
	{
		// Triangle normal.

		if(!TestSeparatingAxis((V2 - V1) ^ (V1 - V0),Point,BoxExtent))
			return 0;

		const FVector& EdgeDir0 = V1 - V0;
		const FVector& EdgeDir1 = V2 - V1;
		const FVector& EdgeDir2 = V0 - V2;

		// Box Z edge x triangle edges.

		if(!TestSeparatingAxis(FVector(EdgeDir0.Y,-EdgeDir0.X,0.0f),Point,BoxExtent))
			return 0;

		if(!TestSeparatingAxis(FVector(EdgeDir1.Y,-EdgeDir1.X,0.0f),Point,BoxExtent))
			return 0;

		if(!TestSeparatingAxis(FVector(EdgeDir2.Y,-EdgeDir2.X,0.0f),Point,BoxExtent))
			return 0;

		// Box Y edge x triangle edges.

		if(!TestSeparatingAxis(FVector(-EdgeDir0.Z,0.0f,EdgeDir0.X),Point,BoxExtent))
			return 0;

		if(!TestSeparatingAxis(FVector(-EdgeDir1.Z,0.0f,EdgeDir1.X),Point,BoxExtent))
			return 0;

		if(!TestSeparatingAxis(FVector(-EdgeDir2.Z,0.0f,EdgeDir2.X),Point,BoxExtent))
			return 0;

		// Box X edge x triangle edges.

		if(!TestSeparatingAxis(FVector(0.0f,EdgeDir0.Z,-EdgeDir0.Y),Point,BoxExtent))
			return 0;

		if(!TestSeparatingAxis(FVector(0.0f,EdgeDir1.Z,-EdgeDir1.Y),Point,BoxExtent))
			return 0;

		if(!TestSeparatingAxis(FVector(0.0f,EdgeDir2.Z,-EdgeDir2.Y),Point,BoxExtent))
			return 0;

		// Box faces. We need to calculate this by crossing edges because BoxX etc are the _edge_ directions - not the faces.
		// The box may be sheared due to non-unfiform scaling and rotation so FaceX normal != BoxX edge direction

		if(!TestSeparatingAxis(FVector(0.0f,0.0f,1.0f),Point,BoxExtent))
			return 0;

		if(!TestSeparatingAxis(FVector(1.0f,0.0f,0.0f),Point,BoxExtent))
			return 0;

		if(!TestSeparatingAxis(FVector(0.0f,1.0f,0.0f),Point,BoxExtent))
			return 0;

		return 1;
	}

	FSeparatingAxisPointCheck(
		const FVector& InV0,
		const FVector& InV1,
		const FVector& InV2,
		const FVector& Point,
		const FVector& BoxExtent,
		float InBestDist
		):
	HitNormal(0,0,0),
		BestDist(InBestDist),
		Hit(0),
		V0(InV0),
		V1(InV1),
		V2(InV2)
	{
		Hit = FindSeparatingAxis(Point,BoxExtent);
	}
};

/**
 *	Line Check With Triangle
 *	Algorithm based on "Fast, Minimum Storage Ray/Triangle Intersection"
 *	Returns true if the line segment does hit the triangle
 */
FORCEINLINE bool LineCheckWithTriangle(FHitResult& Result,const FVector& V1,const FVector& V2,const FVector& V3,const FVector& Start,const FVector& End,const FVector& Direction)
{
	FVector	Edge1 = V3 - V1,
		Edge2 = V2 - V1,
		P = Direction ^ Edge2;
	float	Determinant = Edge1 | P;

	if(Determinant < DELTA)
	{
		return false;
	}

	FVector	T = Start - V1;
	float	U = T | P;

	if(U < 0.0f || U > Determinant)
	{
		return false;
	}

	FVector	Q = T ^ Edge1;
	float	V = Direction | Q;

	if(V < 0.0f || U + V > Determinant)
	{
		return false;
	}

	float	Time = (Edge2 | Q) / Determinant;

	if(Time < 0.0f || Time > Result.Time)
	{
		return false;
	}

	Result.Normal = ((V3-V2)^(V2-V1)).SafeNormal();
	Result.Time = ((V1 - Start)|Result.Normal) / (Result.Normal|Direction);

	return true;
}

