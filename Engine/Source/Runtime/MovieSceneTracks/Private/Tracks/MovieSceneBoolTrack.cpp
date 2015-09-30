// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneBoolSection.h"
#include "MovieSceneBoolTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneBoolTrackInstance.h"


UMovieSceneBoolTrack::UMovieSceneBoolTrack( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
}

UMovieSceneSection* UMovieSceneBoolTrack::CreateNewSection()
{
	return NewObject<UMovieSceneSection>(this, UMovieSceneBoolSection::StaticClass(), NAME_None, RF_Transactional);
}

TSharedPtr<IMovieSceneTrackInstance> UMovieSceneBoolTrack::CreateInstance()
{
	return MakeShareable( new FMovieSceneBoolTrackInstance( *this ) );
}

bool UMovieSceneBoolTrack::AddKeyToSection( float Time, bool Value, FKeyParams KeyParams)
{
	const UMovieSceneSection* NearestSection = MovieSceneHelpers::FindNearestSectionAtTime( Sections, Time );
	if (!NearestSection || KeyParams.bAddKeyEvenIfUnchanged || CastChecked<UMovieSceneBoolSection>(NearestSection)->NewKeyIsNewData(Time, Value, KeyParams))
	{
		Modify();

		UMovieSceneBoolSection* NewSection = CastChecked<UMovieSceneBoolSection>(FindOrAddSection(  Time ));
	
		NewSection->AddKey( Time, Value, KeyParams );

		return true;
	}
	return false;
}

bool UMovieSceneBoolTrack::Eval( float Position, float LastPostion, bool& OutBool ) const
{	
	const UMovieSceneSection* Section = MovieSceneHelpers::FindNearestSectionAtTime( Sections, Position );

	if( Section )
	{
		if (!Section->IsInfinite())
		{
			Position = FMath::Clamp(Position, Section->GetStartTime(), Section->GetEndTime());
		}

		OutBool = CastChecked<UMovieSceneBoolSection>( Section )->Eval( Position );
	}

	return Section != NULL;
}

bool UMovieSceneBoolTrack::CanKeyTrack(float Time, bool Value, FKeyParams KeyParams) const
{
	const UMovieSceneSection* NearestSection = MovieSceneHelpers::FindNearestSectionAtTime( Sections, Time );
	if (!NearestSection || CastChecked<UMovieSceneBoolSection>(NearestSection)->NewKeyIsNewData(Time, Value, KeyParams))
	{
		return true;
	}
	return false;
}
