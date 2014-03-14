// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

template< class T > class FInterpCurve
{
public:
	TArray< FInterpCurvePoint<T> >	Points;

	/** Constructor */
	FInterpCurve();

	/** Add a new keypoint to the InterpCurve with the supplied In and Out value. Returns the index of the new key.*/
	int32 AddPoint( const float InVal, const T &OutVal );

	/** Move a keypoint to a new In value. This may change the index of the keypoint, so the new key index is returned. */
	int32 MovePoint( int32 PointIndex, float NewInVal );

	/** Clear all keypoints from InterpCurve. */
	void Reset();

	/** 
	 *	Evaluate the output for an arbitary input value. 
	 *	For inputs outside the range of the keys, the first/last key value is assumed.
	 */
	T Eval( const float InVal, const T& Default, int32* PtIdx = NULL ) const;

	/** 
	 *	Evaluate the derivative at a point on the curve.
	 */
	T EvalDerivative( const float InVal, const T& Default, int32* PtIdx = NULL ) const;

	/** 
	 *	Evaluate the second derivative at a point on the curve.
	 */
	T EvalSecondDerivative( const float InVal, const T& Default, int32* PtIdx = NULL ) const;

	/** 
	 * Find the nearest point on spline to the given point.
	 *
	 * @param PointInSpace - the given point
	 * @param OutDistanceSq - output - the squared distance between the given point and the closest found point.
	 * @return The key (the 't' parameter) of the nearest point. 
	 */
	float InaccurateFindNearest( const T &PointInSpace, float& OutDistanceSq ) const;

	/** 
	 * Find the nearest point (to the given point) on segment between Points[PtIdx] and Points[PtIdx+1]
	 *
	 * @param PointInSpace - the given point
	 * @return The key (the 't' parameter) of the found point. 
	 */
	float InaccurateFindNearestOnSegment( const T &PointInSpace, int32 PtIdx, float& OutSquaredDistance ) const;

	/** Automatically set the tangents on the curve based on surrounding points */
	void AutoSetTangents(float Tension = 0.f);

	/** Calculate the min/max out value that can be returned by this InterpCurve. */
	void CalcBounds(T& OutMin, T& OutMax, const T& Default) const;


	/**
	 * Serializes the interp curve.
	 *
	 * @param Ar Reference to the serialization archive.
	 * @param Curve Reference to the interp curve being serialized.
	 *
	 * @return Reference to the Archive after serialization.
	 */
	friend FArchive& operator<<( FArchive& Ar, FInterpCurve& Curve )
	{
		// NOTE: This is not used often for FInterpCurves.  Most of the time these are serialized
		//   as inline struct properties in UnClass.cpp!

		Ar << Curve.Points;

		return Ar;
	}

};

template< class T > 
FORCEINLINE FInterpCurve<T>::FInterpCurve() {}

template< class T > 
FORCEINLINE int32 FInterpCurve<T>::AddPoint( const float InVal, const T &OutVal )
{
	int32 i=0; for( i=0; i<Points.Num() && Points[i].InVal < InVal; i++);
	Points.InsertUninitialized(i);
	Points[i] = FInterpCurvePoint< T >(InVal, OutVal);
	return i;
}

template< class T > 
FORCEINLINE int32 FInterpCurve<T>::MovePoint( int32 PointIndex, float NewInVal )
{
	if( PointIndex < 0 || PointIndex >= Points.Num() )
		return PointIndex;

	const T OutVal = Points[PointIndex].OutVal;
	const EInterpCurveMode Mode = Points[PointIndex].InterpMode;
	const T ArriveTan = Points[PointIndex].ArriveTangent;
	const T LeaveTan = Points[PointIndex].LeaveTangent;

	Points.RemoveAt(PointIndex);

	const int32 NewPointIndex = AddPoint( NewInVal, OutVal );
	Points[NewPointIndex].InterpMode = Mode;
	Points[NewPointIndex].ArriveTangent = ArriveTan;
	Points[NewPointIndex].LeaveTangent = LeaveTan;

	return NewPointIndex;
}

