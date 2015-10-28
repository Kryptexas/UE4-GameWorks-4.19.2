// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneSequenceInstance.h"

#include "MovieSceneSpawnTrack.h"
#include "MovieSceneSpawnTrackInstance.h"


FMovieSceneSpawnTrackInstance::FMovieSceneSpawnTrackInstance(UMovieSceneSpawnTrack& InTrack)
{
	Track = &InTrack;
}

void FMovieSceneSpawnTrackInstance::Update(float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance)
{
	if (!SequenceInstance.GetTimeRange().Contains(Position))
	{
		SequenceInstance.DestroySpawnedObject(Track->GetObject());
		return;
	}

	bool bIsSpawned = false;
	if (Track->Eval(Position, LastPosition, bIsSpawned))
	{
		// Spawn the object if needed
		if (bIsSpawned && RuntimeObjects.Num() == 0)
		{
			SequenceInstance.SpawnObject(Track->GetObject());
		}

		// Destroy the object if needed
		if (!bIsSpawned && RuntimeObjects.Num() != 0)
		{
			SequenceInstance.DestroySpawnedObject(Track->GetObject());
		}
	}
}

void FMovieSceneSpawnTrackInstance::RestoreState(const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance)
{
	if (RuntimeObjects.Num())
	{
		SequenceInstance.DestroySpawnedObject(Track->GetObject());
	}
}

void FMovieSceneSpawnTrackInstance::ClearInstance(IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance)
{
	SequenceInstance.DestroySpawnedObject(Track->GetObject());
}