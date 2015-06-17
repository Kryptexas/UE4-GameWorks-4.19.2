// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCoreTypesPCH.h"
#include "MovieSceneFloatSection.h"


UMovieSceneFloatSection::UMovieSceneFloatSection( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
}

float UMovieSceneFloatSection::Eval( float Position ) const
{
	return FloatCurve.Eval( Position );
}

void UMovieSceneFloatSection::MoveSection( float DeltaPosition, TSet<FKeyHandle>& KeyHandles )
{
	Super::MoveSection( DeltaPosition, KeyHandles );

	// Move the curve
	FloatCurve.ShiftCurve(DeltaPosition, KeyHandles);
}

void UMovieSceneFloatSection::DilateSection( float DilationFactor, float Origin, TSet<FKeyHandle>& KeyHandles )
{	
	Super::DilateSection(DilationFactor, Origin, KeyHandles);
	
	FloatCurve.ScaleCurve(Origin, DilationFactor, KeyHandles);
}

void UMovieSceneFloatSection::GetKeyHandles(TSet<FKeyHandle>& KeyHandles) const
{
	for (auto It(FloatCurve.GetKeyHandleIterator()); It; ++It)
	{
		float Time = FloatCurve.GetKeyTime(It.Key());
		if (IsTimeWithinSection(Time))
		{
			KeyHandles.Add(It.Key());
		}
	}
}

void UMovieSceneFloatSection::AddKey( float Time, float Value )
{
	Modify();
	FloatCurve.UpdateOrAddKey(Time, Value);
}

bool UMovieSceneFloatSection::NewKeyIsNewData(float Time, float Value) const
{
	return FloatCurve.GetNumKeys() == 0 || !FMath::IsNearlyEqual(FloatCurve.Eval(Time), Value);
}
