// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "MovieScene2DTransformSection.h"
#include "MovieScene2DTransformTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieScene2DTransformTrackInstance.h"
#include "MovieSceneCommonHelpers.h"

UMovieScene2DTransformTrack::UMovieScene2DTransformTrack( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
}

UMovieSceneSection* UMovieScene2DTransformTrack::CreateNewSection()
{
	return NewObject<UMovieSceneSection>(this, UMovieScene2DTransformSection::StaticClass(), NAME_None, RF_Transactional);
}

TSharedPtr<IMovieSceneTrackInstance> UMovieScene2DTransformTrack::CreateInstance()
{
	return MakeShareable( new FMovieScene2DTransformTrackInstance( *this ) );
}

bool UMovieScene2DTransformTrack::Eval(float Position, float LastPosition, FWidgetTransform& InOutTransform) const
{
	const UMovieSceneSection* Section = MovieSceneHelpers::FindNearestSectionAtTime(Sections, Position);

	if(Section)
	{
		const UMovieScene2DTransformSection* TransformSection = CastChecked<UMovieScene2DTransformSection>(Section);

		if (!Section->IsInfinite())
		{
			Position = FMath::Clamp(Position, Section->GetStartTime(), Section->GetEndTime());
		}

		InOutTransform = TransformSection->Eval(Position, InOutTransform);
	}

	return Section != NULL;
}


bool UMovieScene2DTransformTrack::AddKeyToSection(float Time, const F2DTransformKey& TransformKey, FKeyParams KeyParams)
{
	const UMovieSceneSection* NearestSection = MovieSceneHelpers::FindNearestSectionAtTime(Sections, Time);
	if(!NearestSection || KeyParams.bAddKeyEvenIfUnchanged || CastChecked<UMovieScene2DTransformSection>(NearestSection)->NewKeyIsNewData(Time, TransformKey.Value, KeyParams))
	{
		Modify();

		UMovieScene2DTransformSection* NewSection = CastChecked<UMovieScene2DTransformSection>(FindOrAddSection(Time));

		NewSection->AddKey(Time, TransformKey, KeyParams);

		return true;
	}
	return false;
}

bool UMovieScene2DTransformTrack::CanKeyTrack(float Time, const F2DTransformKey& TransformKey, FKeyParams KeyParams) const
{
	const UMovieSceneSection* NearestSection = MovieSceneHelpers::FindNearestSectionAtTime( Sections, Time );
	if (!NearestSection || CastChecked<UMovieScene2DTransformSection>(NearestSection)->NewKeyIsNewData(Time, TransformKey.Value, KeyParams))
	{
		return true;
	}
	return false;
}
