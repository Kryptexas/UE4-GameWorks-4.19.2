// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCoreTypesPCH.h"
#include "MovieSceneTransformSection.h"
#include "MovieSceneTransformTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneTransformTrackInstance.h"

bool FTransformKey::ShouldKeyTranslation( EAxis::Type Axis) const
{
	// If the previous transform is not valid we have nothing to compare against so assume we always key. Otherwise check for differences
	return !PreviousTransform.IsValid() || GetVectorComponentIfDifferent( Axis, NewTransform.Translation, PreviousTransform.Translation );
}

bool FTransformKey::ShouldKeyRotation( EAxis::Type Axis ) const
{
	// If the previous transform is not valid we have nothing to compare against so assume we always key. Otherwise check for differences
	return !PreviousTransform.IsValid() || GetVectorComponentIfDifferent( Axis, NewTransform.Rotation.Euler(), PreviousTransform.Rotation.Euler() );
}

bool FTransformKey::ShouldKeyScale( EAxis::Type Axis ) const
{
	// If the previous transform is not valid we have nothing to compare against so assume we always key. Otherwise check for differences
	return !PreviousTransform.IsValid() || GetVectorComponentIfDifferent( Axis, NewTransform.Scale, PreviousTransform.Scale );
}

bool FTransformKey::ShouldKeyAny() const
{
	return ShouldKeyTranslation(EAxis::X) || ShouldKeyTranslation(EAxis::Y) || ShouldKeyTranslation(EAxis::Z) ||
		ShouldKeyRotation(EAxis::X) || ShouldKeyRotation(EAxis::Y) || ShouldKeyRotation(EAxis::Z) ||
		ShouldKeyScale(EAxis::X) || ShouldKeyScale(EAxis::Y) || ShouldKeyScale(EAxis::Z);
}

bool FTransformKey::GetVectorComponentIfDifferent( EAxis::Type Axis, const FVector& Current, const FVector& Previous ) const
{
	bool bShouldAddKey = false;

	// Using lower precision to avoid small changes due to floating point error
	// @todo Sequencer - Floating point error introduced by matrix math causing extra keys to be added
	const float Precision = 1e-3;

	// Add a key if there is no previous transform to compare against or the current and previous values are not equal

	if( Axis == EAxis::X && !FMath::IsNearlyEqual( Current.X, Previous.X, Precision ) )
	{
		bShouldAddKey = true;
	}
	else if( Axis == EAxis::Y && !FMath::IsNearlyEqual( Current.Y, Previous.Y, Precision ) )
	{
		bShouldAddKey = true;
	}
	else if( Axis == EAxis::Z && !FMath::IsNearlyEqual( Current.Z, Previous.Z, Precision ) )
	{
		bShouldAddKey = true;
	}

	return bShouldAddKey;
}

UMovieSceneTransformTrack::UMovieSceneTransformTrack( const FPostConstructInitializeProperties& PCIP )
	: Super( PCIP )
{
}

UMovieSceneSection* UMovieSceneTransformTrack::CreateNewSection()
{
	return ConstructObject<UMovieSceneSection>( UMovieSceneTransformSection::StaticClass(), this );
}

TSharedPtr<IMovieSceneTrackInstance> UMovieSceneTransformTrack::CreateInstance()
{
	return MakeShareable( new FMovieSceneTransformTrackInstance( *this ) );
}

bool UMovieSceneTransformTrack::AddKeyToSection( const FGuid& ObjectHandle, const FTransformKey& InKey, const bool bUnwindRotation )
{
	const UMovieSceneSection* NearestSection = MovieSceneHelpers::FindSectionAtTime(Sections, InKey.GetKeyTime());
	if (!NearestSection || CastChecked<UMovieSceneTransformSection>(NearestSection)->NewKeyIsNewData(InKey))
	{
		UMovieSceneTransformSection* NewSection = Cast<UMovieSceneTransformSection>( FindOrAddSection( InKey.GetKeyTime() ) );

		// key each component of the transform
		NewSection->AddTranslationKeys( InKey );
		NewSection->AddRotationKeys( InKey, bUnwindRotation );
		NewSection->AddScaleKeys( InKey );

		return true;
	}
	return false;
}


bool UMovieSceneTransformTrack::Eval( float Position, float LastPosition, FVector& OutTranslation, FRotator& OutRotation, FVector& OutScale, bool& OutHasTranslationKeys, bool& OutHasRotationKeys, bool& OutHasScaleKeys ) const
{
	const UMovieSceneSection* Section = MovieSceneHelpers::FindSectionAtTime( Sections, Position );

	if( Section )
	{
		const UMovieSceneTransformSection* TransformSection = CastChecked<UMovieSceneTransformSection>( Section );

		// Evalulate translation,rotation, and scale curves.  If no keys were found on one of these, that component of the transform will remain unchained
		OutHasTranslationKeys = TransformSection->EvalTranslation( Position, OutTranslation );
		OutHasRotationKeys = TransformSection->EvalRotation( Position, OutRotation );
		OutHasScaleKeys = TransformSection->EvalScale( Position, OutScale );
	}

	return Section != NULL;
}