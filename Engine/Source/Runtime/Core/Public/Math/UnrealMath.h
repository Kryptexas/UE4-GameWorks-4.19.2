// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UnMath.h: Unreal math Types and inline implementation 
=============================================================================*/

#pragma once
#include "Color.h"
#include "ColorList.h"
#include "IntPoint.h"
#include "IntVector.h"
#include "Vector2D.h"
#include "IntRect.h"
#include "Vector.h"
#include "Vector4.h"
#include "TwoVectors.h"
#include "Edge.h"
#include "Plane.h"
#include "Sphere.h"
#include "Rotator.h"
#include "RangeBound.h"
#include "Range.h"
#include "RangeSet.h"
#include "Interval.h"
#include "Box.h"
#include "BoxSphereBounds.h"
#include "OrientedBox.h"
#include "Axis.h"
#include "Matrix.h"
#include "RotationTranslationMatrix.h"
#include "ScaleRotationTranslationMatrix.h"
#include "RotationMatrix.h"
#include "Quat.h"
#include "PerspectiveMatrix.h"
#include "OrthoMatrix.h"
#include "TranslationMatrix.h"
#include "QuatRotationTranslationMatrix.h"
#include "InverseRotationMatrix.h"
#include "ScaleMatrix.h"
#include "MirrorMatrix.h"
#include "ClipProjectionMatrix.h"
#include "InterpCurvePoint.h"
#include "InterpCurve.h"
#include "CurveEdInterface.h"
#include "Float32.h"
#include "Float16.h"
#include "Float16Color.h"
#include "Vector2DHalf.h"
#include "AlphaBlendType.h"
#include "ScalarRegister.h"
#include "ConvexHull2d.h"

// FVector2D Implementation
FORCEINLINE FVector2D::FVector2D( const FVector& V )
: X(V.X), Y(V.Y)
{
}

inline FVector FVector2D::SphericalToUnitCartesian() const
{
	const float SinTheta = FMath::Sin(X);
	return FVector(FMath::Cos(Y) * SinTheta, FMath::Sin(Y) * SinTheta, FMath::Cos(X));
}

// FVector Implementation
FORCEINLINE FVector::FVector( const FVector4& V )
	: X(V.X), Y(V.Y), Z(V.Z)
{
}

inline FVector FVector::MirrorByPlane( const FPlane& Plane ) const
{
	return *this - Plane * (2.f * Plane.PlaneDot(*this) );
}

inline FVector FVector::PointPlaneProject(const FVector& Point, const FPlane& Plane)
{
	//Find the distance of X from the plane
	//Add the distance back along the normal from the point
	return Point - Plane.PlaneDot(Point) * Plane;
}

inline FVector FVector::PointPlaneProject(const FVector& Point, const FVector& A, const FVector& B, const FVector& C)
{
	//Compute the plane normal from ABC
	FPlane Plane(A, B, C);

	//Find the distance of X from the plane
	//Add the distance back along the normal from the point
	return Point - Plane.PlaneDot(Point) * Plane;
}

inline FPlane FPlane::TransformBy( const FMatrix& M ) const
{
	const FMatrix tmpTA = M.TransposeAdjoint();
	const float DetM = M.Determinant();
	return this->TransformByUsingAdjointT(M, DetM, tmpTA);
}

inline FPlane FPlane::TransformByUsingAdjointT( const FMatrix& M, float DetM, const FMatrix& TA ) const
{
	FVector newNorm = TA.TransformVector(*this).SafeNormal();

	if(DetM < 0.f)
	{
		newNorm *= -1.0f;
	}

	return FPlane(M.TransformPosition(*this * W), newNorm);
}

// FMath Implementation
inline FVector FMath::LinePlaneIntersection
	(
	const FVector &Point1,
	const FVector &Point2,
	const FVector &PlaneOrigin,
	const FVector &PlaneNormal
	)
{
	return
		Point1
		+	(Point2-Point1)
		*	(((PlaneOrigin - Point1)|PlaneNormal) / ((Point2 - Point1)|PlaneNormal));
}
inline FVector FMath::LinePlaneIntersection
	(
	const FVector &Point1,
	const FVector &Point2,
	const FPlane  &Plane
	)
{
	return
		Point1
		+	(Point2-Point1)
		*	((Plane.W - (Point1|Plane))/((Point2 - Point1)|Plane));
}

inline bool FMath::PointBoxIntersection
	(
	const FVector&	Point,
	const FBox&		Box
	)
{
	if(Point.X >= Box.Min.X && Point.X <= Box.Max.X &&
		Point.Y >= Box.Min.Y && Point.Y <= Box.Max.Y &&
		Point.Z >= Box.Min.Z && Point.Z <= Box.Max.Z)
		return 1;
	else
		return 0;
}

