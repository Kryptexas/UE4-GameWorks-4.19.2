// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneFadeSection.h"
#include "MovieSceneFadeTrack.h"
#include "MovieSceneFadeTrackInstance.h"


/* UMovieSceneEventTrack interface
 *****************************************************************************/

TSharedPtr<IMovieSceneTrackInstance> UMovieSceneFadeTrack::CreateInstance()
{
	return MakeShareable(new FMovieSceneFadeTrackInstance(*this)); 
}


UMovieSceneSection* UMovieSceneFadeTrack::CreateNewSection()
{
	return NewObject<UMovieSceneSection>(this, UMovieSceneFadeSection::StaticClass(), NAME_None, RF_Transactional);
}


FName UMovieSceneFadeTrack::GetTrackName() const
{
	return FName("Fade");
}
