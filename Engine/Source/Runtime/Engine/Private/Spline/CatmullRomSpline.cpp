// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

FCatmullRomSpline::FCatmullRomSpline()
{

}

FVector FCatmullRomSpline::GetPosition(UWorld* World) const
{
	if(IsValid() == false)
	{
		UE_LOG(LogSpline, Warning, TEXT("Trying to use a spline with only %i points!"), ControlPoints.Num());
		return FVector(0);
	}

	check(World)

	//We need to figure out where we are along our curve
	//Cap between 0 and duration.
	if(StartTime == 0.0f)
	{
		return ControlPoints[1];
	}

	//grab how far along we are
	float Time = FMath::Clamp<float>(World->GetTimeSeconds() - StartTime, 0.0f, Duration);

	//Get the point using time (0 - Duration)
	return GetPositionFromTime(Time);
}

FVector FCatmullRomSpline::GetTanget(UWorld* World) const
{
	if(IsValid() == false)
	{
		UE_LOG(LogSpline, Warning, TEXT("Trying to use a spline with only %i points!"), ControlPoints.Num());
		return FVector(0);
	}

	check(World)

	float Time = FMath::Clamp<float>(World->GetTimeSeconds() - StartTime, 0.0f, Duration);

	return GetTangentFromTime(Time);
}

void FCatmullRomSpline::AddControlPoint(const FVector& Point, int32 Index)
{
	if(Index == -1)
	{
		ControlPoints.Add(Point);
	}
	else
	{
		ControlPoints.Insert(Point, Index);
	}
}

float FCatmullRomSpline::GetLength() const
{
	return FMath::Clamp<float>((float)(ControlPoints.Num()-3), 0, FLT_MAX);
}

FVector FCatmullRomSpline::GetPositionFromTime(float Time) const
{
	if(IsValid() == false)
	{
		UE_LOG(LogSpline, Warning, TEXT("Trying to use a spline with only %i points!"), ControlPoints.Num());
		return FVector(0);
	}

	//Now, we need to change this number into the range 0-(N-3)
	Time = FMath::Clamp<float>(Time/Duration, 0.0f, 1.0f);

	//now, go between 0 - (N-3)
	Time = FMath::Clamp<float>(Time * GetLength(), 0.0f, GetLength());

	//if we are at one, we can just bail now
	if(FMath::IsNearlyEqual(Time, GetLength()) == true)
	{
		return ControlPoints[ControlPoints.Num()-2];
	}

	FVector P0,P1,P2,P3;
	GetCorrectPoints( Time, P0, P1, P2, P3 );

	//Return our calculated point
	return GetCatMullRomPoint(P0, P1, P2, P3, Time - FMath::TruncToFloat(Time));
}

bool FCatmullRomSpline::IsValid() const
{
	return (GetNumOfControlPoints() >= 4);
}

int32 FCatmullRomSpline::GetNumOfControlPoints() const
{
	return ControlPoints.Num();
}

FVector FCatmullRomSpline::GetTangentFromTime(float Time) const
{
	if(IsValid() == false)
	{
		UE_LOG(LogSpline, Warning, TEXT("Trying to use a spline with only %i points!"), ControlPoints.Num());
		return FVector(0);
	}

	//Now, we need to change this number into the range 0-(N-3)
	Time = FMath::Clamp<float>(Time/Duration, 0.0f, 1.0f);

	//now, go between 0 - (N-3)
	Time = FMath::Clamp<float>(Time * GetLength(), 0.0f, GetLength());

	FVector P0,P1,P2,P3;
	GetCorrectPoints( Time, P0, P1, P2, P3 );

	//Return our calculated point
	return InternalGetTangetAtPoint(P0, P1, P2, P3, Time - FMath::TruncToFloat(Time));
}

FVector FCatmullRomSpline::GetNormalFromTime(float Time) const
{
	FVector Tan = GetTangentFromTime(Time);
	return (Tan ^ (FVector(0.0f, 0.0f, 1.0f) ^ Tan)).SafeNormal();
}

FVector FCatmullRomSpline::GetBinormalFromTime(float Time) const
{
	return (GetNormalFromTime(Time) ^ GetTangentFromTime(Time)).SafeNormal();
}

FVector FCatmullRomSpline::GetStartPoint() const
{
	if(IsValid() == false)
	{
		UE_LOG(LogSpline, Warning, TEXT("Trying to use a spline with only %i points!"), ControlPoints.Num());
		return FVector(0);
	}

	return ControlPoints[1];
}

FVector FCatmullRomSpline::GetEndPoint() const
{
	if(IsValid() == false)
	{
		UE_LOG(LogSpline, Warning, TEXT("Trying to use a spline with only %i points!"), ControlPoints.Num());
		return FVector(0);
	}

	return ControlPoints[GetNumOfControlPoints() - 2];
}

