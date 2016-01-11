// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "MovieSceneMarginSection.h"
#include "MovieSceneMarginTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneMarginTrackInstance.h"
#include "MovieSceneCommonHelpers.h"

UMovieSceneMarginTrack::UMovieSceneMarginTrack(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

UMovieSceneSection* UMovieSceneMarginTrack::CreateNewSection()
{
	return NewObject<UMovieSceneSection>(this, UMovieSceneMarginSection::StaticClass(), NAME_None, RF_Transactional);
}

TSharedPtr<IMovieSceneTrackInstance> UMovieSceneMarginTrack::CreateInstance()
{
	return MakeShareable( new FMovieSceneMarginTrackInstance( *this ) ); 
}


bool UMovieSceneMarginTrack::AddKeyToSection( float Time, const FMarginKey& MarginKey, FKeyParams KeyParams )
{
	const UMovieSceneSection* NearestSection = MovieSceneHelpers::FindNearestSectionAtTime( Sections, Time );
	if (!NearestSection || KeyParams.bAddKeyEvenIfUnchanged || CastChecked<UMovieSceneMarginSection>(NearestSection)->NewKeyIsNewData(Time, MarginKey.Value, KeyParams) )
	{
		Modify();

		UMovieSceneMarginSection* NewSection = CastChecked<UMovieSceneMarginSection>( FindOrAddSection( Time ) );

		NewSection->AddKey( Time, MarginKey, KeyParams );

		return true;
	}
	return false;
}


bool UMovieSceneMarginTrack::Eval( float Position, float LastPosition, FMargin& InOutMargin ) const
{
	const UMovieSceneSection* Section = MovieSceneHelpers::FindNearestSectionAtTime( Sections, Position );

	if( Section )
	{
		const UMovieSceneMarginSection* MarginSection = CastChecked<UMovieSceneMarginSection>( Section );

		if (!Section->IsInfinite())
		{
			Position = FMath::Clamp(Position, Section->GetStartTime(), Section->GetEndTime());
		}

		InOutMargin = MarginSection->Eval( Position, InOutMargin );
	}

	return Section != NULL;
}

bool UMovieSceneMarginTrack::CanKeyTrack(float Time, const FMarginKey& MarginKey, FKeyParams KeyParams) const
{
	const UMovieSceneSection* NearestSection = MovieSceneHelpers::FindNearestSectionAtTime( Sections, Time );
	if (!NearestSection || CastChecked<UMovieSceneMarginSection>(NearestSection)->NewKeyIsNewData(Time, MarginKey.Value, KeyParams))
	{
		return true;
	}
	return false;
}
