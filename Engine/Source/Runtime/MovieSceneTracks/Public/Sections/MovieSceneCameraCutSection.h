// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneSection.h"
#include "MovieSceneCameraCutSection.generated.h"


/**
 * Movie CameraCuts are sections on the CameraCuts track, that show what the viewer "sees"
 */
UCLASS(MinimalAPI)
class UMovieSceneCameraCutSection 
	: public UMovieSceneSection
{
	GENERATED_BODY()

public:

	/** Gets the camera guid for this CameraCut section */
	FGuid GetCameraGuid() const
	{
		return CameraGuid;
	}

	/** Sets the camera guid for this CameraCut section */
	void SetCameraGuid(const FGuid& InGuid)
	{
		CameraGuid = InGuid;
	}

private:

	/** The camera possessable or spawnable that this movie CameraCut uses */
	UPROPERTY()
	FGuid CameraGuid;
};
