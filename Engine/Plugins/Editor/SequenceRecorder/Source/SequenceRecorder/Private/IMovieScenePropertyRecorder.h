// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class IMovieScenePropertyRecorder
{
public:
	virtual void CreateSection(class AActor* SourceActor, class UMovieScene* MovieScene, const FGuid& Guid, bool bRecord) = 0;
	virtual void FinalizeSection() = 0;
	virtual void Record(float CurrentTime) = 0;
};