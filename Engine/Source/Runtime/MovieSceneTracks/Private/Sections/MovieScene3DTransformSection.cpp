// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieScene3DTransformSection.h"
#include "MovieScene3DTransformTrack.h"

UMovieScene3DTransformSection::UMovieScene3DTransformSection( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{

}

void UMovieScene3DTransformSection::MoveSection( float DeltaTime, TSet<FKeyHandle>& KeyHandles )
{
	Super::MoveSection( DeltaTime, KeyHandles );

	// Move all the curves in this section
	for( int32 Axis = 0; Axis < 3; ++Axis )
	{
		Translation[Axis].ShiftCurve( DeltaTime, KeyHandles );
		Rotation[Axis].ShiftCurve( DeltaTime, KeyHandles );
		Scale[Axis].ShiftCurve( DeltaTime, KeyHandles );
	}
}

void UMovieScene3DTransformSection::DilateSection( float DilationFactor, float Origin, TSet<FKeyHandle>& KeyHandles )
{
	Super::DilateSection(DilationFactor, Origin, KeyHandles);

	for( int32 Axis = 0; Axis < 3; ++Axis )
	{
		Translation[Axis].ScaleCurve( Origin, DilationFactor, KeyHandles );
		Rotation[Axis].ScaleCurve( Origin, DilationFactor, KeyHandles );
		Scale[Axis].ScaleCurve( Origin, DilationFactor, KeyHandles );
	}
}

void UMovieScene3DTransformSection::GetKeyHandles(TSet<FKeyHandle>& KeyHandles) const
{
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		for (auto It(Translation[Axis].GetKeyHandleIterator()); It; ++It)
		{
			float Time = Translation[Axis].GetKeyTime(It.Key());
			if (IsTimeWithinSection(Time))
			{
				KeyHandles.Add(It.Key());
			}
		}
		for (auto It(Rotation[Axis].GetKeyHandleIterator()); It; ++It)
		{
			float Time = Rotation[Axis].GetKeyTime(It.Key());
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
	}
}	

void UMovieScene3DTransformSection::EvalTranslation( float Time, FVector& OutTranslation ) const
{
	OutTranslation.X = Translation[0].Eval( Time );
	OutTranslation.Y = Translation[1].Eval( Time );
	OutTranslation.Z = Translation[2].Eval( Time );
}

void UMovieScene3DTransformSection::EvalRotation( float Time, FRotator& OutRotation ) const
{
	OutRotation.Roll = Rotation[0].Eval( Time );
	OutRotation.Pitch = Rotation[1].Eval( Time );
	OutRotation.Yaw = Rotation[2].Eval( Time );
}

void UMovieScene3DTransformSection::EvalScale( float Time, FVector& OutScale ) const
{
	OutScale.X = Scale[0].Eval( Time );
	OutScale.Y = Scale[1].Eval( Time );
	OutScale.Z = Scale[2].Eval( Time );
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
	case EAxis::Z:
		return &Curves[2];
		break;
	default:
		check( false );
		return NULL;
		break;
	}
}

FRichCurve& UMovieScene3DTransformSection::GetTranslationCurve( EAxis::Type Axis ) 
{
	return *ChooseCurve( Axis, Translation );
}

FRichCurve& UMovieScene3DTransformSection::GetRotationCurve( EAxis::Type Axis )
{
	return *ChooseCurve( Axis, Rotation );
}

FRichCurve& UMovieScene3DTransformSection::GetScaleCurve( EAxis::Type Axis )
{
	return *ChooseCurve( Axis, Scale );
}