inline bool FMath::LineBoxIntersection
	(
	const FBox&		Box,
	const FVector&	Start,
	const FVector&	End,
	const FVector&	Direction,
	const FVector&	OneOverDirection
	)
{
	FVector	Time;
	bool	bStartIsOutside = false;

	if(Start.X < Box.Min.X)
	{
		bStartIsOutside = true;
		if(End.X >= Box.Min.X)
		{
			Time.X = (Box.Min.X - Start.X) * OneOverDirection.X;
		}
		else
		{
			return false;
		}
	}
	else if(Start.X > Box.Max.X)
	{
		bStartIsOutside = true;
		if(End.X <= Box.Max.X)
		{
			Time.X = (Box.Max.X - Start.X) * OneOverDirection.X;
		}
		else
		{
			return false;
		}
	}
	else
	{
		Time.X = 0.0f;
	}

	if(Start.Y < Box.Min.Y)
	{
		bStartIsOutside = true;
		if(End.Y >= Box.Min.Y)
		{
			Time.Y = (Box.Min.Y - Start.Y) * OneOverDirection.Y;
		}
		else
		{
			return false;
		}
	}
	else if(Start.Y > Box.Max.Y)
	{
		bStartIsOutside = true;
		if(End.Y <= Box.Max.Y)
		{
			Time.Y = (Box.Max.Y - Start.Y) * OneOverDirection.Y;
		}
		else
		{
			return false;
		}
	}
	else
	{
		Time.Y = 0.0f;
	}

	if(Start.Z < Box.Min.Z)
	{
		bStartIsOutside = true;
		if(End.Z >= Box.Min.Z)
		{
			Time.Z = (Box.Min.Z - Start.Z) * OneOverDirection.Z;
		}
		else
		{
			return false;
		}
	}
	else if(Start.Z > Box.Max.Z)
	{
		bStartIsOutside = true;
		if(End.Z <= Box.Max.Z)
		{
			Time.Z = (Box.Max.Z - Start.Z) * OneOverDirection.Z;
		}
		else
		{
			return false;
		}
	}
	else
	{
		Time.Z = 0.0f;
	}

	if(bStartIsOutside)
	{
		const float	MaxTime = Max3(Time.X,Time.Y,Time.Z);

		if(MaxTime >= 0.0f && MaxTime <= 1.0f)
		{
			const FVector Hit = Start + Direction * MaxTime;
			const float BOX_SIDE_THRESHOLD = 0.1f;
			if(	Hit.X > Box.Min.X - BOX_SIDE_THRESHOLD && Hit.X < Box.Max.X + BOX_SIDE_THRESHOLD &&
				Hit.Y > Box.Min.Y - BOX_SIDE_THRESHOLD && Hit.Y < Box.Max.Y + BOX_SIDE_THRESHOLD &&
				Hit.Z > Box.Min.Z - BOX_SIDE_THRESHOLD && Hit.Z < Box.Max.Z + BOX_SIDE_THRESHOLD)
			{
				return true;
			}
		}

		return false;
	}
	else
	{
		return true;
	}
}

inline bool FMath::LineSphereIntersection(const FVector& Start,const FVector& Dir,float Length,const FVector& Origin,float Radius)
{
	const FVector	EO = Start - Origin;
	const float		v = (Dir | (Origin - Start));
	const float		disc = Radius * Radius - ((EO | EO) - v * v);

	if(disc >= 0.0f)
	{
		const float	Time = (v - Sqrt(disc)) / Length;

		if(Time >= 0.0f && Time <= 1.0f)
			return 1;
		else
			return 0;
	}
	else
		return 0;
}

inline bool FMath::IntersectPlanes3( FVector& I, const FPlane& P1, const FPlane& P2, const FPlane& P3 )
{
	// Compute determinant, the triple product P1|(P2^P3)==(P1^P2)|P3.
	const float Det = (P1 ^ P2) | P3;
	if( Square(Det) < Square(0.001f) )
	{
		// Degenerate.
		I = FVector::ZeroVector;
		return 0;
	}
	else
	{
		// Compute the intersection point, guaranteed valid if determinant is nonzero.
		I = (P1.W*(P2^P3) + P2.W*(P3^P1) + P3.W*(P1^P2)) / Det;
	}
	return 1;
}

inline bool FMath::IntersectPlanes2( FVector& I, FVector& D, const FPlane& P1, const FPlane& P2 )
{
	// Compute line direction, perpendicular to both plane normals.
	D = P1 ^ P2;
	const float DD = D.SizeSquared();
	if( DD < Square(0.001f) )
	{
		// Parallel or nearly parallel planes.
		D = I = FVector::ZeroVector;
		return 0;
	}
	else
	{
		// Compute intersection.
		I = (P1.W*(P2^D) + P2.W*(D^P1)) / DD;
		D.Normalize();
		return 1;
	}
}

FORCEINLINE float FMath::GetRangePct(FVector2D const& Range, float Value)
{
	return (Range.X != Range.Y) ? (Value - Range.X) / (Range.Y - Range.X) : Range.X;
}

