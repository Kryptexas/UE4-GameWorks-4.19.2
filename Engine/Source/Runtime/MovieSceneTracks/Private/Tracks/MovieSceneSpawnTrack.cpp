// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneSpawnTrack.h"
#include "MovieSceneSpawnTrackInstance.h"
#include "MovieSceneBoolSection.h"


UMovieSceneSpawnTrack::UMovieSceneSpawnTrack(const FObjectInitializer& Obj)
	: Super(Obj)
{
}

UMovieSceneSection* UMovieSceneSpawnTrack::CreateNewSection()
{
	UMovieSceneBoolSection* Section = NewObject<UMovieSceneBoolSection>(this, NAME_None, RF_Transactional);
	Section->DefaultValue = true;
	return Section;
}

TSharedPtr<IMovieSceneTrackInstance> UMovieSceneSpawnTrack::CreateInstance()
{
	return MakeShareable( new FMovieSceneSpawnTrackInstance( *this ) );
}

bool UMovieSceneSpawnTrack::Eval(float Position, float LastPostion, bool& bOutSpawned) const
{
	const UMovieSceneBoolSection* Section = Cast<UMovieSceneBoolSection>(MovieSceneHelpers::FindNearestSectionAtTime(Sections, Position));
	if (!Section)
	{
		return false;
	}

	if (!Section->IsInfinite())
	{
		Position = FMath::Clamp(Position, Section->GetStartTime(), Section->GetEndTime());
	}

	bOutSpawned = Section->Eval(Position);
	return true;
}