// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "MovieSceneMarginSection.h"


UMovieSceneMarginSection::UMovieSceneMarginSection( const FPostConstructInitializeProperties& PCIP )
	: Super( PCIP )
{
}

void UMovieSceneMarginSection::MoveSection( float DeltaTime )
{
	Super::MoveSection( DeltaTime );

	// Move all the curves in this section
	TopCurve.ShiftCurve(DeltaTime);
	LeftCurve.ShiftCurve(DeltaTime);
	RightCurve.ShiftCurve(DeltaTime);
	BottomCurve.ShiftCurve(DeltaTime);
}

void UMovieSceneMarginSection::DilateSection( float DilationFactor, float Origin )
{
	TopCurve.ScaleCurve(Origin, DilationFactor);
	LeftCurve.ScaleCurve(Origin, DilationFactor);
	RightCurve.ScaleCurve(Origin, DilationFactor);
	BottomCurve.ScaleCurve(Origin, DilationFactor);

	Super::DilateSection(DilationFactor, Origin);
}

FMargin UMovieSceneMarginSection::Eval( float Position ) const
{
	return FMargin(	TopCurve.Eval(Position),
					LeftCurve.Eval(Position),
					RightCurve.Eval(Position),
					BottomCurve.Eval(Position));
}

void UMovieSceneMarginSection::AddKey( float Time, const FMargin& Value )
{
	AddKeyToCurve(TopCurve, Time, Value.Top);
	AddKeyToCurve(LeftCurve, Time, Value.Left);
	AddKeyToCurve(RightCurve, Time, Value.Right);
	AddKeyToCurve(BottomCurve, Time, Value.Bottom);
}

bool UMovieSceneMarginSection::NewKeyIsNewData(float Time, const FMargin& Value) const
{
	return TopCurve.GetNumKeys() == 0 ||
		LeftCurve.GetNumKeys() == 0 ||
		RightCurve.GetNumKeys() == 0 ||
		BottomCurve.GetNumKeys() == 0 ||
		(Eval(Time) != Value);
}



void UMovieSceneMarginSection::AddKeyToCurve( FRichCurve& InCurve, float Time, float Value )
{
	if( IsTimeWithinSection(Time) )
	{
		InCurve.UpdateOrAddKey(Time, Value);
	}
}
