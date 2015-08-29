// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneColorSection.h"
#include "MovieSceneColorTrack.h"


UMovieSceneColorSection::UMovieSceneColorSection( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
}

void UMovieSceneColorSection::MoveSection( float DeltaTime, TSet<FKeyHandle>& KeyHandles )
{
	Super::MoveSection( DeltaTime, KeyHandles );

	// Move all the curves in this section
	RedCurve.ShiftCurve(DeltaTime, KeyHandles);
	GreenCurve.ShiftCurve(DeltaTime, KeyHandles);
	BlueCurve.ShiftCurve(DeltaTime, KeyHandles);
	AlphaCurve.ShiftCurve(DeltaTime, KeyHandles);
}

void UMovieSceneColorSection::DilateSection( float DilationFactor, float Origin, TSet<FKeyHandle>& KeyHandles )
{
	Super::DilateSection(DilationFactor, Origin, KeyHandles);

	RedCurve.ScaleCurve(Origin, DilationFactor, KeyHandles);
	GreenCurve.ScaleCurve(Origin, DilationFactor, KeyHandles);
	BlueCurve.ScaleCurve(Origin, DilationFactor, KeyHandles);
	AlphaCurve.ScaleCurve(Origin, DilationFactor, KeyHandles);
}

void UMovieSceneColorSection::GetKeyHandles(TSet<FKeyHandle>& KeyHandles) const
{
	for (auto It(RedCurve.GetKeyHandleIterator()); It; ++It)
	{
		float Time = RedCurve.GetKeyTime(It.Key());
		if (IsTimeWithinSection(Time))
		{
			KeyHandles.Add(It.Key());
		}
	}
	for (auto It(GreenCurve.GetKeyHandleIterator()); It; ++It)
	{
		float Time = GreenCurve.GetKeyTime(It.Key());
		if (IsTimeWithinSection(Time))
		{
			KeyHandles.Add(It.Key());
		}
	}
	for (auto It(BlueCurve.GetKeyHandleIterator()); It; ++It)
	{
		float Time = BlueCurve.GetKeyTime(It.Key());
		if (IsTimeWithinSection(Time))
		{
			KeyHandles.Add(It.Key());
		}
	}
	for (auto It(AlphaCurve.GetKeyHandleIterator()); It; ++It)
	{
		float Time = AlphaCurve.GetKeyTime(It.Key());
		if (IsTimeWithinSection(Time))
			KeyHandles.Add(It.Key());
	}
}

FLinearColor UMovieSceneColorSection::Eval( float Position, const FLinearColor& DefaultColor ) const
{
	return FLinearColor(RedCurve.Eval(Position, DefaultColor.R),
						GreenCurve.Eval(Position, DefaultColor.G),
						BlueCurve.Eval(Position, DefaultColor.B),
						AlphaCurve.Eval(Position, DefaultColor.A));
}

void UMovieSceneColorSection::AddKey( float Time, const FColorKey& Key )
{
	Modify();

	if( Key.CurveName == NAME_None )
	{
		AddKeyToCurve(RedCurve, Time, Key.Value.R);
		AddKeyToCurve(GreenCurve, Time, Key.Value.G);
		AddKeyToCurve(BlueCurve, Time, Key.Value.B);
		AddKeyToCurve(AlphaCurve, Time, Key.Value.A);
	}
	else
	{
		AddKeyToNamedCurve( Time, Key );
	}
}

bool UMovieSceneColorSection::NewKeyIsNewData(float Time, FLinearColor Value) const
{
	return RedCurve.GetNumKeys() == 0 ||
		GreenCurve.GetNumKeys() == 0 ||
		BlueCurve.GetNumKeys() == 0 ||
		AlphaCurve.GetNumKeys() == 0 ||
		!Eval(Time,Value).Equals(Value);
}

void UMovieSceneColorSection::AddKeyToNamedCurve(float Time, const FColorKey& Key)
{
	static FName R("R");
	static FName G("G");
	static FName B("B");
	static FName A("A");

	FName CurveName = Key.CurveName;
	if (CurveName == R)
	{
		AddKeyToCurve(RedCurve, Time, Key.Value.R);
	}
	else if (CurveName == G)
	{
		AddKeyToCurve(GreenCurve, Time, Key.Value.G);
	}
	else if (CurveName == B)
	{
		AddKeyToCurve(BlueCurve, Time, Key.Value.B);
	}
	else if (CurveName == A)
	{
		AddKeyToCurve(AlphaCurve, Time, Key.Value.A);
	}
}
