// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMovieSceneCapture.generated.h"

class FViewport;
struct FMovieSceneCaptureSettings;
struct FMovieSceneCaptureHandle;

UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UMovieSceneCaptureInterface : public UInterface
{
public:
	GENERATED_BODY()
};

/** Interface for a movie capture object */
class IMovieSceneCaptureInterface
{
public:
	GENERATED_BODY()

	/** Initialize this capture object by binding it to the specified viewport */
	virtual void Initialize(FViewport* Viewport) = 0;

	/** Shut down this movie capture */
	virtual void Close() = 0;

	/** Get a unique handle to this object */
	virtual FMovieSceneCaptureHandle GetHandle() const = 0;

	/** Access specific movie scene capture settings */
	virtual const FMovieSceneCaptureSettings& GetSettings() const = 0;
	
	/** Get the name of a map package name that this capture wants to load */
	virtual FString GetPackageName() const { return FString(); }
};