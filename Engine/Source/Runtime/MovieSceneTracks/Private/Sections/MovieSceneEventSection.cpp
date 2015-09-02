// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneEventSection.h"


/* UMovieSceneSection overrides
 *****************************************************************************/

void UMovieSceneEventSection::AddKey(float Time, const FName& EventName, FKeyParams KeyParams)
{
	Keys.HeapPush(FMovieSceneEventSectionKey(EventName, Time));
}


/* UMovieSceneSection overrides
 *****************************************************************************/

void UMovieSceneEventSection::DilateSection(float DilationFactor, float Origin, TSet<FKeyHandle>& KeyHandles)
{
	Super::DilateSection(DilationFactor, Origin, KeyHandles);

	for (auto& Key : Keys)
	{
		Key.Time = Key.Time * DilationFactor - Origin;
	}
}


void UMovieSceneEventSection::GetKeyHandles(TSet<FKeyHandle>& KeyHandles) const
{
	 // @todo gmp: implement event tracks
}


void UMovieSceneEventSection::GetSnapTimes(TArray<float>& OutSnapTimes, bool bGetSectionBorders) const
{
	Super::GetSnapTimes(OutSnapTimes, bGetSectionBorders);

	for (auto& Key : Keys)
	{
		OutSnapTimes.Add(Key.Time);
	}
}


void UMovieSceneEventSection::MoveSection(float DeltaPosition, TSet<FKeyHandle>& KeyHandles)
{
	Super::MoveSection(DeltaPosition, KeyHandles);

	for (auto& Key : Keys)
	{
		Key.Time += DeltaPosition;
	}
}
