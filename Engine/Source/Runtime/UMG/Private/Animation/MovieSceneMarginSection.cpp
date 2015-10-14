// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "MovieSceneMarginSection.h"
#include "MovieSceneMarginTrack.h"

UMovieSceneMarginSection::UMovieSceneMarginSection( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
}

void UMovieSceneMarginSection::MoveSection( float DeltaTime, TSet<FKeyHandle>& KeyHandles )
{
	Super::MoveSection( DeltaTime, KeyHandles );

	// Move all the curves in this section
	LeftCurve.ShiftCurve(DeltaTime, KeyHandles);
	TopCurve.ShiftCurve(DeltaTime, KeyHandles);
	RightCurve.ShiftCurve(DeltaTime, KeyHandles);
	BottomCurve.ShiftCurve(DeltaTime, KeyHandles);
}

void UMovieSceneMarginSection::DilateSection( float DilationFactor, float Origin, TSet<FKeyHandle>& KeyHandles )
{
	Super::DilateSection(DilationFactor, Origin, KeyHandles);

	LeftCurve.ScaleCurve(Origin, DilationFactor, KeyHandles);
	TopCurve.ScaleCurve(Origin, DilationFactor, KeyHandles);
	RightCurve.ScaleCurve(Origin, DilationFactor, KeyHandles);
	BottomCurve.ScaleCurve(Origin, DilationFactor, KeyHandles);
}

void UMovieSceneMarginSection::GetKeyHandles(TSet<FKeyHandle>& KeyHandles) const
{
	for (auto It(LeftCurve.GetKeyHandleIterator()); It; ++It)
	{
		float Time = LeftCurve.GetKeyTime(It.Key());
		if (IsTimeWithinSection(Time))
		{
			KeyHandles.Add(It.Key());
		}
	}
	for (auto It(TopCurve.GetKeyHandleIterator()); It; ++It)
	{
		float Time = TopCurve.GetKeyTime(It.Key());
		if (IsTimeWithinSection(Time))
		{
			KeyHandles.Add(It.Key());
		}
	}
	for (auto It(RightCurve.GetKeyHandleIterator()); It; ++It)
	{
		float Time = RightCurve.GetKeyTime(It.Key());
		if (IsTimeWithinSection(Time))
		{
			KeyHandles.Add(It.Key());
		}
	}
	for (auto It(BottomCurve.GetKeyHandleIterator()); It; ++It)
	{
		float Time = BottomCurve.GetKeyTime(It.Key());
		if (IsTimeWithinSection(Time))
		{
			KeyHandles.Add(It.Key());
		}
	}
}

FMargin UMovieSceneMarginSection::Eval( float Position, const FMargin& DefaultValue ) const
{
	return FMargin(	LeftCurve.Eval(Position, DefaultValue.Left),
					TopCurve.Eval(Position, DefaultValue.Top),
					RightCurve.Eval(Position, DefaultValue.Right),
					BottomCurve.Eval(Position, DefaultValue.Bottom));
}

void UMovieSceneMarginSection::AddKey( float Time, const FMarginKey& MarginKey, FKeyParams KeyParams )
{
	Modify();

	static FName LeftName("Left");
	static FName TopName("Top");
	static FName RightName("Right");
	static FName BottomName("Bottom");

	FName CurveName = MarginKey.CurveName;

	bool bTopKeyExists = TopCurve.IsKeyHandleValid(TopCurve.FindKey(Time));
	bool bLeftKeyExists = LeftCurve.IsKeyHandleValid(LeftCurve.FindKey(Time));
	bool bRightKeyExists = RightCurve.IsKeyHandleValid(RightCurve.FindKey(Time));
	bool bBottomKeyExists = BottomCurve.IsKeyHandleValid(BottomCurve.FindKey(Time));

	if ( (CurveName == NAME_None || CurveName == LeftName) &&
			(KeyParams.bAddKeyEvenIfUnchanged || !(!bLeftKeyExists && !KeyParams.bAutoKeying && LeftCurve.GetNumKeys() > 0) ) )
	{
		AddKeyToCurve(LeftCurve, Time, MarginKey.Value.Left, KeyParams);
	}

	if ( (CurveName == NAME_None || CurveName == TopName) &&
			(KeyParams.bAddKeyEvenIfUnchanged || !(!bTopKeyExists && !KeyParams.bAutoKeying && TopCurve.GetNumKeys() > 0) ) )
	{
		AddKeyToCurve(TopCurve, Time, MarginKey.Value.Top, KeyParams);
	}

	if ( (CurveName == NAME_None || CurveName == RightName) &&
			(KeyParams.bAddKeyEvenIfUnchanged || !(!bRightKeyExists && !KeyParams.bAutoKeying && RightCurve.GetNumKeys() > 0) ) )
	{
		AddKeyToCurve(RightCurve, Time, MarginKey.Value.Right, KeyParams);
	}

	if ( (CurveName == NAME_None || CurveName == BottomName) &&
			(KeyParams.bAddKeyEvenIfUnchanged || !(!bBottomKeyExists && !KeyParams.bAutoKeying && BottomCurve.GetNumKeys() > 0) ) )
	{
		AddKeyToCurve(BottomCurve, Time, MarginKey.Value.Bottom, KeyParams);
	}
}

bool UMovieSceneMarginSection::NewKeyIsNewData(float Time, const FMargin& Value, FKeyParams KeyParams) const
{
	bool bHasEmptyKeys = TopCurve.GetNumKeys() == 0 ||
		LeftCurve.GetNumKeys() == 0 ||
		RightCurve.GetNumKeys() == 0 ||
		BottomCurve.GetNumKeys() == 0 ||
		(Eval(Time,Value) != Value);

	if (bHasEmptyKeys || (KeyParams.bAutoKeying && Eval(Time, Value) != Value))
	{
		return true;
	}
	return false;
}