FORCEINLINE float FMath::GetRangeValue(FVector2D const& Range, float Pct)
{
	return Lerp<float>(Range.X, Range.Y, Pct);
}

inline FVector FMath::VRand()
{
	FVector Result;

	float L;

	do
	{
		// Check random vectors in the unit sphere so result is statistically uniform.
		Result.X = FRand() * 2 - 1;
		Result.Y = FRand() * 2 - 1;
		Result.Z = FRand() * 2 - 1;

		L = Result.SizeSquared();
	} while(L > 1.0f || L < 0.001f);

	return Result * (1.0f / Sqrt(L));
}

inline FString FMath::FormatIntToHumanReadable(int32 Val)
{
	FString Src = *FString::Printf( TEXT("%i"), Val );
	FString Dst;

	if( Val > 999 )
	{
		Dst = FString::Printf( TEXT(",%s"), *Src.Mid( Src.Len() - 3, 3 ) );
		Src = Src.Left( Src.Len() - 3 );

	}

	if( Val > 999999 )
	{
		Dst = FString::Printf( TEXT(",%s%s"), *Src.Mid( Src.Len() - 3, 3 ), *Dst );
		Src = Src.Left( Src.Len() - 3 );
	}

	Dst = Src + Dst;

	return Dst;
}

template<class U>
FORCEINLINE_DEBUGGABLE FRotator FMath::Lerp( const FRotator& A, const FRotator& B, const U& Alpha)
{
	return A + (B - A).GetNormalized() * Alpha;
}


template<class U>
FORCEINLINE_DEBUGGABLE FQuat FMath::Lerp( const FQuat& A, const FQuat& B, const U& Alpha)
{
	return FQuat::Slerp(A, B, Alpha);
}

template<class U>
FORCEINLINE_DEBUGGABLE FQuat FMath::BiLerp(const FQuat& P00, const FQuat& P10, const FQuat& P01, const FQuat& P11, float FracX, float FracY)
{
	FQuat Result;

	Result = Lerp(
		Lerp(P00,P10,FracX),
		Lerp(P01,P11,FracX),
		FracY
		);

	Result.Normalize();

	return Result;
}

template<class U>
FORCEINLINE_DEBUGGABLE FQuat FMath::CubicInterp( const FQuat& P0, const FQuat& T0, const FQuat& P1, const FQuat& T1, const U& A)
{
	return FQuat::Squad(P0, T0, P1, T1, A);
}

/**
 * Performs a sphere vs box intersection test using Arvo's algorithm:
 *
 *	for each i in (x, y, z)
 *		if (SphereCenter(i) < BoxMin(i)) d2 += (SphereCenter(i) - BoxMin(i)) ^ 2
 *		else if (SphereCenter(i) > BoxMax(i)) d2 += (SphereCenter(i) - BoxMax(i)) ^ 2
 *
 * @param Sphere the center of the sphere being tested against the AABB
 * @param RadiusSquared the size of the sphere being tested
 * @param AABB the box being tested against
 *
 * @return Whether the sphere/box intersect or not.
 */
FORCEINLINE bool FMath::SphereAABBIntersection(const FVector& SphereCenter,const float RadiusSquared,const FBox& AABB)
{
	// Accumulates the distance as we iterate axis
	float DistSquared = 0.f;
	// Check each axis for min/max and add the distance accordingly
	// NOTE: Loop manually unrolled for > 2x speed up
	if (SphereCenter.X < AABB.Min.X)
	{
		DistSquared += FMath::Square(SphereCenter.X - AABB.Min.X);
	}
	else if (SphereCenter.X > AABB.Max.X)
	{
		DistSquared += FMath::Square(SphereCenter.X - AABB.Max.X);
	}
	if (SphereCenter.Y < AABB.Min.Y)
	{
		DistSquared += FMath::Square(SphereCenter.Y - AABB.Min.Y);
	}
	else if (SphereCenter.Y > AABB.Max.Y)
	{
		DistSquared += FMath::Square(SphereCenter.Y - AABB.Max.Y);
	}
	if (SphereCenter.Z < AABB.Min.Z)
	{
		DistSquared += FMath::Square(SphereCenter.Z - AABB.Min.Z);
	}
	else if (SphereCenter.Z > AABB.Max.Z)
	{
		DistSquared += FMath::Square(SphereCenter.Z - AABB.Max.Z);
	}
	// If the distance is less than or equal to the radius, they intersect
	return DistSquared <= RadiusSquared;
}

/**
 * Converts a sphere into a point plus radius squared for the test above
 */
FORCEINLINE bool FMath::SphereAABBIntersection(const FSphere& Sphere,const FBox& AABB)
{
	float RadiusSquared = FMath::Square(Sphere.W);
	// If the distance is less than or equal to the radius, they intersect
	return SphereAABBIntersection(Sphere.Center,RadiusSquared,AABB);
}




