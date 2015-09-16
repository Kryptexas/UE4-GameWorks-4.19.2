// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieScene3DTransformSection.h"
#include "MovieScene3DTransformTrack.h"


UMovieScene3DTransformSection::UMovieScene3DTransformSection( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{ }


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

	if( TransformKey.KeyParams.bAddKeyEvenIfUnchanged || TransformKey.ShouldKeyTranslation( EAxis::X ) || Translation[0].GetNumKeys() == 0 )
	{
		AddKeyToCurve( Translation[0], Time, TransformKey.GetTranslationValue().X, TransformKey.KeyParams );
	}

	if( TransformKey.KeyParams.bAddKeyEvenIfUnchanged || TransformKey.ShouldKeyTranslation( EAxis::Y ) || Translation[1].GetNumKeys() == 0 )
	{ 
		AddKeyToCurve( Translation[1], Time, TransformKey.GetTranslationValue().Y, TransformKey.KeyParams );
	}

	if( TransformKey.KeyParams.bAddKeyEvenIfUnchanged || TransformKey.ShouldKeyTranslation( EAxis::Z ) || Translation[2].GetNumKeys() == 0 )
	{ 
		AddKeyToCurve( Translation[2], Time, TransformKey.GetTranslationValue().Z, TransformKey.KeyParams );
	}
}


void UMovieScene3DTransformSection::AddRotationKeys( const FTransformKey& TransformKey, const bool bUnwindRotation )
{
	const float Time = TransformKey.GetKeyTime();

	if( TransformKey.KeyParams.bAddKeyEvenIfUnchanged || TransformKey.ShouldKeyRotation( EAxis::X ) || Rotation[0].GetNumKeys() == 0 )
	{
		AddKeyToCurve( Rotation[0], Time, TransformKey.GetRotationValue().Roll, TransformKey.KeyParams, bUnwindRotation );
	}

	if( TransformKey.KeyParams.bAddKeyEvenIfUnchanged || TransformKey.ShouldKeyRotation( EAxis::Y ) || Rotation[1].GetNumKeys() == 0 )
	{ 
		AddKeyToCurve( Rotation[1], Time, TransformKey.GetRotationValue().Pitch, TransformKey.KeyParams, bUnwindRotation );
	}

	if( TransformKey.KeyParams.bAddKeyEvenIfUnchanged || TransformKey.ShouldKeyRotation( EAxis::Z ) || Rotation[2].GetNumKeys() == 0 )
	{ 
		AddKeyToCurve( Rotation[2], Time, TransformKey.GetRotationValue().Yaw, TransformKey.KeyParams, bUnwindRotation );
	}
}


void UMovieScene3DTransformSection::AddScaleKeys( const FTransformKey& TransformKey )
{
	const float Time = TransformKey.GetKeyTime();

	if( TransformKey.KeyParams.bAddKeyEvenIfUnchanged || TransformKey.ShouldKeyScale( EAxis::X ) || Scale[0].GetNumKeys() == 0 )
	{
		AddKeyToCurve( Scale[0], Time, TransformKey.GetScaleValue().X, TransformKey.KeyParams );
	}

	if( TransformKey.KeyParams.bAddKeyEvenIfUnchanged || TransformKey.ShouldKeyScale( EAxis::Y ) || Scale[1].GetNumKeys() == 0 )
	{ 
		AddKeyToCurve( Scale[1], Time, TransformKey.GetScaleValue().Y, TransformKey.KeyParams );
	}

	if( TransformKey.KeyParams.bAddKeyEvenIfUnchanged || TransformKey.ShouldKeyScale( EAxis::Z ) || Scale[2].GetNumKeys() == 0 )
	{ 
		AddKeyToCurve( Scale[2], Time, TransformKey.GetScaleValue().Z, TransformKey.KeyParams );
	}
}


bool UMovieScene3DTransformSection::NewKeyIsNewData(const FTransformKey& TransformKey) const
{
	bool bHasEmptyKeys = false;
	for (int32 i = 0; i < 3; ++i)
	{
		bHasEmptyKeys = bHasEmptyKeys ||
			Translation[i].GetNumKeys() == 0 ||
			Rotation[i].GetNumKeys() == 0 ||
			Scale[i].GetNumKeys() == 0;
	}

	if ( bHasEmptyKeys || (TransformKey.KeyParams.bAutoKeying && TransformKey.ShouldKeyAny() ) )
	{
		return true;
	}

	return false;
}