void UMovieScene3DTransformSection::AddTranslationKeys( const FTransformKey& TransformKey )
{
	const float Time = TransformKey.GetKeyTime();

	bool bTxKeyExists = Translation[0].IsKeyHandleValid(Translation[0].FindKey(Time));
	bool bTyKeyExists = Translation[1].IsKeyHandleValid(Translation[1].FindKey(Time));
	bool bTzKeyExists = Translation[2].IsKeyHandleValid(Translation[2].FindKey(Time));

	// Key each component. Don't add a keyframe if there are existing keys and auto key is not enabled.
	if( TransformKey.KeyParams.bAddKeyEvenIfUnchanged || (TransformKey.ShouldKeyTranslation( EAxis::X ) && !(!bTxKeyExists && !TransformKey.KeyParams.bAutoKeying && Translation[0].GetNumKeys() > 0)) )
	{
		AddKeyToCurve( Translation[0], Time, TransformKey.GetTranslationValue().X, TransformKey.KeyParams );
	}

	if( TransformKey.KeyParams.bAddKeyEvenIfUnchanged || (TransformKey.ShouldKeyTranslation( EAxis::Y ) && !(!bTyKeyExists && !TransformKey.KeyParams.bAutoKeying && Translation[1].GetNumKeys() > 0)) )
	{ 
		AddKeyToCurve( Translation[1], Time, TransformKey.GetTranslationValue().Y, TransformKey.KeyParams );
	}

	if( TransformKey.KeyParams.bAddKeyEvenIfUnchanged || (TransformKey.ShouldKeyTranslation( EAxis::Z ) && !(!bTzKeyExists && !TransformKey.KeyParams.bAutoKeying && Translation[2].GetNumKeys() > 0)) )
	{ 
		AddKeyToCurve( Translation[2], Time, TransformKey.GetTranslationValue().Z, TransformKey.KeyParams );
	}
}

void UMovieScene3DTransformSection::AddRotationKeys( const FTransformKey& TransformKey, const bool bUnwindRotation )
{
	const float Time = TransformKey.GetKeyTime();

	bool bRxKeyExists = Rotation[0].IsKeyHandleValid(Rotation[0].FindKey(Time));
	bool bRyKeyExists = Rotation[1].IsKeyHandleValid(Rotation[1].FindKey(Time));
	bool bRzKeyExists = Rotation[2].IsKeyHandleValid(Rotation[2].FindKey(Time));

	// Key each component. Don't add a keyframe if there are existing keys and auto key is not enabled.
	if( TransformKey.KeyParams.bAddKeyEvenIfUnchanged || (TransformKey.ShouldKeyRotation( EAxis::X ) && !(!bRxKeyExists && !TransformKey.KeyParams.bAutoKeying && Rotation[0].GetNumKeys() > 0)) )
	{
		AddKeyToCurve( Rotation[0], Time, TransformKey.GetRotationValue().Roll, TransformKey.KeyParams, bUnwindRotation );
	}

	if( TransformKey.KeyParams.bAddKeyEvenIfUnchanged || (TransformKey.ShouldKeyRotation( EAxis::Y ) && !(!bRyKeyExists && !TransformKey.KeyParams.bAutoKeying && Rotation[1].GetNumKeys() > 0)) )
	{ 
		AddKeyToCurve( Rotation[1], Time, TransformKey.GetRotationValue().Pitch, TransformKey.KeyParams, bUnwindRotation );
	}

	if( TransformKey.KeyParams.bAddKeyEvenIfUnchanged || (TransformKey.ShouldKeyRotation( EAxis::Z ) && !(!bRzKeyExists && !TransformKey.KeyParams.bAutoKeying && Rotation[2].GetNumKeys() > 0)) )
	{ 
		AddKeyToCurve( Rotation[2], Time, TransformKey.GetRotationValue().Yaw, TransformKey.KeyParams, bUnwindRotation );
	}
}

