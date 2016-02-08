// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequenceRecorderPrivatePCH.h"
#include "MovieSceneVisibilityPropertyRecorder.h"
#include "MovieSceneVisibilitySection.h"
#include "MovieSceneVisibilityTrack.h"
#include "MovieScene.h"

static const FName VisibilityTrackName = TEXT("Visibility");

void FMovieSceneVisibilityPropertyRecorder::CreateSection(AActor* SourceActor, UMovieScene* MovieScene, const FGuid& Guid, bool bRecord)
{
	ActorToRecord = SourceActor;

	UMovieSceneVisibilityTrack* VisibilityTrack = MovieScene->AddTrack<UMovieSceneVisibilityTrack>(Guid);
	if(VisibilityTrack)
	{
		VisibilityTrack->SetPropertyNameAndPath(VisibilityTrackName, VisibilityTrackName.ToString());

		MovieSceneSection = Cast<UMovieSceneVisibilitySection>(VisibilityTrack->CreateNewSection());

		VisibilityTrack->AddSection(*MovieSceneSection);

		MovieSceneSection->SetDefault(false);

		MovieSceneSection->SetStartTime(0.0f);
	}

	bRecording = bRecord;

	bWasVisible = false;
}

void FMovieSceneVisibilityPropertyRecorder::FinalizeSection()
{
	bRecording = false;
}

void FMovieSceneVisibilityPropertyRecorder::Record(float CurrentTime)
{
	if(ActorToRecord.IsValid())
	{
		MovieSceneSection->SetEndTime(CurrentTime);

		if(bRecording)
		{
			USkeletalMeshComponent* SkeletalMeshComponent = ActorToRecord->FindComponentByClass<USkeletalMeshComponent>();

			const bool bVisible = SkeletalMeshComponent ? !SkeletalMeshComponent->bHiddenInGame : !ActorToRecord->bHidden;
			if(bVisible != bWasVisible)
			{
				MovieSceneSection->AddKey(CurrentTime, bVisible, EMovieSceneKeyInterpolation::Break);
			}
			bWasVisible = bVisible;
		}
	}
}