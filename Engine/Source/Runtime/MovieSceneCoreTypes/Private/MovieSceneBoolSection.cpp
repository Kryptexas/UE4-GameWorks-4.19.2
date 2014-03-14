// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCoreTypesPCH.h"
#include "MovieSceneBoolSection.h"


UMovieSceneBoolSection::UMovieSceneBoolSection( const FPostConstructInitializeProperties& PCIP )
	: Super( PCIP )
{
}

bool UMovieSceneBoolSection::Eval( float Position ) const
{
	return !!BoolCurve.Evaluate(Position);
}

void UMovieSceneBoolSection::MoveSection( float DeltaPosition )
{
	Super::MoveSection( DeltaPosition );

	BoolCurve.ShiftCurve(DeltaPosition);
}

void UMovieSceneBoolSection::DilateSection( float DilationFactor, float Origin )
{
	BoolCurve.ScaleCurve(Origin, DilationFactor);

	Super::DilateSection(DilationFactor, Origin);
}

void UMovieSceneBoolSection::AddKey( float Time, bool Value )
{
	BoolCurve.UpdateOrAddKey(Time, Value ? 1 : 0);
}

bool UMovieSceneBoolSection::NewKeyIsNewData(float Time, bool Value) const
{
	return BoolCurve.GetNumKeys() == 0 || Eval(Time) != Value;
}