template< class T > 
FORCEINLINE void FInterpCurve<T>::Reset()
{
	Points.Empty();
}

template< class T > 
FORCEINLINE T FInterpCurve<T>::Eval( const float InVal, const T& Default, int32* PtIdx ) const
{
	const int32 NumPoints = Points.Num();

	// If no point in curve, return the Default value we passed in.
	if( NumPoints == 0 )
	{
		if( PtIdx )
		{
			*PtIdx = -1;
		}
		return Default;
	}

	// If only one point, or before the first point in the curve, return the first points value.
	if( NumPoints < 2 || (InVal <= Points[0].InVal) )
	{
		if( PtIdx )
		{
			*PtIdx = 0;
		}
		return Points[0].OutVal;
	}

	// If beyond the last point in the curve, return its value.
	if( InVal >= Points[NumPoints-1].InVal )
	{
		if( PtIdx )
		{
			*PtIdx = NumPoints - 1;
		}
		return Points[NumPoints-1].OutVal;
	}

	// Somewhere with curve range - linear search to find value.
	for( int32 i=1; i<NumPoints; i++ )
	{	
		if( InVal < Points[i].InVal )
		{
			const float Diff = Points[i].InVal - Points[i-1].InVal;

			if( Diff > 0.f && Points[i-1].InterpMode != CIM_Constant )
			{
				const float Alpha = (InVal - Points[i-1].InVal) / Diff;

				if( PtIdx )
				{
					*PtIdx = i - 1;
				}

				if( Points[i-1].InterpMode == CIM_Linear )
				{
					return FMath::Lerp( Points[i-1].OutVal, Points[i].OutVal, Alpha );
				}
				else
				{
					return FMath::CubicInterp( Points[i-1].OutVal, Points[i-1].LeaveTangent * Diff, Points[i].OutVal, Points[i].ArriveTangent * Diff, Alpha );
				}
			}
			else
			{
				if( PtIdx )
				{
					*PtIdx = i - 1;
				}

				return Points[i-1].OutVal;
			}
		}
	}

	// Shouldn't really reach here.
	if( PtIdx )
	{
		*PtIdx = NumPoints - 1;
	}

	return Points[NumPoints-1].OutVal;
}

template< class T > 
FORCEINLINE T FInterpCurve<T>::EvalDerivative( const float InVal, const T& Default, int32* PtIdx ) const
{
	const int32 NumPoints = Points.Num();

	// If no point in curve, return the Default value we passed in.
	if( NumPoints == 0 )
	{
		if( PtIdx )
		{
			*PtIdx = -1;
		}
		return Default;
	}

	// If only one point, or before the first point in the curve, return the first points value.
	if( NumPoints < 2 || (InVal <= Points[0].InVal) )
	{
		if( PtIdx )
		{
			*PtIdx = 0;
		}
		return Points[0].LeaveTangent;
	}

	// If beyond the last point in the curve, return its value.
	if( InVal >= Points[NumPoints-1].InVal )
	{
		if( PtIdx )
		{
			*PtIdx = NumPoints - 1;
		}
		return Points[NumPoints-1].ArriveTangent;
	}

	// Somewhere with curve range - linear search to find value.
	for( int32 i=1; i<NumPoints; i++ )
	{	
		if( InVal < Points[i].InVal )
		{
			const float Diff = Points[i].InVal - Points[i-1].InVal;

			if( Diff > 0.f && Points[i-1].InterpMode != CIM_Constant )
			{
				if( PtIdx )
				{
					*PtIdx = NumPoints - 1;
				}

				const float Alpha = (InVal - Points[i-1].InVal) / Diff;

				if( Points[i-1].InterpMode == CIM_Linear )
				{
					return FMath::Lerp( Points[i-1].OutVal, Points[i].OutVal, Alpha );
				}
				else
				{
					return FMath::CubicInterpDerivative( Points[i-1].OutVal, Points[i-1].LeaveTangent * Diff, Points[i].OutVal, Points[i].ArriveTangent * Diff, Alpha );
				}
			}
			else
			{
				if( PtIdx )
				{
					*PtIdx = -1;
				}

				return T(0.f);
			}
		}
	}

	if( PtIdx )
	{
		*PtIdx = NumPoints - 1;
	}

	// Shouldn't really reach here.
	return Points[NumPoints-1].OutVal;
}

