// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

DEFINE_LOG_CATEGORY(LogSpline);

//////////////////////////////////////////////////////////////
// FSplineBase
/////////////////////////////////////////////////////////////

void FSplineBase::SetupSpline(TArray<FVector> ControlPoints, float InDuration)
{

}

bool FSplineBase::IsValid() const
{
	return false;
}

void FSplineBase::SetDuration(float InDuration) 
{ 
	Duration = InDuration; 
}

float FSplineBase::GetDuration() const 
{ 
	return Duration; 
}

void FSplineBase::ClearSpline()
{

}

FVector FSplineBase::GetStartPoint() const
{
	return FVector(0);
}

FVector FSplineBase::GetEndPoint() const
{
	return FVector(0);
}

FVector FSplineBase::GetPositionFromTime( float Time ) const
{
	return FVector(0);
}

FVector FSplineBase::GetTangentFromTime( float Time ) const
{
	return FVector(0);
}

FVector FSplineBase::GetNormalFromTime( float Time ) const
{
	return FVector(0);
}

FVector FSplineBase::GetBinormalFromTime( float Time ) const
{
	return FVector(0);
}

float FSplineBase::GetSplineLengthAtTime( float Time, float StepSize /*= 0.1f*/ ) const
{
	return 0.0f;
}

float FSplineBase::GetSplineLength( float StepSize /*= 0.1f*/ ) const
{
	return 0.0f;
}

void FSplineBase::AddControlPoint(const FVector& Point, int32 Index)
{

}

bool FSplineBase::IsAtStart(UWorld* World) const
{
	//no world? No automatic play function
	check(World);

	return StartTime == 0.0f || FMath::IsNearlyEqual(StartTime, World->GetTimeSeconds());
}

bool FSplineBase::IsAtEnd(UWorld* World) const
{
	//no world? No automatic play function
	check(World);

	return ( StartTime > 0.0f ) && ( (StartTime + Duration) <= World->GetTimeSeconds() );
}

void FSplineBase::DrawDebugSpline(UWorld* World, int32 Steps, float LifeTime, float Thickness, bool bPersistent, bool bJustDrawSpline, float BasisLength) const
{
	if(IsValid() == false)
	{
		UE_LOG(LogSpline, Warning, TEXT("Trying to use an Invalid spline!"));
		return;
	}

	if(World == NULL)
	{
		UE_LOG(LogSpline, Warning, TEXT("No world passed into DrawDebugSpline!"));
		return;
	}

	//now, draw the curve
	const float TStep = GetDuration() / static_cast<float>(Steps);
	float T = 0.0f;
	for(int32 i = 0; i < Steps; ++i)
	{
		float T0 = T;
		float T1 = FMath::Clamp<float>(T0 + TStep, 0, GetDuration());

		FVector P0 = GetPositionFromTime(T0);
		FVector P1 = GetPositionFromTime(T1);

		DrawDebugLine(World, P0, P1, FColor::White, bPersistent, LifeTime, SDPG_World, Thickness);

		if(bJustDrawSpline == false)
		{
			FVector Tangent = GetTangentFromTime(T0);
			FVector Normal = GetNormalFromTime(T0);
			FVector Binormal = GetBinormalFromTime(T0);

			DrawDebugLine(World, P0, P0 + (Tangent * BasisLength), FColor::Red, bPersistent, LifeTime, SDPG_World, Thickness);

			DrawDebugLine(World, P0, P0 + (Normal * BasisLength), FColor::Blue, bPersistent, LifeTime, SDPG_World, Thickness);

			DrawDebugLine(World, P0, P0 + (Binormal * BasisLength), FColor::Green, bPersistent, LifeTime, SDPG_World, Thickness);
		}

		T = FMath::Clamp<float>(T + TStep, 0, GetDuration());
	}
}

void FSplineBase::Play(UWorld* World)
{
	//no world? No automatic play function
	check(World);

	StartTime = World->GetTimeSeconds();
}

bool FSplineBase::IsPlaying(UWorld* World) const
{
	//no world? No automatic play function
	check(World);

	return (StartTime != 0.0f) && (World->GetTimeSeconds() >= StartTime) && ( World->GetTimeSeconds() <= (StartTime + Duration) );
}

FVector FSplineBase::GetPosition(UWorld* World) const 
{ 
	return FVector(0);
};

FVector FSplineBase::GetTanget(UWorld* World) const 
{ 
	return FVector(0); 
};

float FSplineBase::GetT(UWorld* World) const
{
	//no world? No automatic play function
	check(World);

	if(StartTime != 0.0)
	{
		return FMath::Clamp<float>( ( World->GetTimeSeconds() - StartTime ) / Duration, 0.0f, 1.0f );
	}
	else
	{
		return 0.0f;
	}
}

