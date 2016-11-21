// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneFloatTrack.h"
#include "MovieSceneSlomoTrack.generated.h"


class IMovieSceneTrackInstance;
class UMovieSceneSection;


/**
 * Implements a movie scene track that controls a movie scene's playback speed.
 */
UCLASS(MinimalAPI)
class UMovieSceneSlomoTrack
	: public UMovieSceneFloatTrack
{
	GENERATED_BODY()

public:
	
	UMovieSceneSlomoTrack(const FObjectInitializer& Init);

	// UMovieSceneTrack interface

	virtual UMovieSceneSection* CreateNewSection() override;
	virtual FMovieSceneEvalTemplatePtr CreateTemplateForSection(const UMovieSceneSection& InSection) const override;

#if WITH_EDITORONLY_DATA
	virtual FText GetDefaultDisplayName() const override;
	virtual bool CanRename() const override { return true; }
#endif
};