template< class T > 
FORCEINLINE T FInterpCurve<T>::EvalSecondDerivative( const float InVal, const T& Default, int32* PtIdx ) const
{
	const int32 NumPoints = Points.Num();

	// If no point in curve, return the Default value we passed in.
	if( NumPoints == 0 )
	{
		if( PtIdx )
		{
			*PtIdx = -1;
		}
		return Default;
	}

	// If only one point, or before the first point in the curve, return the first points value.
	if( NumPoints < 2 || (InVal <= Points[0].InVal) )
	{
		if( PtIdx )
		{
			*PtIdx = 0;
		}
		return Points[0].OutVal;
	}

	// If beyond the last point in the curve, return its value.
	if( InVal >= Points[NumPoints-1].InVal )
	{
		if( PtIdx )
		{
			*PtIdx = NumPoints - 1;
		}
		return Points[NumPoints-1].OutVal;
	}

	// Somewhere with curve range - linear search to find value.
	for( int32 i=1; i<NumPoints; i++ )
	{	
		if( InVal < Points[i].InVal )
		{
			const float Diff = Points[i].InVal - Points[i-1].InVal;

			if( Diff > 0.f && Points[i-1].InterpMode != CIM_Constant )
			{
				if( PtIdx )
				{
					*PtIdx = i - 1;
				}
				const float Alpha = (InVal - Points[i-1].InVal) / Diff;

				if( Points[i-1].InterpMode == CIM_Linear )
				{
					return FMath::Lerp( Points[i-1].OutVal, Points[i].OutVal, Alpha );
				}
				else
				{
					return FMath::CubicInterpSecondDerivative( Points[i-1].OutVal, Points[i-1].LeaveTangent * Diff, Points[i].OutVal, Points[i].ArriveTangent * Diff, Alpha );
				}
			}
			else
			{
				if( PtIdx )
				{
					*PtIdx = -1;
				}
				return Default;
			}
		}
	}
	if( PtIdx )
	{
		*PtIdx = NumPoints - 1;
	}

	// Shouldn't really reach here.
	return Points[NumPoints-1].OutVal;
}

template< class T > 
FORCEINLINE float FInterpCurve<T>::InaccurateFindNearest( const T &PointInSpace, float& OutDistanceSq ) const
{
	const int32 NumPoints = Points.Num();
	if(NumPoints > 1)
	{
		float BestDistanceSq;
		float BestResult = InaccurateFindNearestOnSegment(PointInSpace, 0, BestDistanceSq);
		for(int32 segment = 1; segment < NumPoints - 1; ++segment)
		{
			float LocalDistanceSq;
			float LocalResult = InaccurateFindNearestOnSegment(PointInSpace, segment, LocalDistanceSq);
			if(LocalDistanceSq < BestDistanceSq)
			{
				BestDistanceSq = LocalDistanceSq;
				BestResult = LocalResult;
			}
		}
		OutDistanceSq = BestDistanceSq;
		return BestResult;
	}
	if( 1 == NumPoints )
	{
		OutDistanceSq = (PointInSpace - Points[0].OutVal).SizeSquared();
		return Points[0].InVal;
	}
	return 0.0f;
}