float FSplineBase::GetTime(UWorld* World) const
{
	//no world? No automatic play function
	check(World);

	if(StartTime != 0.0)
	{
		return FMath::Clamp<float>( ( World->GetTimeSeconds() - StartTime ), 0.0f, Duration);
	}
	else
	{
		return 0.0f;
	}
}

int32 FSplineBase::GetNumOfControlPoints() const 
{ 
	return 0; 
}

void FSplineBase::MakeSplineValid(int32 NumOfPoints) 
{

}

//////////////////////////////////////////////////////////////
// USpline
/////////////////////////////////////////////////////////////
USpline::USpline(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	bConstantVelocity = false;
}

void USpline::SetDuration(float InDuration)
{
	Spline.SetDuration(InDuration);
}

void USpline::SetupSpline(TArray<FVector> ControlPoints, float InDuration)
{
	//MUST have points
	check(ControlPoints.Num() >= 4);

	//clear out the old points if we have any
	ClearSpline();

	//add all the points in order
	for(int32 i = 0; i < ControlPoints.Num(); ++i)
	{
		AddControlPoint(ControlPoints[i]);
	}

	//setup the duration
	SetDuration(InDuration);
	BuildVelocitySpline();
}

void USpline::AddControlPoint(const FVector& Point, int32 Index)
{
	Spline.AddControlPoint(Point, Index);
	BuildVelocitySpline();
}

FVector USpline::GetPositionFromTime(float Time) const
{
	return Spline.GetPositionFromTime(GetVelocityCorrectedTime(Time));
}

bool USpline::IsValid() const
{
	return Spline.IsValid();
}

float USpline::GetDuration() const
{
	return Spline.GetDuration();
}

void USpline::ClearSpline()
{
	Spline.ClearSpline();
}

FVector USpline::GetStartPoint() const
{
	return Spline.GetStartPoint();
}

FVector USpline::GetEndPoint() const
{
	return Spline.GetEndPoint();
}

FVector USpline::GetTangentFromTime(float Time) const
{
	return Spline.GetTangentFromTime(GetVelocityCorrectedTime(Time));
}

FVector USpline::GetNormalFromTime(float Time) const
{
	return Spline.GetNormalFromTime(GetVelocityCorrectedTime(Time));
}

FVector USpline::GetBinormalFromTime(float Time) const
{
	return Spline.GetBinormalFromTime(GetVelocityCorrectedTime(Time));
}

float USpline::GetSplineLengthAtTime(float Time, float StepSize) const
{
	return Spline.GetSplineLengthAtTime(Time, StepSize);
}

float USpline::GetSplineLength(float StepSize) const
{
	return Spline.GetSplineLength(StepSize);
}

void USpline::DrawDebugSpline(UObject* WorldContextObject, int32 Steps, float LifeTime, float Thickness, bool bPersistent, bool bJustDrawSpline, float BasisLength)
{
	UWorld* World = NULL;
	if(WorldContextObject)
	{
		World = GEngine->GetWorldFromContextObject(WorldContextObject);
	}
	Spline.DrawDebugSpline(World, Steps, LifeTime, Thickness, bPersistent, bJustDrawSpline, BasisLength);
}

void USpline::BuildVelocitySpline()
{
	SplineLookupTable.Reset();
	if(Spline.IsValid() == false || bConstantVelocity == false)
	{
		return;
	}

	const float SplineLength = Spline.GetSplineLength();
	const float Steps = 10.0f * (Spline.GetNumOfControlPoints()-3);

	for( float i = 0.0f; i <= 1.0f; i += ( 1.0f / Steps ) )
	{
		float LengthAt = Spline.GetSplineLengthAtTime( i * Spline.GetDuration() ) / SplineLength;
		SplineLookupTable.AddPoint(LengthAt, i);
	}
}

float USpline::GetVelocityCorrectedTime(float Time) const
{
	if( bConstantVelocity && Spline.IsValid() )
	{
		Time /= Spline.GetDuration();

		Time = SplineLookupTable.Eval(Time, 0.0f);

		Time *= Spline.GetDuration();
	}
	return Time;
}

void USpline::EnableConstantVelocity(bool ConstVelEnabled)
{
	if(ConstVelEnabled != bConstantVelocity)
	{
		bConstantVelocity = ConstVelEnabled;
		BuildVelocitySpline();
	}
}

int32 USpline::GetNumberOfControlPoints() const
{
	return Spline.GetNumOfControlPoints();
}