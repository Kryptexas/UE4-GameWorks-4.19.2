// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Tracks/MovieScenePropertyTrack.h"
#include "MovieScene3DTransformTrack.generated.h"

struct FMovieSceneInterrogationKey;

/**
 * Handles manipulation of component transforms in a movie scene
 */
UCLASS(MinimalAPI)
class UMovieScene3DTransformTrack
	: public UMovieScenePropertyTrack
{
	GENERATED_UCLASS_BODY()

public:

	// UMovieSceneTrack interface

	virtual UMovieSceneSection* CreateNewSection() override;

	/**
	 * Access the interrogation key for transform data - any interrgation data stored with this key is guaranteed to be of type 'FTransform'
	 */
	MOVIESCENETRACKS_API static FMovieSceneInterrogationKey GetInterrogationKey();
};
