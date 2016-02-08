// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMovieScenePropertyRecorder.h"

class FMovieScene3DTransformPropertyRecorder : public IMovieScenePropertyRecorder
{
public:
	virtual ~FMovieScene3DTransformPropertyRecorder() {}

	virtual void CreateSection(class AActor* SourceActor, class UMovieScene* MovieScene, const FGuid& Guid, bool bRecord) override;
	virtual void FinalizeSection() override;
	virtual void Record(float CurrentTime) override;

private:
	TLazyObjectPtr<class AActor> ActorToRecord;

	TWeakObjectPtr<class UMovieScene3DTransformSection> MovieSceneSection;

	bool bRecording;
};