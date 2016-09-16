// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneBoolSection.h"
#include "MovieSceneBoolTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneBoolTrackInstance.h"


UMovieSceneSection* UMovieSceneBoolTrack::CreateNewSection()
{
	return NewObject<UMovieSceneSection>(this, UMovieSceneBoolSection::StaticClass(), NAME_None, RF_Transactional);
}


TSharedPtr<IMovieSceneTrackInstance> UMovieSceneBoolTrack::CreateInstance()
{
	return MakeShareable( new FMovieSceneBoolTrackInstance( *this ) );
}


bool UMovieSceneBoolTrack::Eval( float Position, float LastPostion, bool& InOutBool ) const
{	
	const UMovieSceneSection* Section = MovieSceneHelpers::FindNearestSectionAtTime( Sections, Position );

	if( Section )
	{
		if (!Section->IsInfinite())
		{
			Position = FMath::Clamp(Position, Section->GetStartTime(), Section->GetEndTime());
		}

		InOutBool = CastChecked<UMovieSceneBoolSection>( Section )->Eval( Position, InOutBool );
	}

	return (Section != nullptr);
}
