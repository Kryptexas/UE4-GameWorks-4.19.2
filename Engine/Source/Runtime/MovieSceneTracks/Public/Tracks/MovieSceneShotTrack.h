// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieScene.h"
#include "MovieSceneTrack.h"
#include "SubMovieSceneTrack.h"
#include "MovieSceneShotTrack.generated.h"

/**
 * Handles manipulation of shot properties in a movie scene
 */
UCLASS( MinimalAPI )
class UMovieSceneShotTrack : public USubMovieSceneTrack
{
	GENERATED_UCLASS_BODY()

public:
	/** UMovieSceneTrack interface */
	virtual FName GetTrackName() const override;
	virtual TSharedPtr<IMovieSceneTrackInstance> CreateInstance() override;

	/** Adds a new shot at the specified time */
	MOVIESCENETRACKS_API void AddNewShot(FGuid CameraHandle, float Time);
};
