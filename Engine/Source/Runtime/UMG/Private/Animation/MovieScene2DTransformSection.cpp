// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "MovieScene2DTransformSection.h"
#include "MovieScene2DTransformTrack.h"

UMovieScene2DTransformSection::UMovieScene2DTransformSection( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{

}

void UMovieScene2DTransformSection::MoveSection( float DeltaTime, TSet<FKeyHandle>& KeyHandles )
{
	Super::MoveSection( DeltaTime, KeyHandles );

	Rotation.ShiftCurve( DeltaTime, KeyHandles );

	// Move all the curves in this section
	for( int32 Axis = 0; Axis < 2; ++Axis )
	{
		Translation[Axis].ShiftCurve( DeltaTime, KeyHandles );
		Scale[Axis].ShiftCurve( DeltaTime, KeyHandles );
		Shear[Axis].ShiftCurve( DeltaTime, KeyHandles );
	}
}

void UMovieScene2DTransformSection::DilateSection( float DilationFactor, float Origin, TSet<FKeyHandle>& KeyHandles )
{
	Super::DilateSection(DilationFactor, Origin, KeyHandles);

	Rotation.ScaleCurve( Origin, DilationFactor, KeyHandles);

	for( int32 Axis = 0; Axis < 2; ++Axis )
	{
		Translation[Axis].ScaleCurve( Origin, DilationFactor, KeyHandles );
		Scale[Axis].ScaleCurve( Origin, DilationFactor, KeyHandles );
		Shear[Axis].ScaleCurve( Origin, DilationFactor, KeyHandles );
	}
}

void UMovieScene2DTransformSection::GetKeyHandles(TSet<FKeyHandle>& KeyHandles) const
{
	for (auto It(Rotation.GetKeyHandleIterator()); It; ++It)
	{
		float Time = Rotation.GetKeyTime(It.Key());
		if (IsTimeWithinSection(Time))
		{
			KeyHandles.Add(It.Key());
		}
	}

	for (int32 Axis = 0; Axis < 2; ++Axis)
	{
		for (auto It(Translation[Axis].GetKeyHandleIterator()); It; ++It)
		{
			float Time = Translation[Axis].GetKeyTime(It.Key());
			if (IsTimeWithinSection(Time))
			{
				KeyHandles.Add(It.Key());
			}
		}
		for (auto It(Scale[Axis].GetKeyHandleIterator()); It; ++It)
		{
			float Time = Scale[Axis].GetKeyTime(It.Key());
			if (IsTimeWithinSection(Time))
			{
				KeyHandles.Add(It.Key());
			}
		}
		for (auto It(Shear[Axis].GetKeyHandleIterator()); It; ++It)
		{
			float Time = Shear[Axis].GetKeyTime(It.Key());
			if (IsTimeWithinSection(Time))
			{
				KeyHandles.Add(It.Key());
			}
		}
	}
}

/**
 * Chooses an appropriate curve from an axis and a set of curves
 */
static FRichCurve* ChooseCurve( EAxis::Type Axis, FRichCurve* Curves )
{
	switch( Axis )
	{
	case EAxis::X:
		return &Curves[0];
		break;
	case EAxis::Y:
		return &Curves[1];
		break;
	default:
		check( false );
		return NULL;
		break;
	}
}

FRichCurve& UMovieScene2DTransformSection::GetTranslationCurve(EAxis::Type Axis)
{
	return *ChooseCurve( Axis, Translation );
}

FRichCurve& UMovieScene2DTransformSection::GetRotationCurve()
{
	return Rotation;
}

FRichCurve& UMovieScene2DTransformSection::GetScaleCurve(EAxis::Type Axis)
{
	return *ChooseCurve(Axis, Scale);
}

FRichCurve& UMovieScene2DTransformSection::GetSheerCurve(EAxis::Type Axis)
{
	return *ChooseCurve(Axis, Shear);
}

FWidgetTransform UMovieScene2DTransformSection::Eval( float Position, const FWidgetTransform& DefaultValue ) const
{
	return FWidgetTransform(	FVector2D( Translation[0].Eval( Position, DefaultValue.Translation.X ), Translation[1].Eval( Position, DefaultValue.Translation.Y ) ),
								FVector2D( Scale[0].Eval( Position, DefaultValue.Scale.X ),	Scale[1].Eval( Position, DefaultValue.Scale.Y ) ),
								FVector2D( Shear[0].Eval( Position, DefaultValue.Shear.X ),	Shear[1].Eval( Position, DefaultValue.Shear.Y ) ),
								Rotation.Eval( Position, DefaultValue.Angle ) );
}


bool UMovieScene2DTransformSection::NewKeyIsNewData( float Time, const FWidgetTransform& Transform, FKeyParams KeyParams ) const
{
	bool bHasEmptyKeys = false;
	for(int32 Axis = 0; Axis < 2; ++Axis)
	{
		bHasEmptyKeys = bHasEmptyKeys ||
			Translation[Axis].GetNumKeys() == 0 ||
			Scale[Axis].GetNumKeys() == 0 ||
			Shear[Axis].GetNumKeys() == 0;
	}

	bHasEmptyKeys |= Rotation.GetNumKeys() == 0;

	if (bHasEmptyKeys || (KeyParams.bAutoKeying && Eval(Time, Transform) != Transform))
	{
		return true;
	}

	return false;
}

void UMovieScene2DTransformSection::AddKey( float Time, const struct F2DTransformKey& TransformKey, FKeyParams KeyParams )
{
	Modify();

	static FName TranslationName("Translation");
	static FName ScaleName("Scale");
	static FName ShearName("Shear");
	static FName AngleName("Angle");

	FName CurveName = TransformKey.CurveName;

	bool bTxKeyExists = Translation[0].IsKeyHandleValid(Translation[0].FindKey(Time));
	bool bTyKeyExists = Translation[1].IsKeyHandleValid(Translation[1].FindKey(Time));

	bool bRKeyExists = Rotation.IsKeyHandleValid(Rotation.FindKey(Time));

	bool bHxKeyExists = Shear[0].IsKeyHandleValid(Shear[0].FindKey(Time));
	bool bHyKeyExists = Shear[1].IsKeyHandleValid(Shear[1].FindKey(Time));

	bool bSxKeyExists = Scale[0].IsKeyHandleValid(Scale[0].FindKey(Time));
	bool bSyKeyExists = Scale[1].IsKeyHandleValid(Scale[1].FindKey(Time));

	if ( (CurveName == NAME_None || CurveName == TranslationName) &&
			(KeyParams.bAddKeyEvenIfUnchanged || !(!bTxKeyExists && !KeyParams.bAutoKeying && Translation[0].GetNumKeys() > 0) ) )
	{
		AddKeyToCurve(Translation[0], Time, TransformKey.Value.Translation.X, KeyParams);
	}

	if ( (CurveName == NAME_None || CurveName == TranslationName) &&
			(KeyParams.bAddKeyEvenIfUnchanged || !(!bTyKeyExists && !KeyParams.bAutoKeying && Translation[1].GetNumKeys() > 0) ) )
	{
		AddKeyToCurve(Translation[1], Time, TransformKey.Value.Translation.Y, KeyParams);
	}

	if ( (CurveName == NAME_None || CurveName == ScaleName) &&
			(KeyParams.bAddKeyEvenIfUnchanged || !(!bSxKeyExists && !KeyParams.bAutoKeying && Scale[0].GetNumKeys() > 0) ) )
	{
		AddKeyToCurve(Scale[0], Time, TransformKey.Value.Scale.X, KeyParams);
	}

	if ( (CurveName == NAME_None || CurveName == ScaleName) &&
			(KeyParams.bAddKeyEvenIfUnchanged || !(!bSyKeyExists && !KeyParams.bAutoKeying && Scale[1].GetNumKeys() > 0) ) )
	{
		AddKeyToCurve(Scale[1], Time, TransformKey.Value.Scale.Y, KeyParams);
	}
		
	if ( (CurveName == NAME_None || CurveName == ShearName) &&
			(KeyParams.bAddKeyEvenIfUnchanged || !(!bHxKeyExists && !KeyParams.bAutoKeying && Shear[0].GetNumKeys() > 0) ) )
	{
		AddKeyToCurve(Shear[0], Time, TransformKey.Value.Shear.X, KeyParams);
	}

	if ( (CurveName == NAME_None || CurveName == ShearName) &&
			(KeyParams.bAddKeyEvenIfUnchanged || !(!bHyKeyExists && !KeyParams.bAutoKeying && Shear[1].GetNumKeys() > 0) ) )
	{
		AddKeyToCurve(Shear[1], Time, TransformKey.Value.Shear.Y, KeyParams);
	}

	if ( (CurveName == NAME_None || CurveName == AngleName) &&
			(KeyParams.bAddKeyEvenIfUnchanged || !(!bRKeyExists && !KeyParams.bAutoKeying && Rotation.GetNumKeys() > 0) ) )
	{
		AddKeyToCurve(Rotation, Time, TransformKey.Value.Angle, KeyParams);
	}
}