template< class T > 
FORCEINLINE float FInterpCurve<T>::InaccurateFindNearestOnSegment( const T &PointInSpace, int32 PtIdx, float& OutSquaredDistance ) const
{
	if( CIM_Constant == Points[PtIdx].InterpMode )
	{
		const float Distance1 = (Points[PtIdx].OutVal - PointInSpace).SizeSquared();
		const float Distance2 = (Points[PtIdx+1].OutVal - PointInSpace).SizeSquared();
		if(Distance1 < Distance2)
		{
			OutSquaredDistance = Distance1;
			return Points[PtIdx].InVal;
		}
		OutSquaredDistance = Distance2;
		return Points[PtIdx+1].InVal;
	}

	const float Diff = Points[PtIdx+1].InVal - Points[PtIdx].InVal;
	if(CIM_Linear == Points[PtIdx].InterpMode )
	{
		// like in function: FMath::ClosestPointOnLine
		const float A = (Points[PtIdx].OutVal-PointInSpace) | (Points[PtIdx+1].OutVal - Points[PtIdx].OutVal);
		const float B = (Points[PtIdx+1].OutVal - Points[PtIdx].OutVal).SizeSquared();
		const float V = FMath::Clamp(-A/B, 0.f, 1.f);
		OutSquaredDistance = (FMath::Lerp( Points[PtIdx].OutVal, Points[PtIdx+1].OutVal, V ) - PointInSpace).SizeSquared();
		return V * Diff + Points[PtIdx].InVal;
	}
		
	{
		const int32 PointsChecked = 3;
		const int32 IterationNum = 3;
		const float Scale = 0.75;

		// Newton's methods is repeated 3 times, starting with t = 0, 0.5, 1.
		float ValuesT[PointsChecked];
		ValuesT[0] = 0.0f; 
		ValuesT[1] = 0.5f; 
		ValuesT[2] = 1.0f;

		T InitialPoints[PointsChecked];
		InitialPoints[0] = Points[PtIdx].OutVal;
		InitialPoints[1] = 
			FMath::CubicInterp( Points[PtIdx].OutVal, Points[PtIdx].LeaveTangent * Diff, Points[PtIdx+1].OutVal, Points[PtIdx+1].ArriveTangent * Diff, ValuesT[1] );
		InitialPoints[2] = Points[PtIdx+1].OutVal;

		float DistancesSq[PointsChecked];

		for(int32 point = 0; point < PointsChecked; ++point)
		{
			//Algorithm explanation: http://permalink.gmane.org/gmane.games.devel.sweng/8285
			T FoundPoint = InitialPoints[point];
			float LastMove = 1.0f;
			for(int32 iter = 0; iter < IterationNum; ++iter)
			{	
				const T LastBestTangent = FMath::CubicInterpDerivative( Points[PtIdx].OutVal, Points[PtIdx].LeaveTangent * Diff, Points[PtIdx+1].OutVal, Points[PtIdx+1].ArriveTangent * Diff, ValuesT[point]);
				const T Delta = (PointInSpace - FoundPoint);
				float Move = (LastBestTangent | Delta)/LastBestTangent.SizeSquared();
				Move = FMath::Clamp(Move, -LastMove*Scale, LastMove*Scale);
				ValuesT[point] += Move;
				ValuesT[point] = FMath::Clamp(ValuesT[point], 0.0f, 1.0f);
				LastMove = FMath::Abs(Move);
				FoundPoint = 
					FMath::CubicInterp( Points[PtIdx].OutVal, Points[PtIdx].LeaveTangent * Diff, Points[PtIdx+1].OutVal, Points[PtIdx+1].ArriveTangent * Diff, ValuesT[point] );
			}
			DistancesSq[point] = (FoundPoint-PointInSpace).SizeSquared();
			ValuesT[point] = ValuesT[point] * Diff + Points[PtIdx].InVal;
		}

		if(DistancesSq[0] <= DistancesSq[1] && DistancesSq[0] <= DistancesSq[2])
		{
			OutSquaredDistance = DistancesSq[0];
			return ValuesT[0];
		}
		if(DistancesSq[1] <= DistancesSq[2])
		{
			OutSquaredDistance = DistancesSq[1];
			return ValuesT[1];
		}
		OutSquaredDistance = DistancesSq[2];
		return ValuesT[2];
	}
}