void UMovieScene3DTransformSection::AddScaleKeys( const FTransformKey& TransformKey )
{
	const float Time = TransformKey.GetKeyTime();

	bool bSxKeyExists = Scale[0].IsKeyHandleValid(Scale[0].FindKey(Time));
	bool bSyKeyExists = Scale[1].IsKeyHandleValid(Scale[1].FindKey(Time));
	bool bSzKeyExists = Scale[2].IsKeyHandleValid(Scale[2].FindKey(Time));

	// Key each component. Don't add a keyframe if there are existing keys and auto key is not enabled.
	if( TransformKey.KeyParams.bAddKeyEvenIfUnchanged || (TransformKey.ShouldKeyScale( EAxis::X ) && !(!bSxKeyExists && !TransformKey.KeyParams.bAutoKeying && Scale[0].GetNumKeys() > 0)) )
	{
		AddKeyToCurve( Scale[0], Time, TransformKey.GetScaleValue().X, TransformKey.KeyParams );
	}

	if( TransformKey.KeyParams.bAddKeyEvenIfUnchanged || (TransformKey.ShouldKeyScale( EAxis::Y ) && !(!bSyKeyExists && !TransformKey.KeyParams.bAutoKeying && Scale[1].GetNumKeys() > 0)) )
	{ 
		AddKeyToCurve( Scale[1], Time, TransformKey.GetScaleValue().Y, TransformKey.KeyParams );
	}

	if( TransformKey.KeyParams.bAddKeyEvenIfUnchanged || (TransformKey.ShouldKeyScale( EAxis::Z ) && !(!bSzKeyExists && !TransformKey.KeyParams.bAutoKeying && Scale[2].GetNumKeys() > 0)) )
	{ 
		AddKeyToCurve( Scale[2], Time, TransformKey.GetScaleValue().Z, TransformKey.KeyParams );
	}
}

bool UMovieScene3DTransformSection::NewKeyIsNewData(const FTransformKey& TransformKey) const
{
	const float KeyTime = TransformKey.GetKeyTime();

	bool bHasEmptyKeys = false;
	for (int32 i = 0; i < 3; ++i)
	{
		bHasEmptyKeys = bHasEmptyKeys ||
			Translation[i].GetNumKeys() == 0 ||
			Rotation[i].GetNumKeys() == 0 ||
			Scale[i].GetNumKeys() == 0;
	}

	if (bHasEmptyKeys || TransformKey.ShouldKeyAny())
	{
		// Don't add a keyframe if there are existing keys and auto key is not enabled.
		bool bTxKeyExists = Translation[0].IsKeyHandleValid(Translation[0].FindKey(KeyTime));
		bool bTyKeyExists = Translation[1].IsKeyHandleValid(Translation[1].FindKey(KeyTime));
		bool bTzKeyExists = Translation[2].IsKeyHandleValid(Translation[2].FindKey(KeyTime));

		bool bRxKeyExists = Rotation[0].IsKeyHandleValid(Rotation[0].FindKey(KeyTime));
		bool bRyKeyExists = Rotation[1].IsKeyHandleValid(Rotation[1].FindKey(KeyTime));
		bool bRzKeyExists = Rotation[2].IsKeyHandleValid(Rotation[2].FindKey(KeyTime));

		bool bSxKeyExists = Scale[0].IsKeyHandleValid(Scale[0].FindKey(KeyTime));
		bool bSyKeyExists = Scale[1].IsKeyHandleValid(Scale[1].FindKey(KeyTime));
		bool bSzKeyExists = Scale[2].IsKeyHandleValid(Scale[2].FindKey(KeyTime));

		if ( !(!bTxKeyExists && !TransformKey.KeyParams.bAutoKeying && Translation[0].GetNumKeys() > 0) ||
			 !(!bTyKeyExists && !TransformKey.KeyParams.bAutoKeying && Translation[1].GetNumKeys() > 0) ||
			 !(!bTzKeyExists && !TransformKey.KeyParams.bAutoKeying && Translation[2].GetNumKeys() > 0) ||
			 !(!bRxKeyExists && !TransformKey.KeyParams.bAutoKeying && Rotation[0].GetNumKeys() > 0) ||
			 !(!bRyKeyExists && !TransformKey.KeyParams.bAutoKeying && Rotation[1].GetNumKeys() > 0) ||
			 !(!bRzKeyExists && !TransformKey.KeyParams.bAutoKeying && Rotation[2].GetNumKeys() > 0) ||
			 !(!bSxKeyExists && !TransformKey.KeyParams.bAutoKeying && Scale[0].GetNumKeys() > 0) ||
			 !(!bSyKeyExists && !TransformKey.KeyParams.bAutoKeying && Scale[1].GetNumKeys() > 0) ||
			 !(!bSzKeyExists && !TransformKey.KeyParams.bAutoKeying && Scale[2].GetNumKeys() > 0) )
		{
			return true;
		}
	}

	return false;
}
