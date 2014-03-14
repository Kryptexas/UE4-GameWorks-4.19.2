// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MoviePlayerSettings.generated.h"

/**
 * Implements the settings for the Windows target platform.
 */
UCLASS(config=Game)
class MOVIEPLAYER_API UMoviePlayerSettings : public UObject
{
public:

	GENERATED_UCLASS_BODY()

	/**
	 * Movies to play on startup. Note that these must be in your game's Game/Content/Movies directory.
	 */
	UPROPERTY(globalconfig, EditAnywhere, Category="Movies", meta=(FilePathFilter = "mp4"))
	TArray<FFilePath> StartupMovies;
};
