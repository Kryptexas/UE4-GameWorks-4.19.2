// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneSlomoSection.h"
#include "MovieSceneSlomoTrack.h"
#include "MovieSceneSlomoTrackInstance.h"


/* UMovieSceneEventTrack interface
 *****************************************************************************/

UMovieSceneSection* UMovieSceneSlomoTrack::CreateNewSection()
{
	return NewObject<UMovieSceneSection>(this, UMovieSceneSlomoSection::StaticClass(), NAME_None, RF_Transactional);
}


FName UMovieSceneSlomoTrack::GetTrackName() const
{
	return FName("Play Rate");
}
