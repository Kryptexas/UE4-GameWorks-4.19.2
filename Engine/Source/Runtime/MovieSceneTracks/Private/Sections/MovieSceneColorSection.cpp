// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneColorSection.h"
#include "MovieSceneColorTrack.h"


UMovieSceneColorSection::UMovieSceneColorSection( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{ }


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


void UMovieSceneColorSection::AddKey( float Time, const FColorKey& Key, FKeyParams KeyParams )
{
	Modify();

	static FName RedName("R");
	static FName GreenName("G");
	static FName BlueName("B");
	static FName AlphaName("A");

	FName CurveName = Key.CurveName;

	bool bRedKeyExists = RedCurve.IsKeyHandleValid(RedCurve.FindKey(Time));
	bool bGreenKeyExists = GreenCurve.IsKeyHandleValid(GreenCurve.FindKey(Time));
	bool bBlueKeyExists = BlueCurve.IsKeyHandleValid(BlueCurve.FindKey(Time));
	bool bAlphaKeyExists = AlphaCurve.IsKeyHandleValid(AlphaCurve.FindKey(Time));

	if ( (CurveName == NAME_None || CurveName == RedName) &&
			(KeyParams.bAddKeyEvenIfUnchanged || !(!bRedKeyExists && !KeyParams.bAutoKeying && RedCurve.GetNumKeys() > 0) ) )
	{
		AddKeyToCurve(RedCurve, Time, Key.Value.R, KeyParams);
	}

	if ( (CurveName == NAME_None || CurveName == GreenName) &&
			(KeyParams.bAddKeyEvenIfUnchanged || !(!bGreenKeyExists && !KeyParams.bAutoKeying && GreenCurve.GetNumKeys() > 0) ) )
	{
		AddKeyToCurve(GreenCurve, Time, Key.Value.G, KeyParams);
	}

	if ( (CurveName == NAME_None || CurveName == BlueName) &&
			(KeyParams.bAddKeyEvenIfUnchanged || !(!bBlueKeyExists && !KeyParams.bAutoKeying && BlueCurve.GetNumKeys() > 0) ) )
	{
		AddKeyToCurve(BlueCurve, Time, Key.Value.B, KeyParams);
	}

	if ( (CurveName == NAME_None || CurveName == AlphaName) &&
			(KeyParams.bAddKeyEvenIfUnchanged || !(!bAlphaKeyExists && !KeyParams.bAutoKeying && AlphaCurve.GetNumKeys() > 0) ) )
	{
		AddKeyToCurve(AlphaCurve, Time, Key.Value.A, KeyParams);
	}
}


bool UMovieSceneColorSection::NewKeyIsNewData(float Time, FLinearColor Value, FKeyParams KeyParams) const
{
	bool bHasEmptyKeys = 
		RedCurve.GetNumKeys() == 0 ||
		GreenCurve.GetNumKeys() == 0 ||
		BlueCurve.GetNumKeys() == 0 ||
		AlphaCurve.GetNumKeys() == 0 ||
		!Eval(Time,Value).Equals(Value);

	if (bHasEmptyKeys)
	{
		// Don't add a keyframe if there are existing keys and auto key is not enabled.
		bool bRedKeyExists = RedCurve.IsKeyHandleValid(RedCurve.FindKey(Time));
		bool bGreenKeyExists = GreenCurve.IsKeyHandleValid(GreenCurve.FindKey(Time));
		bool bBlueKeyExists = BlueCurve.IsKeyHandleValid(BlueCurve.FindKey(Time));
		bool bAlphaKeyExists = AlphaCurve.IsKeyHandleValid(AlphaCurve.FindKey(Time));

		if ( !(!bRedKeyExists && !KeyParams.bAutoKeying && RedCurve.GetNumKeys() > 0) ||
			 !(!bGreenKeyExists && !KeyParams.bAutoKeying && GreenCurve.GetNumKeys() > 0) ||
			 !(!bBlueKeyExists && !KeyParams.bAutoKeying && BlueCurve.GetNumKeys() > 0) ||
			 !(!bAlphaKeyExists && !KeyParams.bAutoKeying && AlphaCurve.GetNumKeys() > 0) )
		{
			return true;
		}
	}

	return false;
}