float FCatmullRomSpline::GetSplineLengthAtTime(float Time, float StepSize) const
{
	if(IsValid() == false)
	{
		UE_LOG(LogSpline, Warning, TEXT("Trying to use a spline with only %i points!"), ControlPoints.Num());
		return 0.0f;
	}

	Time = FMath::Clamp<float>(Time, 0.0f, GetDuration());
	int32 Steps = FMath::CeilToInt(Time / StepSize);
	float Length = 0.0f;

	//now, draw the curve
	float T = 0.0f;
	for(int32 i = 0; i < Steps; ++i)
	{
		//grab the two vectors
		FVector P0 = FCatmullRomSpline::GetPositionFromTime(T);
		FVector P1 = FCatmullRomSpline::GetPositionFromTime(FMath::Clamp<float>(T + StepSize, 0.0f, Time));

		//add the length
		Length += (P1 - P0).Size();

		//inc the T value
		T = FMath::Clamp<float>(T + StepSize, 0.0f, Time);
	}

	return Length;
}

float FCatmullRomSpline::GetSplineLength(float StepSize) const
{
	return GetSplineLengthAtTime(GetDuration(), StepSize);
}

void FCatmullRomSpline::DrawDebugSpline(UWorld* World, int32 Steps, float LifeTime, float Thickness, bool bPersistent, bool bJustDrawSpline, float BasisLength) const
{
	if(World == NULL)
	{
		UE_LOG(LogSpline, Warning, TEXT("Has no world! Can't debug draw!"));
		return;
	}

	//if we don't have a valid spline, then we can't draw it either.
	if (IsValid() == false)
	{
		UE_LOG(LogSpline, Warning, TEXT("FCatmullRomSpline::DrawDebugSpline: Not a valid Spline! Can't draw!"));
		return;
	}

	//draw the normal debug drawing
	FSplineBase::DrawDebugSpline(World, Steps, LifeTime, Thickness, bPersistent, bJustDrawSpline, BasisLength);

	//now, also draw the tangents
	DrawDebugLine(World, ControlPoints[0],                     ControlPoints[1],                     FColor::Yellow, bPersistent, LifeTime, SDPG_World, Thickness);
	DrawDebugLine(World, ControlPoints[ControlPoints.Num()-2], ControlPoints[ControlPoints.Num()-1], FColor::Yellow, bPersistent, LifeTime, SDPG_World, Thickness);
}

void FCatmullRomSpline::ClearSpline()
{
	ControlPoints.Reset();
	StartTime = 0.0f;
	Duration = 1.0f;
}

void FCatmullRomSpline::MakeSplineValid(int32 NumOfPoints)
{
	NumOfPoints = FMath::Max(NumOfPoints, 4);
	for(int32 i = GetNumOfControlPoints(); i < NumOfPoints; ++i)
	{
		AddControlPoint(FVector(0));
	}
}

FVector& FCatmullRomSpline::operator[]( int32 Index )
{
	//if the spline is outside of the bounds, add it
	if(Index >= GetNumOfControlPoints())
	{
		MakeSplineValid(Index + 1);
	}

	return ControlPoints[Index];
}

void FCatmullRomSpline::GetCorrectPoints(float TValue,
	FVector& P0, 
	FVector& P1, 
	FVector& P2, 
	FVector& P3) const
{
	TValue = FMath::Clamp<float>(TValue, 0.0f, GetLength());

	//get the index of the first point
	int32 Index = FMath::FloorToInt(TValue);

	//if we are at one, we can just bail now
	if(FMath::IsNearlyEqual(TValue, GetLength()) == true)
	{
		Index = ControlPoints.Num()-4;
	}

	//Sweet, now we need to get the start index of our
	check(Index <= (ControlPoints.Num()-4));

	//setup these points
	P0 = ControlPoints[Index++];
	P1 = ControlPoints[Index++];
	P2 = ControlPoints[Index++];
	P3 = ControlPoints[Index++];
}

FVector FCatmullRomSpline::GetCatMullRomPoint(const FVector& P0, 
	const FVector& P1, 
	const FVector& P2, 
	const FVector& P3, 
	float Time) const
{
	FVector Ret = 0.5f *( ( 2.0f * P1 ) +
		( -P0 + P2 ) * Time +
		( (2.0f*P0) - (5.0f*P1) + (4.0f*P2) - P3) * (Time*Time) +
		( -P0 + (3.0f*P1) - (3.0f*P2) + P3 ) * (Time*Time*Time) );
	return Ret;
}

FVector FCatmullRomSpline::InternalGetTangetAtPoint(const FVector& P0, 
	const FVector& P1, 
	const FVector& P2, 
	const FVector& P3, 
	float Time) const
{
	FVector T0 = (P1-P0) * 0.5f;
	FVector T1 = (P3-P2) * 0.5f;
	return FMath::CubicInterpDerivative<FVector, float>(P1, T0, P2, T1, Time).SafeNormal();
}