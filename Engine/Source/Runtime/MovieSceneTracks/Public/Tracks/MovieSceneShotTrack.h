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
	virtual void RemoveSection( UMovieSceneSection* Section ) override;

	/** 
	 * Adds a new shot at the specified time
	 *
	 * @param CameraHandle		Handle to the camera that the shot switches to when active
	 * @param ShotMovieScene	MovieScene for the shot (each shot has a unique movie scene containing per tracks only active during the shot)
	 * @param TimeRange		The range within this track's movie scene where the shot is initially placed
	 * @param ShotName		The display name of the shot
	 * @param ShotNumber		The number of the shot.  This is used to assist with auto-generated shot names when new shots are added
	 */
	MOVIESCENETRACKS_API void AddNewShot(FGuid CameraHandle, UMovieScene& ShotMovieScene, const TRange<float>& TimeRange, const FText& ShotName, int32 ShotNumber );

#if WITH_EDITOR
	virtual void OnSectionMoved( UMovieSceneSection& Section ) override;
#endif

private:
	/** Sorts shots according to their start time */
	void SortShots();
};
