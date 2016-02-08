// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMovieScenePropertyRecorder.h"

class FMovieSceneVisibilityPropertyRecorder : public IMovieScenePropertyRecorder
{
public:
	virtual ~FMovieSceneVisibilityPropertyRecorder() {}

	virtual void CreateSection(class AActor* SourceActor, class UMovieScene* MovieScene, const FGuid& Guid, bool bRecord) override;
	virtual void FinalizeSection() override;
	virtual void Record(float CurrentTime) override;

private:
	TLazyObjectPtr<class AActor> ActorToRecord;

	TWeakObjectPtr<class UMovieSceneVisibilitySection> MovieSceneSection;

	bool bRecording;

	bool bWasVisible;
};