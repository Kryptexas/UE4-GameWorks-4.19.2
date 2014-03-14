// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCoreTypesPCH.h"
#include "MovieSceneFloatSection.h"


UMovieSceneFloatSection::UMovieSceneFloatSection( const FPostConstructInitializeProperties& PCIP )
	: Super( PCIP )
{
}

float UMovieSceneFloatSection::Eval( float Position ) const
{
	return FloatCurve.Eval( Position );
}

void UMovieSceneFloatSection::MoveSection( float DeltaPosition )
{
	Super::MoveSection( DeltaPosition );

	// Move the curve
	FloatCurve.ShiftCurve(DeltaPosition );
}

void UMovieSceneFloatSection::DilateSection( float DilationFactor, float Origin )
{
	FloatCurve.ScaleCurve(Origin, DilationFactor);

	Super::DilateSection(DilationFactor, Origin);
}

void UMovieSceneFloatSection::AddKey( float Time, float Value )
{
	FloatCurve.UpdateOrAddKey(Time, Value);
}

bool UMovieSceneFloatSection::NewKeyIsNewData(float Time, float Value) const
{
	return FloatCurve.GetNumKeys() == 0 || !FMath::IsNearlyEqual(FloatCurve.Eval(Time), Value);
}
