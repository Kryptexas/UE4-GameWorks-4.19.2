// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneSpawnSectionRecorder.h"
#include "GameFramework/Actor.h"
#include "KeyParams.h"
#include "Sections/MovieSceneBoolSection.h"
#include "Tracks/MovieSceneSpawnTrack.h"
#include "MovieScene.h"
#include "SequenceRecorderSettings.h"

TSharedPtr<IMovieSceneSectionRecorder> FMovieSceneSpawnSectionRecorderFactory::CreateSectionRecorder(const struct FActorRecordingSettings& InActorRecordingSettings) const
{
	return MakeShareable(new FMovieSceneSpawnSectionRecorder);
}

bool FMovieSceneSpawnSectionRecorderFactory::CanRecordObject(UObject* InObjectToRecord) const
{
	return InObjectToRecord->IsA<AActor>();
}

void FMovieSceneSpawnSectionRecorder::CreateSection(UObject* InObjectToRecord, UMovieScene* MovieScene, const FGuid& Guid, float Time)
{
	ObjectToRecord = InObjectToRecord;

	UMovieSceneSpawnTrack* SpawnTrack = MovieScene->AddTrack<UMovieSceneSpawnTrack>(Guid);
	if(SpawnTrack)
	{
		MovieSceneSection = Cast<UMovieSceneBoolSection>(SpawnTrack->CreateNewSection());

		SpawnTrack->AddSection(*MovieSceneSection);
		SpawnTrack->SetObjectId(Guid);

		MovieSceneSection->SetDefault(false);
		MovieSceneSection->AddKey(0.0f, false, EMovieSceneKeyInterpolation::Break);

		MovieSceneSection->SetStartTime(Time);
		MovieSceneSection->SetIsInfinite(true);
	}

	bWasSpawned = false;
}

void FMovieSceneSpawnSectionRecorder::FinalizeSection()
{
	const bool bSpawned = ObjectToRecord.IsValid();
	if(bSpawned != bWasSpawned)
	{
		MovieSceneSection->AddKey(MovieSceneSection->GetEndTime(), bSpawned, EMovieSceneKeyInterpolation::Break);
	}

	// If the section is degenerate, assume the actor was spawned and destroyed. Give it a 1 frame spawn section.
	if (MovieSceneSection->GetRange().IsDegenerate())
	{
		float OneFrameInterval = 1.0f/GetDefault<USequenceRecorderSettings>()->DefaultAnimationSettings.SampleRate;
		MovieSceneSection->AddKey(MovieSceneSection->GetEndTime() - OneFrameInterval, true, EMovieSceneKeyInterpolation::Break);
		MovieSceneSection->SetStartTime(MovieSceneSection->GetEndTime() - OneFrameInterval);
	}
}

void FMovieSceneSpawnSectionRecorder::Record(float CurrentTime)
{
	if(ObjectToRecord.IsValid())
	{
		MovieSceneSection->SetEndTime(CurrentTime);
	}

	const bool bSpawned = ObjectToRecord.IsValid();
	if(bSpawned != bWasSpawned)
	{
		MovieSceneSection->AddKey(CurrentTime, bSpawned, EMovieSceneKeyInterpolation::Break);
	}
	bWasSpawned = bSpawned;
}
