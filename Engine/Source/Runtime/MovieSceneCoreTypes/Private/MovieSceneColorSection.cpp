// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCoreTypesPCH.h"
#include "MovieSceneColorSection.h"


UMovieSceneColorSection::UMovieSceneColorSection( const FPostConstructInitializeProperties& PCIP )
	: Super( PCIP )
{
}

void UMovieSceneColorSection::MoveSection( float DeltaTime )
{
	Super::MoveSection( DeltaTime );

	// Move all the curves in this section
	RedCurve.ShiftCurve(DeltaTime);
	GreenCurve.ShiftCurve(DeltaTime);
	BlueCurve.ShiftCurve(DeltaTime);
	AlphaCurve.ShiftCurve(DeltaTime);
}

void UMovieSceneColorSection::DilateSection( float DilationFactor, float Origin )
{
	RedCurve.ScaleCurve(Origin, DilationFactor);
	GreenCurve.ScaleCurve(Origin, DilationFactor);
	BlueCurve.ScaleCurve(Origin, DilationFactor);
	AlphaCurve.ScaleCurve(Origin, DilationFactor);

	Super::DilateSection(DilationFactor, Origin);
}

FLinearColor UMovieSceneColorSection::Eval( float Position ) const
{
	return FLinearColor(RedCurve.Eval(Position),
						GreenCurve.Eval(Position),
						BlueCurve.Eval(Position),
						AlphaCurve.Eval(Position));
}

void UMovieSceneColorSection::AddKey( float Time, FLinearColor Value )
{
	AddKeyToCurve(RedCurve, Time, Value.R);
	AddKeyToCurve(GreenCurve, Time, Value.G);
	AddKeyToCurve(BlueCurve, Time, Value.B);
	AddKeyToCurve(AlphaCurve, Time, Value.A);
}

bool UMovieSceneColorSection::NewKeyIsNewData(float Time, FLinearColor Value) const
{
	return RedCurve.GetNumKeys() == 0 ||
		GreenCurve.GetNumKeys() == 0 ||
		BlueCurve.GetNumKeys() == 0 ||
		AlphaCurve.GetNumKeys() == 0 ||
		!Eval(Time).Equals(Value);
}



void UMovieSceneColorSection::AddKeyToCurve( FRichCurve& InCurve, float Time, float Value )
{
	if( IsTimeWithinSection(Time) )
	{
		InCurve.UpdateOrAddKey(Time, Value);
	}
}