template< class T > 
FORCEINLINE void FInterpCurve<T>::AutoSetTangents(float Tension)
{
	// Iterate over all points in this InterpCurve
	for(int32 PointIndex=0; PointIndex<Points.Num(); PointIndex++)
	{
		T ArriveTangent = Points[PointIndex].ArriveTangent;
		T LeaveTangent = Points[PointIndex].LeaveTangent;

		if(PointIndex == 0)
		{
			if(PointIndex < Points.Num()-1) // Start point
			{
				// If first section is not a curve, or is a curve and first point has manual tangent setting.
				if( Points[PointIndex].InterpMode == CIM_CurveAuto || Points[PointIndex].InterpMode == CIM_CurveAutoClamped )
				{
					FMemory::Memset( &LeaveTangent, 0, sizeof(T) );
				}
			}
			else // Only point
			{
				FMemory::Memset( &LeaveTangent, 0, sizeof(T) );
			}
		}
		else
		{
			if(PointIndex < Points.Num()-1) // Inner point
			{
				if( Points[PointIndex].InterpMode == CIM_CurveAuto || Points[PointIndex].InterpMode == CIM_CurveAutoClamped )
				{
					if( Points[PointIndex-1].IsCurveKey() && Points[PointIndex].IsCurveKey() )
					{
						const bool bWantClamping = ( Points[ PointIndex ].InterpMode == CIM_CurveAutoClamped );

						ComputeCurveTangent(
							Points[ PointIndex - 1 ].InVal,		// Previous time
							Points[ PointIndex - 1 ].OutVal,	// Previous point
							Points[ PointIndex ].InVal,			// Current time
							Points[ PointIndex ].OutVal,		// Current point
							Points[ PointIndex + 1 ].InVal,		// Next time
							Points[ PointIndex + 1 ].OutVal,	// Next point
							Tension,							// Tension
							bWantClamping,						// Want clamping?
							ArriveTangent );					// Out

						// In 'auto' mode, arrive and leave tangents are always the same
						LeaveTangent = ArriveTangent;
					}
					else if( Points[PointIndex-1].InterpMode == CIM_Constant || Points[PointIndex].InterpMode == CIM_Constant )
					{
						FMemory::Memset( &ArriveTangent, 0, sizeof(T) );
						FMemory::Memset( &LeaveTangent, 0, sizeof(T) );
					}
				}
			}
			else // End point
			{
				// If last section is not a curve, or is a curve and final point has manual tangent setting.
				if( Points[PointIndex].InterpMode == CIM_CurveAuto || Points[PointIndex].InterpMode == CIM_CurveAutoClamped )
				{
					FMemory::Memset( &ArriveTangent, 0, sizeof(T) );
				}
			}
		}

		Points[PointIndex].ArriveTangent = ArriveTangent;
		Points[PointIndex].LeaveTangent = LeaveTangent;
	}
}

template< class T > 
FORCEINLINE void FInterpCurve<T>::CalcBounds(T& OutMin, T& OutMax, const T& Default) const
{
	if(Points.Num() == 0)
	{
		OutMin = Default;
		OutMax = Default;
	}
	else if(Points.Num() == 1)
	{
		OutMin = Points[0].OutVal;
		OutMax = Points[0].OutVal;
	}
	else
	{
		OutMin = Points[0].OutVal;
		OutMax = Points[0].OutVal;

		for(int32 i=1; i<Points.Num(); i++)
		{
			CurveFindIntervalBounds( Points[i-1], Points[i], OutMin, OutMax, 0.f );
		}
	}
}


typedef FInterpCurve<float>				FInterpCurveFloat;
typedef FInterpCurve<FVector2D>			FInterpCurveVector2D;
typedef FInterpCurve<FVector>			FInterpCurveVector;
typedef FInterpCurve<FQuat>				FInterpCurveQuat;
typedef FInterpCurve<FTwoVectors>		FInterpCurveTwoVectors;
typedef FInterpCurve<FLinearColor>		FInterpCurveLinearColor;
